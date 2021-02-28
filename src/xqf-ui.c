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
#include <stdarg.h>     /* va_start, va_end */
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "srv-list.h"
#include "source.h"
#include "game.h"
#include "config.h"
#include "sort.h"
#include "pref.h"
#include "debug.h"

static GSList *xqf_windows = NULL;
static GtkWidget *target_window = NULL;

GtkWidget *pane1_widget;
GtkWidget *pane2_widget;
GtkWidget *pane3_widget;

GtkWidget *filter_buttons[FILTERS_TOTAL] = {0};

/* If you add a column here to appear in the server
   list, you need to also add an entry in sort.h and sort.c
*/

static struct clist_column server_columns[] =
{
	{
		.name =      N_("Name"),
		.width =     180,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_SERVER_NAME, SORT_SERVER_TYPE, -1 },
		.sort_name = { NULL, N_("Type") },
	},
	{
		.name =      N_("Address"),
		.width =     140,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_SERVER_ADDRESS, SORT_SERVER_COUNTRY, -1 },
		.sort_name = { NULL, N_("Country") },
	},
	{
		.name =      N_("Ping"),
		.width =     45,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_SERVER_PING, -1 }
	},
	{
		.name =      N_("TO"),
		.width =     35,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_SERVER_TO, -1 }
	},
	{
		.name =      N_("Priv"),
		.width =     35,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_SERVER_PRIVATE, SORT_SERVER_ANTICHEAT, -1 },
		// .Translator = "PunkBuster"
		.sort_name = { NULL, N_("PB") },
	},
	{
		.name =      N_("Players"),
		.width =     65,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_SERVER_PLAYERS, SORT_SERVER_MAXPLAYERS, -1 },
		// .Translator = Max as in max players
		.sort_name = { NULL, N_("Max") },
	},
	{
		.name =      N_("Map"),
		.width =     55,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_SERVER_MAP, -1 }
	},
	{
		.name =      N_("Game"),
		.width =     55,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_SERVER_GAME, -1 }
	},
	{
		.name =      N_("GameType"),
		.width =     55,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_SERVER_GAMETYPE, -1 }
	},
};


struct clist_def server_clist_def = {
	CWIDGET_CLIST,
	"Server List",
	server_columns,
	9,
	GTK_SELECTION_EXTENDED,
	630, 270,
	SORT_SERVER_PING, GTK_SORT_ASCENDING
};

static struct clist_column player_columns[] =
{
	{
		.name =      N_("Name"),
		.width =     100,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_PLAYER_NAME, -1 },
	},
	{
		.name =      N_("Frags"),
		.width =     50,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_PLAYER_FRAGS, -1 },
	},
	{
		.name =      N_("Colors"),
		.width =     60,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_PLAYER_COLOR, -1 },
	},
	{
		.name =      N_("Skin"),
		.width =     50,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_PLAYER_SKIN, -1 },
	},
	{
		.name =      N_("Ping"),
		.width =     45,
		.justify =   GTK_JUSTIFY_RIGHT,
		.sort_mode = { SORT_PLAYER_PING, -1 },
	},
	{
		.name =      N_("Time"),
		.width =     45,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_PLAYER_TIME, -1 },
	}
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

static struct clist_column srvinf_columns[] =
{
	{
		.name =      N_("Rule"),
		.width =     90,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_INFO_RULE, -1 },
	},
	{
		.name =      N_("Value"),
		.width =     80,
		.justify =   GTK_JUSTIFY_LEFT,
		.sort_mode = { SORT_INFO_VALUE, -1 },
	}
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
		buf[0] = ' ';   /* indent */
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
	debug (6, "%p", window);
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

	debug (6, "%p", window);
}


GtkWidget *top_window (void) {
	if (xqf_windows)
		return (GtkWidget *) xqf_windows->data;
	else
		return NULL;
}

