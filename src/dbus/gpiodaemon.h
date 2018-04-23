// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#ifndef __GPIODAEMON_H__
#define __GPIODAEMON_H__

#include <glib-object.h>
#include <gio/gio.h>

struct _GPIODaemon;
typedef struct _GPIODaemon GPIODaemon;

#define GPIO_DAEMON_TYPE (gpio_daemon_get_type())
#define GPIO_DAEMON(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), GPIO_DAEMON_TYPE, GPIODaemon))

GPIODaemon *gpio_daemon_new(void);

void gpio_daemon_listen(GPIODaemon *daemon, GDBusConnection *conn);

#endif /* __GPIODAEMON_H__ */
