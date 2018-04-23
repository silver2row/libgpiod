// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * This file is part of libgpiod.
 *
 * Copyright (C) 2018 Bartosz Golaszewski <bartekgola@gmail.com>
 */

#include <stdlib.h>
#include <glib.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <gpiod.h>

#include "gpiodaemon.h"

static G_GNUC_NORETURN void die(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	g_logv(NULL, G_LOG_LEVEL_CRITICAL, fmt, va);
	va_end(va);

	exit(EXIT_FAILURE);
}

static const gchar *log_level_to_priority(GLogLevelFlags lvl)
{
	if (lvl & G_LOG_LEVEL_ERROR)
		/*
		 * GLib's ERROR log level is always fatal so translate it
		 * to syslog's EMERG level.
		 */
		return "0";
	else if (lvl & G_LOG_LEVEL_CRITICAL)
		/*
		 * Use GLib's CRITICAL level for error messages. We don't
		 * necessarily want to abort() everytime an error occurred.
		 */
		return "3";
	else if (lvl & G_LOG_LEVEL_WARNING)
		return "4";
	else if (lvl & G_LOG_LEVEL_MESSAGE)
		return "5";
	else if (lvl & G_LOG_LEVEL_INFO)
		return "6";
	else if (lvl & G_LOG_LEVEL_DEBUG)
		return "7";

	/* Default to LOG_NOTICE. */
	return "5";
}

static void handle_log_debug(const gchar *domain, GLogLevelFlags lvl,
			     const gchar *msg, gpointer data G_GNUC_UNUSED)
{
	g_log_structured(domain, lvl, "MESSAGE", msg);
}

static GLogWriterOutput log_write(GLogLevelFlags lvl, const GLogField *fields,
				  gsize n_fields, gpointer data G_GNUC_UNUSED)
{
	const gchar *msg = NULL, *prio;
	const GLogField *field;
	gsize i;

	for (i = 0; i < n_fields; i++) {
		field = &fields[i];

		/* We're only interested in the MESSAGE field. */
		if (!g_strcmp0(field->key, "MESSAGE")) {
			msg = (const gchar *)field->value;
			break;
		}
	}
	if (!msg)
		return G_LOG_WRITER_UNHANDLED;

	prio = log_level_to_priority(lvl);

	g_printerr("<%s>%s\n", prio, msg);

	return G_LOG_WRITER_HANDLED;
}

static gboolean on_sigterm(gpointer data)
{
	GMainLoop *loop = data;

	g_debug("SIGTERM received");

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

static gboolean on_sigint(gpointer data)
{
	GMainLoop *loop = data;

	g_debug("SIGINT received");

	g_main_loop_quit(loop);

	return G_SOURCE_REMOVE;
}

static gboolean on_sighup(gpointer data G_GNUC_UNUSED)
{
	g_debug("SIGHUB received");

	return G_SOURCE_CONTINUE;
}

static void on_bus_acquired(GDBusConnection *conn,
			    const gchar *name G_GNUC_UNUSED, gpointer data)
{
	GPIODaemon *daemon = data;

	g_debug("DBus connection acquired");

	gpio_daemon_listen(daemon, conn);
}

static void on_name_acquired(GDBusConnection *conn G_GNUC_UNUSED,
			     const gchar *name G_GNUC_UNUSED,
			     gpointer data G_GNUC_UNUSED)
{
	g_debug("DBus name acquired: '%s'", name);
}

static void on_name_lost(GDBusConnection *conn,
			 const gchar *name, gpointer data G_GNUC_UNUSED)
{
	g_debug("DBus name lost: '%s'", name);

	if (!conn)
		die("unable to make connection to the bus");

	if (g_dbus_connection_is_closed(conn))
		die("connection to the bus closed, dying...");

	die("name '%s' lost on the bus, dying...", name);
}

static void parse_opts(int argc, char **argv)
{
	gboolean rv, opt_debug = FALSE;
	GError *error = NULL;
	GOptionContext *ctx;
	gchar *summary;

	GOptionEntry opts[] = {
		{
			.long_name		= "debug",
			.short_name		= 'd',
			.flags			= 0,
			.arg			= G_OPTION_ARG_NONE,
			.arg_data		= &opt_debug,
			.description		= "print additional debug messages",
			.arg_description	= NULL,
		},
		{ }
	};

	ctx = g_option_context_new(NULL);

	summary = g_strdup_printf("%s (libgpiod) v%s - dbus daemon for libgpiod",
				  g_get_prgname(), gpiod_version_string());
	g_option_context_set_summary(ctx, summary);
	g_free(summary);

	g_option_context_add_main_entries(ctx, opts, NULL);

	rv = g_option_context_parse(ctx, &argc, &argv, &error);
	if (!rv)
		die("option parsing failed: %s", error->message);

	g_option_context_free(ctx);

	if (opt_debug)
		g_log_set_handler(NULL,
				  G_LOG_LEVEL_DEBUG | G_LOG_LEVEL_INFO,
				  handle_log_debug, NULL);
}

int main(int argc, char **argv)
{
	GPIODaemon *daemon;
	GMainLoop *loop;
	guint bus_id;

	g_log_set_writer_func(log_write, NULL, NULL);
	g_set_prgname(program_invocation_short_name);

	parse_opts(argc, argv);

	g_message("initiating %s", g_get_prgname());

	loop = g_main_loop_new(NULL, FALSE);
	daemon = gpio_daemon_new();

	g_unix_signal_add(SIGTERM, on_sigterm, loop);
	g_unix_signal_add(SIGINT, on_sigint, loop);
	g_unix_signal_add(SIGHUP, on_sighup, NULL); /* Ignore SIGHUP. */

	bus_id = g_bus_own_name(G_BUS_TYPE_SYSTEM, "org.gpiod",
				G_BUS_NAME_OWNER_FLAGS_NONE,
				on_bus_acquired,
				on_name_acquired,
				on_name_lost,
				daemon, NULL);

	g_message("%s started", g_get_prgname());

	g_main_loop_run(loop);

	g_bus_unown_name(bus_id);
	g_object_unref(daemon);
	g_main_loop_unref(loop);

	g_message("%s exiting cleanly", g_get_prgname());

	return EXIT_SUCCESS;
}
