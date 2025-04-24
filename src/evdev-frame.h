
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

#include <stdbool.h>
#include <linux/input.h>

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
	struct input_event events[];
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

static inline struct input_event *
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
	assert(frame->count > 0);

	for (size_t i = 0; i < frame->count; i++)
		input_event_set_time(&frame->events[i], time);
}

static inline uint64_t
evdev_frame_get_time(const struct evdev_frame *frame)
{
	assert(frame->count > 0);

	return input_event_time(&frame->events[frame->count - 1]);
}

static inline int
evdev_frame_reset(struct evdev_frame *frame)
{
	memset(frame->events, 0, frame->max_size * sizeof(struct input_event));
	frame->count = 1; /* SYN_REPORT is always there */

	return 0;
}

static inline struct evdev_frame *
evdev_frame_new(size_t max_size)
{
	struct evdev_frame *frame = zalloc(max_size * sizeof(struct input_event) + sizeof(*frame));

	frame->refcount = 1;
	frame->max_size = max_size;
	frame->count = 1; /* SYN_REPORT is always there */

	return frame;
}

static inline struct evdev_frame *
evdev_frame_new_on_stack(size_t max_size)
{
	assert(max_size <= 64);
	struct evdev_frame *frame = alloca(max_size * sizeof(struct input_event) + sizeof(*frame));

	frame->refcount = 1;
	frame->max_size = max_size;
	frame->count = 1; /* SYN_REPORT is always there */
	memset(frame->events, 0, max_size * sizeof(struct input_event));

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
		   const struct input_event *events,
		   size_t nevents)
{
	assert(nevents > 0);

	uint64_t time = 0;

	for (size_t i = 0; i < nevents; i++) {
		if (events[i].type == EV_SYN && events[i].code == SYN_REPORT) {
			nevents = i;
			time = input_event_time(&events[i]);
			break;
		}
		time = max(time, input_event_time(&events[i]));
	}

	if (nevents > 0) {
		if (frame->count + nevents > frame->max_size)
			return -ENOMEM;

		memcpy(frame->events + frame->count - 1, events, nevents * sizeof(struct input_event));
		frame->count += nevents;
	}
	if (time)
		evdev_frame_set_time(frame, time);

	return 0;
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
		const struct input_event *events,
		size_t nevents)
{
	assert(nevents > 0);

	size_t count = nevents;

	for (size_t i = 0; i < nevents; i++) {
		if (events[i].type == EV_SYN && events[i].code == SYN_REPORT) {
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
	struct input_event *events = evdev_frame_get_events(frame, &nevents);
	struct evdev_frame *clone = evdev_frame_new(nevents);

	evdev_frame_append(clone, events, nevents);
	evdev_frame_set_time(clone, evdev_frame_get_time(frame));

	return clone;
}