#ifdef GUI_GTK2
static void clist_column_set_title (GtkCList *clist, struct clist_def *cldef, int set_mark) {
	char buf[256];

	if (set_mark) {
		const char* name = cldef->cols[clist->sort_column].sort_name[cldef->cols[clist->sort_column].current_sort_mode];
		g_snprintf (buf, 128, "%s %c", _(cldef->cols[clist->sort_column].name),
				(clist->sort_type == GTK_SORT_DESCENDING)? '>' : '<');

		if (name) {
			snprintf (buf+strlen(buf), sizeof(buf)-strlen(buf), " (%s)", _(name));
		}
		gtk_label_set (GTK_LABEL (cldef->cols[clist->sort_column].widget), buf);
	}
	else {
		gtk_label_set (GTK_LABEL (cldef->cols[clist->sort_column].widget),
				_(cldef->cols[clist->sort_column].name));
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
			gtk_ctree_set_expander_style (GTK_CTREE (clist), GTK_CTREE_EXPANDER_TRIANGLE);
			gtk_ctree_set_indent (GTK_CTREE (clist), 10);
			break;

		default:
			return NULL;
	}

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	GTK_CLIST_SET_FLAG (GTK_CLIST (clist), CLIST_SHOW_TITLES);
	gtk_container_add (GTK_CONTAINER (scrollwin), clist);

	gtk_clist_set_selection_mode (GTK_CLIST (clist), cldef->mode);

	for (i = 0; i < cldef->columns; i++) {
		g_snprintf (buf, 256, "/" CONFIG_FILE "/%s Geometry/%s=%d",
				cldef->name, cldef->cols[i].name, cldef->cols[i].width);
		gtk_clist_set_column_width (GTK_CLIST (clist), i, config_get_int (buf));
		if (cldef->cols[i].justify != GTK_JUSTIFY_LEFT) {
			gtk_clist_set_column_justification (GTK_CLIST (clist), i, cldef->cols[i].justify);
		}

		alignment = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);

		label = gtk_label_new (_(cldef->cols[i].name));
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

#define DIMOF(arr) (sizeof(arr)/sizeof(arr[0]))

void clist_set_sort_column (GtkCList *clist, int column, struct clist_def *cldef) {
	if (column == clist->sort_column) {
		if (clist->sort_type == GTK_SORT_DESCENDING) {
			cldef->cols[column].current_sort_mode = (cldef->cols[column].current_sort_mode+1)%DIMOF(cldef->cols[column].sort_mode);
			if (cldef->cols[column].sort_mode[cldef->cols[column].current_sort_mode] == -1)
				cldef->cols[column].current_sort_mode = 0;
		}

		gtk_clist_set_sort_type (clist, GTK_SORT_DESCENDING + GTK_SORT_ASCENDING - clist->sort_type);
	}
	else {
		cldef->cols[column].current_sort_mode = 0;
		clist_column_set_title (clist, cldef, FALSE);
		gtk_clist_set_sort_column (clist, column);
	}

	debug (3, "%d %hhd", column, cldef->cols[column].current_sort_mode);

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

	gtk_ctree_get_node_info (GTK_CTREE (ctree), node, NULL, NULL, NULL, NULL, NULL, NULL, &is_leaf, &expanded);

	gtk_ctree_set_node_info (GTK_CTREE (ctree), node, _(m->name), 4,
			(pix)? pix->pix : NULL, (pix)? pix->mask : NULL,
			(pix)? pix->pix : NULL, (pix)? pix->mask : NULL,
			is_leaf, expanded);
}


static void source_ctree_enable_master_group (GtkWidget *ctree, struct master *m, int expand) {
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

	g_snprintf (cfgkey, 128, "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", m->name);

	if (expand)
		config_set_bool (cfgkey, expanded = TRUE);
	else
		expanded = TRUE - config_get_bool (cfgkey);

	/* Find the place to insert new master group */

	list = g_slist_nth (master_groups, m->type);
	if (list)
		list = list->next;

	while (!sibling && list) {
		sibling = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, (struct master *) list->data);
		list = list->next;
	}

	node = gtk_ctree_insert_node (GTK_CTREE (ctree), NULL, sibling, NULL, 4, NULL, NULL, NULL, NULL, FALSE, expanded);
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

	// If set to display only configured games, and game is not configured,
	// and it's not the 'Favorites' master, just return so the display
	// isn't updated with the game type and masters.
	if (!(games[m->type].cmd) && default_show_only_configured_games && m != favorites)
		return;

	if (m->type != UNKNOWN_SERVER) {
		enum server_type type = m->master_type == MASTER_LAN? LAN_SERVER : m->type;
		group = (struct master *) g_slist_nth_data (master_groups, type);
		source_ctree_enable_master_group (ctree, group, TRUE);
	}

	node = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, m);

	if (!node) {
		if (group) {
			parent = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, group);
		}
		node = gtk_ctree_insert_node (GTK_CTREE (ctree), parent, NULL, NULL, 4, NULL, NULL, NULL, NULL, TRUE, FALSE);
		gtk_ctree_node_set_row_data (GTK_CTREE (ctree), node, m);
	}
	source_ctree_show_node_status (ctree, m);
}


