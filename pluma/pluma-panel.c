/*
 * pluma-panel.c
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

#include "pluma-panel.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "pluma-close-button.h"
#include "pluma-debug.h"
#include "pluma-window.h"

#define PANEL_ITEM_KEY "PlumaPanelItemKey"

struct _PlumaPanelPrivate {
  GtkOrientation orientation;

  /* Title bar (vertical panel only) */
  GtkWidget *title_image;
  GtkWidget *title_label;

  /* Notebook */
  GtkWidget *notebook;
};

typedef struct _PlumaPanelItem PlumaPanelItem;

struct _PlumaPanelItem {
  gchar *name;
  GtkWidget *icon;
};

/* Properties */
enum { PROP_0, PROP_ORIENTATION };

/* Signals */
enum { ITEM_ADDED, ITEM_REMOVED, CLOSE, FOCUS_DOCUMENT, LAST_SIGNAL };

static guint signals[LAST_SIGNAL] = {0};

static GObject *pluma_panel_constructor(
    GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties);

G_DEFINE_TYPE_WITH_PRIVATE(PlumaPanel, pluma_panel, GTK_TYPE_BOX)

static void pluma_panel_finalize(GObject *obj) {
  if (G_OBJECT_CLASS(pluma_panel_parent_class)->finalize)
    (*G_OBJECT_CLASS(pluma_panel_parent_class)->finalize)(obj);
}

