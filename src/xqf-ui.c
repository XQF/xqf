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
#include "xqf-lists.h"
#include "srv-list.h"
#include "source.h"
#include "game.h"
#include "config.h"
#include "sort.h"
#include "pref.h"
#include "debug.h"

static GSList *xqf_windows = NULL;
static GtkWidget *target_window = NULL;
static GtkTreeStore *source_store = NULL;

enum source_col {
	SOURCE_COL_MASTER = 0,
	SOURCE_COL_PIXBUF,
	SOURCE_COL_NAME,
	SOURCE_COL_COUNT
};

struct _source_find_ctx { struct master *target; GtkTreeIter result; gboolean found; };

static gboolean
_source_find_cb (GtkTreeModel *model, GtkTreePath *path G_GNUC_UNUSED,
                 GtkTreeIter *iter, gpointer data)
{
	struct _source_find_ctx *ctx = data;
	gpointer mp = NULL;
	gtk_tree_model_get (model, iter, SOURCE_COL_MASTER, &mp, -1);
	if (mp == ctx->target) {
		ctx->result = *iter;
		ctx->found  = TRUE;
		return TRUE;
	}
	return FALSE;
}

static gboolean
source_find_master (struct master *m, GtkTreeIter *out)
{
	struct _source_find_ctx ctx = { m, { 0 }, FALSE };
	gtk_tree_model_foreach (GTK_TREE_MODEL (source_store), _source_find_cb, &ctx);
	if (ctx.found && out)
		*out = ctx.result;
	return ctx.found;
}

GtkWidget *pane1_widget;
GtkWidget *pane2_widget;
GtkWidget *pane3_widget;

GtkWidget *filter_buttons[FILTERS_TOTAL] = {0};

/* If you add a column here to appear in the server
   list, you need to also add an entry in sort.h and sort.c
*/

static struct list_column server_columns[] =
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


struct list_def server_list_def = {
	CVIEW_LIST,
	"Server List",
	server_columns,
	9,
	GTK_SELECTION_EXTENDED,
	630, 270,
	SORT_SERVER_PING, GTK_SORT_ASCENDING
};

static struct list_column player_columns[] =
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

struct list_def player_list_def = {
	CVIEW_LIST,
	"Player List",
	player_columns,
	6,
	GTK_SELECTION_SINGLE,
	400, 180,
	SORT_PLAYER_FRAGS, GTK_SORT_DESCENDING
};


