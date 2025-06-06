/*
 * Copyright © 2025 Red Hat, Inc.
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

#include "util-mem.h"
#include <mtdev-plumbing.h>

#include "evdev.h"
#include "evdev-plugin.h"

_unused_
static inline void
evdev_print_event(struct evdev_device *device,
		  const struct evdev_event *e,
		  uint64_t time_in_us)
{
	static uint32_t offset = 0;
	static uint32_t last_time = 0;
	uint32_t time = us2ms(time_in_us);

	if (offset == 0) {
		offset = time;
		last_time = time - offset;
	}

	time -= offset;

	switch (evdev_usage_enum(e->usage)) {
	case EVDEV_SYN_REPORT:
		evdev_log_debug(device,
			  "%u.%03u ----------------- EV_SYN ----------------- +%ums\n",
			  time / 1000,
			  time % 1000,
			  time - last_time);

		last_time = time;
		break;
	case EVDEV_MSC_SERIAL:
		evdev_log_debug(device,
			"%u.%03u %-16s %-16s %#010x\n",
			time / 1000,
			time % 1000,
			evdev_event_get_type_name(e),
			evdev_event_get_code_name(e),
			e->value);
		break;
	default:
		evdev_log_debug(device,
			  "%u.%03u %-16s %-20s %4d\n",
			  time / 1000,
			  time % 1000,
			  evdev_event_get_type_name(e),
			  evdev_event_get_code_name(e),
			  e->value);
		break;
	}
}

static inline void
evdev_process_event(struct evdev_device *device,
		    struct evdev_event *e,
		    uint64_t time)
{
	struct evdev_dispatch *dispatch = device->dispatch;

#if EVENT_DEBUGGING
	evdev_print_event(device, e, time);
#endif

	libinput_timer_flush(evdev_libinput_context(device), time);

	dispatch->interface->process(dispatch, device, e, time);
}

static inline void
evdev_device_dispatch_one(struct libinput_plugin *plugin,
			  struct libinput_device *libinput_device,
			  struct evdev_frame *frame)
{
	struct evdev_device *device = evdev_device(libinput_device);
	uint64_t time = evdev_frame_get_time(frame);

	size_t nevents;
	struct evdev_event *events = evdev_frame_get_events(frame, &nevents);
	for (size_t i = 0; i < nevents; i++) {
		struct evdev_event *ev = &events[i];
		if (!device->mtdev) {
			evdev_process_event(device, ev, time);
		} else {
			struct input_event e = evdev_event_to_input_event(ev, time);
			mtdev_put_event(device->mtdev, &e);
			if (evdev_usage_eq(ev->usage, EVDEV_SYN_REPORT)) {
				while (!mtdev_empty(device->mtdev)) {
					struct input_event e;

					mtdev_get_event(device->mtdev, &e);

					uint64_t time;
					struct evdev_event ev = evdev_event_from_input_event(&e, &time);
					evdev_process_event(device, &ev, time);
				}
			}
		}
	}

	/* Discard event to make the plugin system aware we're done */
	evdev_frame_reset(frame);
}

static const struct libinput_plugin_interface interface = {
	.run = NULL,
	.destroy = NULL,
	.device_new = NULL,
	.device_ignored = NULL,
	.device_added = NULL,
	.device_removed = NULL,
	.evdev_frame = evdev_device_dispatch_one,
};

void
libinput_evdev_dispatch_plugin(struct libinput *libinput)
{
	_unref_(libinput_plugin) *p = libinput_plugin_new(libinput,
							  "evdev",
							  &interface,
							  NULL);
}
