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

#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>	/* va_start, va_end */

#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "srv-list.h"
#include "source.h"
#include "game.h"
#include "config.h"
#include "sort.h"


static GSList *xqf_windows = NULL;
static GtkWidget *target_window = NULL;

GtkWidget *pane1_widget;
GtkWidget *pane2_widget;
GtkWidget *pane3_widget;

static struct clist_column server_columns[8] = {
  { "Name",    180,  GTK_JUSTIFY_LEFT,   NULL },
  { "Address", 140,  GTK_JUSTIFY_LEFT,   NULL },
  { "Ping",     45,  GTK_JUSTIFY_RIGHT,  NULL },
  { "TO",       35,  GTK_JUSTIFY_RIGHT,  NULL },
  { "Players",  65,  GTK_JUSTIFY_RIGHT,  NULL },
  { "Map",      55,  GTK_JUSTIFY_LEFT,   NULL },
  { "Game",     55,  GTK_JUSTIFY_LEFT,   NULL },
  { "MOD",      55,  GTK_JUSTIFY_LEFT,   NULL }
};

struct clist_def server_clist_def = {
  CWIDGET_CLIST,
  "Server List",
  server_columns,
  8,
  GTK_SELECTION_EXTENDED,
  630, 270,
  SORT_SERVER_PING, GTK_SORT_ASCENDING
};
	
static struct clist_column player_columns[6] = {
  { "Name",    100,  GTK_JUSTIFY_LEFT,   NULL },
  { "Frags",    50,  GTK_JUSTIFY_RIGHT,  NULL },
  { "Colors",   60,  GTK_JUSTIFY_LEFT,   NULL },
  { "Skin",     50,  GTK_JUSTIFY_LEFT,   NULL },
  { "Ping",     45,  GTK_JUSTIFY_RIGHT,  NULL },
  { "Time",     45,  GTK_JUSTIFY_LEFT,   NULL }
};

struct clist_def player_clist_def = {
  CWIDGET_CLIST,
  "Player List",
  player_columns,
  6,
  GTK_SELECTION_SINGLE,
  400, 180,
  SORT_PLAYER_FRAGS, GTK_SORT_DESCENDING
};
						  
static struct clist_column srvinf_columns[2] = {
  { "Rule",     90,  GTK_JUSTIFY_LEFT,   NULL },
  { "Value",    80,  GTK_JUSTIFY_LEFT,   NULL }
};

struct clist_def srvinf_clist_def = {
  CWIDGET_CTREE,
  "Rule List",
  srvinf_columns,
  2,
  GTK_SELECTION_SINGLE,
  210, 180,
  SORT_INFO_RULE, GTK_SORT_ASCENDING
};


void print_status (GtkWidget *sbar, char *fmt, ...) {
  unsigned context_id;
  char buf[1024];
  va_list ap;

  if (sbar) {
    buf[0] = ' ';		/* indent */
    buf[1] = '\0';

    if (fmt) {
      va_start (ap, fmt);
      g_vsnprintf (buf + 1, 1024 - 1, fmt, ap);
      va_end (ap);
    }

#ifdef DEBUG
    fprintf (stderr, "Status: %s\n", buf);
#endif

    context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (sbar), "XQF");

    gtk_statusbar_pop (GTK_STATUSBAR (sbar), context_id);
    gtk_statusbar_push (GTK_STATUSBAR (sbar), context_id, buf);
  }
}


int window_delete_event_callback (GtkWidget *widget, gpointer data) {
  target_window = widget;
  gtk_widget_destroy ((GtkWidget *) (xqf_windows->data));
  return TRUE;
}


void register_window (GtkWidget *window) {
  xqf_windows = g_slist_prepend (xqf_windows, window);
}


void unregister_window (GtkWidget *window) {
  GSList *first;

  first = xqf_windows;
  xqf_windows = g_slist_next (xqf_windows);
  g_slist_free_1 (first);

  if (target_window && target_window != window)
    gtk_widget_destroy ((GtkWidget *) (xqf_windows->data));
  else
    target_window = NULL;
}


GtkWidget *top_window (void) {
  if (xqf_windows)
    return (GtkWidget *) xqf_windows->data;
  else
    return NULL;
}