void print_status (GtkWidget *sbar, char *fmt, ...) {
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

		gtk_label_set_text (GTK_LABEL (sbar), buf);
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


void source_treeview_show_node_status (struct master *m) {
	GtkTreeIter iter;
	struct pixmap *pix = NULL;

	if (!source_find_master (m, &iter))
		return;

	if (m->isgroup || m == favorites)
		pix = games[m->type].pix;
	else
		pix = &server_status[m->state];

	gtk_tree_store_set (source_store, &iter,
		SOURCE_COL_PIXBUF, pix ? pix->texture : NULL,
		SOURCE_COL_NAME,   _(m->name),
		-1);
}


static void source_treeview_enable_master_group (struct master *m) {
	GtkTreeIter iter, sibling_iter;
	gboolean has_sibling = FALSE;
	GSList *list;

	if (!m->isgroup)
		return;

	if (source_find_master (m, NULL))
		return; /* already exists */

	/* Find where to insert: before the first already-present later group */
	list = g_slist_nth (master_groups, m->type);
	if (list)
		list = list->next;
	while (!has_sibling && list) {
		has_sibling = source_find_master ((struct master *) list->data, &sibling_iter);
		list = list->next;
	}

	if (has_sibling)
		gtk_tree_store_insert_before (source_store, &iter, NULL, &sibling_iter);
	else
		gtk_tree_store_append (source_store, &iter, NULL);

	gtk_tree_store_set (source_store, &iter,
		SOURCE_COL_MASTER, m,
		SOURCE_COL_PIXBUF, NULL,
		SOURCE_COL_NAME,   "",
		-1);
	source_treeview_show_node_status (m);
}


/*
 *  Add master or update master's name/icon if master is already in tree.
 *  This function works only on non-group masters.
 */
void source_treeview_add_master (struct master *m) {
	GtkTreeIter parent_iter, iter;
	gboolean has_parent = FALSE;
	struct master *group = NULL;

	if (m->isgroup)
		return;

	/* If showing only configured games, skip unconfigured non-favorites */
	if (!(games[m->type].cmd) && default_show_only_configured_games && m != favorites)
		return;

	if (m->type != UNKNOWN_SERVER) {
		enum server_type type = m->master_type == MASTER_LAN ? LAN_SERVER : m->type;
		group = (struct master *) g_slist_nth_data (master_groups, type);
		source_treeview_enable_master_group (group);
	}

	if (source_find_master (m, &iter)) {
		source_treeview_show_node_status (m);
		return;
	}

	if (group)
		has_parent = source_find_master (group, &parent_iter);

	gtk_tree_store_append (source_store, &iter, has_parent ? &parent_iter : NULL);
	gtk_tree_store_set (source_store, &iter,
		SOURCE_COL_MASTER, m,
		SOURCE_COL_PIXBUF, NULL,
		SOURCE_COL_NAME,   "",
		-1);
	source_treeview_show_node_status (m);

	/* Expand parent so the new child is visible */
	if (has_parent) {
		GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (source_store), &parent_iter);
		gtk_tree_view_expand_row (GTK_TREE_VIEW (source_treeview), path, FALSE);
		gtk_tree_path_free (path);
	}
}


void source_treeview_remove_master_group (struct master *m) {
	GtkTreeIter iter;

	if (!m->isgroup)
		return;

	if (source_find_master (m, &iter))
		gtk_tree_store_remove (source_store, &iter);
}


void source_treeview_delete_master (struct master *m) {
	GtkTreeIter iter;
	struct master *group;

	if (!source_find_master (m, &iter))
		return;

	gtk_tree_store_remove (source_store, &iter);

	/* Remove parent group if it is now empty */
	if (m->type != UNKNOWN_SERVER) {
		group = (struct master *) g_slist_nth_data (master_groups, m->type);
		if (group && (group->masters == NULL ||
		              (g_slist_length (group->masters) == 1 && group->masters->data == m)))
			source_treeview_remove_master_group (group);
	}
}


gboolean source_treeview_has_master (struct master *m) {
	return source_find_master (m, NULL);
}


static void fill_source_treeview (void) {
	GSList *list, *list2;
	GtkTreeIter parent_iter, iter;
	struct master *group, *m;

	source_treeview_add_master (favorites);

	for (list = master_groups; list; list = list->next) {
		group = (struct master *) list->data;
		if (!group->masters)
			continue;
		source_treeview_enable_master_group (group);

		if (!source_find_master (group, &parent_iter))
			continue;

		for (list2 = group->masters; list2; list2 = list2->next) {
			m = (struct master *) list2->data;
			gtk_tree_store_append (source_store, &iter, &parent_iter);
			gtk_tree_store_set (source_store, &iter,
				SOURCE_COL_MASTER, m,
				SOURCE_COL_PIXBUF, NULL,
				SOURCE_COL_NAME,   "",
				-1);
			source_treeview_show_node_status (m);
		}
	}
}

/* Restore expand/collapse state for each group; call after source_treeview is set. */
void source_treeview_restore_expand_state (void) {
	GSList *list;
	GtkTreeIter parent_iter;
	struct master *group;
	char cfgkey[128];

	for (list = master_groups; list; list = list->next) {
		group = (struct master *) list->data;
		if (!group->masters)
			continue;
		if (!source_find_master (group, &parent_iter))
			continue;
		g_snprintf (cfgkey, 128, "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", group->name);
		if (!config_get_bool (cfgkey)) {
			GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (source_store), &parent_iter);
			gtk_tree_view_expand_row (GTK_TREE_VIEW (source_treeview), path, FALSE);
			gtk_tree_path_free (path);
		}
	}
}

