/*
 * Copyright © 2018 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <errno.h>
#include <linux/input.h>
#include <libevdev/libevdev.h>
#include <sys/signalfd.h>
#include <sys/utsname.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>

#include "libinput-util.h"
#include "libinput-version.h"

static const int FILE_VERSION_NUMBER = 1;

struct record_device {
	struct list link;
	char *devnode;		/* device node of the source device */
	struct libevdev *evdev;

	struct input_event *events;
	size_t nevents;
	size_t events_sz;
};

struct record_context {
	int timeout;
	bool show_keycodes;

	uint64_t offset;

	struct list devices;
	int ndevices;

	char *outfile; /* file name given on cmdline */
	char *output_file; /* full file name with suffix */

	int out_fd;
	unsigned int indent;
};

static inline bool
obfuscate_keycode(struct input_event *ev)
{
	switch (ev->type) {
	case EV_KEY:
		if (ev->code >= KEY_ESC && ev->code < KEY_ZENKAKUHANKAKU) {
			ev->code = KEY_A;
			return true;
		}
		break;
	case EV_MSC:
		if (ev->code == MSC_SCAN) {
			ev->value = 30; /* KEY_A scancode */
			return true;
		}
		break;
	}

	return false;
}

static inline void
indent_push(struct record_context *ctx)
{
	ctx->indent += 2;
}

static inline void
indent_pop(struct record_context *ctx)
{
	assert(ctx->indent >= 2);
	ctx->indent -= 2;
}

/**
 * Indented dprintf, indentation is given as second parameter.
 */
static inline void
iprintf(const struct record_context *ctx, const char *format, ...)
{
	va_list args;
	char fmt[1024];
	static const char space[] = "                                     ";
	static const size_t len = sizeof(space);
	unsigned int indent = ctx->indent;
	int rc;

	assert(indent < len);
	assert(strlen(format) > 1);

	/* Special case: if we're printing a new list item, we want less
	 * indentation because the '- ' takes up one level of indentation
	 *
	 * This is only needed because I don't want to deal with open/close
	 * lists statements.
	 */
	if (format[0] == '-')
		indent -= 2;

	snprintf(fmt, sizeof(fmt), "%s%s", &space[len - indent - 1], format);
	va_start(args, format);
	rc = vdprintf(ctx->out_fd, fmt, args);
	va_end(args);

	assert(rc != -1 && (unsigned int)rc > indent);
}

/**
 * Normal printf, just wrapped for the context
 */
static inline void
noiprintf(const struct record_context *ctx, const char *format, ...)
{
	va_list args;
	int rc;

	va_start(args, format);
	rc = vdprintf(ctx->out_fd, format, args);
	va_end(args);
	assert(rc != -1 && (unsigned int)rc > 0);
}

static inline void
print_evdev_event(struct record_context *ctx, struct input_event *ev)
{
	const char *cname;
	bool was_modified = false;
	char desc[1024];

	if (ctx->offset == 0)
		ctx->offset = tv2us(&ev->time);
	ev->time = us2tv(tv2us(&ev->time) - ctx->offset);

	/* Don't leak passwords unless the user wants to */
	if (!ctx->show_keycodes)
		was_modified = obfuscate_keycode(ev);

	cname = libevdev_event_code_get_name(ev->type, ev->code);

	if (ev->type == EV_SYN && ev->code == SYN_MT_REPORT) {
		snprintf(desc,
			 sizeof(desc),
			 "++++++++++++ %s (%d) ++++++++++",
			 cname,
			 ev->value);
	} else if (ev->type == EV_SYN) {
		static unsigned long last_ms = 0;
		unsigned long time, dt;


		time = us2ms(tv2us(&ev->time));
		if (last_ms == 0)
			last_ms = time;
		dt = time - last_ms;
		last_ms = time;

		snprintf(desc,
			 sizeof(desc),
			"------------ %s (%d) ---------- %+ldms",
			cname,
			ev->value,
			dt);
	} else {
		const char *tname = libevdev_event_type_get_name(ev->type);

		snprintf(desc,
			 sizeof(desc),
			 "%s / %-20s %4d%s",
			 tname,
			 cname,
			 ev->value,
			 was_modified ? " (obfuscated)" : "");
	}

	iprintf(ctx,
		"- [%3lu, %6u, %3d, %3d, %5d] # %s\n",
		ev->time.tv_sec,
		(unsigned int)ev->time.tv_usec,
		ev->type,
		ev->code,
		ev->value,
		desc);
}

