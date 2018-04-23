// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __GPIOLINE_H__
#define __GPIOLINE_H__

#include <glib-object.h>
#include <gio/gio.h>

struct _GPIOLine;
typedef struct _GPIOLine GPIOLine;

#define GPIO_LINE_TYPE (gpio_line_get_type())
#define GPIO_LINE(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIO_LINE_TYPE, GPIOLine))

GPIOLine *gpio_line_new(void);

#endif /* __GPIOLINE_H__ */