static void clist_column_set_title (GtkCList *clist, struct clist_def *cldef, 
                                                               int set_mark) {
  char buf[128];

  if (set_mark) {
    g_snprintf (buf, 128, "%s %c", cldef->cols[clist->sort_column].name, 
                        (clist->sort_type == GTK_SORT_DESCENDING)? '>' : '<');
    gtk_label_set (GTK_LABEL (cldef->cols[clist->sort_column].widget), buf);
  }
  else {
    gtk_label_set (GTK_LABEL (cldef->cols[clist->sort_column].widget), 
                                        cldef->cols[clist->sort_column].name);
  }
}


GtkWidget *create_cwidget (GtkWidget *scrollwin, struct clist_def *cldef) {
  GtkWidget *alignment;
  GtkWidget *label;
  GtkWidget *clist;
  char buf[256];
  int i;

  switch (cldef->type) {

  case CWIDGET_CLIST:
    clist = gtk_clist_new (cldef->columns);
    break;

  case CWIDGET_CTREE:
    clist = gtk_ctree_new (cldef->columns, 0);
    gtk_ctree_set_line_style (GTK_CTREE (clist), GTK_CTREE_LINES_NONE);
    gtk_ctree_set_expander_style (GTK_CTREE (clist), 
                                                 GTK_CTREE_EXPANDER_TRIANGLE);
    gtk_ctree_set_indent (GTK_CTREE (clist), 10);
    break;

  default:
    return NULL;

  }

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  GTK_CLIST_SET_FLAG (GTK_CLIST (clist), CLIST_SHOW_TITLES);
  gtk_container_add (GTK_CONTAINER (scrollwin), clist);

  gtk_clist_set_selection_mode (GTK_CLIST (clist), cldef->mode);

  for (i = 0; i < cldef->columns; i++) {
    g_snprintf (buf, 256, "/" CONFIG_FILE "/%s Geometry/%s=%d",
                      cldef->name, cldef->cols[i].name, cldef->cols[i].width);
    gtk_clist_set_column_width (GTK_CLIST (clist), i, config_get_int (buf));
    if (cldef->cols[i].justify != GTK_JUSTIFY_LEFT) {
      gtk_clist_set_column_justification (GTK_CLIST (clist), i, 
                                                      cldef->cols[i].justify);
    }

    alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);

    label = gtk_label_new (cldef->cols[i].name);
    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
    gtk_container_add (GTK_CONTAINER (alignment), label);
    gtk_widget_show (label);

    cldef->cols[i].widget = label;

    gtk_clist_set_column_widget (GTK_CLIST (clist), i, alignment);
    gtk_widget_show (alignment);
  }

  gtk_clist_set_sort_column (GTK_CLIST (clist), cldef->sort_column);
  gtk_clist_set_sort_type (GTK_CLIST (clist), cldef->sort_type);

  clist_column_set_title (GTK_CLIST (clist), cldef, TRUE);

  return clist;
}


void clist_set_sort_column (GtkCList *clist, int column, 
                                                    struct clist_def *cldef) {
  if (column == clist->sort_column) {
    gtk_clist_set_sort_type (clist, 
                 GTK_SORT_DESCENDING + GTK_SORT_ASCENDING - clist->sort_type);
  }
  else {
    clist_column_set_title (clist, cldef, FALSE);
    gtk_clist_set_sort_column (clist, column);
  }

  clist_column_set_title (clist, cldef, TRUE);
  gtk_clist_sort (clist);

  if (clist == server_clist) {
    server_clist_selection_visible ();
  }
}


void source_ctree_show_node_status (GtkWidget *ctree, struct master *m) {
  GtkCTreeNode *node;
  struct pixmap *pix = NULL;
  int is_leaf;
  int expanded;

  node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);

  if (m->isgroup || m == favorites) {
    pix = games[m->type].pix;
  }
  else {
    pix = &server_status[m->state];
  }

  gtk_ctree_get_node_info (GTK_CTREE (ctree), node, NULL, NULL,
                                 NULL, NULL, NULL, NULL, &is_leaf, &expanded);

  gtk_ctree_set_node_info (GTK_CTREE (ctree), node, m->name, 4,
			      (pix)? pix->pix : NULL, (pix)? pix->mask : NULL,
			      (pix)? pix->pix : NULL, (pix)? pix->mask : NULL,
                              is_leaf, expanded);
}


