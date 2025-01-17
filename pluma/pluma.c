/*
 * pluma.c
 * This file is part of pluma
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/*
 * Modified by the pluma Team, 2005. See the AUTHORS file for a
 * list of people on the pluma Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#ifdef ENABLE_NLS
#include <locale.h>
#endif /* ENABLE_NLS */

#include <errno.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_INTROSPECTION
#include <girepository.h>
#endif

#include "eggdesktopfile.h"
#include "eggsmclient.h"
#include "pluma-app.h"
#include "pluma-commands.h"
#include "pluma-debug.h"
#include "pluma-dirs.h"
#include "pluma-encodings.h"
#include "pluma-plugins-engine.h"
#include "pluma-session.h"
#include "pluma-settings.h"
#include "pluma-utils.h"
#include "pluma-window.h"

#ifndef ENABLE_GVFS_METADATA
#include "pluma-metadata-manager.h"
#endif

#include "bacon-message-connection.h"

static guint32 startup_timestamp = 0;

static BaconMessageConnection *connection;

/* command line */
static gint line_position = 0;
static gchar *encoding_charset = NULL;
static gboolean new_window_option = FALSE;
static gboolean new_document_option = FALSE;
static gchar **remaining_args = NULL;
static GSList *file_list = NULL;

static void show_version_and_quit(void) {
  g_print("%s - Version %s\n", g_get_application_name(), VERSION);

  exit(EXIT_SUCCESS);
}

static void list_encodings_and_quit(void) {
  gint i = 0;
  const PlumaEncoding *enc;

  while ((enc = pluma_encoding_get_from_index(i)) != NULL) {
    g_print("%s\n", pluma_encoding_get_charset(enc));

    ++i;
  }

  exit(EXIT_SUCCESS);
}

