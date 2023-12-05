#include "common/darktable.h"
#include "control/control.h"
#include "gui/actions/menu.h"
#include "gui/gtk.h"
#include "gui/accelerators.h"
#include "gui/window_manager.h"

#include <gtk/gtk.h>


/** FULL SCREEN MODE **/
gboolean full_screen_checked_callback(GtkWidget *w)
{
  GtkWidget *widget = dt_ui_main_window(darktable.gui->ui);
  return gdk_window_get_state(gtk_widget_get_window(widget)) & GDK_WINDOW_STATE_FULLSCREEN;
}

static void full_screen_callback()
{
  GtkWidget *widget = dt_ui_main_window(darktable.gui->ui);

  if(full_screen_checked_callback(widget))
    gtk_window_unfullscreen(GTK_WINDOW(widget));
  else
    gtk_window_fullscreen(GTK_WINDOW(widget));

  dt_dev_invalidate(darktable.develop);
  dt_dev_refresh_ui_images(darktable.develop);

  /* redraw center view */
  gtk_widget_queue_draw(widget);

#ifdef __APPLE__
  // workaround for GTK Quartz backend bug
  gtk_window_set_title(GTK_WINDOW(widget), widget == dt_ui_main_window(darktable.gui->ui)
                                         ? "Ansel" : _("Ansel - Darkroom preview"));
#endif
}

/** SIDE PANELS COLLAPSE **/
static gboolean _panel_is_visible(dt_ui_panel_t panel)
{
  gchar *key = panels_get_view_path("panel_collaps_state");
  if(dt_conf_get_int(key))
  {
    g_free(key);
    return FALSE;
  }
  key = panels_get_panel_path(panel, "_visible");
  const gboolean ret = dt_conf_get_bool(key);
  g_free(key);
  return ret;
}

static void _toggle_side_borders_accel_callback(dt_action_t *action)
{
  dt_ui_toggle_panels_visibility(darktable.gui->ui);

  /* trigger invalidation of centerview to reprocess pipe */
  dt_dev_invalidate(darktable.develop);
  dt_dev_refresh_ui_images(darktable.develop);
  gtk_widget_queue_draw(dt_ui_center(darktable.gui->ui));
}

void dt_ui_toggle_panels_visibility(struct dt_ui_t *ui)
{
  gchar *key = panels_get_view_path("panel_collaps_state");
  const uint32_t state = dt_conf_get_int(key);

  if(state) dt_conf_set_int(key, 0);
  else dt_conf_set_int(key, 1);

  dt_ui_restore_panels(ui);
  g_free(key);
}

void dt_ui_panel_show(dt_ui_t *ui, const dt_ui_panel_t p, gboolean show, gboolean write)
{
  g_return_if_fail(GTK_IS_WIDGET(ui->panels[p]));

  // for left and right sides, panels are inside a gtkoverlay
  GtkWidget *over_panel = NULL;
  if(p == DT_UI_PANEL_LEFT || p == DT_UI_PANEL_RIGHT || p == DT_UI_PANEL_BOTTOM)
    over_panel = gtk_widget_get_parent(ui->panels[p]);

  if(show)
  {
    gtk_widget_show(ui->panels[p]);
    if(over_panel) gtk_widget_show(over_panel);
  }
  else
  {
    gtk_widget_hide(ui->panels[p]);
    if(over_panel) gtk_widget_hide(over_panel);
  }

  if(write)
  {
    gchar *key;
    if(show)
    {
      // we reset the collaps_panel value if we show a panel
      key = panels_get_view_path("panel_collaps_state");
      if(dt_conf_get_int(key) != 0)
      {
        dt_conf_set_int(key, 0);
        g_free(key);
        // we ensure that all panels state are recorded as hidden
        for(int k = 0; k < DT_UI_PANEL_SIZE; k++)
        {
          key = panels_get_panel_path(k, "_visible");
          dt_conf_set_bool(key, FALSE);
          g_free(key);
        }
      }
      else
        g_free(key);
      key = panels_get_panel_path(p, "_visible");
      dt_conf_set_bool(key, show);
      g_free(key);
    }
    else
    {
      // if it was the last visible panel, we set collaps_panel value instead
      // so collapsing panels after will have an effect
      gboolean collapse = TRUE;
      for(int k = 0; k < DT_UI_PANEL_SIZE; k++)
      {
        if(k != p && dt_ui_panel_visible(ui, k))
        {
          collapse = FALSE;
          break;
        }
      }

      if(collapse)
      {
        key = panels_get_view_path("panel_collaps_state");
        dt_conf_set_int(key, 1);
        g_free(key);
      }
      else
      {
        key = panels_get_panel_path(p, "_visible");
        dt_conf_set_bool(key, show);
        g_free(key);
      }
    }
  }
}