//static void source_ctree_remove_master_group (GtkWidget *ctree,
void source_ctree_remove_master_group (GtkWidget *ctree, struct master *m) {
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
		if (group && (group->masters == NULL || (g_slist_length (group->masters) == 1 && group->masters->data == m))) {
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
			// If set to display only configured games, and game is not configured,
			// don't update the display with the master.
			if (games[group->type].cmd || !default_show_only_configured_games) {
				source_ctree_enable_master_group (ctree, group, FALSE);
				parent = gtk_ctree_find_by_row_data (GTK_CTREE (ctree), NULL, group);

				for (list2 = group->masters; list2; list2 = list2->next) {
					m = (struct master *) list2->data;
					node = gtk_ctree_insert_node (GTK_CTREE (ctree), parent, NULL, NULL, 4, NULL, NULL, NULL, NULL, TRUE, FALSE);
					gtk_ctree_node_set_row_data (GTK_CTREE (ctree), node, m);
					source_ctree_show_node_status (ctree, m);
				}
			}
		}
	}
}

GtkWidget *create_source_ctree (GtkWidget *scrollwin) {
	char *titles[1] = { _("Source") };
	GtkWidget *ctree;

/*	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
*/
	ctree = gtk_ctree_new_with_titles (1, 0, titles);
	gtk_container_add (GTK_CONTAINER (scrollwin), ctree);

	gtk_clist_set_selection_mode (GTK_CLIST (ctree), GTK_SELECTION_EXTENDED);

	gtk_ctree_set_line_style (GTK_CTREE (ctree), GTK_CTREE_LINES_NONE);
	gtk_ctree_set_expander_style (GTK_CTREE (ctree), GTK_CTREE_EXPANDER_TRIANGLE);
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

	height=pix_h+1;

	return height;
}
#endif

void set_toolbar_appearance (GtkToolbar *toolbar, int style, int tips) {
	switch (style) {
		case 0: gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS); break;
		case 1: gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_TEXT);  break;
		case 2: gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_BOTH);  break;
		default: break;
	}

	gtk_toolbar_set_tooltips (toolbar, tips);
}

/*******************************  Progress Bar  *****************************/

int pbar_pulse_mode;
int pbar_timeout_id;

void progress_bar_destroy_event (GtkWidget* widget) {
	progress_bar_reset(widget);
}

GtkWidget *create_progress_bar (void) {
	GtkWidget *pbar;

	pbar = gtk_progress_bar_new ();

	g_signal_connect (pbar, "destroy", G_CALLBACK (progress_bar_destroy_event), NULL);

	return pbar;
}

void progress_bar_reset (GtkWidget *pbar) {
	if (pbar_pulse_mode == TRUE) {
		g_source_remove (pbar_timeout_id);
		pbar_pulse_mode = FALSE;
	}
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (pbar), 0.0);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (pbar), NULL);
}

