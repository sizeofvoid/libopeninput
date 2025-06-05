
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

#pragma once

#include "config.h"

#include "util-mem.h"
#include "util-input-event.h"
#include "util-newtype.h"

#include <stdbool.h>
#include <linux/input.h>

#define _evbit(t_, c_) ((t_) << 16 | (c_))

/**
 * This is an enum to have the compiler help us a bit.
 * The enum doesn't need to contain all event codes, only
 * the ones we use in libinput - add to here as required.
 *
 * The order doesn't matter either since each enum value
 * is just the type | code value anyway, keep it in somewhat
 * logical groups where possible.
 */
enum evdev_usage {
	EVDEV_SYN_REPORT = _evbit(EV_SYN, SYN_REPORT),

	EVDEV_KEY_RESERVED = _evbit(EV_KEY, KEY_RESERVED),
	EVDEV_KEY_ESC= _evbit(EV_KEY, KEY_ESC),
	EVDEV_KEY_MICMUTE = _evbit(EV_KEY, KEY_MICMUTE),
	EVDEV_KEY_OK = _evbit(EV_KEY, KEY_OK),
	EVDEV_KEY_LIGHTS_TOGGLE = _evbit(EV_KEY, KEY_LIGHTS_TOGGLE),
	EVDEV_KEY_ALS_TOGGLE = _evbit(EV_KEY, KEY_ALS_TOGGLE),
	EVDEV_KEY_MAX = _evbit(EV_KEY, KEY_MAX),

	EVDEV_BTN_LEFT = _evbit(EV_KEY, BTN_LEFT),
	EVDEV_BTN_RIGHT = _evbit(EV_KEY, BTN_RIGHT),
	EVDEV_BTN_MIDDLE = _evbit(EV_KEY, BTN_MIDDLE),
	EVDEV_BTN_SIDE = _evbit(EV_KEY, BTN_SIDE),
	EVDEV_BTN_EXTRA = _evbit(EV_KEY, BTN_EXTRA),
	EVDEV_BTN_FORWARD = _evbit(EV_KEY, BTN_FORWARD),
	EVDEV_BTN_BACK = _evbit(EV_KEY, BTN_BACK),
	EVDEV_BTN_TASK = _evbit(EV_KEY, BTN_TASK),

	EVDEV_BTN_JOYSTICK = _evbit(EV_KEY, BTN_JOYSTICK),

	EVDEV_BTN_0 = _evbit(EV_KEY, BTN_0),
	EVDEV_BTN_1 = _evbit(EV_KEY, BTN_1),
	EVDEV_BTN_2 = _evbit(EV_KEY, BTN_2),

	EVDEV_BTN_STYLUS = _evbit(EV_KEY, BTN_STYLUS),
	EVDEV_BTN_STYLUS2 = _evbit(EV_KEY, BTN_STYLUS2),
	EVDEV_BTN_STYLUS3 = _evbit(EV_KEY, BTN_STYLUS3),

	EVDEV_BTN_TOUCH = _evbit(EV_KEY, BTN_TOUCH),
	EVDEV_BTN_TOOL_PEN = _evbit(EV_KEY, BTN_TOOL_PEN),
	EVDEV_BTN_TOOL_RUBBER = _evbit(EV_KEY, BTN_TOOL_RUBBER),
	EVDEV_BTN_TOOL_BRUSH = _evbit(EV_KEY, BTN_TOOL_BRUSH),
	EVDEV_BTN_TOOL_PENCIL = _evbit(EV_KEY, BTN_TOOL_PENCIL),
	EVDEV_BTN_TOOL_AIRBRUSH = _evbit(EV_KEY, BTN_TOOL_AIRBRUSH),
	EVDEV_BTN_TOOL_MOUSE = _evbit(EV_KEY, BTN_TOOL_MOUSE),
	EVDEV_BTN_TOOL_LENS = _evbit(EV_KEY, BTN_TOOL_LENS),
	EVDEV_BTN_TOOL_QUINTTAP = _evbit(EV_KEY, BTN_TOOL_QUINTTAP),
	EVDEV_BTN_TOOL_DOUBLETAP = _evbit(EV_KEY, BTN_TOOL_DOUBLETAP),
	EVDEV_BTN_TOOL_TRIPLETAP = _evbit(EV_KEY, BTN_TOOL_TRIPLETAP),
	EVDEV_BTN_TOOL_QUADTAP = _evbit(EV_KEY, BTN_TOOL_QUADTAP),
	EVDEV_BTN_TOOL_FINGER = _evbit(EV_KEY, BTN_TOOL_FINGER),
	EVDEV_BTN_MISC = _evbit(EV_KEY, BTN_MISC),
	EVDEV_BTN_GEAR_UP = _evbit(EV_KEY, BTN_GEAR_UP),
	EVDEV_BTN_DPAD_UP = _evbit(EV_KEY, BTN_DPAD_UP),
	EVDEV_BTN_DPAD_RIGHT = _evbit(EV_KEY, BTN_DPAD_RIGHT),
	EVDEV_BTN_TRIGGER_HAPPY = _evbit(EV_KEY, BTN_TRIGGER_HAPPY),
	EVDEV_BTN_TRIGGER_HAPPY40 = _evbit(EV_KEY, BTN_TRIGGER_HAPPY40),

