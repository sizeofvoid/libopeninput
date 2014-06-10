/*
 * Copyright © 2014 Red Hat, Inc.
 * Copyright © 2014 Stephen Chandler "Lyude" Paul
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include "config.h"
#include "evdev-tablet.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#define tablet_set_status(tablet_,s_) ((tablet_)->status |= (s_))
#define tablet_unset_status(tablet_,s_) ((tablet_)->status &= ~(s_))
#define tablet_has_status(tablet_,s_) (!!((tablet_)->status & (s_)))

static void
tablet_process_absolute(struct tablet_dispatch *tablet,
			struct evdev_device *device,
			struct input_event *e,
			uint32_t time)
{
	enum libinput_tablet_axis axis;

	switch (e->code) {
	case ABS_X:
	case ABS_Y:
		axis = evcode_to_axis(e->code);
		if (axis == LIBINPUT_TABLET_AXIS_NONE) {
			log_bug_libinput("Invalid ABS event code %#x\n",
					 e->code);
			break;
		}

		set_bit(tablet->changed_axes, axis);
		tablet_set_status(tablet, TABLET_AXES_UPDATED);
		break;
	default:
		log_info("Unhandled ABS event code %#x\n", e->code);
		break;
	}
}

static void
tablet_update_tool(struct tablet_dispatch *tablet,
		   enum libinput_tool_type tool,
		   bool enabled)
{
	assert(tool != LIBINPUT_TOOL_NONE);

	if (enabled && tool != tablet->current_tool_type) {
		tablet->current_tool_type = tool;
		tablet_set_status(tablet, TABLET_TOOL_UPDATED);
	}
}

static void
tablet_check_notify_axes(struct tablet_dispatch *tablet,
			 struct evdev_device *device,
			 uint32_t time)
{
	struct libinput_device *base = &device->base;
	bool axis_update_needed = false;
	int a;

	for (a = 0; a < LIBINPUT_TABLET_AXIS_CNT; a++) {
		const struct input_absinfo *absinfo;

		if (!bit_is_set(tablet->changed_axes, a))
			continue;

		absinfo = libevdev_get_abs_info(device->evdev,
						axis_to_evcode(a));

		switch (a) {
		case LIBINPUT_TABLET_AXIS_X:
		case LIBINPUT_TABLET_AXIS_Y:
			tablet->axes[a] = absinfo->value;
			break;
		default:
			log_bug_libinput("Invalid axis update: %d\n", a);
			break;
		}

		axis_update_needed = true;
	}

	if (axis_update_needed) {
		tablet_notify_axis(base, time, tablet->changed_axes, tablet->axes);
		memset(tablet->changed_axes, 0, sizeof(tablet->changed_axes));
	}
}

static void
tablet_process_key(struct tablet_dispatch *tablet,
		   struct evdev_device *device,
		   struct input_event *e,
		   uint32_t time)
{
	switch (e->code) {
	case BTN_TOOL_PEN:
	case BTN_TOOL_RUBBER:
	case BTN_TOOL_BRUSH:
	case BTN_TOOL_PENCIL:
	case BTN_TOOL_AIRBRUSH:
	case BTN_TOOL_FINGER:
	case BTN_TOOL_MOUSE:
	case BTN_TOOL_LENS:
		/* These codes have an equivalent libinput_tool value */
		tablet_update_tool(tablet, e->code, e->value);
		break;
	default:
		break;
	}
}

static void
tablet_process_misc(struct tablet_dispatch *tablet,
		    struct evdev_device *device,
		    struct input_event *e,
		    uint32_t time)
{
	switch (e->code) {
	case MSC_SERIAL:
		if (e->value != (signed)tablet->current_tool_serial &&
		    e->value != -1) {
			tablet->current_tool_serial = e->value;
			tablet_set_status(tablet, TABLET_TOOL_UPDATED);
		}
		break;
	default:
		log_info("Unhandled MSC event code %#x\n", e->code);
		break;
	}
}

static void
tablet_notify_tool(struct tablet_dispatch *tablet,
		   struct evdev_device *device,
		   uint32_t time)
{
	struct libinput_device *base = &device->base;
	struct libinput_tool *tool;
	struct libinput_tool *new_tool = NULL;

	/* Check if we already have the tool in our list of tools */
	list_for_each(tool, &base->seat->libinput->tool_list, link) {
		if (tablet->current_tool_type == tool->type &&
		    tablet->current_tool_serial == tool->serial) {
			new_tool = tool;
			break;
		}
	}

	/* If we didn't already have the tool in our list of tools, add it */
	if (new_tool == NULL) {
		new_tool = zalloc(sizeof *new_tool);
		*new_tool = (struct libinput_tool) {
			.type = tablet->current_tool_type,
			.serial = tablet->current_tool_serial,
			.refcount = 1,
		};

		list_insert(&base->seat->libinput->tool_list, &new_tool->link);
	}

	tablet_notify_tool_update(base, time, new_tool);
}

static void
tablet_flush(struct tablet_dispatch *tablet,
	     struct evdev_device *device,
	     uint32_t time)
{
	if (tablet_has_status(tablet, TABLET_TOOL_UPDATED))
		tablet_notify_tool(tablet, device, time);

	if (tablet_has_status(tablet, TABLET_AXES_UPDATED)) {
		tablet_check_notify_axes(tablet, device, time);
		tablet_unset_status(tablet, TABLET_AXES_UPDATED);
	}
}

static void
tablet_process(struct evdev_dispatch *dispatch,
	       struct evdev_device *device,
	       struct input_event *e,
	       uint64_t time)
{
	struct tablet_dispatch *tablet =
		(struct tablet_dispatch *)dispatch;

	switch (e->type) {
	case EV_ABS:
		tablet_process_absolute(tablet, device, e, time);
		break;
	case EV_KEY:
		tablet_process_key(tablet, device, e, time);
		break;
	case EV_MSC:
		tablet_process_misc(tablet, device, e, time);
		break;
	case EV_SYN:
		tablet_flush(tablet, device, time);
		break;
	default:
		log_error("Unexpected event type %#x\n", e->type);
		break;
	}
}

static void
tablet_destroy(struct evdev_dispatch *dispatch)
{
	struct tablet_dispatch *tablet =
		(struct tablet_dispatch*)dispatch;

	free(tablet);
}

static struct evdev_dispatch_interface tablet_interface = {
	tablet_process,
	tablet_destroy
};

static int
tablet_init(struct tablet_dispatch *tablet,
	    struct evdev_device *device)
{
	enum libinput_tablet_axis a;

	tablet->base.interface = &tablet_interface;
	tablet->device = device;
	tablet->status = TABLET_NONE;
	tablet->current_tool_type = LIBINPUT_TOOL_NONE;

	/* Mark any axes the tablet has as changed */
	for (a = 0; a < LIBINPUT_TABLET_AXIS_CNT; a++) {
		if (libevdev_has_event_code(device->evdev,
					    EV_ABS,
					    axis_to_evcode(a)))
			set_bit(tablet->changed_axes, a);
	}

	return 0;
}

struct evdev_dispatch *
evdev_tablet_create(struct evdev_device *device)
{
	struct tablet_dispatch *tablet;

	tablet = zalloc(sizeof *tablet);
	if (!tablet)
		return NULL;

	if (tablet_init(tablet, device) != 0) {
		tablet_destroy(&tablet->base);
		return NULL;
	}

	return &tablet->base;
}