static void pluma_panel_get_property(GObject *object, guint prop_id,
                                     GValue *value, GParamSpec *pspec) {
  PlumaPanel *panel = PLUMA_PANEL(object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      g_value_set_enum(value, panel->priv->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void pluma_panel_set_property(GObject *object, guint prop_id,
                                     const GValue *value, GParamSpec *pspec) {
  PlumaPanel *panel = PLUMA_PANEL(object);

  switch (prop_id) {
    case PROP_ORIENTATION:
      panel->priv->orientation = g_value_get_enum(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
      break;
  }
}

static void pluma_panel_close(PlumaPanel *panel) {
  gtk_widget_hide(GTK_WIDGET(panel));
}

static void pluma_panel_focus_document(PlumaPanel *panel) {
  GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(panel));
  if (gtk_widget_is_toplevel(toplevel) && PLUMA_IS_WINDOW(toplevel)) {
    PlumaView *view;

    view = pluma_window_get_active_view(PLUMA_WINDOW(toplevel));
    if (view != NULL) gtk_widget_grab_focus(GTK_WIDGET(view));
  }
}

static void pluma_panel_grab_focus(GtkWidget *w) {
  gint n;
  GtkWidget *tab;
  PlumaPanel *panel = PLUMA_PANEL(w);

  n = gtk_notebook_get_current_page(GTK_NOTEBOOK(panel->priv->notebook));
  if (n == -1) return;

  tab = gtk_notebook_get_nth_page(GTK_NOTEBOOK(panel->priv->notebook), n);
  g_return_if_fail(tab != NULL);

  gtk_widget_grab_focus(tab);
}

static void pluma_panel_class_init(PlumaPanelClass *klass) {
  GtkBindingSet *binding_set;
  GObjectClass *object_class = G_OBJECT_CLASS(klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

  object_class->constructor = pluma_panel_constructor;
  object_class->finalize = pluma_panel_finalize;
  object_class->get_property = pluma_panel_get_property;
  object_class->set_property = pluma_panel_set_property;

  g_object_class_install_property(
      object_class, PROP_ORIENTATION,
      g_param_spec_enum("panel-orientation", "Panel Orientation",
                        "The panel's orientation", GTK_TYPE_ORIENTATION,
                        GTK_ORIENTATION_VERTICAL,
                        G_PARAM_WRITABLE | G_PARAM_READABLE |
                            G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));

  widget_class->grab_focus = pluma_panel_grab_focus;

  klass->close = pluma_panel_close;
  klass->focus_document = pluma_panel_focus_document;

  signals[ITEM_ADDED] = g_signal_new(
      "item_added", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET(PlumaPanelClass, item_added), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);
  signals[ITEM_REMOVED] = g_signal_new(
      "item_removed", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_FIRST,
      G_STRUCT_OFFSET(PlumaPanelClass, item_removed), NULL, NULL,
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, GTK_TYPE_WIDGET);

  /* Keybinding signals */
  signals[CLOSE] = g_signal_new(
      "close", G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
      G_STRUCT_OFFSET(PlumaPanelClass, close), NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  signals[FOCUS_DOCUMENT] =
      g_signal_new("focus_document", G_OBJECT_CLASS_TYPE(klass),
                   G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
                   G_STRUCT_OFFSET(PlumaPanelClass, focus_document), NULL, NULL,
                   g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
  binding_set = gtk_binding_set_by_class(klass);

  gtk_binding_entry_add_signal(binding_set, GDK_KEY_Escape, 0, "close", 0);
  gtk_binding_entry_add_signal(binding_set, GDK_KEY_Return, GDK_CONTROL_MASK,
                               "focus_document", 0);
}

/* This is ugly, since it supports only known
 * storage types of GtkImage, otherwise fall back
 * to the empty icon.
 * See http://bugzilla.gnome.org/show_bug.cgi?id=317520.
 */
static void set_gtk_image_from_gtk_image(GtkImage *image, GtkImage *source) {
  switch (gtk_image_get_storage_type(source)) {
    case GTK_IMAGE_EMPTY:
      gtk_image_clear(image);
      break;
    case GTK_IMAGE_PIXBUF: {
      GdkPixbuf *pb;

      pb = gtk_image_get_pixbuf(source);
      gtk_image_set_from_pixbuf(image, pb);
    } break;
    case GTK_IMAGE_ANIMATION: {
      GdkPixbufAnimation *a;

      a = gtk_image_get_animation(source);
      gtk_image_set_from_animation(image, a);
    } break;
    case GTK_IMAGE_ICON_NAME: {
      const gchar *n;
      GtkIconSize s;

      gtk_image_get_icon_name(source, &n, &s);
      gtk_image_set_from_icon_name(image, n, s);
    } break;
    default:
      gtk_image_set_from_icon_name(image, "text-x-generic", GTK_ICON_SIZE_MENU);
  }
}

static void sync_title(PlumaPanel *panel, PlumaPanelItem *item) {
  if (panel->priv->orientation != GTK_ORIENTATION_VERTICAL) return;

  if (item != NULL) {
    gtk_label_set_text(GTK_LABEL(panel->priv->title_label), item->name);

    set_gtk_image_from_gtk_image(GTK_IMAGE(panel->priv->title_image),
                                 GTK_IMAGE(item->icon));
  } else {
    gtk_label_set_text(GTK_LABEL(panel->priv->title_label), _("Empty"));

    gtk_image_set_from_icon_name(GTK_IMAGE(panel->priv->title_image),
                                 "text-x-generic", GTK_ICON_SIZE_MENU);
  }
}

static void notebook_page_changed(GtkNotebook *notebook, GtkWidget *page,
                                  guint page_num, PlumaPanel *panel) {
  GtkWidget *item;
  PlumaPanelItem *data;

  item = gtk_notebook_get_nth_page(notebook, page_num);
  g_return_if_fail(item != NULL);

  data = (PlumaPanelItem *)g_object_get_data(G_OBJECT(item), PANEL_ITEM_KEY);
  g_return_if_fail(data != NULL);

  sync_title(panel, data);
}

static void panel_show(PlumaPanel *panel, gpointer user_data) {
  gint page;
  GtkNotebook *nb;

  nb = GTK_NOTEBOOK(panel->priv->notebook);

  page = gtk_notebook_get_current_page(nb);

  if (page != -1) notebook_page_changed(nb, NULL, page, panel);
}

static void pluma_panel_init(PlumaPanel *panel) {
  panel->priv = pluma_panel_get_instance_private(panel);

  gtk_orientable_set_orientation(GTK_ORIENTABLE(panel),
                                 GTK_ORIENTATION_VERTICAL);
}

static void close_button_clicked_cb(GtkWidget *widget, GtkWidget *panel) {
  gtk_widget_hide(panel);
}

static GtkWidget *create_close_button(PlumaPanel *panel) {
  GtkWidget *button;

  button = pluma_close_button_new();

  gtk_widget_set_tooltip_text(button, _("Hide panel"));

  g_signal_connect(button, "clicked", G_CALLBACK(close_button_clicked_cb),
                   panel);

  return button;
}

static void build_notebook_for_panel(PlumaPanel *panel) {
  /* Create the panel notebook */
  panel->priv->notebook = gtk_notebook_new();

  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(panel->priv->notebook), GTK_POS_BOTTOM);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(panel->priv->notebook), TRUE);
  gtk_notebook_popup_enable(GTK_NOTEBOOK(panel->priv->notebook));

  gtk_widget_show(GTK_WIDGET(panel->priv->notebook));

  g_signal_connect(panel->priv->notebook, "switch-page",
                   G_CALLBACK(notebook_page_changed), panel);
}

static void build_horizontal_panel(PlumaPanel *panel) {
  GtkWidget *box;
  GtkWidget *sidebar;
  GtkWidget *close_button;

  box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  gtk_box_pack_start(GTK_BOX(box), panel->priv->notebook, TRUE, TRUE, 0);

  /* Toolbar, close button and first separator */
  sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(sidebar), 4);

  gtk_box_pack_start(GTK_BOX(box), sidebar, FALSE, FALSE, 0);

  close_button = create_close_button(panel);

  gtk_box_pack_start(GTK_BOX(sidebar), close_button, FALSE, FALSE, 0);

  gtk_widget_show_all(box);

  gtk_box_pack_start(GTK_BOX(panel), box, TRUE, TRUE, 0);
}

static void build_vertical_panel(PlumaPanel *panel) {
  GtkWidget *close_button;
  GtkWidget *title_hbox;
  GtkWidget *icon_name_hbox;
  GtkWidget *dummy_label;

  /* Create title hbox */
  title_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(title_hbox), 5);

  gtk_box_pack_start(GTK_BOX(panel), title_hbox, FALSE, FALSE, 0);

  icon_name_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start(GTK_BOX(title_hbox), icon_name_hbox, TRUE, TRUE, 0);

  panel->priv->title_image =
      gtk_image_new_from_icon_name("text-x-generic", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start(GTK_BOX(icon_name_hbox), panel->priv->title_image, FALSE,
                     TRUE, 0);

  dummy_label = gtk_label_new(" ");

  gtk_box_pack_start(GTK_BOX(icon_name_hbox), dummy_label, FALSE, FALSE, 0);

  panel->priv->title_label = gtk_label_new(_("Empty"));
  gtk_label_set_xalign(GTK_LABEL(panel->priv->title_label), 0.0);
  gtk_label_set_ellipsize(GTK_LABEL(panel->priv->title_label),
                          PANGO_ELLIPSIZE_END);

  gtk_box_pack_start(GTK_BOX(icon_name_hbox), panel->priv->title_label, TRUE,
                     TRUE, 0);

  close_button = create_close_button(panel);

  gtk_box_pack_start(GTK_BOX(title_hbox), close_button, FALSE, FALSE, 0);

  gtk_widget_show_all(title_hbox);

  gtk_box_pack_start(GTK_BOX(panel), panel->priv->notebook, TRUE, TRUE, 0);
}

static GObject *pluma_panel_constructor(
    GType type, guint n_construct_properties,
    GObjectConstructParam *construct_properties) {
  /* Invoke parent constructor. */
  PlumaPanelClass *klass =
      PLUMA_PANEL_CLASS(g_type_class_peek(PLUMA_TYPE_PANEL));
  GObjectClass *parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
  GObject *obj = parent_class->constructor(type, n_construct_properties,
                                           construct_properties);

  /* Build the panel, now that we know the orientation
                     (_init has been called previously) */
  PlumaPanel *panel = PLUMA_PANEL(obj);

  build_notebook_for_panel(panel);
  if (panel->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    build_horizontal_panel(panel);
  else
    build_vertical_panel(panel);

  g_signal_connect(panel, "show", G_CALLBACK(panel_show), NULL);

  return obj;
}

/**
 * pluma_panel_new:
 * @orientation: a #GtkOrientation
 *
 * Creates a new #PlumaPanel with the given @orientation. You shouldn't create
 * a new panel use pluma_window_get_side_panel() or
 * pluma_window_get_bottom_panel() instead.
 *
 * Returns: a new #PlumaPanel object.
 */
GtkWidget *pluma_panel_new(GtkOrientation orientation) {
  return GTK_WIDGET(
      g_object_new(PLUMA_TYPE_PANEL, "panel-orientation", orientation, NULL));
}

static GtkWidget *build_tab_label(PlumaPanel *panel, GtkWidget *item,
                                  const gchar *name, GtkWidget *icon) {
  GtkWidget *hbox, *label_hbox, *label_ebox;
  GtkWidget *label;

  /* set hbox spacing and label padding (see below) so that there's an
   * equal amount of space around the label */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  label_ebox = gtk_event_box_new();
  gtk_event_box_set_visible_window(GTK_EVENT_BOX(label_ebox), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), label_ebox, TRUE, TRUE, 0);

  label_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_container_add(GTK_CONTAINER(label_ebox), label_hbox);

  /* setup icon */
  gtk_box_pack_start(GTK_BOX(label_hbox), icon, FALSE, FALSE, 0);

  /* setup label */
  label = gtk_label_new(name);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0);
  gtk_widget_set_margin_start(label, 0);
  gtk_widget_set_margin_end(label, 0);
  gtk_widget_set_margin_top(label, 0);
  gtk_widget_set_margin_bottom(label, 0);
  gtk_box_pack_start(GTK_BOX(label_hbox), label, TRUE, TRUE, 0);

  gtk_widget_set_tooltip_text(label_ebox, name);

  gtk_widget_show_all(hbox);

  if (panel->priv->orientation == GTK_ORIENTATION_VERTICAL)
    gtk_widget_hide(label);

  g_object_set_data(G_OBJECT(item), "label", label);
  g_object_set_data(G_OBJECT(item), "hbox", hbox);

  return hbox;
}

/**
 * pluma_panel_add_item:
 * @panel: a #PlumaPanel
 * @item: the #GtkWidget to add to the @panel
 * @name: the name to be shown in the @panel
 * @image: the image to be shown in the @panel
 *
 * Adds a new item to the @panel.
 */
void pluma_panel_add_item(PlumaPanel *panel, GtkWidget *item, const gchar *name,
                          GtkWidget *image) {
  PlumaPanelItem *data;
  GtkWidget *tab_label;
  GtkWidget *menu_label;
  gint w, h;

  g_return_if_fail(PLUMA_IS_PANEL(panel));
  g_return_if_fail(GTK_IS_WIDGET(item));
  g_return_if_fail(name != NULL);
  g_return_if_fail(image == NULL || GTK_IS_IMAGE(image));

  data = g_new(PlumaPanelItem, 1);

  data->name = g_strdup(name);

  if (image == NULL) {
    /* default to empty */
    data->icon =
        gtk_image_new_from_icon_name("text-x-generic", GTK_ICON_SIZE_MENU);
  } else {
    data->icon = image;
  }

  gtk_icon_size_lookup(GTK_ICON_SIZE_MENU, &w, &h);
  gtk_widget_set_size_request(data->icon, w, h);

  g_object_set_data(G_OBJECT(item), PANEL_ITEM_KEY, data);

  tab_label = build_tab_label(panel, item, data->name, data->icon);

  menu_label = gtk_label_new(name);
  gtk_label_set_xalign(GTK_LABEL(menu_label), 0.0);

  if (!gtk_widget_get_visible(item)) gtk_widget_show(item);

  gtk_notebook_append_page_menu(GTK_NOTEBOOK(panel->priv->notebook), item,
                                tab_label, menu_label);

  g_signal_emit(G_OBJECT(panel), signals[ITEM_ADDED], 0, item);
}

/**
 * pluma_panel_add_item_with_icon:
 * @panel: a #PlumaPanel
 * @item: the #GtkWidget to add to the @panel
 * @name: the name to be shown in the @panel
 * @icon_name: a icon name
 *
 * Same as pluma_panel_add_item() but using an image from icon name.
 */
void pluma_panel_add_item_with_icon(PlumaPanel *panel, GtkWidget *item,
                                    const gchar *name, const gchar *icon_name) {
  GtkWidget *icon = NULL;

  if (icon_name != NULL) {
    icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
  }

  pluma_panel_add_item(panel, item, name, icon);
}

/**
 * pluma_panel_remove_item:
 * @panel: a #PlumaPanel
 * @item: the item to be removed from the panel
 *
 * Removes the widget @item from the panel if it is in the @panel and returns
 * %TRUE if there was not any problem.
 *
 * Returns: %TRUE if it was well removed.
 */
gboolean pluma_panel_remove_item(PlumaPanel *panel, GtkWidget *item) {
  PlumaPanelItem *data;
  gint page_num;

  g_return_val_if_fail(PLUMA_IS_PANEL(panel), FALSE);
  g_return_val_if_fail(GTK_IS_WIDGET(item), FALSE);

  page_num = gtk_notebook_page_num(GTK_NOTEBOOK(panel->priv->notebook), item);

  if (page_num == -1) return FALSE;

  data = (PlumaPanelItem *)g_object_get_data(G_OBJECT(item), PANEL_ITEM_KEY);
  g_return_val_if_fail(data != NULL, FALSE);

  g_free(data->name);
  g_free(data);

  g_object_set_data(G_OBJECT(item), PANEL_ITEM_KEY, NULL);

  /* ref the item to keep it alive during signal emission */
  g_object_ref(G_OBJECT(item));

  gtk_notebook_remove_page(GTK_NOTEBOOK(panel->priv->notebook), page_num);

  /* if we removed all the pages, reset the title */
  if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(panel->priv->notebook)) == 0)
    sync_title(panel, NULL);

  g_signal_emit(G_OBJECT(panel), signals[ITEM_REMOVED], 0, item);

  g_object_unref(G_OBJECT(item));

  return TRUE;
}