	EVDEV_REL_X = _evbit(EV_REL, REL_X),
	EVDEV_REL_Y = _evbit(EV_REL, REL_Y),
	EVDEV_REL_WHEEL = _evbit(EV_REL, REL_WHEEL),
	EVDEV_REL_WHEEL_HI_RES = _evbit(EV_REL, REL_WHEEL_HI_RES),
	EVDEV_REL_HWHEEL = _evbit(EV_REL, REL_HWHEEL),
	EVDEV_REL_HWHEEL_HI_RES = _evbit(EV_REL, REL_HWHEEL_HI_RES),
	EVDEV_REL_DIAL = _evbit(EV_REL, REL_DIAL),
	EVDEV_REL_MAX = _evbit(EV_REL, REL_MAX),

	EVDEV_ABS_X = _evbit(EV_ABS, ABS_X),
	EVDEV_ABS_Y = _evbit(EV_ABS, ABS_Y),
	EVDEV_ABS_Z = _evbit(EV_ABS, ABS_Z),
	EVDEV_ABS_RX = _evbit(EV_ABS, ABS_RX),
	EVDEV_ABS_RY = _evbit(EV_ABS, ABS_RY),
	EVDEV_ABS_RZ = _evbit(EV_ABS, ABS_RZ),
	EVDEV_ABS_PRESSURE = _evbit(EV_ABS, ABS_PRESSURE),
	EVDEV_ABS_DISTANCE = _evbit(EV_ABS, ABS_DISTANCE),
	EVDEV_ABS_THROTTLE = _evbit(EV_ABS, ABS_THROTTLE),
	EVDEV_ABS_RUDDER = _evbit(EV_ABS, ABS_RUDDER),
	EVDEV_ABS_WHEEL = _evbit(EV_ABS, ABS_WHEEL),
	EVDEV_ABS_MISC = _evbit(EV_ABS, ABS_MISC),
	EVDEV_ABS_TILT_X = _evbit(EV_ABS, ABS_TILT_X),
	EVDEV_ABS_TILT_Y = _evbit(EV_ABS, ABS_TILT_Y),

	EVDEV_ABS_MT_SLOT = _evbit(EV_ABS, ABS_MT_SLOT),
	EVDEV_ABS_MT_POSITION_X = _evbit(EV_ABS, ABS_MT_POSITION_X),
	EVDEV_ABS_MT_POSITION_Y = _evbit(EV_ABS, ABS_MT_POSITION_Y),
	EVDEV_ABS_MT_TOOL_TYPE = _evbit(EV_ABS, ABS_MT_TOOL_TYPE),
	EVDEV_ABS_MT_TRACKING_ID = _evbit(EV_ABS, ABS_MT_TRACKING_ID),
	EVDEV_ABS_MT_TOUCH_MAJOR = _evbit(EV_ABS, ABS_MT_TOUCH_MAJOR),
	EVDEV_ABS_MT_TOUCH_MINOR = _evbit(EV_ABS, ABS_MT_TOUCH_MINOR),
	EVDEV_ABS_MT_ORIENTATION = _evbit(EV_ABS, ABS_MT_ORIENTATION),
	EVDEV_ABS_MT_PRESSURE = _evbit(EV_ABS, ABS_MT_PRESSURE),
	EVDEV_ABS_MT_DISTANCE = _evbit(EV_ABS, ABS_MT_DISTANCE),
	EVDEV_ABS_MAX = _evbit(EV_ABS, ABS_MAX),

	EVDEV_SW_LID = _evbit(EV_SW, SW_LID),
	EVDEV_SW_TABLET_MODE = _evbit(EV_SW, SW_TABLET_MODE),
	EVDEV_SW_MAX = _evbit(EV_SW, SW_MAX),

