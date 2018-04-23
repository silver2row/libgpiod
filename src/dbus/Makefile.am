# SPDX-License-Identifier: LGPL-2.1-or-later

#
# This file is part of libgpiod.
#
# Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
#

SUBDIRS = data examples

AM_CFLAGS = -I$(top_srcdir)/include/ -include $(top_builddir)/config.h
AM_CFLAGS += -Wall -Wextra -g $(GLIB_CFLAGS) $(GIO_CFLAGS) $(GIO_UNIX_CFLAGS) $(GUDEV_CFLAGS)

LDADD = $(top_builddir)/src/lib/libgpiod.la $(GLIB_LIBS) $(GIO_LIBS) $(GIO_UNIX_LIBS) $(GUDEV_LIBS)

bin_PROGRAMS = gpio-dbus

generated-gpio-dbus.h: generated-gpio-dbus.c
generated-gpio-dbus.c: data/org.gpiod.xml
	@gdbus-codegen \
		--interface-prefix org.gpiod \
		--c-namespace GPIOGDBus \
		--generate-c-code generated-gpio-dbus \
		--c-generate-object-manager \
		data/org.gpiod.xml

gpio_dbus_SOURCES = main.c gpiodaemon.c gpiodaemon.h gpiochip.c gpiochip.h gpioline.c gpioline.h
nodist_gpio_dbus_SOURCES = generated-gpio-dbus.c generated-gpio-dbus.h
