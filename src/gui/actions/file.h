#include "gui/actions/menu.h"
#include "common/collection.h"
#include "libs/collect.h"


static void pretty_print_collection(const char *buf, char *out, size_t outsize)
{
  memset(out, 0, outsize);

  if(!buf || buf[0] == '\0') return;

  int num_rules = 0;
  char str[400] = { 0 };
  int mode, item;
  int c;
  sscanf(buf, "%d", &num_rules);
  while(buf[0] != '\0' && buf[0] != ':') buf++;
  if(buf[0] == ':') buf++;

  for(int k = 0; k < num_rules; k++)
  {
    const int n = sscanf(buf, "%d:%d:%399[^$]", &mode, &item, str);

    if(n == 3)
    {
      if(k > 0) switch(mode)
        {
          case DT_LIB_COLLECT_MODE_AND:
            c = g_strlcpy(out, _(" and "), outsize);
            out += c;
            outsize -= c;
            break;
          case DT_LIB_COLLECT_MODE_OR:
            c = g_strlcpy(out, _(" or "), outsize);
            out += c;
            outsize -= c;
            break;
          default: // case DT_LIB_COLLECT_MODE_AND_NOT:
            c = g_strlcpy(out, _(" but not "), outsize);
            out += c;
            outsize -= c;
            break;
        }
      int i = 0;
      while(str[i] != '\0' && str[i] != '$') i++;
      if(str[i] == '$') str[i] = '\0';

      c = snprintf(out, outsize, "%s %s", item < DT_COLLECTION_PROP_LAST ? dt_collection_name(item) : "???",
                   item == 0 ? dt_image_film_roll_name(str) : str);
      out += c;
      outsize -= c;
    }
    while(buf[0] != '$' && buf[0] != '\0') buf++;
    if(buf[0] == '$') buf++;
  }
}


void update_collection_callback(GtkWidget *widget)
{
  // Grab the position of current menuitem in menu list
  const int index = GPOINTER_TO_INT(get_custom_data(widget));

  // Grab the corresponding config line
  char confname[200];
  snprintf(confname, sizeof(confname), "plugins/lighttable/recentcollect/line%1d", index);
  const char *collection = dt_conf_get_string_const(confname);

  snprintf(confname, sizeof(confname), "plugins/lighttable/recentcollect/pos%1d", index);
  const int position = dt_conf_get_int(confname);

  // Update the collection to the value defined in config line
  dt_collection_deserialize(collection);

  // Restore position in collection
  dt_thumbtable_t *table = dt_ui_thumbtable(darktable.gui->ui);
  dt_thumbtable_set_offset(table, position, TRUE);
}


void init_collection_line(gpointer instance,
                          dt_collection_change_t query_change,
                          dt_collection_properties_t changed_property, gpointer imgs, int next,
                          gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);

  // Grab the position of current menuitem in menu list
  const int index = GPOINTER_TO_INT(get_custom_data(widget));

  // Grab the corresponding config line
  char confname[200];
  snprintf(confname, sizeof(confname), "plugins/lighttable/recentcollect/line%1d", index);

  // Get the human-readable name of the collection
  const char *collection = dt_conf_get_string_const(confname);


  if(collection && collection[0] != '\0')
  {
    char label[2048] = { 0 };
    pretty_print_collection(collection, label, sizeof(label));
    dt_capitalize_label(label);

    // Update the menu entry label for current collection name
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(widget));
    gtk_label_set_markup(GTK_LABEL(child), label);
  }
}


void append_file(GtkWidget **menus, GList **lists, const dt_menus_t index)
{
  dt_action_t *pnl = dt_action_section(&darktable.control->actions_global, N_("File"));
  dt_action_t *ac;

  add_top_submenu_entry(menus, lists, _("Recent collections"), index);
  GtkWidget *parent = get_last_widget(lists);

  for(int i = 0; i < NUM_LAST_COLLECTIONS; i++)
  {
    // Pass the position of current menuitem in list as custom-data pointer
    add_sub_sub_menu_entry(parent, lists, "", index, GINT_TO_POINTER(i), update_collection_callback, NULL, NULL, NULL);

    // Call init directly just this once
    GtkWidget *this = get_last_widget(lists);
    init_collection_line(NULL, DT_COLLECTION_CHANGE_NONE, DT_COLLECTION_PROP_UNDEF, NULL, 0, this);

    // Connect init to collection_changed signal for future updates
    DT_DEBUG_CONTROL_SIGNAL_CONNECT(darktable.signals, DT_SIGNAL_COLLECTION_CHANGED,
                              G_CALLBACK(init_collection_line), (gpointer)this);
  }

  add_menu_separator(menus[index]);

  add_sub_menu_entry(menus, lists, _("Copy files on disk…"), index, NULL, dt_control_copy_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Copy files on disk"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_copy_images, 0, 0);

  add_sub_menu_entry(menus, lists, _("Move files on disk…"), index, NULL, dt_control_move_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Move files on disk"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_move_images, 0, 0);

  add_sub_menu_entry(menus, lists, _("Create a blended HDR"), index, NULL, dt_control_merge_hdr, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Create a blended HDR"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_merge_hdr, 0, 0);

  add_menu_separator(menus[index]);

  add_sub_menu_entry(menus, lists, _("Copy distant images locally"), index, NULL, dt_control_set_local_copy_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Copy distant images locally"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_set_local_copy_images, 0, 0);

  add_sub_menu_entry(menus, lists, _("Resynchronize distant images"), index, NULL, dt_control_reset_local_copy_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Resynchronize distant images"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_reset_local_copy_images, 0, 0);

  add_menu_separator(menus[index]);

  add_sub_menu_entry(menus, lists, _("Remove from library"), index, NULL, (void *)dt_control_remove_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Remove files from library"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, (void *)dt_control_remove_images, GDK_KEY_Delete, 0);

  add_sub_menu_entry(menus, lists, _("Delete on disk"), index, NULL, dt_control_delete_images, NULL, NULL,
                     sensitive_if_selected);
  ac = dt_action_define(pnl, NULL, N_("Delete files on disk"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_delete_images, GDK_KEY_Delete, GDK_SHIFT_MASK);

  add_menu_separator(menus[index]);

  add_sub_menu_entry(menus, lists, _("Quit"), index, NULL, dt_control_quit, NULL, NULL, NULL);
  ac = dt_action_define(pnl, NULL, N_("Quit"), get_last_widget(lists), NULL);
  dt_action_register(ac, NULL, dt_control_quit, GDK_KEY_q, GDK_CONTROL_MASK);
}
