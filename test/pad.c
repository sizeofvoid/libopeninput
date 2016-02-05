/*
 * Copyright © 2016 Red Hat, Inc.
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
#include "litest.h"

START_TEST(pad_cap)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;

	ck_assert(libinput_device_has_capability(device,
						 LIBINPUT_DEVICE_CAP_TABLET_PAD));

}
END_TEST

START_TEST(pad_no_cap)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;

	ck_assert(!libinput_device_has_capability(device,
						  LIBINPUT_DEVICE_CAP_TABLET_PAD));
}
END_TEST

START_TEST(pad_num_buttons)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;
	unsigned int code;
	unsigned int nbuttons = 0;

	for (code = BTN_0; code < KEY_MAX; code++) {
		/* BTN_STYLUS is set for compatibility reasons but not
		 * actually hooked up */
		if (code == BTN_STYLUS)
			continue;

		if (libevdev_has_event_code(dev->evdev, EV_KEY, code))
			nbuttons++;
	}

	ck_assert_int_eq(libinput_device_tablet_pad_get_num_buttons(device),
			 nbuttons);
}
END_TEST

START_TEST(pad_button)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	unsigned int code;
	unsigned int expected_number = 0;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;

	litest_drain_events(li);

	for (code = BTN_LEFT; code < KEY_MAX; code++) {
		if (!libevdev_has_event_code(dev->evdev, EV_KEY, code))
			continue;

		litest_button_click(dev, code, 1);
		litest_button_click(dev, code, 0);
		libinput_dispatch(li);

		switch (code) {
		case BTN_STYLUS:
			litest_assert_empty_queue(li);
			continue;
		default:
			break;
		}

		ev = libinput_get_event(li);
		pev = litest_is_pad_button_event(ev,
						 expected_number,
						 LIBINPUT_BUTTON_STATE_PRESSED);
		ev = libinput_event_tablet_pad_get_base_event(pev);
		libinput_event_destroy(ev);

		ev = libinput_get_event(li);
		pev = litest_is_pad_button_event(ev,
						 expected_number,
						 LIBINPUT_BUTTON_STATE_RELEASED);
		ev = libinput_event_tablet_pad_get_base_event(pev);
		libinput_event_destroy(ev);

		expected_number++;
	}

	litest_assert_empty_queue(li);
}
END_TEST

START_TEST(pad_has_ring)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;
	int nrings;

	nrings = libinput_device_tablet_pad_get_num_rings(device);
	ck_assert_int_ge(nrings, 1);
}
END_TEST

START_TEST(pad_ring)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;
	int val;
	double degrees, expected;

	litest_pad_ring_start(dev, 10);

	litest_drain_events(li);

	/* Wacom's 0 value is at 275 degrees */
	expected = 270;

	for (val = 0; val < 100; val += 10) {
		litest_pad_ring_change(dev, val);
		libinput_dispatch(li);

		ev = libinput_get_event(li);
		pev = litest_is_pad_ring_event(ev,
					       0,
					       LIBINPUT_TABLET_PAD_RING_SOURCE_FINGER);

		degrees = libinput_event_tablet_pad_get_ring_position(pev);
		ck_assert_double_ge(degrees, 0.0);
		ck_assert_double_lt(degrees, 360.0);

		/* rounding errors, mostly caused by small physical range */
		ck_assert_double_ge(degrees, expected - 2);
		ck_assert_double_le(degrees, expected + 2);

		libinput_event_destroy(ev);

		expected = fmod(degrees + 36, 360);
	}

	litest_pad_ring_end(dev);
}
END_TEST

START_TEST(pad_ring_finger_up)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;
	double degrees;

	litest_pad_ring_start(dev, 10);

	litest_drain_events(li);

	litest_pad_ring_end(dev);
	libinput_dispatch(li);

	ev = libinput_get_event(li);
	pev = litest_is_pad_ring_event(ev,
				       0,
				       LIBINPUT_TABLET_PAD_RING_SOURCE_FINGER);

	degrees = libinput_event_tablet_pad_get_ring_position(pev);
	ck_assert_double_eq(degrees, -1.0);
	libinput_event_destroy(ev);

	litest_assert_empty_queue(li);
}
END_TEST

START_TEST(pad_has_strip)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;
	int nstrips;

	nstrips = libinput_device_tablet_pad_get_num_strips(device);
	ck_assert_int_ge(nstrips, 1);
}
END_TEST

START_TEST(pad_strip)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;
	int val;
	double pos, expected;

	litest_pad_strip_start(dev, 10);

	litest_drain_events(li);

	expected = 0;

	/* 9.5 works with the generic axis scaling without jumping over a
	 * value. */
	for (val = 0; val < 100; val += 9.5) {
		litest_pad_strip_change(dev, val);
		libinput_dispatch(li);

		ev = libinput_get_event(li);
		pev = litest_is_pad_strip_event(ev,
						0,
						LIBINPUT_TABLET_PAD_STRIP_SOURCE_FINGER);

		pos = libinput_event_tablet_pad_get_strip_position(pev);
		ck_assert_double_ge(pos, 0.0);
		ck_assert_double_lt(pos, 1.0);

		/* rounding errors, mostly caused by small physical range */
		ck_assert_double_ge(pos, expected - 0.02);
		ck_assert_double_le(pos, expected + 0.02);

		libinput_event_destroy(ev);

		expected = pos + 0.08;
	}

	litest_pad_strip_change(dev, 100);
	libinput_dispatch(li);

	ev = libinput_get_event(li);
	pev = litest_is_pad_strip_event(ev,
					   0,
					   LIBINPUT_TABLET_PAD_STRIP_SOURCE_FINGER);
	pos = libinput_event_tablet_pad_get_strip_position(pev);
	ck_assert_double_eq(pos, 1.0);
	libinput_event_destroy(ev);

	litest_pad_strip_end(dev);
}
END_TEST