	EVDEV_MSC_SCAN = _evbit(EV_MSC, MSC_SCAN),
	EVDEV_MSC_SERIAL = _evbit(EV_MSC, MSC_SERIAL),
	EVDEV_MSC_TIMESTAMP = _evbit(EV_MSC, MSC_TIMESTAMP),
};

/**
 * Declares evdev_usage_t as uint32_t wrapper that we
 * use for passing event codes around.
 *
 * This way we can't accidentally mix up a code vs
 * type or a random integer with what needs to be a usage.
 */
DECLARE_NEWTYPE(evdev_usage, uint32_t);

static inline evdev_usage_t
evdev_usage_from(enum evdev_usage usage)
{
	return evdev_usage_from_uint32_t((uint32_t)usage);
}

static inline enum evdev_usage
evdev_usage_enum(evdev_usage_t usage)
{
	return (enum evdev_usage)evdev_usage_as_uint32_t(usage);
}

static inline evdev_usage_t
evdev_usage_from_code(unsigned int type, unsigned int code)
{
	return evdev_usage_from_uint32_t(_evbit(type, code));
}

static inline uint16_t
evdev_usage_type(evdev_usage_t usage)
{
	return evdev_usage_as_uint32_t(usage) >> 16;
}

static inline uint16_t
evdev_usage_code(evdev_usage_t usage)
{
	return evdev_usage_as_uint32_t(usage) & 0xFFFF;
}

static inline const char *
evdev_usage_code_name(evdev_usage_t usage)
{
	return libevdev_event_code_get_name(evdev_usage_type(usage),
					    evdev_usage_code(usage));
}

static inline const char *
evdev_usage_type_name(evdev_usage_t usage)
{
	return libevdev_event_type_get_name(evdev_usage_type(usage));
}

struct evdev_event {
	/* this may be a value outside the known usages above but it's just an int */
	evdev_usage_t usage;
	int32_t value;
};

static inline uint16_t
evdev_event_type(const struct evdev_event *e)
{
	return evdev_usage_type(e->usage);
}

static inline uint16_t
evdev_event_code(const struct evdev_event *e)
{
	return evdev_usage_code(e->usage);
}

static inline const char *
evdev_event_get_type_name(const struct evdev_event *e)
{
	return evdev_usage_type_name(e->usage);
}

static inline const char *
evdev_event_get_code_name(const struct evdev_event *e)
{
	return evdev_usage_code_name(e->usage);
}

static inline struct input_event
evdev_event_to_input_event(const struct evdev_event *e, uint64_t time)
{
	struct timeval tv = us2tv(time);
	return (struct input_event) {
		.type = evdev_event_type(e),
		.code = evdev_event_code(e),
		.value = e->value,
		.input_event_sec = tv.tv_sec,
		.input_event_usec = tv.tv_usec,
	};
}

static inline struct evdev_event
evdev_event_from_input_event(const struct input_event *e, uint64_t *time)
{
	if (time)
		*time = input_event_time(e);
	return (struct evdev_event) {
		.usage = evdev_usage_from_code(e->type, e->code),
		.value = e->value,
	};
}

/**
 * A wrapper around a SYN_REPORT-terminated set of input events.
 *
 * This struct always has a count of >= 1 (the SYN_REPORT)
 * and the timestamp of the SYN_REPORT is always that of the
 * most recently appended event (if nonzero)
 *
 * The event frame is of a fixed size given in
 * evdev_frame_new() and cannot be resized via helpers.
 *
 * The struct should be considered opaque, use the helpers
 * to access the various fields.
 */
struct evdev_frame {
	int refcount;
	size_t max_size;
	size_t count;
	uint64_t time;
	struct evdev_event events[];
};

static inline struct evdev_frame *
evdev_frame_ref(struct evdev_frame *frame)
{
	assert(frame->refcount > 0);
	++frame->refcount;
	return frame;
}

static inline struct evdev_frame *
evdev_frame_unref(struct evdev_frame *frame)
{
	if (frame) {
		assert(frame->refcount > 0);
		if (--frame->refcount == 0) {
			frame->max_size = 0;
			frame->count = 0;
			free(frame);
		}
	}
	return NULL;
}

DEFINE_UNREF_CLEANUP_FUNC(evdev_frame);

static inline bool
evdev_frame_is_empty(const struct evdev_frame *frame)
{
	return frame->count == 1;
}

static inline size_t
evdev_frame_get_count(const struct evdev_frame *frame)
{
	return frame->count;
}

static inline struct evdev_event *
evdev_frame_get_events(struct evdev_frame *frame, size_t *nevents)
{
	if (nevents)
		*nevents = frame->count;

	return frame->events;
}

/**
 * Set the timestamp for all events in this event frame.
 */