static inline void
print_evdev_events(struct record_context *ctx, struct input_event *e, size_t nevents)
{
	iprintf(ctx, "- evdev:\n");
	indent_push(ctx);

	for (size_t i = 0; i < nevents; i++)
		print_evdev_event(ctx, &e[i]);

	indent_pop(ctx);
}

static inline size_t
handle_frame(struct record_context *ctx, struct record_device *d, bool print)
{
	struct libevdev *evdev = d->evdev;
	struct input_event e;
	size_t count = 0;

	while (libevdev_next_event(evdev,
				   LIBEVDEV_READ_FLAG_NORMAL,
				   &e) == LIBEVDEV_READ_STATUS_SUCCESS) {

		if (d->nevents == d->events_sz) {
			void *tmp;

			d->events_sz += 1000;
			tmp = realloc(d->events, d->events_sz * sizeof(*d->events));
			assert(tmp);
			d->events = tmp;
		}
		d->events[d->nevents++] = e;
		count++;

		if (e.type == EV_SYN && e.code == SYN_REPORT)
			break;
	}

	return count;
}

static inline void
handle_events(struct record_context *ctx, struct record_device *d, bool print)
{
	while(true) {
		size_t first_idx = d->nevents;
		size_t evcount;

		evcount = handle_frame(ctx, d, print);
		if (evcount == 0)
			break;

		if (!print)
			continue;

		print_evdev_events(ctx,
				   &d->events[first_idx],
				   evcount);
	}
}

static inline void
print_libinput_header(struct record_context *ctx)
{
	iprintf(ctx, "libinput:\n");
	indent_push(ctx);
	iprintf(ctx, "version: \"%s\"\n", LIBINPUT_VERSION);
	if (ctx->timeout > 0)
		iprintf(ctx, "autorestart: %d\n", ctx->timeout);
	indent_pop(ctx);
}

static inline void
print_system_header(struct record_context *ctx)
{
	struct utsname u;
	const char *kernel = "unknown";
	FILE *dmi;
	char modalias[2048] = "unknown";

	if (uname(&u) != -1)
		kernel = u.release;

	dmi = fopen("/sys/class/dmi/id/modalias", "r");
	if (dmi) {
		if (fgets(modalias, sizeof(modalias), dmi)) {
			modalias[strlen(modalias) - 1] = '\0'; /* linebreak */
		} else {
			sprintf(modalias, "unknown");
		}
		fclose(dmi);
	}

	iprintf(ctx, "system:\n");
	indent_push(ctx);
	iprintf(ctx, "kernel: \"%s\"\n", kernel);
	iprintf(ctx, "dmi: \"%s\"\n", modalias);
	indent_pop(ctx);
}

static inline void
print_header(struct record_context *ctx)
{
	iprintf(ctx, "version: %d\n", FILE_VERSION_NUMBER);
	iprintf(ctx, "ndevices: %d\n", ctx->ndevices);
	print_libinput_header(ctx);
	print_system_header(ctx);
}

static inline void
print_description_abs(struct record_context *ctx,
		      struct libevdev *dev,
		      unsigned int code)
{
	const struct input_absinfo *abs;

	abs = libevdev_get_abs_info(dev, code);
	assert(abs);

	iprintf(ctx, "#       Value      %6d\n", abs->value);
	iprintf(ctx, "#       Min        %6d\n", abs->minimum);
	iprintf(ctx, "#       Max        %6d\n", abs->maximum);
	iprintf(ctx, "#       Fuzz       %6d\n", abs->fuzz);
	iprintf(ctx, "#       Flat       %6d\n", abs->flat);
	iprintf(ctx, "#       Resolution %6d\n", abs->resolution);
}

static inline void
print_description_state(struct record_context *ctx,
			struct libevdev *dev,
			unsigned int type,
			unsigned int code)
{
	int state = libevdev_get_event_value(dev, type, code);
	iprintf(ctx, "#       State %d\n", state);
}

