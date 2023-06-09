/*
    This file is part of darktable,
    Copyright (C) 2011-2021 darktable developers.

    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gdk/gdkkeysyms.h>

#include "common/collection.h"
#include "common/debug.h"
#include "common/selection.h"
#include "control/conf.h"
#include "control/control.h"
#include "dtgtk/button.h"
#include "dtgtk/thumbtable.h"
#include "dtgtk/togglebutton.h"
#include "gui/accelerators.h"
#include "gui/gtk.h"
#include "libs/lib.h"
#include "libs/lib_api.h"

DT_MODULE(1)

typedef struct dt_lib_tool_lighttable_t
{
  GtkWidget *zoom;
  GtkWidget *zoom_entry;
  int current_zoom;
  gboolean combo_evt_reset;
} dt_lib_tool_lighttable_t;

/* set zoom proxy function */
static void _lib_lighttable_set_zoom(dt_lib_module_t *self, gint zoom);
static gint _lib_lighttable_get_zoom(dt_lib_module_t *self);

/* zoom slider change callback */
static void _lib_lighttable_zoom_slider_changed(GtkRange *range, gpointer user_data);
/* zoom entry change callback */
static gboolean _lib_lighttable_zoom_entry_changed(GtkWidget *entry, GdkEventKey *event,
                                                   dt_lib_module_t *self);

static void _set_zoom(dt_lib_module_t *self, int zoom);

const char *name(dt_lib_module_t *self)
{
  return _("lighttable");
}

const char **views(dt_lib_module_t *self)
{
  static const char *v[] = {"lighttable", NULL};
  return v;
}

uint32_t container(dt_lib_module_t *self)
{
  return DT_UI_CONTAINER_PANEL_TOP_SECOND_ROW;
}

int expandable(dt_lib_module_t *self)
{
  return 0;
}

int position()
{
  return 1001;
}

void gui_init(dt_lib_module_t *self)
{
  /* initialize ui widgets */
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)g_malloc0(sizeof(dt_lib_tool_lighttable_t));
  self->data = (void *)d;

  self->widget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign(self->widget, GTK_ALIGN_END);

  d->current_zoom = dt_conf_get_int("plugins/lighttable/images_in_row");

  /* Zoom */
  GtkWidget *label = gtk_label_new(C_("quickfilter", "Zoom"));
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(label), FALSE, FALSE, 0);
  dt_gui_add_class(label, "quickfilter-label");

  /* create horizontal zoom slider */
  d->zoom = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, DT_LIGHTTABLE_MAX_ZOOM, 1);
  gtk_widget_set_size_request(GTK_WIDGET(d->zoom), DT_PIXEL_APPLY_DPI(140), -1);
  gtk_scale_set_draw_value(GTK_SCALE(d->zoom), FALSE);
  gtk_range_set_increments(GTK_RANGE(d->zoom), 1, 1);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(d->zoom), FALSE, FALSE, 0);

  /* manual entry of the zoom level */
  d->zoom_entry = gtk_entry_new();
  dt_gui_add_class(GTK_WIDGET(d->zoom_entry), "menu-text-entry");
  gtk_entry_set_alignment(GTK_ENTRY(d->zoom_entry), 1.0);
  gtk_entry_set_max_length(GTK_ENTRY(d->zoom_entry), 2);
  gtk_entry_set_width_chars(GTK_ENTRY(d->zoom_entry), 3);
  gtk_entry_set_max_width_chars(GTK_ENTRY(d->zoom_entry), 3);
  gtk_box_pack_start(GTK_BOX(self->widget), GTK_WIDGET(d->zoom_entry), FALSE, FALSE, 0);

  g_signal_connect(G_OBJECT(d->zoom), "value-changed", G_CALLBACK(_lib_lighttable_zoom_slider_changed), self);
  g_signal_connect(d->zoom_entry, "key-press-event", G_CALLBACK(_lib_lighttable_zoom_entry_changed), self);
  gtk_range_set_value(GTK_RANGE(d->zoom), d->current_zoom);

  _lib_lighttable_zoom_slider_changed(GTK_RANGE(d->zoom), self); // the slider defaults to 1 and GTK doesn't
                                                                 // fire a value-changed signal when setting
                                                                 // it to 1 => empty text box
  darktable.view_manager->proxy.lighttable.module = self;
  darktable.view_manager->proxy.lighttable.set_zoom = _lib_lighttable_set_zoom;
  darktable.view_manager->proxy.lighttable.get_zoom = _lib_lighttable_get_zoom;
}

void gui_cleanup(dt_lib_module_t *self)
{
  g_free(self->data);
  self->data = NULL;
}

