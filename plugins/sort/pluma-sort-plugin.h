/*
 * pluma-sort-plugin.h
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

#ifndef __PLUMA_SORT_PLUGIN_H__
#define __PLUMA_SORT_PLUGIN_H__

#include <glib-object.h>
#include <glib.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define PLUMA_TYPE_SORT_PLUGIN (pluma_sort_plugin_get_type())
#define PLUMA_SORT_PLUGIN(o) \
  (G_TYPE_CHECK_INSTANCE_CAST((o), PLUMA_TYPE_SORT_PLUGIN, PlumaSortPlugin))
#define PLUMA_SORT_PLUGIN_CLASS(k) \
  (G_TYPE_CHECK_CLASS_CAST((k), PLUMA_TYPE_SORT_PLUGIN, PlumaSortPluginClass))
#define PLUMA_IS_SORT_PLUGIN(o) \
  (G_TYPE_CHECK_INSTANCE_TYPE((o), PLUMA_TYPE_SORT_PLUGIN))
#define PLUMA_IS_SORT_PLUGIN_CLASS(k) \
  (G_TYPE_CHECK_CLASS_TYPE((k), PLUMA_TYPE_SORT_PLUGIN))
#define PLUMA_SORT_PLUGIN_GET_CLASS(o) \
  (G_TYPE_INSTANCE_GET_CLASS((o), PLUMA_TYPE_SORT_PLUGIN, PlumaSortPluginClass))

/* Private structure type */
typedef struct _PlumaSortPluginPrivate PlumaSortPluginPrivate;

/*
 * Main object structure
 */
typedef struct _PlumaSortPlugin PlumaSortPlugin;

struct _PlumaSortPlugin {
  PeasExtensionBase parent_instance;

  /*< private >*/
  PlumaSortPluginPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _PlumaSortPluginClass PlumaSortPluginClass;

struct _PlumaSortPluginClass {
  PeasExtensionBaseClass parent_class;
};

/*
 * Public methods
 */
GType pluma_sort_plugin_get_type(void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT void peas_register_types(PeasObjectModule *module);

G_END_DECLS

#endif /* __PLUMA_SORT_PLUGIN_H__ */
