/*
 * Copyright © 2019 Matt Mayfield
 * Copyright © 2019 Red Hat, Inc.
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
#include "evdev-mt-touchpad.h"

#define THUMB_MOVE_TIMEOUT ms2us(300)

bool
tp_thumb_ignored(const struct tp_dispatch *tp, const struct tp_touch *t)
{
	return t->thumb.state == THUMB_STATE_YES;
}

static inline const char*
thumb_state_to_str(enum tp_thumb_state state)
{
	switch(state){
	CASE_RETURN_STRING(THUMB_STATE_NO);
	CASE_RETURN_STRING(THUMB_STATE_YES);
	CASE_RETURN_STRING(THUMB_STATE_MAYBE);
	}

	return NULL;
}

void
tp_thumb_set_state(struct tp_dispatch *tp,
		   struct tp_touch *t,
		   enum tp_thumb_state state)
{
	if (t->thumb.state == state)
		return;

	evdev_log_debug(tp->device,
			"thumb: touch %d, %s → %s\n",
			t->index,
			thumb_state_to_str(t->thumb.state),
			thumb_state_to_str(state));

	t->thumb.state = state;
}

void
tp_thumb_reset(struct tp_dispatch *tp, struct tp_touch *t)
{
	t->thumb.state = THUMB_STATE_MAYBE;
}

static bool
tp_thumb_in_exclusion_area(const struct tp_dispatch *tp,
			   const struct tp_touch *t,
			   uint64_t time)
{
	return (t->point.y > tp->thumb.lower_thumb_line &&
		tp->scroll.method != LIBINPUT_CONFIG_SCROLL_EDGE &&
	        t->thumb.first_touch_time + THUMB_MOVE_TIMEOUT < time);

}

static bool
tp_thumb_detect_pressure_size(const struct tp_dispatch *tp,
			      const struct tp_touch *t,
			      uint64_t time)
{
	bool is_thumb = false;

	if (tp->thumb.use_pressure &&
	    t->pressure > tp->thumb.pressure_threshold &&
	    tp_thumb_in_exclusion_area(tp, t, time)) {
		is_thumb = true;
	}

	if (tp->thumb.use_size &&
	    (t->major > tp->thumb.size_threshold) &&
	    (t->minor < (tp->thumb.size_threshold * 0.6))) {
		is_thumb = true;
	}

	return is_thumb;
}

void
tp_thumb_suppress(struct tp_dispatch *tp, struct tp_touch *t)
{
	tp_thumb_set_state(tp, t, THUMB_STATE_YES);
}

void
tp_thumb_update_touch(struct tp_dispatch *tp,
		      struct tp_touch *t,
		      uint64_t time)
{
	/* once a thumb, always a thumb, once ruled out always ruled out */
	if (!tp->thumb.detect_thumbs ||
	    t->thumb.state != THUMB_STATE_MAYBE)
		return;

	if (t->point.y < tp->thumb.upper_thumb_line) {
		/* if a potential thumb is above the line, it won't ever
		 * label as thumb */
		tp_thumb_set_state(tp, t, THUMB_STATE_NO);
		return;
	}

	/* If the thumb moves by more than 7mm, it's not a resting thumb */
	if (t->state == TOUCH_BEGIN) {
		t->thumb.initial = t->point;
	} else if (t->state == TOUCH_UPDATE) {
		struct device_float_coords delta;
		struct phys_coords mm;

		delta = device_delta(t->point, t->thumb.initial);
		mm = tp_phys_delta(tp, delta);
		if (length_in_mm(mm) > 7) {
			tp_thumb_set_state(tp, t, THUMB_STATE_NO);
			return;
		}
	}

	/* If the finger is below the upper thumb line and we have another
	 * finger in the same area, neither finger is a thumb (unless we've
	 * already labeled it as such).
	 */
	if (t->point.y > tp->thumb.upper_thumb_line &&
	    tp->nfingers_down > 1) {
		struct tp_touch *other;

		tp_for_each_touch(tp, other) {
			if (other->state != TOUCH_BEGIN &&
			    other->state != TOUCH_UPDATE)
				continue;

			if (other->point.y > tp->thumb.upper_thumb_line) {
				tp_thumb_set_state(tp, t, THUMB_STATE_NO);
				if (other->thumb.state == THUMB_STATE_MAYBE)
					tp_thumb_set_state(tp,
							   other,
							   THUMB_STATE_NO);
				break;
			}
		}
	}

	/* Note: a thumb at the edge of the touchpad won't trigger the
	 * threshold, the surface area is usually too small. So we have a
	 * two-stage detection: pressure and time within the area.
	 * A finger that remains at the very bottom of the touchpad becomes
	 * a thumb.
	 */
	if (tp_thumb_detect_pressure_size(tp, t, time) ||
	    tp_thumb_in_exclusion_area(tp, t, time))
		tp_thumb_set_state(tp, t, THUMB_STATE_YES);

	/* now what? we marked it as thumb, so:
	 *
	 * - pointer motion must ignore this touch
	 * - clickfinger must ignore this touch for finger count
	 * - software buttons are unaffected
	 * - edge scrolling unaffected
	 * - gestures: unaffected
	 * - tapping: honour thumb on begin, ignore it otherwise for now,
	 *   this gets a tad complicated otherwise
	 */
}