GtkWidget *create_source_treeview (GtkWidget *scrollwin) {
	GtkWidget *tv;
	GtkCellRenderer *cr;
	GtkTreeViewColumn *col;
	GtkTreeSelection *sel;

	source_store = gtk_tree_store_new (SOURCE_COL_COUNT,
	                                   G_TYPE_POINTER,    /* MASTER */
	                                   GDK_TYPE_TEXTURE,  /* PIXBUF */
	                                   G_TYPE_STRING);    /* NAME   */

	tv = gtk_tree_view_new_with_model (GTK_TREE_MODEL (source_store));
	g_object_unref (source_store);

	col = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (col, _("Source"));

	cr = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (col, cr, FALSE);
	gtk_tree_view_column_set_attributes (col, cr, "texture", SOURCE_COL_PIXBUF, NULL);

	cr = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (col, cr, TRUE);
	gtk_tree_view_column_set_attributes (col, cr, "text", SOURCE_COL_NAME, NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tv), col);

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (tv));
	gtk_tree_selection_set_mode (sel, GTK_SELECTION_EXTENDED);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
	                                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), tv);

	fill_source_treeview ();

	return tv;
}


void source_treeview_select_source (struct master *m) {
	GtkTreeView      *tv  = GTK_TREE_VIEW (source_treeview);
	GtkTreeSelection *sel = gtk_tree_view_get_selection (tv);
	GtkTreeIter       iter;
	GtkTreePath      *path;

	if (!source_find_master (m, &iter))
		return;

	gtk_tree_selection_unselect_all (sel);
	gtk_tree_selection_select_iter (sel, &iter);

	path = gtk_tree_model_get_path (GTK_TREE_MODEL (source_store), &iter);
	gtk_tree_view_scroll_to_cell (tv, path, NULL, FALSE, 0.2f, 0.0f);
	gtk_tree_path_free (path);
}


int calculate_row_height (GtkWidget *widget G_GNUC_UNUSED, struct pixmap *pix) {
	int pix_h;
	int height;

	pix_h = pixmap_height (pix);

	height=pix_h+1;

	return height;
}