static const GOptionEntry options[] = {
    {"version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     show_version_and_quit, N_("Show the application's version"), NULL},

    {"encoding", '\0', 0, G_OPTION_ARG_STRING, &encoding_charset,
     N_("Set the character encoding to be used to open the files listed on the "
        "command line"),
     N_("ENCODING")},

    {"list-encodings", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
     list_encodings_and_quit,
     N_("Display list of possible values for the encoding option"), NULL},

    {"new-window", '\0', 0, G_OPTION_ARG_NONE, &new_window_option,
     N_("Create a new top-level window in an existing instance of pluma"),
     NULL},

    {"new-document", '\0', 0, G_OPTION_ARG_NONE, &new_document_option,
     N_("Create a new document in an existing instance of pluma"), NULL},

    {G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &remaining_args,
     NULL, N_("[FILE...]")}, /* collects file arguments */

    {NULL, 0, 0, G_OPTION_ARG_NONE, NULL, NULL, NULL}};

static void free_command_line_data(void) {
  g_slist_free_full(file_list, g_object_unref);
  file_list = NULL;

  g_strfreev(remaining_args);
  remaining_args = NULL;

  g_free(encoding_charset);
  encoding_charset = NULL;

  new_window_option = FALSE;
  new_document_option = FALSE;
  line_position = 0;
}

static void pluma_get_command_line_data(void) {
  if (remaining_args) {
    gint i;

    for (i = 0; remaining_args[i]; i++) {
      if (*remaining_args[i] == '+') {
        if (*(remaining_args[i] + 1) == '\0')
          /* goto the last line of the document */
          line_position = G_MAXINT;
        else
          line_position = atoi(remaining_args[i] + 1);
      } else {
        GFile *file;

        file = g_file_new_for_commandline_arg(remaining_args[i]);
        file_list = g_slist_prepend(file_list, file);
      }
    }

    file_list = g_slist_reverse(file_list);
  }

  if (encoding_charset &&
      (pluma_encoding_get_from_charset(encoding_charset) == NULL)) {
    g_print(_("%s: invalid encoding.\n"), encoding_charset);
  }
}

static guint32 get_startup_timestamp(void) {
  const gchar *startup_id_env;
  gchar *startup_id = NULL;
  gchar *time_str;
  gchar *end;
  gulong retval = 0;

  /* we don't unset the env, since startup-notification
   * may still need it */
  startup_id_env = g_getenv("DESKTOP_STARTUP_ID");
  if (startup_id_env == NULL) goto out;

  startup_id = g_strdup(startup_id_env);

  time_str = g_strrstr(startup_id, "_TIME");
  if (time_str == NULL) goto out;

  errno = 0;

  /* Skip past the "_TIME" part */
  time_str += 5;

  retval = strtoul(time_str, &end, 0);
  if (end == time_str || errno != 0) retval = 0;

out:
  g_free(startup_id);

  return (retval > 0) ? retval : 0;
}

static GdkDisplay *display_open_if_needed(const gchar *name) {
  GSList *displays;
  GSList *l;
  GdkDisplay *display = NULL;

  displays = gdk_display_manager_list_displays(gdk_display_manager_get());

  for (l = displays; l != NULL; l = l->next) {
    if (strcmp(gdk_display_get_name((GdkDisplay *)l->data), name) == 0) {
      display = l->data;
      break;
    }
  }

  g_slist_free(displays);

  return display != NULL ? display : gdk_display_open(name);
}

/* serverside */
static void on_message_received(const char *message, gpointer data) {
  const PlumaEncoding *encoding = NULL;
  gchar **commands;
  gchar **params;
  gint workspace;
  gint viewport_x;
  gint viewport_y;
  gchar *display_name;
  gint i;
  PlumaApp *app;
  PlumaWindow *window;
  GdkDisplay *display;
  GdkScreen *screen;

  g_return_if_fail(message != NULL);

  pluma_debug_message(DEBUG_APP, "Received message:\n%s\n", message);

  commands = g_strsplit(message, "\v", -1);

  /* header */
  params = g_strsplit(commands[0], "\t", 6);
  startup_timestamp = atoi(params[0]);
  display_name = params[1];
  workspace = atoi(params[3]);
  viewport_x = atoi(params[4]);
  viewport_y = atoi(params[5]);

  display = display_open_if_needed(display_name);
  if (display == NULL) {
    g_warning("Could not open display %s\n", display_name);
    g_strfreev(params);
    goto out;
  }

  screen = gdk_display_get_default_screen(display);

  g_strfreev(params);

  /* body */
  for (i = 1; commands[i] != NULL; i++) {
    params = g_strsplit(commands[i], "\t", -1);

    if (strcmp(params[0], "NEW-WINDOW") == 0) {
      new_window_option = TRUE;
    } else if (strcmp(params[0], "NEW-DOCUMENT") == 0) {
      new_document_option = TRUE;
    } else if (strcmp(params[0], "OPEN-URIS") == 0) {
      gint n_uris, j;
      gchar **uris;

      line_position = atoi(params[1]);

      if (*params[2] != '\0')
        encoding = pluma_encoding_get_from_charset(params[2]);

      n_uris = atoi(params[3]);
      uris = g_strsplit(params[4], " ", n_uris);

      for (j = 0; j < n_uris; j++) {
        GFile *file;

        file = g_file_new_for_uri(uris[j]);
        file_list = g_slist_prepend(file_list, file);
      }

      file_list = g_slist_reverse(file_list);

      /* the list takes ownerhip of the strings,
       * only free the array */
      g_free(uris);
    } else {
      g_warning("Unexpected bacon command");
    }

    g_strfreev(params);
  }

  /* execute the commands */

  app = pluma_app_get_default();

  if (new_window_option) {
    window = pluma_app_create_window(app, screen);
  } else {
    /* get a window in the current workspace (if exists) and raise it */
    window = _pluma_app_get_window_in_viewport(app, screen, workspace,
                                               viewport_x, viewport_y);
  }

  if (file_list != NULL) {
    _pluma_cmd_load_files_from_prompt(window, file_list, encoding,
                                      line_position);

    if (new_document_option) pluma_window_create_tab(window, TRUE);
  } else {
    PlumaDocument *doc;
    doc = pluma_window_get_active_document(window);

    if (doc == NULL || !pluma_document_is_untouched(doc) || new_document_option)
      pluma_window_create_tab(window, TRUE);
  }

  /* set the proper interaction time on the window.
   * Fall back to roundtripping to the X server when we
   * don't have the timestamp, e.g. when launched from
   * terminal. We also need to make sure that the window
   * has been realized otherwise it will not work. lame.
   */
  if (!gtk_widget_get_realized(GTK_WIDGET(window)))
    gtk_widget_realize(GTK_WIDGET(window));

  if (GDK_IS_X11_DISPLAY(gdk_display_get_default())) {
    if (startup_timestamp <= 0)
      startup_timestamp =
          gdk_x11_get_server_time(gtk_widget_get_window(GTK_WIDGET(window)));

    gdk_x11_window_set_user_time(gtk_widget_get_window(GTK_WIDGET(window)),
                                 startup_timestamp);
    gtk_window_present(GTK_WINDOW(window));
  } else
    gtk_window_present_with_time(GTK_WINDOW(window), GPOINTER_TO_UINT(data));

out:
  g_strfreev(commands);

  free_command_line_data();
}

/* clientside */
static void send_bacon_message(void) {
  GdkScreen *screen;
  GdkDisplay *display;
  const gchar *display_name;
  gint screen_number;
  gint ws;
  gint viewport_x;
  gint viewport_y;
  GString *command;

  /* the messages have the following format:
   * <---                                  header ---> <----            body
   * -----> timestamp \t display_name \t screen_number \t workspace \t
   * viewport_x \t viewport_y \v OP1 \t arg \t arg \v OP2 \t arg \t arg|...
   *
   * when the arg is a list of uri, they are separated by a space.
   * So the delimiters are \v for the commands, \t for the tokens in
   * a command and ' ' for the uris: note that such delimiters cannot
   * be part of an uri, this way parsing is easier.
   */

  pluma_debug(DEBUG_APP);

  screen = gdk_screen_get_default();
  display = gdk_screen_get_display(screen);

  display_name = gdk_display_get_name(display);

  if (GDK_IS_X11_DISPLAY(gdk_display_get_default()))
    screen_number = gdk_x11_screen_get_screen_number(screen);
  else
    screen_number = 0;

  pluma_debug_message(DEBUG_APP, "Display: %s", display_name);
  pluma_debug_message(DEBUG_APP, "Screen: %d", screen_number);

  ws = pluma_utils_get_current_workspace(screen);
  pluma_utils_get_current_viewport(screen, &viewport_x, &viewport_y);

  command = g_string_new(NULL);

  /* header */
  g_string_append_printf(command, "%" G_GUINT32_FORMAT "\t%s\t%d\t%d\t%d\t%d",
                         startup_timestamp, display_name, screen_number, ws,
                         viewport_x, viewport_y);

  /* NEW-WINDOW command */
  if (new_window_option) {
    command = g_string_append_c(command, '\v');
    command = g_string_append(command, "NEW-WINDOW");
  }

  /* NEW-DOCUMENT command */
  if (new_document_option) {
    command = g_string_append_c(command, '\v');
    command = g_string_append(command, "NEW-DOCUMENT");
  }

  /* OPEN_URIS command, optionally specify line_num and encoding */
  if (file_list) {
    GSList *l;

    command = g_string_append_c(command, '\v');
    command = g_string_append(command, "OPEN-URIS");

    g_string_append_printf(command, "\t%d\t%s\t%u\t", line_position,
                           encoding_charset ? encoding_charset : "",
                           g_slist_length(file_list));

    for (l = file_list; l != NULL; l = l->next) {
      gchar *uri;

      uri = g_file_get_uri(G_FILE(l->data));
      command = g_string_append(command, uri);
      if (l->next != NULL) command = g_string_append_c(command, ' ');

      g_free(uri);
    }
  }

  pluma_debug_message(DEBUG_APP, "Bacon Message: %s", command->str);

  bacon_message_connection_send(connection, command->str);

  g_string_free(command, TRUE);
}

int main(int argc, char *argv[]) {
  GOptionContext *context;
  PlumaPluginsEngine *engine;
  PlumaWindow *window;
  PlumaApp *app;
  gboolean restored = FALSE;
  GError *error = NULL;

  /* Setup debugging */
  pluma_debug_init();
  pluma_debug_message(DEBUG_APP, "Startup");

#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, PLUMA_LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);
#endif /* ENABLE_NLS */

  startup_timestamp = get_startup_timestamp();

  /* Setup command line options */
  context = g_option_context_new(_("- Edit text files"));
  g_option_context_add_main_entries(context, options, GETTEXT_PACKAGE);
  g_option_context_add_group(context, gtk_get_option_group(TRUE));
  g_option_context_add_group(context, egg_sm_client_get_option_group());

#ifdef HAVE_INTROSPECTION
  g_option_context_add_group(context, g_irepository_get_option_group());
#endif

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print(_("%s\nRun '%s --help' to see a full list of available command "
              "line options.\n"),
            error->message, argv[0]);
    g_error_free(error);
    g_option_context_free(context);
    return 1;
  }

  g_option_context_free(context);

  pluma_debug_message(DEBUG_APP, "Create bacon connection");

  connection = bacon_message_connection_new("pluma");

  if (connection != NULL) {
    if (!bacon_message_connection_get_is_server(connection)) {
      pluma_debug_message(DEBUG_APP, "I'm a client");

      pluma_get_command_line_data();

      send_bacon_message();

      free_command_line_data();

      /* we never popup a window... tell startup-notification
       * that we are done.
       */
      gdk_notify_startup_complete();

      bacon_message_connection_free(connection);

      exit(EXIT_SUCCESS);
    } else {
      pluma_debug_message(DEBUG_APP, "I'm a server");

      bacon_message_connection_set_callback(connection, on_message_received,
                                            NULL);
    }
  } else {
    g_warning("Cannot create the 'pluma' connection.");
  }

  pluma_debug_message(DEBUG_APP, "Set icon");
  gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
                                    PLUMA_DATADIR "/icons");

  /* Set the associated .desktop file */
  egg_set_desktop_file(DATADIR "/applications/pluma.desktop");

  /* Init plugins engine */
  pluma_debug_message(DEBUG_APP, "Init plugins");
  engine = pluma_plugins_engine_get_default();

  /* Initialize session management */
  pluma_debug_message(DEBUG_APP, "Init session manager");
  pluma_session_init();

  if (pluma_session_is_restored()) restored = pluma_session_load();

  if (!restored) {
    pluma_debug_message(DEBUG_APP, "Analyze command line data");
    pluma_get_command_line_data();

    pluma_debug_message(DEBUG_APP, "Get default app");
    app = pluma_app_get_default();

    pluma_debug_message(DEBUG_APP, "Create main window");
    window = pluma_app_create_window(app, NULL);
    gtk_widget_set_size_request(GTK_WIDGET(window), 250, 250);

    if (file_list != NULL) {
      const PlumaEncoding *encoding = NULL;

      if (encoding_charset)
        encoding = pluma_encoding_get_from_charset(encoding_charset);

      pluma_debug_message(DEBUG_APP, "Load files");
      _pluma_cmd_load_files_from_prompt(window, file_list, encoding,
                                        line_position);
    } else {
      pluma_debug_message(DEBUG_APP, "Create tab");
      pluma_window_create_tab(window, TRUE);
    }

    pluma_debug_message(DEBUG_APP, "Show window");
    gtk_widget_show(GTK_WIDGET(window));

    free_command_line_data();
  }

  pluma_debug_message(DEBUG_APP, "Start gtk-main");

  gtk_main();

  bacon_message_connection_free(connection);

  /* We kept the original engine reference here. So let's unref it to
   * finalize it properly.
   */
  g_object_unref(engine);

  pluma_settings_unref_singleton();

#ifndef ENABLE_GVFS_METADATA
  pluma_metadata_manager_shutdown();
#endif

  return EXIT_SUCCESS;
}
