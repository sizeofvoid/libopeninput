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
#include "filter.h"
#include "wscons.h"
#include "input-event-codes.h"
#include "libinput-util.h"
#include "libinput-private.h"

static const char default_seat[] = "seat0";
static const char default_seat_name[] = "default";

static struct libinput_seat* wscons_seat_get(struct libinput *, const char *,
    const char *);
static void wscons_device_dispatch(void *);

static void
wscons_device_init_pointer_acceleration(struct wscons_device *device,
              struct motion_filter *filter);

static int
udev_input_enable(struct libinput *libinput)
{
	struct libinput_seat *seat;
	struct libinput_device *device;

	seat = wscons_seat_get(libinput, default_seat, default_seat_name);
	list_for_each(device, &seat->devices_list, link) {
		device->fd = open_restricted(libinput, device->devname, O_RDWR);
		device->source =
		    libinput_add_fd(libinput, device->fd,
			wscons_device_dispatch, device);
		if (!device->source) {
			return -ENOMEM;
		}
	}
	return 0;
}

static void
udev_input_disable(struct libinput *libinput)
{
	struct libinput_seat *seat;
	struct libinput_device *device;

	seat = wscons_seat_get(libinput, default_seat, default_seat_name);
	list_for_each(device, &seat->devices_list, link) {
		if (device->source) {
			libinput_remove_source(libinput, device->source);
			device->source = NULL;
		}
		close_restricted(libinput, device->fd);
	}
}

