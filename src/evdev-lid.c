/*
 * Copyright © 2017 James Ye <jye836@gmail.com>
 * Copyright © 2017 Red Hat, Inc.
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

#include "libinput.h"
#include "evdev.h"
#include "libinput-private.h"

struct lid_switch_dispatch {
	struct evdev_dispatch base;
	struct evdev_device *device;
	enum switch_reliability reliability;

	bool lid_is_closed;

	struct {
		struct evdev_device *keyboard;
		struct libinput_event_listener listener;
	} keyboard;
};

static inline struct lid_switch_dispatch*
lid_dispatch(struct evdev_dispatch *dispatch)
{
	struct lid_switch_dispatch *l;

	evdev_verify_dispatch_type(dispatch, DISPATCH_LID_SWITCH);

	return container_of(dispatch, l, base);
}

static void
lid_switch_keyboard_event(uint64_t time,
			  struct libinput_event *event,
			  void *data)
{
	struct lid_switch_dispatch *dispatch = lid_dispatch(data);

	if (!dispatch->lid_is_closed)
		return;

	if (event->type != LIBINPUT_EVENT_KEYBOARD_KEY)
		return;

	if (dispatch->reliability == RELIABILITY_WRITE_OPEN) {
		int fd = libevdev_get_fd(dispatch->device->evdev);
		struct input_event ev[2] = {
			{{ 0, 0 }, EV_SW, SW_LID, 0 },
			{{ 0, 0 }, EV_SYN, SYN_REPORT, 0 },
		};

		(void)write(fd, ev, sizeof(ev));
		/* In case write() fails, we sync the lid state manually
		 * regardless. */
	}

	dispatch->lid_is_closed = false;
	switch_notify_toggle(&dispatch->device->base,
			     time,
			     LIBINPUT_SWITCH_LID,
			     dispatch->lid_is_closed);
}

static void
lid_switch_toggle_keyboard_listener(struct lid_switch_dispatch *dispatch,
				    bool is_closed)
{
	if (!dispatch->keyboard.keyboard)
		return;

	if (is_closed) {
		libinput_device_add_event_listener(
					   &dispatch->keyboard.keyboard->base,
					   &dispatch->keyboard.listener,
					   lid_switch_keyboard_event,
					   dispatch);
	} else {
		libinput_device_remove_event_listener(
						      &dispatch->keyboard.listener);
	}
}

static void
lid_switch_process_switch(struct lid_switch_dispatch *dispatch,
			  struct evdev_device *device,
			  struct input_event *e,
			  uint64_t time)
{
	bool is_closed;

	switch (e->code) {
	case SW_LID:
		is_closed = !!e->value;

		if (dispatch->lid_is_closed == is_closed)
			return;

		lid_switch_toggle_keyboard_listener(dispatch,
						    is_closed);

		dispatch->lid_is_closed = is_closed;

		switch_notify_toggle(&device->base,
				     time,
				     LIBINPUT_SWITCH_LID,
				     dispatch->lid_is_closed);
		break;
	}
}

static void
lid_switch_process(struct evdev_dispatch *evdev_dispatch,
		   struct evdev_device *device,
		   struct input_event *event,
		   uint64_t time)
{
	struct lid_switch_dispatch *dispatch = lid_dispatch(evdev_dispatch);

	switch (event->type) {
	case EV_SW:
		lid_switch_process_switch(dispatch, device, event, time);
		break;
	case EV_SYN:
		break;
	default:
		assert(0 && "Unknown event type");
		break;
	}
}

static inline enum switch_reliability
evdev_read_switch_reliability_prop(struct evdev_device *device)
{
	const char *prop;
	enum switch_reliability r;

	prop = udev_device_get_property_value(device->udev_device,
					      "LIBINPUT_ATTR_LID_SWITCH_RELIABILITY");
	if (!parse_switch_reliability_property(prop, &r)) {
		evdev_log_error(device,
				"%s: switch reliability set to unknown value '%s'\n",
				device->devname,
				prop);
		r =  RELIABILITY_UNKNOWN;
	} else if (r == RELIABILITY_WRITE_OPEN) {
		evdev_log_info(device,
			       "%s: will write switch open events\n",
			       device->devname);
	}

	return r;
}

