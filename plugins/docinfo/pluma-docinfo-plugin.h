/*
 * pluma-docinfo-plugin.h
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2012-2021 MATE Developers
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 */

#ifndef __PLUMA_DOCINFO_PLUGIN_H__
#define __PLUMA_DOCINFO_PLUGIN_H__

#include <glib-object.h>
#include <glib.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PLUMA_TYPE_DOCINFO_PLUGIN (pluma_docinfo_plugin_get_type())
#define PLUMA_DOCINFO_PLUGIN(o)                               \
  (G_TYPE_CHECK_INSTANCE_CAST((o), PLUMA_TYPE_DOCINFO_PLUGIN, \
                              PlumaDocInfoPlugin))
#define PLUMA_DOCINFO_PLUGIN_CLASS(k)                      \
  (G_TYPE_CHECK_CLASS_CAST((k), PLUMA_TYPE_DOCINFO_PLUGIN, \
                           PlumaDocInfoPluginClass))
#define PLUMA_IS_DOCINFO_PLUGIN(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE((o), PLUMA_TYPE_DOCINFO_PLUGIN))
#define PLUMA_IS_DOCINFO_PLUGIN_CLASS(k) \
  (G_TYPE_CHECK_CLASS_TYPE((k), PLUMA_TYPE_DOCINFO_PLUGIN))
#define PLUMA_DOCINFO_PLUGIN_GET_CLASS(o)                    \
  (G_TYPE_INSTANCE_GET_CLASS((o), PLUMA_TYPE_DOCINFO_PLUGIN, \
                             PlumaDocInfoPluginClass))

/* Private structure type */
typedef struct _PlumaDocInfoPluginPrivate PlumaDocInfoPluginPrivate;

/*
 * Main object structure
 */
typedef struct _PlumaDocInfoPlugin PlumaDocInfoPlugin;

struct _PlumaDocInfoPlugin {
  PeasExtensionBase parent_instance;

  /*< private >*/
  PlumaDocInfoPluginPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _PlumaDocInfoPluginClass PlumaDocInfoPluginClass;

struct _PlumaDocInfoPluginClass {
  PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType pluma_docinfo_plugin_get_type(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __PLUMA_DOCINFO_PLUGIN_H__ */