static inline void
evdev_frame_set_time(struct evdev_frame *frame, uint64_t time)
{
	frame->time = time;
}

static inline uint64_t
evdev_frame_get_time(const struct evdev_frame *frame)
{
	return frame->time;
}

static inline int
evdev_frame_reset(struct evdev_frame *frame)
{
	memset(frame->events, 0, frame->max_size * sizeof(*frame->events));
	frame->count = 1; /* SYN_REPORT is always there */

	return 0;
}

static inline struct evdev_frame *
evdev_frame_new(size_t max_size)
{
	struct evdev_frame *frame = zalloc(max_size * sizeof(sizeof(*frame->events)) + sizeof(*frame));

	frame->refcount = 1;
	frame->max_size = max_size;
	frame->count = 1; /* SYN_REPORT is always there */

	return frame;
}

static inline struct evdev_frame *
evdev_frame_new_on_stack(size_t max_size)
{
	assert(max_size <= 64);
	struct evdev_frame *frame = alloca(max_size * sizeof(*frame->events) + sizeof(*frame));

	frame->refcount = 1;
	frame->max_size = max_size;
	frame->count = 1; /* SYN_REPORT is always there */
	memset(frame->events, 0, max_size * sizeof(*frame->events));

	return frame;
}

/**
 * Append events to the event frame. nevents must be larger than 0
 * and specifies the number of elements in events. If any events in
 * the given events is a EV_SYN/SYN_REPORT event, that event is the last
 * one appended even if nevents states a higher number of events (roughly
 * equivalent to having a \0 inside a string).
 *
 * This function guarantees the frame is terminated with a SYN_REPORT event.
 * Appending SYN_REPORTS to a frame does not increase the count of events in the
 * frame - the new SYN_REPORT will simply replace the existing SYN_REPORT.
 *
 * The timestamp of the SYN_REPORT (if any) is used for this event
 * frame. If the appended sequence does not contain a SYN_REPORT, the highest
 * timestamp of any event appended is used. This timestamp will overwrite the
 * frame's timestamp even if the timestamp of the frame is higher.
 *
 * If all to-be-appended events (including the SYN_REPORT) have a timestamp of
 * 0, the existing frame's timestamp is left as-is.
 *
 * The caller SHOULD terminate the events with a SYN_REPORT event with a
 * valid timestamp to ensure correct behavior.
 *
 * Returns 0 on success, or a negative errno on failure
 */
static inline int
evdev_frame_append(struct evdev_frame *frame,
		   const struct evdev_event *events,
		   size_t nevents)
{
	assert(nevents > 0);

	for (size_t i = 0; i < nevents; i++) {
		if (evdev_usage_eq(events[i].usage, EVDEV_SYN_REPORT)) {
			nevents = i;
			break;
		}
	}

	if (nevents > 0) {
		if (frame->count + nevents > frame->max_size)
			return -ENOMEM;

		memcpy(frame->events + frame->count - 1, events, nevents * sizeof(*events));
		frame->count += nevents;
	}

	return 0;
}

static inline int
evdev_frame_append_input_event(struct evdev_frame *frame,
			       const struct input_event *event)
{
	struct evdev_event e = evdev_event_from_input_event(event, NULL);
	if (evdev_usage_as_uint32_t(e.usage) == EVDEV_SYN_REPORT) {
		uint64_t time = input_event_time(event);
		evdev_frame_set_time(frame, time);
	}
	return evdev_frame_append(frame, &e, 1);
}

/**
 * Behaves like evdev_frame_append() but resets the frame before appending.
 *
 * On error the frame is left as-is.
 *
 * Returns 0 on success, or a negative errno on failure
 */
static inline int
evdev_frame_set(struct evdev_frame *frame,
		const struct evdev_event *events,
		size_t nevents)
{
	assert(nevents > 0);

	size_t count = nevents;

	for (size_t i = 0; i < nevents; i++) {
		if (evdev_usage_as_uint32_t(events[i].usage) == EVDEV_SYN_REPORT) {
			count = i;
			break;
		}
	}

	if (count > frame->max_size - 1)
		return -ENOMEM;

	evdev_frame_reset(frame);
	return evdev_frame_append(frame, events, nevents);
}

static inline struct evdev_frame *
evdev_frame_clone(struct evdev_frame *frame)
{
	size_t nevents;
	struct evdev_event *events = evdev_frame_get_events(frame, &nevents);
	struct evdev_frame *clone = evdev_frame_new(nevents);

	evdev_frame_append(clone, events, nevents);
	evdev_frame_set_time(clone, evdev_frame_get_time(frame));

	return clone;
}