static void source_ctree_enable_master_group (GtkWidget *ctree, 
					      struct master *m,
					      int expand) {
  GtkCTreeNode *node;
  GtkCTreeNode *sibling = NULL;
  char cfgkey[128];
  GSList *list;
  int expanded;

  if (!m->isgroup)
    return;

  node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);

  if (node != NULL) {
    if (expand)
      gtk_ctree_expand (GTK_CTREE (ctree), node);
    return;
  }

  g_snprintf (cfgkey, 128, 
             "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", m->name);

  if (expand)
    config_set_bool (cfgkey, expanded = TRUE);
  else
    expanded = TRUE - config_get_bool (cfgkey);

    /* Find the place to insert new master group */

  list = g_slist_nth (master_groups, m->type);
  if (list)
    list = list->next;

  while (!sibling && list) {
    sibling = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, 
					        (struct master *) list->data);
    list = list->next;
  }

  node = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, sibling, NULL, 4,
                                     NULL, NULL, NULL, NULL, FALSE, expanded);
  gtk_ctree_node_set_row_data (GTK_CTREE (ctree), node, m);
  source_ctree_show_node_status (ctree, m);
}


/*
 *  Add master or update master's name if master is already in tree
 *  This function works only on non-group masters
 */

void source_ctree_add_master (GtkWidget *ctree, struct master *m) {
  GtkCTreeNode *node;
  GtkCTreeNode *parent = NULL;
  struct master *group = NULL;

  if (m->isgroup)
    return;

  if (m->type != UNKNOWN_SERVER) {
    group = (struct master *) g_slist_nth_data (master_groups, m->type);
    source_ctree_enable_master_group (ctree, group, TRUE);
  }

  node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);

  if (!node) {
    if (group) {
      parent = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, group);
    }
    node = gtk_ctree_insert_node (GTK_CTREE (ctree), parent, NULL, NULL, 4,
                                         NULL, NULL, NULL, NULL, TRUE, FALSE);
    gtk_ctree_node_set_row_data (GTK_CTREE (ctree), node, m);
  }
  source_ctree_show_node_status (ctree, m);
}


static void source_ctree_remove_master_group (GtkWidget *ctree,
					      struct master *m) {
  GtkCTreeNode *node;

  if (!m->isgroup)
    return;

  node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);
  if (node) {
    gtk_ctree_remove_node (GTK_CTREE (ctree), node);
  }
}


void source_ctree_delete_master (GtkWidget *ctree, struct master *m) {
  GtkCTreeNode *node;
  struct master *group;

  node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);
  if (!node)
    return;
  
  gtk_ctree_remove_node (GTK_CTREE (ctree), node);

  /* Remove empty master group from the tree */

  if (m->type != UNKNOWN_SERVER) {
    group = (struct master *) g_slist_nth_data (master_groups, m->type);
    if (group && (group->masters == NULL || 
       (g_slist_length (group->masters) == 1 && group->masters->data == m))) {
      source_ctree_remove_master_group (ctree, group);
    }
  }
}


static void fill_source_ctree (GtkWidget *ctree) {
  GSList *list;
  GSList *list2;
  GtkCTreeNode *node;
  GtkCTreeNode *parent;
  struct master *group;
  struct master *m;

  source_ctree_add_master (ctree, favorites);

  for (list = master_groups; list; list = list->next) {
    group = (struct master *) list->data;
    if (group->masters) {
      source_ctree_enable_master_group (ctree, group, FALSE);
      parent = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, group);

      for (list2 = group->masters; list2; list2 = list2->next) {
	m = (struct master *) list2->data;
	node = gtk_ctree_insert_node (GTK_CTREE (ctree), parent, NULL, NULL, 4,
                                         NULL, NULL, NULL, NULL, TRUE, FALSE);
	gtk_ctree_node_set_row_data (GTK_CTREE (ctree), node, m);
	source_ctree_show_node_status (ctree, m);
      }
    }
  }
}


GtkWidget *create_source_ctree (GtkWidget *scrollwin) {
  char *titles[1] = { "Source" };
  GtkWidget *ctree;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  ctree = gtk_ctree_new_with_titles (1, 0, titles);
  gtk_container_add (GTK_CONTAINER (scrollwin), ctree);

  gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_EXTENDED);

  gtk_ctree_set_line_style (GTK_CTREE (ctree), GTK_CTREE_LINES_NONE);
  gtk_ctree_set_expander_style (GTK_CTREE (ctree), 
                                                 GTK_CTREE_EXPANDER_TRIANGLE);
  gtk_ctree_set_indent (GTK_CTREE (ctree), 10);

  fill_source_ctree (ctree);

  return ctree;
}