static void
lid_switch_destroy(struct evdev_dispatch *evdev_dispatch)
{
	struct lid_switch_dispatch *dispatch = lid_dispatch(evdev_dispatch);

	free(dispatch);
}

static void
lid_switch_pair_keyboard(struct evdev_device *lid_switch,
			 struct evdev_device *keyboard)
{
	struct lid_switch_dispatch *dispatch =
		lid_dispatch(lid_switch->dispatch);
	unsigned int bus_kbd = libevdev_get_id_bustype(keyboard->evdev);

	if ((keyboard->tags & EVDEV_TAG_KEYBOARD) == 0)
		return;

	/* If we already have a keyboard paired, override it if the new one
	 * is a serio device. Otherwise keep the current one */
	if (dispatch->keyboard.keyboard) {
		if (bus_kbd != BUS_I8042)
			return;
		libinput_device_remove_event_listener(&dispatch->keyboard.listener);
	}

	dispatch->keyboard.keyboard = keyboard;
	evdev_log_debug(lid_switch,
			"lid: keyboard paired with %s<->%s\n",
			lid_switch->devname,
			keyboard->devname);

	/* We don't init the event listener yet - we don't care about
	 * keyboard events until the lid is closed */
}

static void
lid_switch_interface_device_added(struct evdev_device *device,
				  struct evdev_device *added_device)
{
	lid_switch_pair_keyboard(device, added_device);
}

static void
lid_switch_interface_device_removed(struct evdev_device *device,
				    struct evdev_device *removed_device)
{
	struct lid_switch_dispatch *dispatch = lid_dispatch(device->dispatch);

	if (removed_device == dispatch->keyboard.keyboard) {
		libinput_device_remove_event_listener(
				      &dispatch->keyboard.listener);
		dispatch->keyboard.keyboard = NULL;
	}
}

static void
lid_switch_sync_initial_state(struct evdev_device *device,
			      struct evdev_dispatch *evdev_dispatch)
{
	struct lid_switch_dispatch *dispatch = lid_dispatch(device->dispatch);
	struct libevdev *evdev = device->evdev;
	bool is_closed = false;

	dispatch->reliability = evdev_read_switch_reliability_prop(device);

	/* For the initial state sync, we depend on whether the lid switch
	 * is reliable. If we know it's reliable, we sync as expected.
	 * If we're not sure, we ignore the initial state and only sync on
	 * the first future lid close event. Laptops with a broken switch
	 * that always have the switch in 'on' state thus don't mess up our
	 * touchpad.
	 */
	switch(dispatch->reliability) {
	case RELIABILITY_UNKNOWN:
	case RELIABILITY_WRITE_OPEN:
		is_closed = false;
		break;
	case RELIABILITY_RELIABLE:
		is_closed = libevdev_get_event_value(evdev, EV_SW, SW_LID);
		break;
	}

	dispatch->lid_is_closed = is_closed;
	if (dispatch->lid_is_closed) {
		uint64_t time;
		time = libinput_now(evdev_libinput_context(device));
		switch_notify_toggle(&device->base,
				     time,
				     LIBINPUT_SWITCH_LID,
				     LIBINPUT_SWITCH_STATE_ON);
	}
}

struct evdev_dispatch_interface lid_switch_interface = {
	lid_switch_process,
	NULL, /* suspend */
	NULL, /* remove */
	lid_switch_destroy,
	lid_switch_interface_device_added,
	lid_switch_interface_device_removed,
	lid_switch_interface_device_removed, /* device_suspended, treat as remove */
	lid_switch_interface_device_added,   /* device_resumed, treat as add */
	lid_switch_sync_initial_state,
	NULL, /* toggle_touch */
};

struct evdev_dispatch *
evdev_lid_switch_dispatch_create(struct evdev_device *lid_device)
{
	struct lid_switch_dispatch *dispatch = zalloc(sizeof *dispatch);

	if (dispatch == NULL)
		return NULL;

	dispatch->base.dispatch_type = DISPATCH_LID_SWITCH;
	dispatch->base.interface = &lid_switch_interface;
	dispatch->device = lid_device;
	libinput_device_init_event_listener(&dispatch->keyboard.listener);

	evdev_init_sendevents(lid_device, &dispatch->base);

	dispatch->lid_is_closed = false;

	return &dispatch->base;
}
