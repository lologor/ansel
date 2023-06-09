#pragma once

#include "common/darktable.h"

#include <gtk/gtk.h>
#include <stdint.h>

#define DT_UI_PANEL_MODULE_SPACING 0
#define DT_UI_PANEL_SIDE_DEFAULT_SIZE 350
#define DT_UI_PANEL_BOTTOM_DEFAULT_SIZE 120

typedef enum dt_ui_panel_t
{
  /* the header panel */
  DT_UI_PANEL_TOP,
  /* center top toolbar panel */
  DT_UI_PANEL_CENTER_TOP,
  /* center bottom toolbar panel */
  DT_UI_PANEL_CENTER_BOTTOM,
  /* left panel */
  DT_UI_PANEL_LEFT,
  /* right panel */
  DT_UI_PANEL_RIGHT,
  /* bottom panel */
  DT_UI_PANEL_BOTTOM,

  DT_UI_PANEL_SIZE
} dt_ui_panel_t;

typedef enum dt_ui_container_t
{
  /* the top container of left panel, the top container
     disables the module expander and does not scroll with other modules
  */
  DT_UI_CONTAINER_PANEL_LEFT_TOP = 0,

  /* the center container of left panel, the center container
     contains the scrollable area that all plugins are placed within and last
     widget is the end marker.
     This container will always expand|fill empty vertical space
  */
  DT_UI_CONTAINER_PANEL_LEFT_CENTER = 1,

  /* the bottom container of left panel, this container works just like
     the top container but will be attached to bottom in the panel, such as
     plugins like background jobs module in lighttable and the plugin selection
     module in darkroom,
  */
  DT_UI_CONTAINER_PANEL_LEFT_BOTTOM = 2,

  DT_UI_CONTAINER_PANEL_RIGHT_TOP = 3,
  DT_UI_CONTAINER_PANEL_RIGHT_CENTER = 4,
  DT_UI_CONTAINER_PANEL_RIGHT_BOTTOM = 5,


  /* the top header bar, left slot where darktable name is placed */
  DT_UI_CONTAINER_PANEL_TOP_FIRST_ROW = 6,
  /* center which is expanded as wide it can */
  DT_UI_CONTAINER_PANEL_TOP_SECOND_ROW = 7,
  /* right side were the different views are accessed */
  DT_UI_CONTAINER_PANEL_TOP_THIRD_ROW = 8,

  DT_UI_CONTAINER_PANEL_CENTER_TOP_LEFT = 9,
  DT_UI_CONTAINER_PANEL_CENTER_TOP_CENTER = 10,
  DT_UI_CONTAINER_PANEL_CENTER_TOP_RIGHT = 11,

  DT_UI_CONTAINER_PANEL_CENTER_BOTTOM_LEFT = 12,
  DT_UI_CONTAINER_PANEL_CENTER_BOTTOM_CENTER = 13,
  DT_UI_CONTAINER_PANEL_CENTER_BOTTOM_RIGHT = 14,

  /* this panel is placed at bottom of ui
     only used by the filmstrip if shown */
  DT_UI_CONTAINER_PANEL_BOTTOM = 15,

  /* Count of containers */
  DT_UI_CONTAINER_SIZE
} dt_ui_container_t;


typedef struct dt_window_manager_t
{
  // Convenience list of GUI sizes for easy sizes sanitization
  GtkAllocation containers[DT_UI_CONTAINER_SIZE];
  GtkAllocation panels[DT_UI_PANEL_SIZE];
  GtkAllocation center;
  GtkAllocation window;
  GtkAllocation viewport;
} dt_window_manager_t;

typedef struct dt_ui_t
{
  /* container widgets */
  GtkWidget *containers[DT_UI_CONTAINER_SIZE];

  /* panel widgets */
  GtkWidget *panels[DT_UI_PANEL_SIZE];

  /* center widget */
  GtkWidget *center;
  GtkWidget *center_base;

  /* main widget */
  GtkWidget *main_window;

  /* thumb table */
  dt_thumbtable_t *thumbtable;

  /* log msg and toast labels */
  GtkWidget *log_msg, *toast_msg;

  /* keep track of sizes for sanitization */
  dt_window_manager_t manager;
} dt_ui_t;

gchar *panels_get_view_path(char *suffix);
gchar *panels_get_panel_path(dt_ui_panel_t panel, char *suffix);

int dt_ui_panel_get_size(dt_ui_t *ui, const dt_ui_panel_t p);
void dt_ui_panel_set_size(dt_ui_t *ui, const dt_ui_panel_t p, int s);

gboolean dt_ui_panel_ancestor(struct dt_ui_t *ui, const dt_ui_panel_t p, GtkWidget *w);
GtkWidget *dt_ui_center(dt_ui_t *ui);
GtkWidget *dt_ui_center_base(dt_ui_t *ui);
dt_thumbtable_t *dt_ui_thumbtable(struct dt_ui_t *ui);
GtkWidget *dt_ui_log_msg(struct dt_ui_t *ui);
GtkWidget *dt_ui_toast_msg(struct dt_ui_t *ui);
GtkWidget *dt_ui_main_window(dt_ui_t *ui);

void init_main_table(GtkWidget *container, struct dt_ui_t *ui);
GtkBox *dt_ui_get_container(struct dt_ui_t *ui, const dt_ui_container_t c);
void dt_ui_container_add_widget(dt_ui_t *ui, const dt_ui_container_t c, GtkWidget *w);

void dt_ui_restore_panels(dt_ui_t *ui);
void dt_ui_update_scrollbars(dt_ui_t *ui);
void dt_ui_scrollbars_show(dt_ui_t *ui, gboolean show);
