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

#include <config.h>

#include <check.h>
#include <errno.h>
#include <fcntl.h>
#include <libinput.h>
#include <unistd.h>
#include <stdbool.h>

#include "libinput-util.h"
#include "evdev-tablet.h"
#include "litest.h"

START_TEST(proximity_in_out)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	bool have_tool_update = false,
	     have_proximity_out = false;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	litest_tablet_proximity_in(dev, 10, 10, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_TOOL_UPDATE) {
			struct libinput_tool * tool;

			have_tool_update++;
			tablet_event = libinput_event_get_tablet_event(event);
			tool = libinput_event_tablet_get_tool(tablet_event);
			ck_assert_int_eq(libinput_tool_get_type(tool),
					 LIBINPUT_TOOL_PEN);
		}
		libinput_event_destroy(event);
	}
	ck_assert(have_tool_update);

	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		if (libinput_event_get_type(event) ==
		    LIBINPUT_EVENT_TABLET_PROXIMITY_OUT)
			have_proximity_out = true;

		libinput_event_destroy(event);
	}
	ck_assert(have_proximity_out);

	/* Proximity out must not emit axis events */
	litest_tablet_proximity_out(dev);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		enum libinput_event_type type = libinput_event_get_type(event);

		ck_assert(type != LIBINPUT_EVENT_TABLET_AXIS);

		libinput_event_destroy(event);
	}
}
END_TEST

START_TEST(proximity_out_clear_buttons)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	uint32_t button;

	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(dev->libinput);

	/* Test that proximity out events send button releases for any currently
	 * pressed stylus buttons
	 */
	for (button = BTN_TOUCH; button <= BTN_STYLUS2; button++) {
		bool button_released = false;
		uint32_t event_button;
		enum libinput_button_state state;

		litest_tablet_proximity_in(dev, 10, 10, axes);
		litest_event(dev, EV_KEY, button, 1);
		litest_event(dev, EV_SYN, SYN_REPORT, 0);
		litest_tablet_proximity_out(dev);

		libinput_dispatch(li);

		while ((event = libinput_get_event(li))) {
			tablet_event = libinput_event_get_tablet_event(event);

			if (libinput_event_get_type(event) ==
			    LIBINPUT_EVENT_TABLET_BUTTON) {

				event_button = libinput_event_tablet_get_button(tablet_event);
				state = libinput_event_tablet_get_button_state(tablet_event);

				if (event_button == button &&
				    state == LIBINPUT_BUTTON_STATE_RELEASED)
					button_released = true;
			}

			libinput_event_destroy(event);
		}

		ck_assert_msg(button_released,
			      "Button %d was not released.",
			      event_button);
	}
}
END_TEST

START_TEST(motion)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event_tablet *tablet_event;
	struct libinput_event *event;
	int test_x, test_y;
	double last_reported_x, last_reported_y;
	enum libinput_event_type type;
	struct axis_replacement axes[] = {
		{ ABS_DISTANCE, 10 },
		{ -1, -1 }
	};

	litest_drain_events(li);

	litest_tablet_proximity_in(dev, 5, 100, axes);
	libinput_dispatch(li);

	while ((event = libinput_get_event(li))) {
		tablet_event = libinput_event_get_tablet_event(event);
		type = libinput_event_get_type(event);

		if (type == LIBINPUT_EVENT_TABLET_AXIS) {
			bool x_changed, y_changed;
			double reported_x, reported_y;

			x_changed = libinput_event_tablet_axis_has_changed(
			    tablet_event, LIBINPUT_TABLET_AXIS_X);
			y_changed = libinput_event_tablet_axis_has_changed(
			    tablet_event, LIBINPUT_TABLET_AXIS_Y);

			ck_assert(x_changed);
			ck_assert(y_changed);

			reported_x = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_X);
			reported_y = libinput_event_tablet_get_axis_value(
			    tablet_event, LIBINPUT_TABLET_AXIS_Y);

			litest_assert_double_lt(reported_x, reported_y);

			last_reported_x = reported_x;
			last_reported_y = reported_y;
		}

		libinput_event_destroy(event);
	}

	for (test_x = 10, test_y = 90;
	     test_x <= 100;
	     test_x += 10, test_y -= 10) {
		bool x_changed, y_changed;
		double reported_x, reported_y;

		litest_tablet_proximity_in(dev, test_x, test_y, axes);
		libinput_dispatch(li);

		while ((event = libinput_get_event(li))) {
			tablet_event = libinput_event_get_tablet_event(event);
			type = libinput_event_get_type(event);

			if (type == LIBINPUT_EVENT_TABLET_AXIS) {
				x_changed = libinput_event_tablet_axis_has_changed(
				    tablet_event, LIBINPUT_TABLET_AXIS_X);
				y_changed = libinput_event_tablet_axis_has_changed(
				    tablet_event, LIBINPUT_TABLET_AXIS_Y);

				ck_assert(x_changed);
				ck_assert(y_changed);

				reported_x = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_X);
				reported_y = libinput_event_tablet_get_axis_value(
				    tablet_event, LIBINPUT_TABLET_AXIS_Y);

				litest_assert_double_gt(reported_x,
							last_reported_x);
				litest_assert_double_lt(reported_y,
							last_reported_y);

				last_reported_x = reported_x;
				last_reported_y = reported_y;
			}

			libinput_event_destroy(event);
		}
	}
}
END_TEST

int
main(int argc, char **argv)
{
	litest_add("tablet:proximity", proximity_out_clear_buttons, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:proximity", proximity_in_out, LITEST_TABLET, LITEST_ANY);
	litest_add("tablet:motion", motion, LITEST_TABLET, LITEST_ANY);

	return litest_run(argc, argv);
}
