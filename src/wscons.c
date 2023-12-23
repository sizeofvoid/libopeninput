/*
 * Copyright © 2015 Martin Pieuchot <mpi@openbsd.org>
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

#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>


#include "libinput.h"
#include "wscons.h"
#include "input-event-codes.h"
#include "libinput-util.h"
#include "libinput-private.h"

static const char default_seat[] = "seat0";
static const char default_seat_name[] = "default";

static int old_value = -1;

static int
udev_input_enable(struct libinput *libinput)
{
	return 0;
}

static void
udev_input_disable(struct libinput *libinput)
{
}

static void
udev_input_destroy(struct libinput *libinput)
{
}

static int
udev_device_change_seat(struct libinput_device *device,
			const char *seat_name)
{
	return 0;
}

static const struct libinput_interface_backend interface_backend = {
	.resume = udev_input_enable,
	.suspend = udev_input_disable,
	.destroy = udev_input_destroy,
	.device_change_seat = udev_device_change_seat,
};

static void
wscons_process(struct libinput_device *device, struct wscons_event *wsevent)
{
	enum libinput_button_state bstate;
	enum libinput_key_state kstate;
	struct normalized_coords accel;
	struct device_float_coords raw;
	uint64_t time;
	int button, key;

	time = s2us(wsevent->time.tv_sec) + ns2us(wsevent->time.tv_nsec);

	switch (wsevent->type) {
	case WSCONS_EVENT_KEY_UP:
	case WSCONS_EVENT_KEY_DOWN:
		key = wsevent->value;
		if (wsevent->type == WSCONS_EVENT_KEY_UP) {
			kstate = LIBINPUT_KEY_STATE_RELEASED;
			old_value = -1;
		} else {
			kstate = LIBINPUT_KEY_STATE_PRESSED;
			/* ignore auto-repeat */
			if (key == old_value)
				return;
			old_value = key;
		}
		keyboard_notify_key(device, time,
		    wskey_transcode(wscons_device(device)->scanCodeMap, key), kstate);
		break;

	case WSCONS_EVENT_MOUSE_UP:
	case WSCONS_EVENT_MOUSE_DOWN:
		/* button to Linux events */
		switch (wsevent->value) {
		case 1:
			button = BTN_MIDDLE;
			break;
		case 2:
			button = BTN_RIGHT;
			break;
		default:
			button = wsevent->value + BTN_LEFT;
			break;
		}
		if (wsevent->type == WSCONS_EVENT_MOUSE_UP)
			bstate = LIBINPUT_BUTTON_STATE_RELEASED;
		else
			bstate = LIBINPUT_BUTTON_STATE_PRESSED;
		pointer_notify_button(device, time, button, bstate);
		break;

	case WSCONS_EVENT_MOUSE_DELTA_X:
	case WSCONS_EVENT_MOUSE_DELTA_Y:
		memset(&raw, 0, sizeof(raw));
		memset(&accel, 0, sizeof(accel));

		if (wsevent->type == WSCONS_EVENT_MOUSE_DELTA_X)
			accel.x = wsevent->value;
		else
			accel.y = -wsevent->value;

		pointer_notify_motion(device, time, &accel, &raw);
		break;

	case WSCONS_EVENT_MOUSE_DELTA_Z:
		memset(&raw, 0, sizeof(raw));
		accel.x = 0;
		accel.y = wsevent->value * 32;
		axis_notify_event(device, time, &accel, &raw);
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_X:
	case WSCONS_EVENT_MOUSE_ABSOLUTE_Y:
		//return LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE;
		break;

	case WSCONS_EVENT_HSCROLL:
		memset(&raw, 0, sizeof(raw));
		accel.x = wsevent->value/8;
		accel.y = 0;
		axis_notify_event(device, time, &accel, &raw);
		break;
	case WSCONS_EVENT_VSCROLL:
		memset(&raw, 0, sizeof(raw));
		accel.x = 0;
		accel.y = wsevent->value/8;
		axis_notify_event(device, time, &accel, &raw);
		break;

	case WSCONS_EVENT_SYNC:
		break;

	case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
	case WSCONS_EVENT_MOUSE_ABSOLUTE_W:
	case WSCONS_EVENT_TOUCH_WIDTH:
	case WSCONS_EVENT_TOUCH_RESET:
		/* ignore those */
		break;
	default:
		fprintf(stderr, "unkown event: %x\n" , wsevent->type);
		/* assert(1 == 0); */
		break;
	}
}

static void
wscons_device_dispatch(void *data)
{
	struct libinput_device *device = data;
	struct wscons_event wsevents[32];
	ssize_t len;
	int count, i;

	len = read(device->fd, wsevents, sizeof(struct wscons_event));
	if (len <= 0 || (len % sizeof(struct wscons_event)) != 0)
		return;

	count = len / sizeof(struct wscons_event);
	for (i = 0; i < count; i++) {
		wscons_process(device, &wsevents[i]);
	}
}