static inline void
print_description_codes(struct record_context *ctx,
			struct libevdev *dev,
			unsigned int type)
{
	int max;

	max = libevdev_event_type_get_max(type);
	if (max == -1)
		return;

	iprintf(ctx,
		"# Event type %d (%s)\n",
		type,
		libevdev_event_type_get_name(type));

	if (type == EV_SYN)
		return;

	for (unsigned int code = 0; code <= (unsigned int)max; code++) {
		if (!libevdev_has_event_code(dev, type, code))
			continue;

		iprintf(ctx,
			"#   Event code %d (%s)\n",
			code,
			libevdev_event_code_get_name(type,
						     code));

		switch (type) {
		case EV_ABS:
			print_description_abs(ctx, dev, code);
			break;
		case EV_LED:
		case EV_SW:
			print_description_state(ctx, dev, type, code);
			break;
		}
	}
}

static inline void
print_description(struct record_context *ctx, struct libevdev *dev)
{
	const struct input_absinfo *x, *y;

	iprintf(ctx, "# Name: %s\n", libevdev_get_name(dev));
	iprintf(ctx,
		"# ID: bus %#02x vendor %#02x product %#02x version %#02x\n",
		libevdev_get_id_bustype(dev),
		libevdev_get_id_vendor(dev),
		libevdev_get_id_product(dev),
		libevdev_get_id_version(dev));

	x = libevdev_get_abs_info(dev, ABS_X);
	y = libevdev_get_abs_info(dev, ABS_Y);
	if (x && y) {
		if (x->resolution || y->resolution) {
			int w, h;

			w = (x->maximum - x->minimum)/x->resolution;
			h = (y->maximum - y->minimum)/y->resolution;
			iprintf(ctx, "# Size in mm: %dx%d\n", w, h);
		} else {
			iprintf(ctx,
				"# Size in mm: unknown, missing resolution\n");
		}
	}

	iprintf(ctx, "# Supported Events:\n");

	for (unsigned int type = 0; type < EV_CNT; type++) {
		if (!libevdev_has_event_type(dev, type))
			continue;

		print_description_codes(ctx, dev, type);
	}

	iprintf(ctx, "# Properties:\n");

	for (unsigned int prop = 0; prop < INPUT_PROP_CNT; prop++) {
		if (libevdev_has_property(dev, prop)) {
			iprintf(ctx,
				"#    Property %d (%s)\n",
				prop,
				libevdev_property_get_name(prop));
		}
	}
}

static inline void
print_bits_info(struct record_context *ctx, struct libevdev *dev)
{
	iprintf(ctx, "name: \"%s\"\n", libevdev_get_name(dev));
	iprintf(ctx,
		"id: [%d, %d, %d, %d]\n",
		libevdev_get_id_bustype(dev),
		libevdev_get_id_vendor(dev),
		libevdev_get_id_product(dev),
		libevdev_get_id_version(dev));
}

static inline void
print_bits_absinfo(struct record_context *ctx, struct libevdev *dev)
{
	const struct input_absinfo *abs;

	if (!libevdev_has_event_type(dev, EV_ABS))
		return;

	iprintf(ctx, "absinfo:\n");
	indent_push(ctx);

	for (unsigned int code = 0; code < ABS_CNT; code++) {
		abs = libevdev_get_abs_info(dev, code);
		if (!abs)
			continue;

		iprintf(ctx,
			"%d: [%d, %d, %d, %d, %d]\n",
			code,
			abs->minimum,
			abs->maximum,
			abs->fuzz,
			abs->flat,
			abs->resolution);
	}
	indent_pop(ctx);
}


static inline void
print_bits_codes(struct record_context *ctx,
		 struct libevdev *dev,
		 unsigned int type)
{
	int max;
	bool first = true;

	max = libevdev_event_type_get_max(type);
	if (max == -1)
		return;

	iprintf(ctx, "%d: [", type);

	for (unsigned int code = 0; code <= (unsigned int)max; code++) {
		if (!libevdev_has_event_code(dev, type, code))
			continue;

		noiprintf(ctx, "%s%d", first ? "" : ", ", code);
		first = false;
	}

	noiprintf(ctx, "] # %s\n", libevdev_event_type_get_name(type));
}