void set_toolbar_appearance (GtkWidget *toolbar) {
	(void)toolbar;
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

void save_view_geometry (GtkWidget *widget, struct list_def *cldef) {
	char buf[256];
	int i;

	g_snprintf (buf, 256, "/" CONFIG_FILE "/%s Geometry/", cldef->name);
	config_push_prefix (buf);

	GListModel *cols = gtk_column_view_get_columns (GTK_COLUMN_VIEW (widget));
	for (i = 0; i < cldef->columns; i++) {
		GtkColumnViewColumn *col = GTK_COLUMN_VIEW_COLUMN (g_list_model_get_item (cols, (guint) i));
		if (col) {
			config_set_int (cldef->cols[i].name, gtk_column_view_column_get_fixed_width (col));
			g_object_unref (col);
		}
	}

	config_pop_prefix ();
}


void ui_done (void) {
	GtkAllocation allocation;
	char cfgkey[128];
	GSList *list;
	struct master *m;

	save_view_geometry (GTK_WIDGET (server_view), &server_list_def);
	save_view_geometry (GTK_WIDGET (player_view), &player_list_def);

	config_push_prefix ("/" CONFIG_FILE "/Main Window Geometry/");

	gtk_widget_get_allocation (main_window, &allocation);
	config_set_int ("height", allocation.height);
	config_set_int ("width", allocation.width);

	gtk_widget_get_allocation (gtk_paned_get_child1 (GTK_PANED (pane1_widget)), &allocation);
	config_set_int ("pane1", allocation.width);

	gtk_widget_get_allocation (gtk_paned_get_child1 (GTK_PANED (pane2_widget)), &allocation);
	config_set_int ("pane2", allocation.height);

	gtk_widget_get_allocation (gtk_paned_get_child1 (GTK_PANED (pane3_widget)), &allocation);
	config_set_int ("pane3", allocation.width);

	config_pop_prefix ();

	config_clean_section ("/" CONFIG_FILE "/Source Tree");

	for (list = master_groups; list; list = list->next) {
		m = (struct master *) list->data;
		if (m->isgroup) {
			GtkTreeIter iter;
			if (source_find_master (m, &iter)) {
				GtkTreePath *path = gtk_tree_model_get_path (
				        GTK_TREE_MODEL (source_store), &iter);
				gboolean expanded = gtk_tree_view_row_expanded (
				        GTK_TREE_VIEW (source_treeview), path);
				gtk_tree_path_free (path);
				if (!expanded) {
					g_snprintf (cfgkey, 128, "/" CONFIG_FILE "/Source Tree/%s node collapsed=false", m->name);
					config_set_bool (cfgkey, TRUE);
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
		/* gtk_widget_set_size_request (GTK_WIDGET (main_window), width, height); */
		gtk_window_set_default_size (GTK_WINDOW (main_window), width, height);
	}

	gtk_paned_set_position (GTK_PANED (pane1_widget), (pane1)? pane1 : 120);
	gtk_paned_set_position (GTK_PANED (pane2_widget), (pane2)? pane2 : server_list_def.height +4);
	gtk_paned_set_position (GTK_PANED (pane3_widget), (pane3)? pane3 : player_list_def.height + 4);
}


// Skip a game if it's not configured and show only configured is enabled
gboolean create_server_type_menu_filter_configured (enum server_type type) {
	if (!games[type].cmd && default_show_only_configured_games)
		return FALSE;
	else
		return TRUE;
}

typedef void (*SeverTypeSelectedFunction)(GtkWidget *widget, enum server_type type);

static void create_server_type_menu_callback (GtkWidget *widget, SeverTypeSelectedFunction callback) {
	GtkComboBox *combo = GTK_COMBO_BOX (widget);
	GtkTreeModel *model = gtk_combo_box_get_model (combo);
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter (combo, &iter)) {
		gint value;

		gtk_tree_model_get (model, &iter, SERVERTYPE_ATTR_TYPE, &value, -1);

		callback (widget, value);
	}
}

GtkWidget *create_server_type_menu (int active_type, gboolean (*filterfunc)(enum server_type), GCallback callback) {
	GtkListStore *store;
	GtkWidget *combo;
	GtkCellRenderer *renderer;
	int i, row = 0, first_row = 0;

	store = gtk_list_store_new (SERVERTYPE_ATTR_COUNT,
	                            G_TYPE_INT,
	                            GDK_TYPE_TEXTURE,
	                            G_TYPE_STRING
	                            );

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; ++i) {
		GtkTreeIter iter;
		GdkTexture *texture = games[i].pix->texture;
		char *name = _(games[i].name);

		if (filterfunc && !filterfunc (i))
			continue;

		gtk_list_store_append (store, &iter);

		gtk_list_store_set (store, &iter,
		                    SERVERTYPE_ATTR_TYPE, i,
		                    SERVERTYPE_ATTR_ICON, texture,
		                    SERVERTYPE_ATTR_NAME, name,
		                    -1);

		if (i == active_type) {
			first_row = row;
		}
		else if (!first_row)
			first_row = row;

		++row; // must be here in case the continue was used
	}

	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));

	g_object_unref (G_OBJECT (store));

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
	                                renderer,
	                                "texture", SERVERTYPE_ATTR_ICON,
	                                NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo),
	                                renderer,
	                                "text", SERVERTYPE_ATTR_NAME,
	                                NULL);

	g_signal_connect (G_OBJECT (combo), "changed",
	                  G_CALLBACK (create_server_type_menu_callback),
	                  (SeverTypeSelectedFunction) callback);

	// initiates callback to set servertype to first configured game
	if (active_type != -1 && first_row) {
		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), first_row);
	}

	return combo;
}