void
tp_thumb_update_multifinger(struct tp_dispatch *tp)
{
	struct tp_touch *t;
	struct tp_touch *first = NULL,
			*second = NULL;
	struct device_coords distance;
	struct phys_coords mm;

	tp_for_each_touch(tp, t) {
		if (t->state == TOUCH_NONE ||
		    t->state == TOUCH_HOVERING)
			continue;

		if (t->state != TOUCH_BEGIN)
			first = t;
		else
			second = t;

		if (first && second)
			break;
	}

	assert(first);
	assert(second);

	if (tp->scroll.method == LIBINPUT_CONFIG_SCROLL_2FG) {
		/* If the second finger comes down next to the other one, we
		 * assume this is a scroll motion.
		 */
		distance.x = abs(first->point.x - second->point.x);
		distance.y = abs(first->point.y - second->point.y);
		mm = evdev_device_unit_delta_to_mm(tp->device, &distance);

		if (mm.x <= 25 && mm.y <= 15)
			return;
	}

	/* Finger are too far apart or 2fg scrolling is disabled, mark
	 * second finger as thumb */
	evdev_log_debug(tp->device,
			"touch %d is speed-based thumb\n",
			second->index);
	tp_thumb_suppress(tp, second);
}

void
tp_init_thumb(struct tp_dispatch *tp)
{
	struct evdev_device *device = tp->device;
	double w = 0.0, h = 0.0;
	struct device_coords edges;
	struct phys_coords mm = { 0.0, 0.0 };
	uint32_t threshold;
	struct quirks_context *quirks;
	struct quirks *q;

	tp->thumb.detect_thumbs = false;

	if (!tp->buttons.is_clickpad)
		return;

	/* if the touchpad is less than 50mm high, skip thumb detection.
	 * it's too small to meaningfully interact with a thumb on the
	 * touchpad */
	evdev_device_get_size(device, &w, &h);
	if (h < 50)
		return;

	tp->thumb.detect_thumbs = true;
	tp->thumb.use_pressure = false;
	tp->thumb.pressure_threshold = INT_MAX;

	/* detect thumbs by pressure in the bottom 15mm, detect thumbs by
	 * lingering in the bottom 8mm */
	mm.y = h * 0.85;
	edges = evdev_device_mm_to_units(device, &mm);
	tp->thumb.upper_thumb_line = edges.y;

	mm.y = h * 0.92;
	edges = evdev_device_mm_to_units(device, &mm);
	tp->thumb.lower_thumb_line = edges.y;

	quirks = evdev_libinput_context(device)->quirks;
	q = quirks_fetch_for_device(quirks, device->udev_device);

	if (libevdev_has_event_code(device->evdev, EV_ABS, ABS_MT_PRESSURE)) {
		if (quirks_get_uint32(q,
				      QUIRK_ATTR_THUMB_PRESSURE_THRESHOLD,
				      &threshold)) {
			tp->thumb.use_pressure = true;
			tp->thumb.pressure_threshold = threshold;
		}
	}

	if (libevdev_has_event_code(device->evdev, EV_ABS, ABS_MT_TOUCH_MAJOR)) {
		if (quirks_get_uint32(q,
				      QUIRK_ATTR_THUMB_SIZE_THRESHOLD,
				      &threshold)) {
			tp->thumb.use_size = true;
			tp->thumb.size_threshold = threshold;
		}
	}

	quirks_unref(q);

	evdev_log_debug(device,
			"thumb: enabled thumb detection (area%s%s)\n",
			tp->thumb.use_pressure ? ", pressure" : "",
			tp->thumb.use_size ? ", size" : "");
}
