/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#ifndef XQF_UI_H__
#define XQF_UI_H__

#include <gtk/gtk.h>

#include <xqf.h>


struct clist_column {
  const char *name;
  int	width;
  GtkJustification justify;
  GtkWidget *widget;
};


enum cwidget_type {
  CWIDGET_CLIST = 0,
  CWIDGET_CTREE
};


struct clist_def {
  enum cwidget_type type;
  char *name;

  struct clist_column *cols;

  int	columns;
  GtkSelectionMode mode;

  int	width;
  int	height;

  int   sort_column;
  GtkSortType sort_type;
};

extern	struct clist_def server_clist_def;
extern	struct clist_def player_clist_def;
extern	struct clist_def srvinf_clist_def;

extern	GtkWidget *pane1_widget;
extern  GtkWidget *pane2_widget;
extern  GtkWidget *pane3_widget;


extern 	GtkWidget *main_window;
extern	GtkWidget *source_ctree;
extern  GtkCList  *server_clist;
extern  GtkCList  *player_clist;
extern  GtkCTree  *srvinf_ctree;

extern	GtkWidget *view_hostnames_menu_item;
extern	GtkWidget *view_defport_menu_item;

extern	int window_delete_event_callback (GtkWidget *widget, gpointer data);
extern	void register_window (GtkWidget *window);
extern	void unregister_window (GtkWidget *window);
extern	GtkWidget *top_window (void);

extern	void set_widgets_sensitivity (void);

extern GtkWidget *server_filter_widget[];

extern	void print_status (GtkWidget *sbar, char *fmt, ...);

extern	int window_delete_event_callback (GtkWidget *widget, gpointer data);
extern	void register_window (GtkWidget *window);
extern	void unregister_window (GtkWidget *window);
extern	GtkWidget *top_window (void);

extern	GtkWidget *create_cwidget (GtkWidget *scrollwin, 
                                                     struct clist_def *cldef);

extern	int clist_change_sort_mode (struct clist_def *cldef, int col);

extern	void clist_set_sort_column (GtkCList *clist, int column, 
                                                     struct clist_def *cldef);

extern	void source_ctree_show_node_status (GtkWidget *ctree, 
                                                            struct master *m);

extern	void source_ctree_add_master (GtkWidget *ctree, struct master *m);
extern	void source_ctree_delete_master (GtkWidget *ctree, struct master *m);
extern	void source_ctree_remove_master_group (GtkWidget *ctree, struct master *m);
extern	GtkWidget *create_source_ctree (GtkWidget *scrollwin);
extern	void source_ctree_select_source (struct master *m);

extern	int calculate_clist_row_height (GtkWidget *clist, GdkPixmap *pixmap);

extern	void set_toolbar_appearance (GtkToolbar *toolbar, int style, int tips);

extern	GtkWidget *create_progress_bar (void);
extern	void progress_bar_reset (GtkWidget *pbar);
extern	void progress_bar_start (GtkWidget *pbar, int activity_mode);
extern	void progress_bar_set_percentage (GtkWidget *pbar, float percentage);

extern	void ui_done (void);
extern	void restore_main_window_geometry (void);

extern 	GtkTooltips *tooltips;

/*
 * This function returns a widget in a component created by Glade.
 * Call it with the toplevel widget in the component (i.e. a window/dialog),
 * or alternatively any widget in the component, and the name of the widget
 * you want returned.
 */
GtkWidget* lookup_widget (GtkWidget* widget, const gchar* widget_name);


/**
  create a GtkOptionMenu that contains all game names
  @param active_type which game to set active by default
  @param filterfunc only add game to menu if this function return true. can be
	  NULL to show all
  @param callback a callback function to connect if a game entry is activated.
	  the user_data will be set to the game type
  @return a GtkOptionMenu
  */
GtkWidget *create_server_type_menu (enum server_type active_type,
	gboolean (*filterfunc)(enum server_type),
	GtkSignalFunc callback);


/** Skip a game if it's not configured and show only configured is enabled */
gboolean create_server_type_menu_filter_configured(enum server_type type);

#endif /* XQF_UI_H__ */