static inline void
print_bits_types(struct record_context *ctx, struct libevdev *dev)
{
	iprintf(ctx, "codes:\n");
	indent_push(ctx);
	for (unsigned int type = 0; type < EV_CNT; type++) {
		if (!libevdev_has_event_type(dev, type))
			continue;
		print_bits_codes(ctx, dev, type);
	}
	indent_pop(ctx);
}

static inline void
print_bits_props(struct record_context *ctx, struct libevdev *dev)
{
	bool first = true;

	iprintf(ctx, "properties: [");
	for (unsigned int prop = 0; prop < INPUT_PROP_CNT; prop++) {
		if (libevdev_has_property(dev, prop)) {
			noiprintf(ctx, "%s%d", first ? "" : ", ", prop);
			first = false;
		}
	}
	noiprintf(ctx, "]\n"); /* last entry, no comma */
}

static inline void
print_evdev_description(struct record_context *ctx, struct record_device *dev)
{
	struct libevdev *evdev = dev->evdev;

	iprintf(ctx, "evdev:\n");
	indent_push(ctx);

	print_description(ctx, evdev);
	print_bits_info(ctx, evdev);
	print_bits_types(ctx, evdev);
	print_bits_absinfo(ctx, evdev);
	print_bits_props(ctx, evdev);

	indent_pop(ctx);
}

static inline void
print_device_description(struct record_context *ctx, struct record_device *dev)
{
	iprintf(ctx, "- node: %s\n", dev->devnode);

	print_evdev_description(ctx, dev);
}

static int is_event_node(const struct dirent *dir) {
	return strneq(dir->d_name, "event", 5);
}

static inline char *
select_device(void)
{
	struct dirent **namelist;
	int ndev, selected_device;
	int rc;
	char *device_path;

	ndev = scandir("/dev/input", &namelist, is_event_node, versionsort);
	if (ndev <= 0)
		return NULL;

	fprintf(stderr, "Available devices:\n");
	for (int i = 0; i < ndev; i++) {
		struct libevdev *device;
		char path[PATH_MAX];
		int fd = -1;

		snprintf(path,
			 sizeof(path),
			 "/dev/input/%s",
			 namelist[i]->d_name);
		fd = open(path, O_RDONLY);
		if (fd < 0)
			continue;

		rc = libevdev_new_from_fd(fd, &device);
		close(fd);
		if (rc != 0)
			continue;

		fprintf(stderr, "%s:	%s\n", path, libevdev_get_name(device));
		libevdev_free(device);
	}

	for (int i = 0; i < ndev; i++)
		free(namelist[i]);
	free(namelist);

	fprintf(stderr, "Select the device event number: ");
	rc = scanf("%d", &selected_device);

	if (rc != 1 || selected_device < 0)
		return NULL;

	rc = xasprintf(&device_path, "/dev/input/event%d", selected_device);
	if (rc == -1)
		return NULL;

	return device_path;
}

static char *
init_output_file(const char *file, bool is_prefix)
{
	char name[PATH_MAX];

	assert(file != NULL);

	if (is_prefix) {
		struct tm *tm;
		time_t t;
		char suffix[64];

		t = time(NULL);
		tm = localtime(&t);
		strftime(suffix, sizeof(suffix), "%F-%T", tm);
		snprintf(name,
			 sizeof(name),
			 "%s.%s",
			 file,
			 suffix);
	} else {
		snprintf(name, sizeof(name), "%s", file);
	}

	return strdup(name);
}

static bool
open_output_file(struct record_context *ctx, bool is_prefix)
{
	int out_fd;

	if (ctx->outfile) {
		char *fname = init_output_file(ctx->outfile, is_prefix);
		ctx->output_file = fname;
		out_fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		if (out_fd < 0)
			return false;
	} else {
		ctx->output_file = safe_strdup("stdout");
		out_fd = STDOUT_FILENO;
	}

	ctx->out_fd = out_fd;

	return true;
}

