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

#include <stdbool.h>

#include "util-files.h"
#include "util-list.h"

#include "libinput-plugin.h"
#include "libinput-plugin-private.h"
#include "libinput-plugin-system.h"
#include "libinput-util.h"
#include "libinput-private.h"

struct libinput_plugin {
	struct libinput *libinput;
	char *name;
	int refcount;
	struct list link;
	void *user_data;

	bool registered;

	const struct libinput_plugin_interface *interface;
};

LIBINPUT_ATTRIBUTE_PRINTF(3, 4)
void
plugin_log_msg(struct libinput_plugin *plugin,
	       enum libinput_log_priority priority,
	       const char *format,
	       ...)
{

	if (!log_is_logged(plugin->libinput, priority))
		return;

	_autofree_ char *prefix = strdup_printf("Plugin:%-15s - ", plugin->name);
	va_list args;
	va_start(args, format);
	_autofree_ char *message = strdup_vprintf(format, args);
	va_end(args);

	log_msg(plugin->libinput, priority, "%s%s", prefix, message);
}

struct libinput_plugin *
libinput_plugin_new(struct libinput *libinput,
		    const char *name,
		    const struct libinput_plugin_interface *interface,
		    void *user_data)
{
	struct libinput_plugin *plugin = zalloc(sizeof(*plugin));

	plugin->registered = true;
	plugin->libinput = libinput;
	plugin->refcount = 1;
	plugin->interface = interface;
	plugin->user_data = user_data;
	plugin->name = strdup(name);

	libinput_plugin_system_register_plugin(&libinput->plugin_system, plugin);

	return plugin;
}

void
libinput_plugin_unregister(struct libinput_plugin *plugin)
{
	struct libinput *libinput = plugin->libinput;
	if (!plugin->registered)
		return;

	plugin->registered = false;

	libinput_plugin_system_unregister_plugin(&libinput->plugin_system,
						 plugin);
}

struct libinput_plugin *
libinput_plugin_ref(struct libinput_plugin *plugin)
{
	assert(plugin->refcount > 0);
	++plugin->refcount;
	return plugin;
}

struct libinput_plugin *
libinput_plugin_unref(struct libinput_plugin *plugin)
{
	assert(plugin->refcount > 0);
	if (--plugin->refcount == 0) {
		list_remove(&plugin->link);
		if (plugin->interface->destroy)
			plugin->interface->destroy(plugin);
		free(plugin->name);
		free(plugin);
	}
	return NULL;
}

void
libinput_plugin_set_user_data(struct libinput_plugin *plugin,
			      void *user_data)
{
	plugin->user_data = user_data;
}

void *
libinput_plugin_get_user_data(struct libinput_plugin *plugin)
{
	return plugin->user_data;
}

const char *
libinput_plugin_get_name(struct libinput_plugin *plugin)
{
	return plugin->name;
}

struct libinput *
libinput_plugin_get_context(struct libinput_plugin *plugin)
{
	return plugin->libinput;
}

void
libinput_plugin_run(struct libinput_plugin *plugin)
{
	if (plugin->interface->run)
		plugin->interface->run(plugin);
}

void
libinput_plugin_notify_device_new(struct libinput_plugin *plugin,
				  struct libinput_device *device,
				  struct libevdev *evdev,
				  struct udev_device *udev_device)
{
	if (plugin->interface->device_new)
		plugin->interface->device_new(plugin, device, evdev, udev_device);
}

void
libinput_plugin_notify_device_added(struct libinput_plugin *plugin,
				    struct libinput_device *device)
{
	if (plugin->interface->device_added)
		plugin->interface->device_added(plugin, device);
}

void
libinput_plugin_notify_device_ignored(struct libinput_plugin *plugin,
				      struct libinput_device *device)
{
	if (plugin->interface->device_ignored)
		plugin->interface->device_ignored(plugin, device);
}

void
libinput_plugin_notify_device_removed(struct libinput_plugin *plugin,
				      struct libinput_device *device)
{
	if (plugin->interface->device_removed)
		plugin->interface->device_removed(plugin, device);
}

void
libinput_plugin_notify_evdev_frame(struct libinput_plugin *plugin,
				   struct libinput_device *device,
				   struct evdev_frame *frame)
{
	if (plugin->interface->evdev_frame)
		plugin->interface->evdev_frame(plugin, device, frame);
}

void
libinput_plugin_system_run(struct libinput_plugin_system *system)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin,
			   &system->plugins,
			   link) {
		libinput_plugin_run(plugin);
	}
}

void
libinput_plugin_system_register_plugin(struct libinput_plugin_system *system,
				       struct libinput_plugin *plugin)
{
	libinput_plugin_ref(plugin);
	list_append(&system->plugins, &plugin->link);
}

void
libinput_plugin_system_unregister_plugin(struct libinput_plugin_system *system,
					 struct libinput_plugin *plugin)
{
	struct libinput_plugin *p;
	list_for_each(p, &system->plugins, link) {
		if (p == plugin) {
			list_remove(&plugin->link);
			list_append(&system->removed_plugins, &plugin->link);
			return;
		}
	}
}

static void
libinput_plugin_system_drop_unregistered_plugins(struct libinput_plugin_system *system)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->removed_plugins, link) {
		list_remove(&plugin->link);
		list_init(&plugin->link); /* allow list_remove in unref */
		libinput_plugin_unref(plugin);
	}
}

void
libinput_plugin_system_init(struct libinput_plugin_system *system)
{
	list_init(&system->plugins);
	list_init(&system->removed_plugins);
}

void
libinput_plugin_system_destroy(struct libinput_plugin_system *system)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin,  &system->plugins, link) {
		libinput_plugin_unregister(plugin);
	}

	libinput_plugin_system_drop_unregistered_plugins(system);

	strv_free(system->directories);
}

void
libinput_plugin_system_notify_device_new(struct libinput_plugin_system *system,
					 struct libinput_device *device,
					 struct libevdev *evdev,
					 struct udev_device *udev_device)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->plugins, link) {
		libinput_plugin_notify_device_new(plugin, device, evdev, udev_device);
	}
	libinput_plugin_system_drop_unregistered_plugins(system);
}

void
libinput_plugin_system_notify_device_added(struct libinput_plugin_system *system,
					   struct libinput_device *device)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->plugins, link) {
		libinput_plugin_notify_device_added(plugin, device);
	}
	libinput_plugin_system_drop_unregistered_plugins(system);
}

void
libinput_plugin_system_notify_device_removed(struct libinput_plugin_system *system,
					     struct libinput_device *device)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->plugins, link) {
		libinput_plugin_notify_device_removed(plugin, device);
	}
	libinput_plugin_system_drop_unregistered_plugins(system);
}

void
libinput_plugin_system_notify_device_ignored(struct libinput_plugin_system *system,
					     struct libinput_device *device)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->plugins, link) {
		libinput_plugin_notify_device_ignored(plugin, device);
	}
	libinput_plugin_system_drop_unregistered_plugins(system);
}

void
libinput_plugin_system_notify_evdev_frame(struct libinput_plugin_system *system,
					  struct libinput_device *device,
					  struct evdev_frame *frame)
{
	struct libinput_plugin *plugin;
	list_for_each_safe(plugin, &system->plugins, link) {
		libinput_plugin_notify_evdev_frame(plugin, device, frame);
	}
	libinput_plugin_system_drop_unregistered_plugins(system);
}