gboolean dt_ui_panel_visible(dt_ui_t *ui, const dt_ui_panel_t p)
{
  g_return_val_if_fail(GTK_IS_WIDGET(ui->panels[p]), FALSE);
  return gtk_widget_get_visible(ui->panels[p]);
}


void panel_left_callback()
{
  dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_LEFT, !_panel_is_visible(DT_UI_PANEL_LEFT), TRUE);
}

static gboolean panel_left_checked_callback(GtkWidget *widget)
{
  return dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_LEFT);
}

void panel_right_callback()
{
  dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_RIGHT, !_panel_is_visible(DT_UI_PANEL_RIGHT), TRUE);
}

static gboolean panel_right_checked_callback(GtkWidget *widget)
{
  return dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_RIGHT);
}

void panel_top_callback()
{
  dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_TOP, !_panel_is_visible(DT_UI_PANEL_CENTER_TOP), TRUE);
}

static gboolean panel_top_checked_callback(GtkWidget *widget)
{
  return dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_CENTER_TOP);
}

void panel_bottom_callback()
{
  dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_CENTER_BOTTOM, !_panel_is_visible(DT_UI_PANEL_CENTER_BOTTOM), TRUE);
}

static gboolean panel_bottom_checked_callback(GtkWidget *widget)
{
  return dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_CENTER_BOTTOM);
}

static void filmstrip_callback()
{
  dt_ui_panel_show(darktable.gui->ui, DT_UI_PANEL_BOTTOM, !_panel_is_visible(DT_UI_PANEL_BOTTOM), TRUE);
}

static gboolean filmstrip_checked_callback(GtkWidget *widget)
{
  return dt_ui_panel_visible(darktable.gui->ui, DT_UI_PANEL_BOTTOM);
}

static gboolean filmstrip_sensitive_callback(GtkWidget *widget)
{
  // Filmstrip is not visible in lighttable
  const dt_view_t *view = dt_view_manager_get_current_view(darktable.view_manager);
  return (view && strcmp(view->module_name, "lighttable"));
}

static gboolean profile_checked_callback(GtkWidget *widget)
{
  dt_colorspaces_color_profile_t *prof = (dt_colorspaces_color_profile_t *)get_custom_data(widget);
  return (prof->type == darktable.color_profiles->display_type
          && (prof->type != DT_COLORSPACE_FILE
              || !strcmp(prof->filename, darktable.color_profiles->display_filename)));
}

static void profile_callback(GtkWidget *widget)
{
  dt_colorspaces_color_profile_t *pp = (dt_colorspaces_color_profile_t *)get_custom_data(widget);

  gboolean profile_changed = FALSE;
  if(darktable.color_profiles->display_type != pp->type
      || (darktable.color_profiles->display_type == DT_COLORSPACE_FILE
           && strcmp(darktable.color_profiles->display_filename, pp->filename)))
  {
    darktable.color_profiles->display_type = pp->type;
    g_strlcpy(darktable.color_profiles->display_filename, pp->filename,
              sizeof(darktable.color_profiles->display_filename));
    profile_changed = TRUE;
  }

  if(!profile_changed)
  {
    // profile not found, fall back to system display profile. shouldn't happen
    fprintf(stderr, "can't find display profile `%s', using system display profile instead\n", pp->filename);
    profile_changed = darktable.color_profiles->display_type != DT_COLORSPACE_DISPLAY;
    darktable.color_profiles->display_type = DT_COLORSPACE_DISPLAY;
    darktable.color_profiles->display_filename[0] = '\0';
  }

  if(profile_changed)
  {
    pthread_rwlock_rdlock(&darktable.color_profiles->xprofile_lock);
    dt_colorspaces_update_display_transforms();
    pthread_rwlock_unlock(&darktable.color_profiles->xprofile_lock);
    DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_CONTROL_PROFILE_USER_CHANGED, DT_COLORSPACES_PROFILE_TYPE_DISPLAY);
    dt_dev_invalidate_all(darktable.develop);
    dt_dev_refresh_ui_images(darktable.develop);
  }
}