/**
 * pluma_panel_activate_item:
 * @panel: a #PlumaPanel
 * @item: the item to be activated
 *
 * Switches to the page that contains @item.
 *
 * Returns: %TRUE if it was activated
 */
gboolean pluma_panel_activate_item(PlumaPanel *panel, GtkWidget *item) {
  gint page_num;

  g_return_val_if_fail(PLUMA_IS_PANEL(panel), FALSE);
  g_return_val_if_fail(GTK_IS_WIDGET(item), FALSE);

  page_num = gtk_notebook_page_num(GTK_NOTEBOOK(panel->priv->notebook), item);

  if (page_num == -1) return FALSE;

  gtk_notebook_set_current_page(GTK_NOTEBOOK(panel->priv->notebook), page_num);

  return TRUE;
}

/**
 * pluma_panel_item_is_active:
 * @panel: a #PlumaPanel
 * @item: a #GtkWidget
 *
 * Returns whether @item is the active widget in @panel
 *
 * Returns: %TRUE if @item is the active widget
 */
gboolean pluma_panel_item_is_active(PlumaPanel *panel, GtkWidget *item) {
  gint cur_page;
  gint page_num;

  g_return_val_if_fail(PLUMA_IS_PANEL(panel), FALSE);
  g_return_val_if_fail(GTK_IS_WIDGET(item), FALSE);

  page_num = gtk_notebook_page_num(GTK_NOTEBOOK(panel->priv->notebook), item);

  if (page_num == -1) return FALSE;

  cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(panel->priv->notebook));

  return (page_num == cur_page);
}