void source_ctree_select_source (struct master *m) {
  GtkCTreeNode *node;
  GtkVisibility vis;

  node = gtk_ctree_find_by_row_data (GTK_CTREE (source_ctree), NULL, m);
  gtk_ctree_unselect_recursive (GTK_CTREE (source_ctree), NULL);
  gtk_ctree_select (GTK_CTREE (source_ctree), node);

  vis = gtk_ctree_node_is_visible (GTK_CTREE (source_ctree), node);

  if (vis != GTK_VISIBILITY_FULL)
    gtk_ctree_node_moveto (GTK_CTREE (source_ctree), node, 0, 0.2, 0.0);
}


int calculate_clist_row_height (GtkWidget *clist, GdkPixmap *pixmap) {
  int pix_h, pix_w;
  int height;

  gdk_window_get_size (pixmap, &pix_w, &pix_h);

  height = MAX (pix_h, clist->style->font->ascent +
                                             clist->style->font->descent + 1);
  if (height == pix_h + 1)
    height++;	/* beautify selection */

  return height;
}


void set_toolbar_appearance (GtkToolbar *toolbar, int style, int tips) {

  switch (style) {

  case 0:
    gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_button_relief (toolbar, GTK_RELIEF_NONE);
    gtk_toolbar_set_space_style (toolbar, GTK_TOOLBAR_SPACE_LINE);
    gtk_toolbar_set_space_size (toolbar, 16);
    break;

  case 1:
    gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_TEXT);
    gtk_toolbar_set_button_relief (toolbar, GTK_RELIEF_NORMAL);
    gtk_toolbar_set_space_style (toolbar, GTK_TOOLBAR_SPACE_EMPTY);
    gtk_toolbar_set_space_size (toolbar, 8);
    break;

  case 2:
    gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_BOTH);
    gtk_toolbar_set_button_relief (toolbar, GTK_RELIEF_NONE);
    gtk_toolbar_set_space_style (toolbar, GTK_TOOLBAR_SPACE_LINE);
    gtk_toolbar_set_space_size (toolbar, 16);
    break;

  default:
    break;
  }

  gtk_toolbar_set_tooltips (toolbar, tips);
}


/*******************************  Progress Bar  *****************************/


struct pbarinfo {
  int activity_mode;
  int timeout_id;
};


static void progress_bar_destroy_event (GtkWidget* widget) {
  struct pbarinfo *info;

  info = gtk_object_get_user_data (GTK_OBJECT (widget));

  if (info->activity_mode)
    gtk_timeout_remove (info->timeout_id);
  
  g_free (info);
}


GtkWidget *create_progress_bar (void) {
  GtkWidget *pbar;
  struct pbarinfo *info;

  pbar = gtk_progress_bar_new ();

  info = g_malloc0 (sizeof (struct pbarinfo));

  gtk_object_set_user_data (GTK_OBJECT (pbar), info);
  gtk_signal_connect (GTK_OBJECT (pbar), "destroy",
                            (GtkSignalFunc) progress_bar_destroy_event, NULL);

  gtk_progress_set_format_string (GTK_PROGRESS (pbar), "%1p%%");
  /* gtk_progress_configure (GTK_PROGRESS (pbar), 0.0, 0.0, 100.0); */
  gtk_progress_bar_set_activity_step (GTK_PROGRESS_BAR (pbar), 5);

  return pbar;
}


void progress_bar_reset (GtkWidget *pbar) {
  struct pbarinfo *info;

  info = gtk_object_get_user_data (GTK_OBJECT (pbar));

  if (info->activity_mode) {
    gtk_timeout_remove (info->timeout_id);
    info->activity_mode = FALSE;
  }

  gtk_progress_set_activity_mode (GTK_PROGRESS (pbar), FALSE);
  gtk_progress_set_show_text (GTK_PROGRESS (pbar), FALSE);
  gtk_progress_set_percentage (GTK_PROGRESS (pbar), 0.0);
}