static void _set_zoom(dt_lib_module_t *self, int zoom)
{
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)self->data;
  dt_conf_set_int("plugins/lighttable/images_in_row", zoom);
  dt_thumbtable_zoom_changed(dt_ui_thumbtable(darktable.gui->ui), d->current_zoom, zoom);
}

static void _lib_lighttable_zoom_slider_changed(GtkRange *range, gpointer user_data)
{
  dt_lib_module_t *self = (dt_lib_module_t *)user_data;
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)self->data;

  const int i = gtk_range_get_value(range);
  gchar *i_as_str = g_strdup_printf("%d", i);
  gtk_entry_set_text(GTK_ENTRY(d->zoom_entry), i_as_str);
  _set_zoom(self, i);
  d->current_zoom = i;
  g_free(i_as_str);
}

static gboolean _lib_lighttable_zoom_entry_changed(GtkWidget *entry, GdkEventKey *event, dt_lib_module_t *self)
{
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)self->data;
  switch(event->keyval)
  {
    case GDK_KEY_Escape:
    case GDK_KEY_Tab:
    {
      // reset
      int i = dt_conf_get_int("plugins/lighttable/images_in_row");
      gchar *i_as_str = g_strdup_printf("%d", i);
      gtk_entry_set_text(GTK_ENTRY(d->zoom_entry), i_as_str);
      g_free(i_as_str);
      gtk_window_set_focus(GTK_WINDOW(dt_ui_main_window(darktable.gui->ui)), NULL);
      return FALSE;
    }

    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
    {
      // apply zoom level
      const gchar *value = gtk_entry_get_text(GTK_ENTRY(d->zoom_entry));
      int i = atoi(value);
      gtk_range_set_value(GTK_RANGE(d->zoom), i);
      gtk_window_set_focus(GTK_WINDOW(dt_ui_main_window(darktable.gui->ui)), NULL);
      return FALSE;
    }

    // allow 0 .. 9, left/right movement using arrow keys and del/backspace
    case GDK_KEY_0:
    case GDK_KEY_KP_0:
    case GDK_KEY_1:
    case GDK_KEY_KP_1:
    case GDK_KEY_2:
    case GDK_KEY_KP_2:
    case GDK_KEY_3:
    case GDK_KEY_KP_3:
    case GDK_KEY_4:
    case GDK_KEY_KP_4:
    case GDK_KEY_5:
    case GDK_KEY_KP_5:
    case GDK_KEY_6:
    case GDK_KEY_KP_6:
    case GDK_KEY_7:
    case GDK_KEY_KP_7:
    case GDK_KEY_8:
    case GDK_KEY_KP_8:
    case GDK_KEY_9:
    case GDK_KEY_KP_9:

    case GDK_KEY_Left:
    case GDK_KEY_Right:
    case GDK_KEY_Delete:
    case GDK_KEY_BackSpace:
      return FALSE;

    default: // block everything else
      return TRUE;
  }
}

static void _lib_lighttable_set_zoom(dt_lib_module_t *self, gint zoom)
{
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)self->data;
  gtk_range_set_value(GTK_RANGE(d->zoom), zoom);
  d->current_zoom = zoom;
}

static gint _lib_lighttable_get_zoom(dt_lib_module_t *self)
{
  dt_lib_tool_lighttable_t *d = (dt_lib_tool_lighttable_t *)self->data;
  return d->current_zoom;
}

#ifdef USE_LUA

static int zoom_level_cb(lua_State *L)
{
  dt_lib_module_t *self = lua_touserdata(L, lua_upvalueindex(1));
  const gint tmp = _lib_lighttable_get_zoom(self);
  if(lua_gettop(L) > 0){
    int value;
    luaA_to(L, int, &value, 1);
    _lib_lighttable_set_zoom(self, value);
  }
  luaA_push(L, int, &tmp);
  return 1;
}

void init(struct dt_lib_module_t *self)
{
  lua_State *L = darktable.lua_state.state;
  int my_type = dt_lua_module_entry_get_type(L, "lib", self->plugin_name);
  lua_pushlightuserdata(L, self);
  dt_lua_gtk_wrap(L);
  lua_pushcclosure(L, dt_lua_type_member_common, 1);
  lua_pushlightuserdata(L, self);
  lua_pushcclosure(L, zoom_level_cb, 1);
  dt_lua_gtk_wrap(L);
  lua_pushcclosure(L, dt_lua_type_member_common, 1);
  dt_lua_type_register_const_type(L, my_type, "zoom_level");
}
#endif
// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
