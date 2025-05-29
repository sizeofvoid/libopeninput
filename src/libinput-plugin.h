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

#include <stdbool.h>
#include <libudev.h>
#include <libevdev/libevdev.h>

/* Forward declarations instead of #includes to make
 * this header self-contained (bindgen, etc.) */
struct evdev_frame;
struct libinput;
struct libinput_device;
struct libinput_plugin;
enum libinput_log_priority;

#define plugin_log_debug(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_DEBUG, __VA_ARGS__)
#define plugin_log_info(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_INFO, __VA_ARGS__)
#define plugin_log_error(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_ERROR, __VA_ARGS__)
#define plugin_log_bug_kernel(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_ERROR, "kernel bug: " __VA_ARGS__)
#define plugin_log_bug_libinput(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_ERROR, "libinput bug: " __VA_ARGS__)
#define plugin_log_bug_client(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_ERROR, "client bug: " __VA_ARGS__)
#define plugin_log_bug(p_, ...) plugin_log_msg((p_), LIBINPUT_LOG_PRIORITY_ERROR, "plugin bug: " __VA_ARGS__)

void
plugin_log_msg(struct libinput_plugin *plugin,
	       enum libinput_log_priority priority,
	       const char *format,
	       ...);

struct libinput_plugin_interface {
	void (*run)(struct libinput_plugin *plugin);
	/**
	 * Notification that the plugin is about to be destroyed.
	 * When this function is called, the plugin has already
	 * been unregistered. The plugin should free any
	 * resources allocated but not the struct libinput_plugin
	 * itself.
	 */
	void (*destroy)(struct libinput_plugin *plugin);
	/**
	 * Notification about a newly added device that has **not** yet
	 * been added by libinput as struct libinput_device.
	 */
	void (*device_new)(struct libinput_plugin *plugin,
			     struct libinput_device *device,
			     struct libevdev *evdev,
			     struct udev_device *udev_device);
        /**
         * Notification that a device (previously announced with device_new)
         * was ignored by libinput and was **never** added as struct
         * libinput_device.
         *
         * If a device was added (device_added) then this callback will
         * not be called for that device.
         */
        void (*device_ignored)(struct libinput_plugin *plugin,
			       struct libinput_device *device);
        /**
         * Notification that a device was added to libinput. Called
         * after the device_new callback if the device matches libinput's
         * expectations.
         */
        void (*device_added)(struct libinput_plugin *plugin,
			     struct libinput_device *device);
	/**
	 * Notification that a previously added device was removed.
	 */
	void (*device_removed)(struct libinput_plugin *plugin,
			       struct libinput_device *device);
	/**
	 * Notification that a device submitted a frame event.
	 */
	void (*evdev_frame)(struct libinput_plugin *plugin,
			    struct libinput_device *device,
			    struct evdev_frame *frame);
};

/**
 * Returns a new plugin with the given interface and, optionally,
 * the user data. The returned plugin has a refcount of at least 1
 * and must be unref'd by the caller.
 * Should an error occur, the plugin must be unregistered by
 * the caller:
 *
 * ```
 * struct libinput_plugin *plugin = libinput_plugin_new(libinput, ...);
 * if (some_error_condition) {
 *     libinput_plugin_unregister(plugin);
 * }
 * libinput_plugin_unref(plugin);
 * ```
 */
struct libinput_plugin *
libinput_plugin_new(struct libinput *libinput,
		    const char *name,
		    const struct libinput_plugin_interface *interface,
		    void *user_data);

const char *
libinput_plugin_get_name(struct libinput_plugin *plugin);

struct libinput *
libinput_plugin_get_context(struct libinput_plugin *plugin);

void
libinput_plugin_unregister(struct libinput_plugin *plugin);

void
libinput_plugin_set_user_data(struct libinput_plugin *plugin,
			      void *user_data);
void *
libinput_plugin_get_user_data(struct libinput_plugin *plugin);

struct libinput_plugin *
libinput_plugin_ref(struct libinput_plugin *plugin);

struct libinput_plugin *
libinput_plugin_unref(struct libinput_plugin *plugin);

#ifdef DEFINE_UNREF_CLEANUP_FUNC
DEFINE_UNREF_CLEANUP_FUNC(libinput_plugin);
#endif

/**
 * Queue an event frame for the next plugin in sequence, after
 * the current event frame being processed.
 *
 * This function can only be called from within the evdev_frame()
 * callback and may be used to queue new frames. If called multiple
 * times with frames F1, F2, ... while processing the current frame C,
 * frame sequence to be passed to the next plugin is
 * C, F1, F2, ...
 *
 * Assuming several plugins P1, P2, P3, and P4 and an event frame E,
 * and P2 and P3 calling this function with an queued event frame Q1 and Q2,
 * respectively.
 *
 * - P1: receives E, optionally modifies E
 * - P2: receives E, appends Q1, optionally modifies E
 * - P3: receives E, appends Q2, optionally modifies E
 * - P3: receives Q1, optionally modifies Q1
 * - P4: receives E, optionally modifies E
 * - P4: receives Q2, optionally modifies Q2
 * - P4: receives Q1, optionally modifies Q1
 *
 * Once plugin processing is complete, the event sequence passed
 * back to libinput is thus [E, Q2, Q1].
 *
 * To discard the original event frame, the plugin needs to
 * call evdev_frame_reset() on the frame passed to it.
 *
 * It is a plugin bug to call this function from outside the
 * evdev_frame() callback.
 */
void
libinput_plugin_append_evdev_frame(struct libinput_plugin *libinput,
				   struct libinput_device *device,
				   struct evdev_frame *frame);

/**
 * Identical to libinput_plugin_append_evdev_frame(), but prepends
 * the event frame to the current event frame being processed.
 * If called multiple times with frames F1, F2, ... while processing
 * the current frame C, frame sequence to be passed to the next
 * plugin is F1, F2, ..., C
 */
void
libinput_plugin_prepend_evdev_frame(struct libinput_plugin *libinput,
				    struct libinput_device *device,
				    struct evdev_frame *frame);