dt_iop_color_intent_t string_to_color_intent(const char *string)
{
  if(!strcmp(string, "perceptual"))
    return DT_INTENT_PERCEPTUAL;
  else if(!strcmp(string, "relative colorimetric"))
    return DT_INTENT_RELATIVE_COLORIMETRIC;
  else if(!strcmp(string, "saturation"))
    return DT_INTENT_SATURATION;
  else if(!strcmp(string, "absolute colorimetric"))
    return DT_INTENT_ABSOLUTE_COLORIMETRIC;
  else
    return DT_INTENT_PERCEPTUAL;

  // Those seem to make no difference with most ICC profiles anyway.
  // Perceptual needs A_to_B and B_to_A LUT defined in the .icc profile to work.
  // Since most profiles don't have them, it falls back to something close to relative colorimetric.
  // Really not sure if it's our implementation or if it's LittleCMS2 that is faulty here.
  // This option just makes it look like pRoFESsional CoLoR mAnAgeMEnt®©.
  // ICC intents are pretty much bogus in the first place… (gamut mapping by RGB clipping…)
}

static void intent_callback(GtkWidget *widget)
{
  dt_iop_color_intent_t old_intent = darktable.color_profiles->display_intent;
  dt_iop_color_intent_t new_intent = string_to_color_intent(get_custom_data(widget));
  if(new_intent != old_intent)
  {
    darktable.color_profiles->display_intent = new_intent;
    pthread_rwlock_rdlock(&darktable.color_profiles->xprofile_lock);
    dt_colorspaces_update_display_transforms();
    pthread_rwlock_unlock(&darktable.color_profiles->xprofile_lock);
    dt_dev_invalidate_all(darktable.develop);
    dt_dev_refresh_ui_images(darktable.develop);
  }
}

static gboolean intent_checked_callback(GtkWidget *widget)
{
  return darktable.color_profiles->display_intent == string_to_color_intent(get_custom_data(widget));
}

static void always_hide_overlays_callback()
{
  dt_thumbtable_set_overlays_mode(dt_ui_thumbtable(darktable.gui->ui), DT_THUMBNAIL_OVERLAYS_NONE);
}

static gboolean always_hide_overlays_checked_callback(GtkWidget *widget)
{
  return dt_conf_get_int("plugins/lighttable/overlays/global") == DT_THUMBNAIL_OVERLAYS_NONE;
}

static void hover_overlays_callback()
{
  dt_thumbtable_set_overlays_mode(dt_ui_thumbtable(darktable.gui->ui), DT_THUMBNAIL_OVERLAYS_HOVER_NORMAL);
}

static gboolean hover_overlays_checked_callback(GtkWidget *widget)
{
  return dt_conf_get_int("plugins/lighttable/overlays/global") == DT_THUMBNAIL_OVERLAYS_HOVER_NORMAL;
}

static void always_show_overlays_callback()
{
  dt_thumbtable_set_overlays_mode(dt_ui_thumbtable(darktable.gui->ui), DT_THUMBNAIL_OVERLAYS_ALWAYS_NORMAL);
}

static gboolean always_show_overlays_checked_callback(GtkWidget *widget)
{
  return dt_conf_get_int("plugins/lighttable/overlays/global") == DT_THUMBNAIL_OVERLAYS_ALWAYS_NORMAL;
}

static void collapse_grouped_callback()
{
  darktable.gui->grouping = !darktable.gui->grouping;
  dt_conf_set_bool("ui_last/grouping", darktable.gui->grouping);
  darktable.gui->expanded_group_id = -1;
  dt_collection_update_query(darktable.collection, DT_COLLECTION_CHANGE_RELOAD, DT_COLLECTION_PROP_GROUPING, NULL);
}

static gboolean collapse_grouped_checked_callback()
{
  return darktable.gui->grouping;
}

static void focus_peaking_callback()
{
  darktable.gui->show_focus_peaking = !darktable.gui->show_focus_peaking;
  // Redraw all thumbnails
  DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_DEVELOP_MIPMAP_UPDATED, -1);
}

static gboolean focus_peaking_checked_callback()
{
  return darktable.gui->show_focus_peaking;
}

