// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <glib.h>
#include <gudev/gudev.h>
#include <gpiod.h>
#include <errno.h>
#include <string.h>

#include "gpiochip.h"
#include "generated-gpio-dbus.h"

struct _GPIOChip {
	GObject parent;

	GDBusObjectManagerServer *manager;
	struct gpiod_chip *chip;
};

struct _GPIOChipClass {
	GObjectClass parent;
};

typedef struct _GPIOChipClass GPIOChipClass;

G_DEFINE_TYPE(GPIOChip, gpio_chip, G_TYPE_OBJECT);

static void gpio_chip_init(GPIOChip *chip G_GNUC_UNUSED)
{

}

GPIOChip *gpio_chip_new(const gchar *devname,
			GDBusObjectManagerServer *manager)
{
	GPIOGDBusObjectSkeleton *obj_skeleton;
	struct gpiod_chip *handle;
	GPIOGDBusChip *chip_obj;
	gchar *obj_path;
	GPIOChip *chip;

	g_debug("creating a dbus object for %s", devname);

	handle = gpiod_chip_open_by_name(devname);
	if (!handle) {
		g_warning("unable to open %s: %s", devname, strerror(errno));
		return NULL;
	}

	chip = GPIO_CHIP(g_object_new(GPIO_CHIP_TYPE, NULL));

	chip->chip = handle;
	chip->manager = manager;
	g_object_ref(manager);

	obj_path = g_strdup_printf("/org/gpiod/%s", devname);
	obj_skeleton = gpiogdbus_object_skeleton_new(obj_path);
	g_free(obj_path);

	chip_obj = gpiogdbus_chip_skeleton_new();
	gpiogdbus_object_skeleton_set_chip(obj_skeleton, chip_obj);
	g_object_unref(chip_obj);

	gpiogdbus_chip_set_name(chip_obj, gpiod_chip_name(chip->chip));
	gpiogdbus_chip_set_label(chip_obj, gpiod_chip_label(chip->chip));
	gpiogdbus_chip_set_num_lines(chip_obj,
				     gpiod_chip_num_lines(chip->chip));

	g_dbus_object_manager_server_export(chip->manager,
					G_DBUS_OBJECT_SKELETON(obj_skeleton));
	g_object_unref(obj_skeleton);

	return chip;
}

static void gpio_chip_finalize(GObject *obj)
{
	GPIOChip *chip = GPIO_CHIP(obj);

	g_debug("destrying dbus object for %s", gpiod_chip_name(chip->chip));

	if (chip->chip)
		gpiod_chip_close(chip->chip);

	if (chip->manager)
		g_object_unref(chip->manager);
}

static void gpio_chip_class_init(GPIOChipClass *chip_class)
{
	GObjectClass *class = G_OBJECT_CLASS(chip_class);

	class->finalize = gpio_chip_finalize;
}