static int
mainloop(struct record_context *ctx)
{
	bool autorestart = (ctx->timeout > 0);
	struct pollfd fds[ctx->ndevices + 1];
	struct record_device *d = NULL;
	struct timespec ts;
	sigset_t mask;
	int idx;

	assert(ctx->timeout != 0);
	assert(!list_empty(&ctx->devices));

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	fds[0].fd = signalfd(-1, &mask, SFD_NONBLOCK);
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	assert(fds[0].fd != -1);

	idx = 1;
	list_for_each(d, &ctx->devices, link) {
		fds[idx].fd = libevdev_get_fd(d->evdev);
		fds[idx].events = POLLIN;
		fds[idx].revents = 0;
		assert(fds[idx].fd != -1);
		idx++;
	}

	/* If we have more than one device, the time starts at recording
	 * start time. Otherwise, the first event starts the recording time.
	 */
	if (ctx->ndevices > 1) {
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ctx->offset = s2us(ts.tv_sec) + ns2us(ts.tv_nsec);
	}

	do {
		int rc;
		bool had_events = false; /* we delete files without events */

		if (!open_output_file(ctx, autorestart)) {
			fprintf(stderr,
				"Failed to open '%s'\n",
				ctx->output_file);
			break;
		}
		fprintf(stderr, "recording to '%s'\n", ctx->output_file);

		print_header(ctx);
		if (autorestart)
			iprintf(ctx,
				"# Autorestart timeout: %d\n",
				ctx->timeout);

		iprintf(ctx, "devices:\n");
		indent_push(ctx);

		/* we only print the first device's description, the
		 * rest is assembled after CTRL+C */
		d = list_first_entry(&ctx->devices, d, link);
		print_device_description(ctx, d);

		iprintf(ctx, "events:\n");
		indent_push(ctx);
		while (true) {
			rc = poll(fds, ARRAY_LENGTH(fds), ctx->timeout);
			if (rc == -1) { /* error */
				fprintf(stderr, "Error: %m\n");
				autorestart = false;
				break;
			} else if (rc == 0) {
				fprintf(stderr,
					" ... timeout%s\n",
					had_events ? "" : " (file is empty)");
				break;
			} else if (fds[0].revents != 0) { /* signal */
				autorestart = false;
				break;
			} else { /* events */
				int is_first = true;
				had_events = true;
				list_for_each(d, &ctx->devices, link) {
					handle_events(ctx, d, is_first);
					is_first = false;
				}
			}
		}
		indent_pop(ctx); /* events: */

		if (autorestart) {
			d = list_first_entry(&ctx->devices, d, link);
			noiprintf(ctx,
				  "# Closing after %ds inactivity",
				  ctx->timeout/1000);
		}

		/* First device is printed, now append all the data from the
		 * other devices, if any */
		list_for_each(d, &ctx->devices, link) {
			if (d == list_first_entry(&ctx->devices, d, link))
				continue;

			print_device_description(ctx, d);
			iprintf(ctx, "events:\n");
			indent_push(ctx);
			print_evdev_events(ctx, d->events, d->nevents);
			indent_pop(ctx);
		}

		indent_pop(ctx); /* devices: */
		assert(ctx->indent == 0);

		fsync(ctx->out_fd);

		/* If we didn't have events, delete the file. */
		if (!isatty(ctx->out_fd)) {
			if (!had_events && ctx->output_file) {
				perror("");
				fprintf(stderr, "No events recorded, deleting '%s'\n", ctx->output_file);
				unlink(ctx->output_file);
			}

			close(ctx->out_fd);
			ctx->out_fd = -1;
		}
		free(ctx->output_file);
		ctx->output_file = NULL;
	} while (autorestart);

	close(fds[0].fd);

	sigprocmask(SIG_UNBLOCK, &mask, NULL);


	return 0;
}

static inline bool
init_device(struct record_context *ctx, char *path)
{
	struct record_device *d;
	int fd, rc;

	d = zalloc(sizeof(*d));
	d->devnode = path;
	d->nevents = 0;
	d->events_sz = 5000;
	d->events = zalloc(d->events_sz * sizeof(*d->events));

	fd = open(d->devnode, O_RDONLY|O_NONBLOCK);
	if (fd < 0) {
		fprintf(stderr,
			"Failed to open device %s (%m)\n",
			d->devnode);
		return false;
	}

	rc = libevdev_new_from_fd(fd, &d->evdev);
	if (rc != 0) {
		fprintf(stderr,
			"Failed to create context for %s (%s)\n",
			d->devnode,
			strerror(-rc));
		close(fd);
		return false;
	}

	libevdev_set_clock_id(d->evdev, CLOCK_MONOTONIC);

	list_insert(&ctx->devices, &d->link);
	ctx->ndevices++;

	return true;
}