static void
udev_seat_destroy(struct libinput_seat *seat)
{
	struct udev_seat *useat = (struct udev_seat*)seat;
	free(useat);
}

static struct libinput_seat*
wscons_seat_get(struct libinput *libinput, const char *seat_name_physical,
	const char *seat_name_logical)
{
	struct libinput_seat *seat;

	list_for_each(seat, &libinput->seat_list, link) {
		if (streq(seat->physical_name, seat_name_physical) &&
		    streq(seat->logical_name, seat_name_logical)) {
			libinput_seat_ref(seat);
			return seat;
		}
	}

	seat = calloc(1, sizeof(*seat));
	if (seat == NULL)
		return NULL;

	libinput_seat_init(seat, libinput, seat_name_physical,
		seat_name_logical, udev_seat_destroy);

	return seat;
}

LIBINPUT_EXPORT struct libinput *
libinput_udev_create_context(const struct libinput_interface *interface,
			     void *user_data,
			     struct udev *udev)
{
	struct libinput *libinput;

	libinput = calloc(1, sizeof(*libinput));
	if (libinput == NULL)
		return NULL;

	if (libinput_init(libinput, interface, &interface_backend, user_data) != 0) {
		free(libinput);
		return NULL;
	}
	return libinput;
}

LIBINPUT_EXPORT int
libinput_udev_assign_seat(struct libinput *libinput, const char *seat_id)
{

	struct libinput_seat *seat;
	struct libinput_device *device;
	uint64_t time;
	struct timespec ts;
	struct libinput_event *event;

	/* Add standard muxes */
	libinput_path_add_device(libinput, "/dev/wskbd");
	libinput_path_add_device(libinput, "/dev/wsmouse");

	seat = wscons_seat_get(libinput, default_seat, default_seat_name);
	list_for_each(device, &seat->devices_list, link) {
		clock_gettime(CLOCK_REALTIME, &ts);
		time = s2us(ts.tv_sec) + ns2us(ts.tv_nsec);
		event = calloc(1, sizeof(*event));
		post_device_event(device, time, LIBINPUT_EVENT_DEVICE_ADDED,
		    event);
	}
	return 0;
}

LIBINPUT_EXPORT struct libinput *
libinput_path_create_context(const struct libinput_interface *interface,
     void *user_data)
{
	struct libinput *libinput;

	libinput = calloc(1, sizeof(*libinput));
	if (libinput == NULL)
		return NULL;

	if (libinput_init(libinput, interface, &interface_backend, user_data) != 0) {
		free(libinput);
		return NULL;
	}

	return libinput;
}

static int
wscons_device_init(struct wscons_device *wscons_device)
{
	struct libinput_device *device = &wscons_device->base;

	if (strncmp(device->devname, "/dev/wsmouse", 12) == 0) {
		/* XXX handle tablets and touchpanel */
		wscons_device->capability = LIBINPUT_DEVICE_CAP_POINTER;
	} else if (strncmp(device->devname, "/dev/wskbd", 10) == 0)  {
		wscons_device->capability = LIBINPUT_DEVICE_CAP_KEYBOARD;
		if (wscons_keyboard_init(wscons_device) == -1)
			return -1;
	}
	return 0;
}

LIBINPUT_EXPORT struct libinput_device *
libinput_path_add_device(struct libinput *libinput,
	const char *path)
{
	struct libinput_seat *seat = NULL;
	struct libinput_device *device = NULL;
	struct wscons_device *wscons_device;
	int fd;

	fd = open_restricted(libinput, path,
			     O_RDWR | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		log_info(libinput,
			 "opening input device '%s' failed (%s).\n",
			 path, strerror(-fd));
		return NULL;
	}

	wscons_device = calloc(1, sizeof(*wscons_device));
	if (wscons_device == NULL)
		return NULL;

	/* Only one (default) seat is supported. */
	seat = wscons_seat_get(libinput, default_seat, default_seat_name);
	if (seat == NULL)
		goto err;

	device = &wscons_device->base;
	libinput_device_init(device, seat);

	device->fd = fd;
	device->devname = strdup(path);
	if (device->devname == NULL)
		goto err;

	device->source =
		libinput_add_fd(libinput, fd, wscons_device_dispatch, device);
	if (!device->source)
		goto err;

	if (wscons_device_init(wscons_device) == -1)
		goto err;
	list_insert(&seat->devices_list, &device->link);

	return device;

err:
	if (device != NULL)
		close_restricted(libinput, device->fd);
	free(wscons_device);
	return NULL;
}

LIBINPUT_EXPORT void
libinput_path_remove_device(struct libinput_device *device)
{
	struct libinput *libinput = device->seat->libinput;

	libinput_remove_source(libinput, device->source);
	device->source = NULL;

	close_restricted(libinput, device->fd);
	device->fd = -1;

	libinput_device_unref(device);
}
