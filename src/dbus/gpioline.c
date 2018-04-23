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

#include "gpioline.h"
#include "generated-gpio-dbus.h"

struct _GPIOLine {
	GObject parent;

	struct gpiod_line *line;
};

struct _GPIOLineClass {
	GObjectClass parent;
};

typedef struct _GPIOLineClass GPIOLineClass;

G_DEFINE_TYPE(GPIOLine, gpio_line, G_TYPE_OBJECT);

static void gpio_line_init(GPIOLine *line G_GNUC_UNUSED)
{

}

GPIOLine *gpio_line_new(void)
{
	return GPIO_LINE(g_object_new(GPIO_LINE_TYPE, NULL));
}

static void gpio_line_finalize(GObject *obj)
{
	GPIOLine *line = GPIO_LINE(obj);

	g_debug("destrying dbus object for %s", gpiod_line_name(line->line));
}

static void gpio_line_class_init(GPIOLineClass *line_class)
{
	GObjectClass *class = G_OBJECT_CLASS(line_class);

	class->finalize = gpio_line_finalize;
}
