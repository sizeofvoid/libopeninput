/*
 * Copyright © 2013 Intel Corporation
 * Copyright © 2013-2015 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef __OpenBSD__
#include "evdev.h"
#endif
#include "udev-seat.h"

static const char default_seat[] = "seat0";
static const char default_seat_name[] = "default";

static struct udev_seat *
udev_seat_create(struct udev_input *input,
		 const char *device_seat,
		 const char *seat_name);
static struct udev_seat *
udev_seat_get_named(struct udev_input *input, const char *seat_name);


static inline bool
filter_duplicates(struct udev_seat *udev_seat,
		  struct udev_device *udev_device)
{
	bool ignore_device = false;
	return ignore_device; // TODO
}

static int
device_added(struct udev_device *udev_device,
	     struct udev_input *input,
	     const char *seat_name)
{
	return 0; // TODO
}

static void
device_removed(struct udev_device *udev_device, struct udev_input *input)
{
	// TODO
}

static int
udev_input_add_devices(struct udev_input *input, struct udev *udev)
{
	return 0; // TODO
}

static void
evdev_udev_handler(void *data)
{
	// TODO
}

static void
udev_input_remove_devices(struct udev_input *input)
{
	// TODO
}

static void
udev_input_disable(struct libinput *libinput)
{
	// TODO
}

static int
udev_input_enable(struct libinput *libinput)
{
	return 0; // TODO
}

static void
udev_input_destroy(struct libinput *input)
{
	// TODO
}

static void
udev_seat_destroy(struct libinput_seat *seat)
{
	struct udev_seat *useat = (struct udev_seat*)seat;
	free(useat);
}

static struct udev_seat *
udev_seat_create(struct udev_input *input,
		 const char *device_seat,
		 const char *seat_name)
{
	return NULL; // TODO
}

static struct udev_seat *
udev_seat_get_named(struct udev_input *input, const char *seat_name)
{
	return NULL; // TODO
}

static int
udev_device_change_seat(struct libinput_device *device,
			const char *seat_name)
{
	return 0xdeadbeef; // TODO
}

static const struct libinput_interface_backend interface_backend = {
	.resume = udev_input_enable,
	.suspend = udev_input_disable,
	.destroy = udev_input_destroy,
	.device_change_seat = udev_device_change_seat,
};

/* udev-seat_openbsd.c:141:1: error: conflicting types for 'libinput_udev_create_context
LIBINPUT_EXPORT struct libinput *
libinput_udev_create_context(const struct libinput_interface *interface,
			     void *user_data,
			     struct udev *udev)
{
	return NULL; // TODO
}
*/

LIBINPUT_EXPORT int
libinput_udev_assign_seat(struct libinput *libinput,
			  const char *seat_id)
{
	return 0; // TODO
}