/**
 * pluma_panel_get_orientation:
 * @panel: a #PlumaPanel
 *
 * Gets the orientation of the @panel.
 *
 * Returns: the #GtkOrientation of #PlumaPanel
 */
GtkOrientation pluma_panel_get_orientation(PlumaPanel *panel) {
  g_return_val_if_fail(PLUMA_IS_PANEL(panel), GTK_ORIENTATION_VERTICAL);

  return panel->priv->orientation;
}

/**
 * pluma_panel_get_n_items:
 * @panel: a #PlumaPanel
 *
 * Gets the number of items in a @panel.
 *
 * Returns: the number of items contained in #PlumaPanel
 */
gint pluma_panel_get_n_items(PlumaPanel *panel) {
  g_return_val_if_fail(PLUMA_IS_PANEL(panel), -1);

  return gtk_notebook_get_n_pages(GTK_NOTEBOOK(panel->priv->notebook));
}

gint _pluma_panel_get_active_item_id(PlumaPanel *panel) {
  gint cur_page;
  GtkWidget *item;
  PlumaPanelItem *data;

  g_return_val_if_fail(PLUMA_IS_PANEL(panel), 0);

  cur_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(panel->priv->notebook));
  if (cur_page == -1) return 0;

  item =
      gtk_notebook_get_nth_page(GTK_NOTEBOOK(panel->priv->notebook), cur_page);

  /* FIXME: for now we use as the hash of the name as id.
   * However the name is not guaranteed to be unique and
   * it is a translated string, so it's subotimal, but should
   * be good enough for now since we don't want to add an
   * ad hoc id argument.
   */

  data = (PlumaPanelItem *)g_object_get_data(G_OBJECT(item), PANEL_ITEM_KEY);
  g_return_val_if_fail(data != NULL, 0);

  return g_str_hash(data->name);
}

void _pluma_panel_set_active_item_by_id(PlumaPanel *panel, gint id) {
  gint n, i;

  g_return_if_fail(PLUMA_IS_PANEL(panel));

  if (id == 0) return;

  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(panel->priv->notebook));

  for (i = 0; i < n; i++) {
    GtkWidget *item;
    PlumaPanelItem *data;

    item = gtk_notebook_get_nth_page(GTK_NOTEBOOK(panel->priv->notebook), i);

    data = (PlumaPanelItem *)g_object_get_data(G_OBJECT(item), PANEL_ITEM_KEY);
    g_return_if_fail(data != NULL);

    if (g_str_hash(data->name) == id) {
      gtk_notebook_set_current_page(GTK_NOTEBOOK(panel->priv->notebook), i);

      return;
    }
  }
}