gboolean progress_bar_pulse (gpointer user_data) {
	gtk_progress_bar_pulse (GTK_PROGRESS_BAR(user_data));

	// Ask g_source to repeat this callback
	return G_SOURCE_CONTINUE;
}

void progress_bar_start (GtkWidget *pbar, int pulse_switch) {

	progress_bar_reset (pbar);

	if (pulse_switch && pbar_pulse_mode == FALSE) {
		pbar_pulse_mode = TRUE;
		gtk_progress_bar_set_pulse_step (GTK_PROGRESS_BAR (pbar), 0.03);
		pbar_timeout_id = g_timeout_add (50, (GSourceFunc) progress_bar_pulse, (gpointer) pbar);
	}
}

void save_cwidget_geometry (GtkWidget *clist, struct clist_def *cldef) {
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
					g_snprintf (cfgkey, 128, "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", m->name);
					config_set_bool (cfgkey, TRUE - expanded);
				}
			}
		}
	}
}


void restore_main_window_geometry (void) {
	int height, width;
	int pane1, pane2, pane3;

	config_push_prefix ("/" CONFIG_FILE "/Main Window Geometry/");

	height = config_get_int ("height=480");
	width  = config_get_int ("width=640");
	pane1  = config_get_int ("pane1");
	pane2  = config_get_int ("pane2");
	pane3  = config_get_int ("pane3");

	config_pop_prefix ();

	if (height && width) {
		/* gtk_widget_set_usize (GTK_WIDGET (main_window), width, height); */
		gtk_window_set_default_size (GTK_WINDOW (main_window), width, height);
	}

	gtk_paned_set_position (GTK_PANED (pane1_widget), (pane1)? pane1 : 120);
	gtk_paned_set_position (GTK_PANED (pane2_widget), (pane2)? pane2 : server_clist_def.height +4);
	gtk_paned_set_position (GTK_PANED (pane3_widget), (pane3)? pane3 : player_clist_def.height + 4);
}

GtkWidget* lookup_widget (GtkWidget* widget, const gchar* widget_name) {
	GtkWidget *parent, *found_widget;

	for (;;) {
		if (GTK_IS_MENU (widget))
			parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
		else
			parent = widget->parent;
		if (parent == NULL)
			break;
		widget = parent;
	}

	found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget), widget_name);
	if (!found_widget)
		g_warning ("Widget not found: %s", widget_name);
	return found_widget;
}

// Skip a game if it's not configured and show only configured is enabled
gboolean create_server_type_menu_filter_configured (enum server_type type) {
	if (!games[type].cmd && default_show_only_configured_games)
		return FALSE;
	else
		return TRUE;
}

GtkWidget *create_server_type_menu (int active_type, gboolean (*filterfunc)(enum server_type), GtkSignalFunc callback) {
	GtkWidget *option_menu = NULL;
	GtkWidget *menu = NULL;
	GtkWidget *menu_item = NULL;
	GtkWidget *first_menu_item = NULL;
	int i;
	int j = 0;
	int menu_type = 0;

	option_menu = gtk_option_menu_new ();

	menu = gtk_menu_new ();

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; ++i) {
		if (filterfunc && !filterfunc (i))
			continue;

		menu_item = gtk_menu_item_new ();

		if (i == active_type) {
			first_menu_item = menu_item;
			menu_type = j;
		}
		else if (!first_menu_item)
			first_menu_item = menu_item;

		gtk_menu_append (GTK_MENU (menu), menu_item);

		gtk_container_add (GTK_CONTAINER (menu_item), game_pixmap_with_label (i));

		if (callback)
			g_signal_connect (G_OBJECT (menu_item), "activate", G_CALLBACK (callback), GINT_TO_POINTER (i));

		gtk_widget_show (menu_item);

		++j; // must be here in case the continue was used
	}

	gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), menu);

	// initiates callback to set servertype to first configured game
	if (active_type != -1 && first_menu_item) {
		gtk_menu_item_activate (GTK_MENU_ITEM (first_menu_item));
		gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), menu_type);
	}

	return option_menu;
}
