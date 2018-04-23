// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <glib.h>
#include <gudev/gudev.h>

#include "gpiodaemon.h"
#include "gpiochip.h"

struct _GPIODaemon {
	GObject parent;

	GDBusConnection *conn;
	GUdevClient *udev;
	GDBusObjectManagerServer *manager;
	GHashTable *chips;

	gboolean listening;
};

struct _GPIODaemonClass {
	GObjectClass parent;
};

typedef struct _GPIODaemonClass GPIODaemonClass;

G_DEFINE_TYPE(GPIODaemon, gpio_daemon, G_TYPE_OBJECT);

static const gchar* const udev_subsystems[] = { "gpio", NULL };

static void gpio_daemon_init(GPIODaemon *daemon G_GNUC_UNUSED)
{
	g_debug("creating GPIO daemon");
}

GPIODaemon *gpio_daemon_new(void)
{
	return GPIO_DAEMON(g_object_new(GPIO_DAEMON_TYPE, NULL));
}

static void gpio_daemon_finalize(GObject *obj)
{
	GPIODaemon *daemon = GPIO_DAEMON(obj);
	GError *err = NULL;
	gboolean rv;

	g_debug("destroying GPIO daemon");

	if (!daemon->listening)
		return;

	g_hash_table_unref(daemon->chips);
	g_object_unref(daemon->udev);
	g_object_unref(daemon->manager);

	g_clear_error(&err);
	rv = g_dbus_connection_close_sync(daemon->conn, NULL, &err);
	if (!rv)
		g_warning("error closing dbus connection: %s",
			  err->message);
}

static void gpio_daemon_class_init(GPIODaemonClass *daemon_class)
{
	GObjectClass *class = G_OBJECT_CLASS(daemon_class);

	class->finalize = gpio_daemon_finalize;
}

static void export_chip_object(GPIODaemon *daemon, const gchar *devname)
{
	GPIOChip *chip;
	gboolean rv;

	chip = gpio_chip_new(devname, daemon->manager);
	g_return_if_fail(chip);

	rv = g_hash_table_insert(daemon->chips, g_strdup(devname), chip);
	/* It's a programming bug if the chip already exists. */
	g_assert_true(rv);
}

static void remove_chip_object(GPIODaemon *daemon, const gchar *devname)
{
	gboolean rv;

	rv = g_hash_table_remove(daemon->chips, devname);
	/* It's a programming bug if the chip didn't exist. */
	g_assert_true(rv);
}

/*
 * We get two uevents per action per gpiochip. One is for the new-style
 * character device, the other for legacy sysfs devices. We are only concerned
 * with the former, which we can tell from the latter by the presence of
 * the device file.
 */
static gboolean is_gpiochip_device(GUdevDevice *dev)
{
	return g_udev_device_get_device_file(dev) != NULL;
}

static void on_uevent(GUdevClient *udev G_GNUC_UNUSED,
		      const gchar *action, GUdevDevice *dev, gpointer data)
{
	GPIODaemon *daemon = data;

	if (!is_gpiochip_device(dev))
		return;

	g_debug("uevent: %s action on %s device",
		action, g_udev_device_get_name(dev));

	if (g_strcmp0(action, "add") == 0)
		export_chip_object(daemon, g_udev_device_get_name(dev));
	else if (g_strcmp0(action, "remove") == 0)
		remove_chip_object(daemon, g_udev_device_get_name(dev));
	else
		g_warning("unknown action for uevent: %s", action);
}

static void handle_chip_dev(gpointer data, gpointer user_data)
{
	GPIODaemon *daemon = user_data;
	GUdevDevice *dev = data;

	if (is_gpiochip_device(dev))
		export_chip_object(daemon, g_udev_device_get_name(dev));

	g_object_unref(dev);
}

void gpio_daemon_listen(GPIODaemon *daemon, GDBusConnection *conn)
{
	GList *devs;
	gulong rv;

	daemon->conn = conn;

	daemon->chips = g_hash_table_new_full(g_str_hash, g_str_equal,
					      g_free, g_object_unref);

	daemon->udev = g_udev_client_new(udev_subsystems);
	/* Subscribe for GPIO uevents. */
	rv = g_signal_connect(daemon->udev, "uevent",
			      G_CALLBACK(on_uevent), daemon);
	g_assert_true(rv);

	daemon->manager = g_dbus_object_manager_server_new("/org/gpiod");

	devs = g_udev_client_query_by_subsystem(daemon->udev, "gpio");
	g_list_foreach(devs, handle_chip_dev, daemon);
	g_list_free(devs);

	g_dbus_object_manager_server_set_connection(daemon->manager,
						    daemon->conn);

	daemon->listening = TRUE;
}