static void
udev_input_destroy(struct libinput *libinput)
{
	struct libinput_seat *seat;
	struct libinput_device *device;

	fprintf(stderr, "%s", __func__);
	seat = wscons_seat_get(libinput, default_seat, default_seat_name);
	list_for_each(device, &seat->devices_list, link) {
		close_restricted(libinput, device->fd);
	}
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
	struct wscons_device *dev = wscons_device(device);
	uint64_t time;
	int button, key;

	time = s2us(wsevent->time.tv_sec) + ns2us(wsevent->time.tv_nsec);

	switch (wsevent->type) {
	case WSCONS_EVENT_KEY_UP:
	case WSCONS_EVENT_KEY_DOWN:
		key = wsevent->value;
		if (wsevent->type == WSCONS_EVENT_KEY_UP) {
			kstate = LIBINPUT_KEY_STATE_RELEASED;
			dev->old_value = -1;
		} else {
			kstate = LIBINPUT_KEY_STATE_PRESSED;
			/* ignore auto-repeat */
			if (key == dev->old_value)
				return;
			dev->old_value = key;
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
			raw.x = wsevent->value;
		else
			raw.y = -wsevent->value;

		if (dev->pointer.filter) {
			accel = filter_dispatch(dev->pointer.filter,
			                        &raw,
			                	device,
			                	time);
		} else {
			accel.x = raw.x;
			accel.y = raw.y;
		}

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

#ifdef WSCONS_EVENT_SYNC
	case WSCONS_EVENT_SYNC:
		break;
#endif

	case WSCONS_EVENT_MOUSE_ABSOLUTE_Z:
	case WSCONS_EVENT_MOUSE_ABSOLUTE_W:
#ifdef WSCONS_EVENT_TOUCH_WIDTH
	case WSCONS_EVENT_TOUCH_WIDTH:
#endif
#ifdef WSCONS_EVENT_TOUCH_RESET
	case WSCONS_EVENT_TOUCH_RESET:
#endif
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

	/* Add standard devices */
	for (int i = 0; i < 10; i++) {
		char name[32];
		int fd;
		snprintf(name, sizeof(name), "/dev/wskbd%d", i);
		if ((fd = open_restricted(libinput, name, O_RDWR|O_NONBLOCK)) >= 0) {
			close_restricted(libinput, fd);
			libinput_path_add_device(libinput, name);
		}
		snprintf(name, sizeof(name), "/dev/wsmouse%d", i);
		if ((fd = open_restricted(libinput, name, O_RDWR|O_NONBLOCK)) >= 0) {
			close_restricted(libinput, fd);
			libinput_path_add_device(libinput, name);
		}
	}

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

static double
wscons_accel_config_get_speed(struct libinput_device *device)
{
	struct wscons_device *dev = wscons_device(device);

	return filter_get_speed(dev->pointer.filter);
}

static enum libinput_config_status
wscons_accel_config_set_speed(struct libinput_device *device, double speed)
{
	struct wscons_device *dev = wscons_device(device);

	if (!filter_set_speed(dev->pointer.filter, speed))
		return LIBINPUT_CONFIG_STATUS_INVALID;

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static double
wscons_accel_config_get_default_speed(struct libinput_device *device)
{
	return 0.0;
}

static int
wscons_accel_config_available(struct libinput_device *device)
{
	/* this function is only called if we set up ptraccel, so we can
	   reply with a resounding "Yes" */
	return 1;
}

static enum libinput_config_accel_profile
wscons_accel_config_get_profile(struct libinput_device *libinput_device)
{
	struct wscons_device *device = wscons_device(libinput_device);

	return filter_get_type(device->pointer.filter);
}

static inline bool
wscons_init_accel(struct wscons_device *device,
		 enum libinput_config_accel_profile which)
{
	struct motion_filter *filter = NULL;

	if (which == LIBINPUT_CONFIG_ACCEL_PROFILE_CUSTOM) {
		filter = create_custom_accelerator_filter();
	} else {
		if (which == LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT)
			filter = create_pointer_accelerator_filter_flat(DEFAULT_MOUSE_DPI);
		else
			filter = create_pointer_accelerator_filter_linear_low_dpi(DEFAULT_MOUSE_DPI,
										  true);
	}

	if (!filter)
		filter = create_pointer_accelerator_filter_linear(DEFAULT_MOUSE_DPI,
								  true);

	if (!filter)
		return false;

	wscons_device_init_pointer_acceleration(device, filter);

	return true;
}

static enum libinput_config_status
wscons_accel_config_set_profile(struct libinput_device *libinput_device,
			       enum libinput_config_accel_profile profile)
{
	struct wscons_device *device = wscons_device(libinput_device);
	struct motion_filter *filter;
	double speed;

	filter = device->pointer.filter;
	if (filter_get_type(filter) == profile)
		return LIBINPUT_CONFIG_STATUS_SUCCESS;

	speed = filter_get_speed(filter);
	device->pointer.filter = NULL;

	if (wscons_init_accel(device, profile)) {
		wscons_accel_config_set_speed(libinput_device, speed);
		filter_destroy(filter);
	} else {
		device->pointer.filter = filter;
		return LIBINPUT_CONFIG_STATUS_UNSUPPORTED;
	}

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static uint32_t
wscons_accel_config_get_profiles(struct libinput_device *libinput_device)
{
	struct wscons_device *device = wscons_device(libinput_device);

	if (!device->pointer.filter)
		return LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;

	return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE |
	       LIBINPUT_CONFIG_ACCEL_PROFILE_FLAT |
	       LIBINPUT_CONFIG_ACCEL_PROFILE_CUSTOM;
}

static enum libinput_config_accel_profile
wscons_accel_config_get_default_profile(struct libinput_device *libinput_device)
{
	struct wscons_device *device = wscons_device(libinput_device);

	if (!device->pointer.filter)
		return LIBINPUT_CONFIG_ACCEL_PROFILE_NONE;

	/* No device has a flat profile as default */
	return LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE;
}

static enum libinput_config_status
wscons_set_accel_config(struct libinput_device *libinput_device,
		       struct libinput_config_accel *accel_config)
{
	assert(wscons_accel_config_get_profile(libinput_device) == accel_config->profile);

	struct wscons_device *dev = wscons_device(libinput_device);

	if (!filter_set_accel_config(dev->pointer.filter, accel_config))
		return LIBINPUT_CONFIG_STATUS_INVALID;

	return LIBINPUT_CONFIG_STATUS_SUCCESS;
}

static void
wscons_device_init_pointer_acceleration(struct wscons_device *device,
				       struct motion_filter *filter)
{
	device->pointer.filter = filter;

	if (device->base.config.accel == NULL) {
		double default_speed;

		device->pointer.config.available = wscons_accel_config_available;
		device->pointer.config.set_speed = wscons_accel_config_set_speed;
		device->pointer.config.get_speed = wscons_accel_config_get_speed;
		device->pointer.config.get_default_speed = wscons_accel_config_get_default_speed;
		device->pointer.config.get_profiles = wscons_accel_config_get_profiles;
		device->pointer.config.set_profile = wscons_accel_config_set_profile;
		device->pointer.config.get_profile = wscons_accel_config_get_profile;
		device->pointer.config.get_default_profile = wscons_accel_config_get_default_profile;
		device->pointer.config.set_accel_config = wscons_set_accel_config;
		device->base.config.accel = &device->pointer.config;

		default_speed = wscons_accel_config_get_default_speed(&device->base);
		wscons_accel_config_set_speed(&device->base, default_speed);
	}
}

static int
wscons_device_init(struct wscons_device *wscons_device)
{
	struct libinput_device *device = &wscons_device->base;

	wscons_device->old_value = -1;

	if (strncmp(device->devname, "/dev/wsmouse", 12) == 0) {
		/* XXX handle tablets and touchpanel */
		wscons_device->capability = LIBINPUT_DEVICE_CAP_POINTER;
		wscons_init_accel(wscons_device, LIBINPUT_CONFIG_ACCEL_PROFILE_ADAPTIVE);
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