static int progress_bar_timeout_callback (GtkWidget *pbar) {
  gfloat new_val;
  GtkAdjustment *adj;

  adj = GTK_PROGRESS (pbar)->adjustment;

  new_val = adj->value + 1;
  if (new_val > adj->upper)
    new_val = adj->lower;

  gtk_progress_set_value (GTK_PROGRESS (pbar), new_val);

  return TRUE;
}


void progress_bar_start (GtkWidget *pbar, int activity_mode) {
  struct pbarinfo *info;

  progress_bar_reset (pbar);

  if (activity_mode) {
    info = gtk_object_get_user_data (GTK_OBJECT (pbar));
    info->activity_mode = TRUE;
    info->timeout_id = gtk_timeout_add (100, 
                           (GtkFunction) progress_bar_timeout_callback, pbar);
    gtk_progress_set_activity_mode (GTK_PROGRESS (pbar), TRUE);
  }
  else {
    gtk_progress_set_show_text (GTK_PROGRESS (pbar), TRUE);
  }
}


void progress_bar_set_percentage (GtkWidget *pbar, float percentage) {
  struct pbarinfo *info;

  info = gtk_object_get_user_data (GTK_OBJECT (pbar));
  if (!info->activity_mode)
    gtk_progress_set_percentage (GTK_PROGRESS (pbar), percentage);
}


static void save_cwidget_geometry (GtkWidget *clist, struct clist_def *cldef) {
  char buf[256];
  int i;

  g_snprintf (buf, 256, "/" CONFIG_FILE "/%s Geometry/", cldef->name);
  config_push_prefix (buf);

  for (i = 0; i < cldef->columns; i++)
    config_set_int (cldef->cols[i].name, GTK_CLIST (clist)->column[i].width);

  config_pop_prefix ();
}


void ui_done (void) {
  GtkCTreeNode *node;
  char cfgkey[128];
  int expanded;
  GSList *list;
  struct master *m;

  save_cwidget_geometry (GTK_WIDGET (server_clist), &server_clist_def);
  save_cwidget_geometry (GTK_WIDGET (player_clist), &player_clist_def);
  save_cwidget_geometry (GTK_WIDGET (srvinf_ctree), &srvinf_clist_def);

  config_push_prefix ("/" CONFIG_FILE "/Main Window Geometry/");

  config_set_int ("height", main_window->allocation.height);
  config_set_int ("width", main_window->allocation.width);
  config_set_int ("pane1", GTK_PANED (pane1_widget)->child1->allocation.width);
  config_set_int ("pane2", GTK_PANED (pane2_widget)->child1->allocation.height);
  config_set_int ("pane3", GTK_PANED (pane3_widget)->child1->allocation.width);

  config_pop_prefix ();

  config_clean_section ("/" CONFIG_FILE "/Source Tree");

  for (list = master_groups; list; list = list->next) {
    m = (struct master *) list->data;
    if (m->isgroup) {
      node = gtk_ctree_find_by_row_data (GTK_CTREE (source_ctree), NULL, m);
      if (node) {
	gtk_ctree_get_node_info (GTK_CTREE (source_ctree), node, 
                         NULL, NULL, NULL, NULL, NULL, NULL, NULL, &expanded);
	if (!expanded) {
	  g_snprintf (cfgkey, 128, 
             "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", m->name);
	  config_set_bool (cfgkey, TRUE - expanded);
	}
      }
    }
  }
}


void restore_main_window_geometry () {
  int height, width;
  int pane1, pane2, pane3;

  config_push_prefix ("/" CONFIG_FILE "/Main Window Geometry/");

  height = config_get_int ("height");
  width  = config_get_int ("width");
  pane1  = config_get_int ("pane1");
  pane2  = config_get_int ("pane2");
  pane3  = config_get_int ("pane3");

  config_pop_prefix ();

  if (height && width) {
    /* gtk_widget_set_usize (GTK_WIDGET (main_window), width, height); */
    gtk_window_set_default_size (GTK_WINDOW (main_window), width, height);
  }

  gtk_paned_set_position (GTK_PANED (pane1_widget), (pane1)? pane1 : 120);
  gtk_paned_set_position (GTK_PANED (pane2_widget), (pane2)? pane2 :
           server_clist_def.height + GTK_PANED (pane2_widget)->gutter_size/2);
  gtk_paned_set_position (GTK_PANED (pane3_widget), (pane3)? pane3 :
           player_clist_def.height + GTK_PANED (pane3_widget)->gutter_size/2);
}