START_TEST(pad_strip_finger_up)
{
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;
	double pos;

	litest_pad_strip_start(dev, 10);
	litest_drain_events(li);

	litest_pad_strip_end(dev);
	libinput_dispatch(li);

	ev = libinput_get_event(li);
	pev = litest_is_pad_strip_event(ev,
					0,
					LIBINPUT_TABLET_PAD_STRIP_SOURCE_FINGER);

	pos = libinput_event_tablet_pad_get_strip_position(pev);
	ck_assert_double_eq(pos, -1.0);
	libinput_event_destroy(ev);

	litest_assert_empty_queue(li);
}
END_TEST

START_TEST(pad_left_handed_default)
{
#if HAVE_LIBWACOM
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;
	enum libinput_config_status status;

	ck_assert(libinput_device_config_left_handed_is_available(device));

	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);
	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 0);

	status = libinput_device_config_left_handed_set(dev->libinput_device, 1);
	ck_assert_int_eq(status, LIBINPUT_CONFIG_STATUS_SUCCESS);

	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 1);
	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);

	status = libinput_device_config_left_handed_set(dev->libinput_device, 0);
	ck_assert_int_eq(status, LIBINPUT_CONFIG_STATUS_SUCCESS);

	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 0);
	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);

#endif
}
END_TEST

START_TEST(pad_no_left_handed)
{
	struct litest_device *dev = litest_current_device();
	struct libinput_device *device = dev->libinput_device;
	enum libinput_config_status status;

	ck_assert(!libinput_device_config_left_handed_is_available(device));

	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);
	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 0);

	status = libinput_device_config_left_handed_set(dev->libinput_device, 1);
	ck_assert_int_eq(status, LIBINPUT_CONFIG_STATUS_UNSUPPORTED);

	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 0);
	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);

	status = libinput_device_config_left_handed_set(dev->libinput_device, 0);
	ck_assert_int_eq(status, LIBINPUT_CONFIG_STATUS_UNSUPPORTED);

	ck_assert_int_eq(libinput_device_config_left_handed_get(device),
			 0);
	ck_assert_int_eq(libinput_device_config_left_handed_get_default(device),
			 0);
}
END_TEST

START_TEST(pad_left_handed_ring)
{
#if HAVE_LIBWACOM
	struct litest_device *dev = litest_current_device();
	struct libinput *li = dev->libinput;
	struct libinput_event *ev;
	struct libinput_event_tablet_pad *pev;
	int val;
	double degrees, expected;

	libinput_device_config_left_handed_set(dev->libinput_device, 1);

	litest_pad_ring_start(dev, 10);

	litest_drain_events(li);

	/* Wacom's 0 value is at 275 degrees -> 90 in left-handed mode*/
	expected = 90;

	for (val = 0; val < 100; val += 10) {
		litest_pad_ring_change(dev, val);
		libinput_dispatch(li);

		ev = libinput_get_event(li);
		pev = litest_is_pad_ring_event(ev,
					       0,
					       LIBINPUT_TABLET_PAD_RING_SOURCE_FINGER);

		degrees = libinput_event_tablet_pad_get_ring_position(pev);
		ck_assert_double_ge(degrees, 0.0);
		ck_assert_double_lt(degrees, 360.0);

		/* rounding errors, mostly caused by small physical range */
		ck_assert_double_ge(degrees, expected - 2);
		ck_assert_double_le(degrees, expected + 2);

		libinput_event_destroy(ev);

		expected = fmod(degrees + 36, 360);
	}

	litest_pad_ring_end(dev);
#endif
}
END_TEST

void
litest_setup_tests(void)
{
	litest_add("pad:cap", pad_cap, LITEST_TABLET_PAD, LITEST_ANY);
	litest_add("pad:cap", pad_no_cap, LITEST_ANY, LITEST_TABLET_PAD);

	litest_add("pad:button", pad_num_buttons, LITEST_TABLET_PAD, LITEST_ANY);
	litest_add("pad:button", pad_button, LITEST_TABLET_PAD, LITEST_ANY);

	litest_add("pad:ring", pad_has_ring, LITEST_RING, LITEST_ANY);
	litest_add("pad:ring", pad_ring, LITEST_RING, LITEST_ANY);
	litest_add("pad:ring", pad_ring_finger_up, LITEST_RING, LITEST_ANY);

	litest_add("pad:strip", pad_has_strip, LITEST_STRIP, LITEST_ANY);
	litest_add("pad:strip", pad_strip, LITEST_STRIP, LITEST_ANY);
	litest_add("pad:strip", pad_strip_finger_up, LITEST_STRIP, LITEST_ANY);

	litest_add_for_device("pad:left_handed", pad_left_handed_default, LITEST_WACOM_INTUOS5_PAD);
	litest_add_for_device("pad:left_handed", pad_no_left_handed, LITEST_WACOM_INTUOS3_PAD);
	litest_add_for_device("pad:left_handed", pad_left_handed_ring, LITEST_WACOM_INTUOS5_PAD);
	/* None of the current strip tablets are left-handed */
}