void append_display(GtkWidget **menus, GList **lists, const dt_menus_t index)
{
  dt_action_t *pnl = dt_action_section(&darktable.control->actions_global, N_("Display"));
  dt_action_t *ac;

  // Parent sub-menu color profile
  add_top_submenu_entry(menus, lists, _("Monitor color profile"), index);
  GtkWidget *parent = get_last_widget(lists);

  // Add available color profiles to the sub-menu
  for(const GList *l = darktable.color_profiles->profiles; l; l = g_list_next(l))
  {
    dt_colorspaces_color_profile_t *prof = (dt_colorspaces_color_profile_t *)l->data;
    if(prof->display_pos > -1)
    {
      add_sub_sub_menu_entry(parent, lists, prof->name, index, prof, profile_callback, profile_checked_callback, NULL, NULL);
      //gtk_check_menu_item_set_draw_as_radio(GTK_CHECK_MENU_ITEM(get_last_widget(lists)), TRUE);
    }
  }

  // Parent sub-menu profile intent
  add_top_submenu_entry(menus, lists, _("Monitor color intent"), index);
  parent = get_last_widget(lists);

  const char *intents[4] = { _("Perceptual"), _("Relative colorimetric"), C_("rendering intent", "Saturation"),
                             _("Absolute colorimetric") };
  // non-translatable strings to store in menu items for later mapping with dt_iop_color_intent_t
  const char *data[4] = { "perceptual", "relative colorimetric", "saturation", "absolute colorimetric" };

  for(int i = 0; i < 4; i++)
    add_sub_sub_menu_entry(parent, lists, intents[i], index, (void *)data[i], intent_callback, intent_checked_callback, NULL, NULL);

  add_menu_separator(menus[index]);

  // Parent sub-menu panels
  add_top_submenu_entry(menus, lists, _("Panels"), index);
  parent = get_last_widget(lists);

  // Children of sub-menu panels
  add_sub_sub_menu_entry(parent, lists, _("Left"), index, NULL,
                         panel_left_callback, panel_left_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Toggle left panel visibility"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, panel_left_callback, GDK_KEY_l, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_sub_sub_menu_entry(parent, lists, _("Right"), index, NULL,
                         panel_right_callback, panel_right_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Toggle right panel visibility"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, panel_right_callback, GDK_KEY_r, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_sub_sub_menu_entry(parent, lists, _("Top"), index, NULL,
                         panel_top_callback, panel_top_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Toggle top bar visibility"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, panel_top_callback, GDK_KEY_t, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_sub_sub_menu_entry(parent, lists, _("Bottom"), index, NULL,
                         panel_bottom_callback, panel_bottom_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Toggle bottom bar visibility"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, panel_bottom_callback, GDK_KEY_b, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_sub_sub_menu_entry(parent, lists, _("Filmstrip"), index, NULL,
                         filmstrip_callback, filmstrip_checked_callback, NULL, filmstrip_sensitive_callback);
  ac = dt_action_define(pnl, NULL, N_("Toggle filmstrip visibility"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, filmstrip_callback, GDK_KEY_f, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_menu_separator(menus[index]);

  // Lighttable & Filmstrip options
  add_top_submenu_entry(menus, lists, _("Thumbnail overlays"), index);
  parent = get_last_widget(lists);

  add_sub_sub_menu_entry(parent, lists, _("Always hide"), index, NULL,
                         always_hide_overlays_callback, always_hide_overlays_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Always hide thumbnail overlays"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, always_hide_overlays_callback, 0, 0);

  add_sub_sub_menu_entry(parent, lists, _("Show on hover"), index, NULL,
                         hover_overlays_callback, hover_overlays_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Show thumbnail overlays on hover"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, hover_overlays_callback, 0, 0);

  add_sub_sub_menu_entry(parent, lists, _("Always show"), index, NULL,
                         always_show_overlays_callback, always_show_overlays_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Always show thumbnail overlays"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, always_show_overlays_callback, 0, 0);

  add_sub_menu_entry(menus, lists, _("Collapse grouped images"), index, NULL, collapse_grouped_callback, collapse_grouped_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Collapse grouped images"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, collapse_grouped_callback, 0, 0);

  add_sub_menu_entry(menus, lists, _("Overlay focus peaking"), index, NULL, focus_peaking_callback, focus_peaking_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Overlay focus peaking"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, focus_peaking_callback, GDK_KEY_p, GDK_CONTROL_MASK | GDK_SHIFT_MASK);

  add_menu_separator(menus[index]);

  add_sub_menu_entry(menus, lists, _("Full screen"), index, NULL, full_screen_callback,
                     full_screen_checked_callback, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Full screen"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, full_screen_callback, GDK_KEY_F11, 0);

  // specific top/bottom toggles
  dt_action_register(pnl, N_("Toggle all panels visibility"), _toggle_side_borders_accel_callback, GDK_KEY_Tab, 0);
}