static inline void
usage(void)
{
	printf("Usage: %s [--help] [--multiple] [--autorestart] [--output-file filename] [/dev/input/event0] [...]\n"
	       "Common use-cases:\n"
	       "\n"
	       " sudo %s -o recording.yml\n"
	       "    Then select the device to record and it Ctrl+C to stop.\n"
	       "    The recorded data is in recording.yml and can be attached to a bug report.\n"
	       "\n"
	       " sudo %s -o recording.yml --autorestart 2\n"
	       "    As above, but restarts after 2s of inactivity on the device.\n"
	       "    Note, the output file is only the prefix.\n"
	       "\n"
	       " sudo %s --multiple -o recording.yml /dev/input/event3 /dev/input/event4\n"
	       "    Records the two devices into the same recordings file.\n"
	       "\n"
	       "For more information, see the %s(1) man page\n",
	       program_invocation_short_name,
	       program_invocation_short_name,
	       program_invocation_short_name,
	       program_invocation_short_name,
	       program_invocation_short_name);
}

enum options {
	OPT_AUTORESTART,
	OPT_HELP,
	OPT_OUTFILE,
	OPT_KEYCODES,
	OPT_MULTIPLE,
};

int
main(int argc, char **argv)
{
	struct record_context ctx = {
		.timeout = -1,
		.show_keycodes = false,
	};
	struct option opts[] = {
		{ "autorestart", required_argument, 0, OPT_AUTORESTART },
		{ "output-file", required_argument, 0, OPT_OUTFILE },
		{ "show-keycodes", no_argument, 0, OPT_KEYCODES },
		{ "multiple", no_argument, 0, OPT_MULTIPLE },
		{ "help", no_argument, 0, OPT_HELP },
		{ 0, 0, 0, 0 },
	};
	struct record_device *d, *tmp;
	const char *output_arg = NULL;
	bool multiple = false;
	int ndevices;
	int rc = 1;

	list_init(&ctx.devices);

	while (1) {
		int c;
		int option_index = 0;

		c = getopt_long(argc, argv, "ho:", opts, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'h':
		case OPT_HELP:
			usage();
			rc = 0;
			goto out;
		case OPT_AUTORESTART:
			if (!safe_atoi(optarg, &ctx.timeout) ||
			    ctx.timeout <= 0) {
				usage();
				goto out;
			}
			ctx.timeout = ctx.timeout * 1000;
			break;
		case 'o':
		case OPT_OUTFILE:
			output_arg = optarg;
			break;
		case OPT_KEYCODES:
			ctx.show_keycodes = true;
			break;
		case OPT_MULTIPLE:
			multiple = true;
			break;
		}
	}

	if (ctx.timeout > 0 && output_arg == NULL) {
		fprintf(stderr,
			"Option --autorestart requires --output-file\n");
		goto out;
	}

	ctx.outfile = safe_strdup(output_arg);

	ndevices = argc - optind;

	if (multiple) {
		if (output_arg == NULL) {
			fprintf(stderr,
				"Option --multiple requires --output-file\n");
			goto out;
		}

		if (ndevices <= 1) {
			fprintf(stderr,
				"Option --multiple requires all device nodes on the commandline\n");
			goto out;
		}

		for (int i = ndevices; i > 0; i -= 1) {
			char *devnode = safe_strdup(argv[optind + i - 1]);

			if (!init_device(&ctx, devnode))
				goto out;
		}
	} else {
		char *path;

		if (ndevices > 1) {
			fprintf(stderr, "More than one device, do you want --multiple?\n");
			goto out;
		}

		path = ndevices <= 0 ? select_device() : safe_strdup(argv[optind++]);
		if (path == NULL) {
			fprintf(stderr, "Invalid device path\n");
			goto out;
		}

		if (!init_device(&ctx, path))
			goto out;
	}

	rc = mainloop(&ctx);
out:
	list_for_each_safe(d, tmp, &ctx.devices, link) {
		free(d->devnode);
		libevdev_free(d->evdev);
	}

	return rc;
}
