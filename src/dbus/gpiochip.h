// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __GPIOCHIP_H__
#define __GPIOCHIP_H__

#include <glib-object.h>
#include <gio/gio.h>

struct _GPIOChip;
typedef struct _GPIOChip GPIOChip;

#define GPIO_CHIP_TYPE (gpio_chip_get_type())
#define GPIO_CHIP(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIO_CHIP_TYPE, GPIOChip))

GPIOChip *gpio_chip_new(const gchar *devname,
			GDBusObjectManagerServer *manager);

#endif /* __GPIOCHIP_H__ */
