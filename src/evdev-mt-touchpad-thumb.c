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
tp_thumb_detect(struct tp_dispatch *tp, struct tp_touch *t, uint64_t time)
{
	enum tp_thumb_state state = t->thumb.state;

	/* once a thumb, always a thumb, once ruled out always ruled out */
	if (!tp->thumb.detect_thumbs ||
	    t->thumb.state != THUMB_STATE_MAYBE)
		return;

	if (t->point.y < tp->thumb.upper_thumb_line) {
		/* if a potential thumb is above the line, it won't ever
		 * label as thumb */
		t->thumb.state = THUMB_STATE_NO;
		goto out;
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
			t->thumb.state = THUMB_STATE_NO;
			goto out;
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
				t->thumb.state = THUMB_STATE_NO;
				if (other->thumb.state == THUMB_STATE_MAYBE)
					other->thumb.state = THUMB_STATE_NO;
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
	if (tp->thumb.use_pressure &&
	    t->pressure > tp->thumb.pressure_threshold) {
		t->thumb.state = THUMB_STATE_YES;
	} else if (tp->thumb.use_size &&
		 (t->major > tp->thumb.size_threshold) &&
		 (t->minor < (tp->thumb.size_threshold * 0.6))) {
		t->thumb.state = THUMB_STATE_YES;
	} else if (t->point.y > tp->thumb.lower_thumb_line &&
		 tp->scroll.method != LIBINPUT_CONFIG_SCROLL_EDGE &&
		 t->thumb.first_touch_time + THUMB_MOVE_TIMEOUT < time) {
		t->thumb.state = THUMB_STATE_YES;
	}

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
out:
	if (t->thumb.state != state)
		evdev_log_debug(tp->device,
			  "thumb state: touch %d, %s → %s\n",
			  t->index,
			  thumb_state_to_str(state),
			  thumb_state_to_str(t->thumb.state));
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
