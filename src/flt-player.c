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
#include <regex.h>
#include <stdio.h>      /* FILE, fopen, fclose, fprintf... */
#include <string.h>     /* strlen, strcmp */
#include <unistd.h>     /* unlink, close */
#include <sys/stat.h>   /* open */
#include <fcntl.h>      /* open */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "pref.h"
#include "pixmaps.h"
#include "dialogs.h"
#include "utils.h"
#include "flt-player.h"
#include "filter.h"
#include "debug.h"

#define REGCOMP_FLAGS (REG_EXTENDED | REG_NOSUB | REG_ICASE)


static GSList *players = NULL;  /* GSList <struct player_pattern *> */
static GSList *curplrs = NULL;  /* GSList <struct player_pattern *> */

enum {
	PLT_COL_GRP0 = 0,
	PLT_COL_GRP1,
	PLT_COL_GRP2,
	PLT_COL_MODE,
	PLT_COL_PATTERN,
	PLT_COL_COUNT
};

static GtkListStore *pattern_store = NULL;
static GtkWidget *pattern_list;
static GtkWidget *pattern_entry;
static GtkWidget *comment_text;
static GtkWidget *mode_buttons[3];
static GtkWidget *delete_button;
static GtkWidget *up_button;
static GtkWidget *down_button;

static GtkTextBuffer *comment_text_buffer;

static int current_row = -1;

static void player_filter_save_patterns (void);
static void player_filter_load_patterns (void);


enum {
	TOKEN_INVALID = G_TOKEN_LAST,
	TOKEN_STRING,
	TOKEN_SUBSTR,
	TOKEN_REGEXP
};

static char *mode_symbols[3] = {
	N_("string"),
	N_("substr"),
	N_("regexp")
};

static const char *mode_names[3] = {
	N_("string"),
	N_("substring"),
	N_("regular expression")
};


int player_filter (struct server *s) {
	/* The 'vars' is ignored in this function, however, since we need it
	   for applying the server filter, we will put it in the arguments. */

	GSList *list;
	GSList *plist;
	struct player_pattern *pp;
	struct player *p;

	s->flags &= ~PLAYER_GROUP_MASK;

	if (!s->players)
		return FALSE;

	for (plist = s->players; plist; plist = plist->next) {
		p = (struct player *) plist->data;
		p->flags &= ~PLAYER_GROUP_MASK;
	}

	if (!players)
		return FALSE;

	for (list = players; list; list = list->next) {
		pp = (struct player_pattern *) list->data;

		for (plist = s->players; plist; plist = plist->next) {
			p = (struct player *) plist->data;

			if (pp->data && !pp->error) {
				if ((pp->mode == PATTERN_MODE_STRING &&
						g_ascii_strcasecmp (p->name, pp->data) == 0) ||
						(pp->mode == PATTERN_MODE_SUBSTR &&
						 lowcasestrstr (p->name, pp->data)) ||
						(pp->mode == PATTERN_MODE_REGEXP &&
						 regexec ((regex_t *) pp->data, p->name, 0, NULL, 0) == 0)) {

					p->flags |= pp->groups;
					s->flags |= pp->groups;
				}
			}
		}
	}

	return ((s->flags & PLAYER_GROUP_MASK) != 0)? TRUE : FALSE;
}


static void free_player_pattern_compiled_data (struct player_pattern *pp) {

	if (!pp)
		return;

	if (pp->error) {
		g_free (pp->error);
		pp->error = NULL;
	}

	if (pp->data) {
		if (pp->mode == PATTERN_MODE_REGEXP) {
			regfree ((regex_t *) pp->data);
		}
		g_free (pp->data);
		pp->data = NULL;
	}
}


static void free_player_pattern (struct player_pattern *pp) {
	free_player_pattern_compiled_data (pp);
	if (pp->pattern) g_free (pp->pattern);
	if (pp->comment) g_free (pp->comment);
	g_free (pp);
}


static char *get_regerror (int errcode, regex_t *compiled) {
	size_t length;
	char *buffer;

	length = regerror (errcode, compiled, NULL, 0);
	buffer = g_malloc (length);
	regerror (errcode, compiled, buffer, length);
	return buffer;
}


static void player_pattern_compile (struct player_pattern *pp) {
	int res;

	free_player_pattern_compiled_data (pp);

	if (pp->pattern && pp->pattern[0]) {

		switch (pp->mode) {

			case PATTERN_MODE_REGEXP:
				pp->data = g_malloc (sizeof (regex_t));

				res = regcomp ((regex_t *) pp->data, pp->pattern, REGCOMP_FLAGS);
				if (res) {
					pp->error = get_regerror (res, (regex_t *) pp->data);
					regfree ((regex_t *) pp->data);
					g_free (pp->data);
					pp->data = NULL;
				}
				break;

			case PATTERN_MODE_STRING:
			case PATTERN_MODE_SUBSTR:
			default:
				pp->data = g_ascii_strdown(pp->pattern, -1);    /* g_ascii_strdown does implicit strndup */
				break;

		}

	}
}


static struct player_pattern *player_pattern_new (struct player_pattern *src) {
	struct player_pattern *pp;

	pp = g_malloc0 (sizeof (struct player_pattern));

	if (src) {
		pp->mode    = src->mode;
		pp->pattern = g_strdup (src->pattern);
		pp->comment = g_strdup (src->comment);
		pp->groups  = src->groups;
	}
	else {
		pp->mode    = PATTERN_MODE_STRING;
		pp->groups  = PLAYER_GROUP_RED;
	}

	pp->dirty = TRUE;
	return pp;
}


static void pattern_list_sync_selection (void) {
	GSList *list;
	struct player_pattern *pp = NULL;

	if (current_row >= 0) {
		list = g_slist_nth (curplrs, current_row);
		pp = (struct player_pattern *) list->data;

		if (gtk_widget_get_realized(comment_text) == FALSE) {
			gtk_widget_realize (comment_text);
		}
	}

	gtk_text_buffer_set_text(comment_text_buffer, "", 0);

	if (current_row >= 0) {
		if (pp->comment) {
			gtk_text_buffer_set_text(comment_text_buffer, pp->comment,
				strlen (pp->comment));
		}
		gtk_text_view_set_editable (GTK_TEXT_VIEW (comment_text), TRUE);

		gtk_entry_set_text (GTK_ENTRY (pattern_entry),
				(pp->pattern)? pp->pattern : "");
		gtk_editable_set_editable (GTK_EDITABLE (pattern_entry), TRUE);

		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_STRING], TRUE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_SUBSTR], TRUE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_REGEXP], TRUE);

		gtk_check_button_set_active (
				GTK_CHECK_BUTTON (mode_buttons[pp->mode]), TRUE);

		gtk_widget_set_sensitive (delete_button, TRUE);
		gtk_widget_set_sensitive (up_button, TRUE);
		gtk_widget_set_sensitive (down_button, TRUE);
	}
	else {
		gtk_text_view_set_editable (GTK_TEXT_VIEW (comment_text), FALSE);

		gtk_entry_set_text (GTK_ENTRY (pattern_entry), "");
		gtk_editable_set_editable (GTK_EDITABLE (pattern_entry), FALSE);

		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_STRING], FALSE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_SUBSTR], FALSE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_REGEXP], FALSE);

		gtk_widget_set_sensitive (delete_button, FALSE);
		gtk_widget_set_sensitive (up_button, FALSE);
		gtk_widget_set_sensitive (down_button, FALSE);
	}
}


static void pattern_list_update_groups (int row, unsigned newstate,
		unsigned oldstate) {
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (pattern_store), &iter, path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	for (int i = 0; i < 3; i++) {
		unsigned mask = 1u << i;
		int col = PLT_COL_GRP0 + i;
		if ((newstate & mask) != 0)
			gtk_list_store_set (pattern_store, &iter, col, group_pix[i].texture, -1);
		else if ((oldstate & mask) != 0)
			gtk_list_store_set (pattern_store, &iter, col, NULL, -1);
	}
}


static void pattern_list_update_row (struct player_pattern *pp, int row) {
	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (pattern_store), &iter, path)) {
		gtk_tree_path_free (path);
		return;
	}
	gtk_tree_path_free (path);

	pattern_list_update_groups (row, pp->groups, ~pp->groups);

	char mode_buf[64];
	const char *mode_str = _(mode_symbols[pp->mode]);
	if (pp->error) {
		g_snprintf (mode_buf, sizeof (mode_buf), "(!) %s", mode_str);
		mode_str = mode_buf;
	}
	gtk_list_store_set (pattern_store, &iter,
			PLT_COL_MODE, mode_str,
			PLT_COL_PATTERN, pp->pattern ? pp->pattern : "",
			-1);
}


static void sync_pattern_data (void) {
	struct player_pattern *pp;
	char *pattern;
	char *comment;
	enum pattern_mode mode;
	int update_pattern = FALSE;
	int update_comment = FALSE;
	GSList *list;

	if (current_row < 0)
		return;

	list = g_slist_nth (curplrs, current_row);
	pp = (struct player_pattern *) list->data;

	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (mode_buttons[PATTERN_MODE_STRING])))
		mode = PATTERN_MODE_STRING;
	else if (gtk_check_button_get_active (GTK_CHECK_BUTTON (mode_buttons[PATTERN_MODE_SUBSTR])))
		mode = PATTERN_MODE_SUBSTR;
	else
		mode = PATTERN_MODE_REGEXP;

	pattern = strdup_strip (gtk_entry_get_text (GTK_ENTRY (pattern_entry)));
	{
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds (comment_text_buffer, &start, &end);
		comment = gtk_text_buffer_get_text (comment_text_buffer, &start, &end, FALSE);
	}

	update_pattern = (pp->pattern && pp->pattern[0])?
		!pattern || !pattern[0] || strcmp (pp->pattern, pattern) :
		pattern && pattern[0];

	update_comment = (pp->comment && pp->comment[0])?
		!comment || !comment[0] || strcmp (pp->comment, comment) :
		comment && comment[0];

	/*
	 *  Test everything but groups.
	 *  They are synchronized by pattern_set_groups()
	 */

	if (mode != pp->mode || update_comment || update_pattern) {

		if (!pp->dirty) {
			pp = player_pattern_new (pp);
			list->data = pp;
		}

		if (update_pattern) {
			if (pp->pattern) g_free (pp->pattern);
			pp->pattern = pattern;
		}

		if (update_comment) {
			if (pp->comment) g_free (pp->comment);
			pp->comment = comment;
		}

		if (pp->mode != mode || update_pattern) {
			free_player_pattern_compiled_data (pp);
			pp->mode = mode;
			player_pattern_compile (pp);
		}

		pattern_list_update_row (pp, current_row);

	}

	if (!update_comment)
		g_free (comment);

	if (!update_pattern)
		g_free (pattern);
}


static int pattern_list_insert_row (struct player_pattern *pp, int row) {
	GtkTreeIter iter;
	if (row < 0) {
		gtk_list_store_append (pattern_store, &iter);
		int n = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (pattern_store), NULL);
		row = n - 1;
	} else {
		gtk_list_store_insert (pattern_store, &iter, row);
	}
	pattern_list_update_row (pp, row);
	return row;
}


static void pattern_set_groups (int row, int column, int add) {
	GSList *list;
	struct player_pattern *pp;
	unsigned newstate;
	unsigned oldstate;

	if (column < 0 || column > 2 || row < 0)
		return;

	list = g_slist_nth (curplrs, row);
	pp = (struct player_pattern *) list->data;

	oldstate = pp->groups;
	newstate = 1 << column;

	if (add) {
		if ((pp->groups & ~newstate & PLAYER_GROUP_MASK) != 0)
			newstate ^= pp->groups;
	}

	if (newstate != oldstate) {
		if (!pp->dirty) {
			pp = player_pattern_new (pp);
			list->data = pp;
			player_pattern_compile (pp);
		}
		pp->groups = newstate;
		pattern_list_update_groups (row, newstate, oldstate);
	}
}


static void show_pattern_error (int row) {
	GSList *list;
	struct player_pattern *pp;

	list = g_slist_nth (curplrs, row);
	pp = (struct player_pattern *) list->data;

	if (pp->error) {
		dialog_ok (_("XQF: Error"), _("Regular Expression Error!\n\n%s\n\n%s."),
				pp->pattern, pp->error);
	}
}


static void on_pattern_list_click (GtkGestureClick *gesture, int n_press,
		double x, double y, gpointer data) {
	(void) n_press; (void) data;
	GtkTreePath *path;
	GtkTreeViewColumn *col;

	if (!gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (pattern_list),
				(int) x, (int) y, &path, &col, NULL, NULL))
		return;

	int row = gtk_tree_path_get_indices (path)[0];
	gtk_tree_path_free (path);

	GList *cols = gtk_tree_view_get_columns (GTK_TREE_VIEW (pattern_list));
	int col_idx = g_list_index (cols, col);
	g_list_free (cols);

	if (col_idx >= 0 && col_idx < 3)
		pattern_set_groups (row, col_idx, TRUE);
}


static void pattern_list_selection_changed (GtkTreeSelection *sel, gpointer data) {
	(void) data;
	if (!GTK_IS_TREE_SELECTION (sel))
		return;

	GtkTreeModel *model;
	GtkTreeIter iter;

	sync_pattern_data ();

	if (!gtk_tree_selection_get_selected (sel, &model, &iter)) {
		current_row = -1;
	} else {
		GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
		int row = gtk_tree_path_get_indices (path)[0];
		gtk_tree_path_free (path);
		if (row >= (int) g_slist_length (curplrs))
			current_row = -1;
		else
			current_row = row;
	}

	pattern_list_sync_selection ();
}


static void new_pattern_callback (GtkWidget *widget, gpointer data) {
	struct player_pattern *pp;
	int row;

	pp = player_pattern_new (NULL);

	if (current_row >= 0) {
		row = current_row;
		current_row++;
	}
	else {
		row = 0;
	}

	curplrs = g_slist_insert (curplrs, pp, row);
	pattern_list_insert_row (pp, row);

	{
		GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pattern_list));
		GtkTreeIter iter;
		GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
		if (gtk_tree_model_get_iter (GTK_TREE_MODEL (pattern_store), &iter, path))
			gtk_tree_selection_select_iter (sel, &iter);
		gtk_tree_path_free (path);
	}

	gtk_widget_grab_focus (pattern_entry);
}


static void delete_pattern_callback (GtkWidget *widget, gpointer data) {
	GSList *link;
	struct player_pattern *pp;
	int row;

	debug(5,"delete_pattern_callback(widget=%x,data=%x)",widget,data);

	if (current_row < 0)
		return;

	link = g_slist_nth (curplrs, current_row);
	pp = (struct player_pattern *) link->data;

	curplrs = g_slist_remove_link (curplrs, link);
	if (pp->dirty)
		free_player_pattern (pp);

	row = current_row;
	current_row = -1;

	GtkTreeIter iter;
	GtkTreePath *path = gtk_tree_path_new_from_indices (row, -1);
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (pattern_store), &iter, path))
		gtk_list_store_remove (pattern_store, &iter);
	gtk_tree_path_free (path);

	pattern_list_sync_selection ();
}


static void move_up_down_pattern_callback (GtkWidget *widget, int dir) {
	int row = current_row;

	debug(5,"move_up_down_pattern_callback(widget=%x, dir=%d) row=%d",widget,dir,row);

	if ((dir != -1 || row <= 0) &&
			(dir != 1  || row < 0 || row == (int) g_slist_length (curplrs) - 1)) {
		return;
	}

	/* Swap data in the GSList */
	GSList *link_a = g_slist_nth (curplrs, row);
	GSList *link_b = g_slist_nth (curplrs, row + dir);
	gpointer tmp = link_a->data;
	link_a->data = link_b->data;
	link_b->data = tmp;

	/* Refresh both rows in the store */
	pattern_list_update_row ((struct player_pattern *) link_a->data, row);
	pattern_list_update_row ((struct player_pattern *) link_b->data, row + dir);

	current_row = row + dir;

	/* Scroll to the moved row */
	GtkTreePath *path = gtk_tree_path_new_from_indices (current_row, -1);
	gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (pattern_list), path, NULL, FALSE, 0.0f, 0.0f);
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter (GTK_TREE_MODEL (pattern_store), &iter, path))
		gtk_tree_selection_select_iter (gtk_tree_view_get_selection (GTK_TREE_VIEW (pattern_list)), &iter);
	gtk_tree_path_free (path);
}


static GtkWidget *aligned_image (struct pixmap *pix) {
	GtkWidget *image;
	GtkWidget *alignment;

	alignment = gtk_alignment_new (0.5, 0.5, 0, 0);

	image = gtk_image_new_from_pixbuf (pix->pixbuf);
	gtk_container_add (GTK_CONTAINER (alignment), image);
	gtk_widget_show (image);

	return alignment;
}


static GtkWidget *player_filter_pattern_editor (void) {
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *grid;
	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *scrollwin;
	int i;

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);

	frame = gtk_frame_new (NULL);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
	gtk_container_set_border_width (GTK_CONTAINER (grid), 6);
	gtk_container_add (GTK_CONTAINER (frame), grid);

	/* Pattern Entry */

	label = gtk_label_new (_("Pattern"));
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
	gtk_widget_show (label);

	pattern_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (pattern_entry), 256);
	gtk_grid_attach (GTK_GRID (grid), pattern_entry, 1, 0, 1, 1);
	gtk_widget_set_hexpand (pattern_entry, TRUE);
	g_signal_connect (pattern_entry, "activate",
			G_CALLBACK (sync_pattern_data), NULL);
	g_signal_connect (pattern_entry, "changed",
			G_CALLBACK (sync_pattern_data), NULL);
	gtk_widget_show (pattern_entry);

	/* Mode Buttons */

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_grid_attach (GTK_GRID (grid), hbox, 1, 1, 1, 1);
	gtk_widget_set_hexpand (hbox, TRUE);

	for (i = 0; i < 3; i++) {
		mode_buttons[i] = gtk_check_button_new_with_label (_(mode_names[i]));
		if (i > 0)
			gtk_check_button_set_group (GTK_CHECK_BUTTON (mode_buttons[i]), GTK_CHECK_BUTTON (mode_buttons[0]));

		g_signal_connect (mode_buttons[i], "toggled", G_CALLBACK (sync_pattern_data), NULL);

		gtk_box_pack_start (GTK_BOX (hbox), mode_buttons[i], FALSE, FALSE, 0);
		gtk_widget_show (mode_buttons[i]);
	}

	gtk_widget_show (hbox);

	gtk_widget_show (grid);
	gtk_widget_show (frame);

	/* Comment */

	label = gtk_label_new (_("Pattern Comment"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_pack_end (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	comment_text_buffer = gtk_text_buffer_new (NULL);
	comment_text = gtk_text_view_new_with_buffer (comment_text_buffer);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (comment_text), GTK_WRAP_WORD_CHAR);
	g_signal_connect (comment_text_buffer, "changed",
			G_CALLBACK (sync_pattern_data), NULL);
	gtk_widget_show (comment_text);

	scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
	                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_has_frame (GTK_SCROLLED_WINDOW (scrollwin), TRUE);
	gtk_widget_set_size_request (scrollwin, -1, 80);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), comment_text);
	gtk_box_pack_end (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);
	gtk_widget_show (scrollwin);

	gtk_widget_show (vbox);

	return vbox;
}


static void player_filter_page_init (void) {
	struct player_pattern *pp;
	GSList *list;

	curplrs = g_slist_copy (players);
	current_row = -1;

	for (list = players; list; list = list->next) {
		pp = (struct player_pattern *) list->data;
		pp->dirty = FALSE;
		pattern_list_insert_row (pp, -1);
	}

	if (!curplrs)
		pattern_list_sync_selection ();
}


void player_filter_page (GtkWidget *notebook) {
	GtkWidget *page_hbox;
	GtkWidget *scrollwin;
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *vbox2;
	GtkWidget *alignment;
	GtkWidget *image;
	GtkWidget *button;
	GtkWidget *peditor;
	int i;

	page_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_container_set_border_width (GTK_CONTAINER (page_hbox), 8);

	label = gtk_label_new (_("Player Filter"));
	gtk_widget_show (label);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_hbox, label);

	/* Pattern list (GtkTreeView) */

	scrollwin = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (page_hbox), scrollwin, FALSE, FALSE, 0);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	pattern_store = gtk_list_store_new (PLT_COL_COUNT,
			GDK_TYPE_TEXTURE, GDK_TYPE_TEXTURE, GDK_TYPE_TEXTURE,
			G_TYPE_STRING, G_TYPE_STRING);
	pattern_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (pattern_store));
	g_object_unref (pattern_store);
	gtk_widget_set_size_request (pattern_list, 260, 200);

	for (i = 0; i < 3; i++) {
		GtkCellRenderer *pbr = gtk_cell_renderer_pixbuf_new ();
		GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (
				"", pbr, "texture", PLT_COL_GRP0 + i, NULL);
		gtk_tree_view_column_set_resizable (col, FALSE);
		gtk_tree_view_column_set_fixed_width (col, pixmap_width (&group_pix[i]) + 8);
		gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
		image = gtk_image_new_from_pixbuf (group_pix[i].pixbuf);
		gtk_widget_show (image);
		gtk_tree_view_column_set_widget (col, image);
		gtk_tree_view_append_column (GTK_TREE_VIEW (pattern_list), col);
	}

	{
		GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
		GtkTreeViewColumn *col = gtk_tree_view_column_new_with_attributes (
				_("Mode"), renderer, "text", PLT_COL_MODE, NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_column_set_fixed_width (col, 55);
		gtk_tree_view_column_set_sizing (col, GTK_TREE_VIEW_COLUMN_FIXED);
		gtk_tree_view_append_column (GTK_TREE_VIEW (pattern_list), col);

		renderer = gtk_cell_renderer_text_new ();
		col = gtk_tree_view_column_new_with_attributes (
				_("Pattern"), renderer, "text", PLT_COL_PATTERN, NULL);
		gtk_tree_view_column_set_resizable (col, TRUE);
		gtk_tree_view_append_column (GTK_TREE_VIEW (pattern_list), col);
	}

	{
		GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (pattern_list));
		gtk_tree_selection_set_mode (sel, GTK_SELECTION_BROWSE);
		g_signal_connect (sel, "changed", G_CALLBACK (pattern_list_selection_changed), NULL);
	}

	{
		GtkGesture *click = gtk_gesture_click_new ();
		gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 0);
		g_signal_connect (click, "pressed", G_CALLBACK (on_pattern_list_click), NULL);
		gtk_widget_add_controller (pattern_list, GTK_EVENT_CONTROLLER (click));
	}

	gtk_container_add (GTK_CONTAINER (scrollwin), pattern_list);
	gtk_widget_show (pattern_list);
	gtk_widget_show (scrollwin);

	/* Buttons */

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_pack_start (GTK_BOX (page_hbox), vbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("New"));
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked", G_CALLBACK (new_pattern_callback), NULL);
	gtk_widget_show (button);

	delete_button = gtk_button_new_with_label (_("Delete"));
	gtk_box_pack_start (GTK_BOX (vbox), delete_button, FALSE, FALSE, 0);
	g_signal_connect (delete_button, "clicked", G_CALLBACK (delete_pattern_callback), NULL);
	gtk_widget_show (delete_button);

	alignment = gtk_alignment_new (0, 0.5, 1, 0);
	gtk_box_pack_end (GTK_BOX (vbox), alignment, TRUE, TRUE, 0);

	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_container_add (GTK_CONTAINER (alignment), vbox2);

	up_button = gtk_button_new_with_label (_("Up"));
	gtk_box_pack_start (GTK_BOX (vbox2), up_button, FALSE, FALSE, 0);
	g_signal_connect (up_button, "clicked", G_CALLBACK (move_up_down_pattern_callback), (void *) -1);
	gtk_widget_show (up_button);

	down_button = gtk_button_new_with_label (_("Down"));
	gtk_box_pack_start (GTK_BOX (vbox2), down_button, FALSE, FALSE, 0);
	g_signal_connect (down_button, "clicked", G_CALLBACK (move_up_down_pattern_callback), (void *) 1);
	gtk_widget_show (down_button);

	gtk_widget_show (vbox2);
	gtk_widget_show (alignment);
	gtk_widget_show (vbox);

	/* Pattern Editor */

	peditor = player_filter_pattern_editor ();
	gtk_box_pack_end (GTK_BOX (page_hbox), peditor, TRUE, TRUE, 0);

	gtk_widget_show (page_hbox);

	player_filter_page_init ();
}


static int strings_are_same (const char *s1, const char *s2) {

	return ((!s1 || *s1 == '\0') && (!s2 || *s2 == '\0')) ||
		(s1 && s2 && strcmp (s1, s2) == 0);
}


/*
 *  'FILTER_DATA_CHANGED' situations that are not catched by
 *   player_pattern_lists_compare():
 *
 *     - string/substring mode: case change in pattern
 *     - regexp mode: errorneous pattern is modified but
 *          the error is not corrected
 */

static int player_pattern_lists_compare (GSList *list1, GSList *list2) {
	enum filter_status changed = FILTER_NOT_CHANGED;
	struct player_pattern *pp1, *pp2;
	GSList *tmp;

	if (g_slist_length (list1) != g_slist_length (list2))
		return FILTER_CHANGED;

	while (list1) {
		pp1 = (struct player_pattern *) list1->data;
		pp2 = (struct player_pattern *) list2->data;

		if (pp1->mode == pp2->mode && pp1->groups == pp2->groups &&
				strings_are_same (pp1->pattern, pp2->pattern)) {
			if (!strings_are_same (pp1->comment, pp2->comment))
				changed = FILTER_DATA_CHANGED;
			list2 = list2->next;
		}
		else {
			changed = FILTER_DATA_CHANGED;

			for (tmp = list2->next; tmp; tmp = tmp->next) {
				pp2 = (struct player_pattern *) tmp->data;

				if (pp1->mode == pp2->mode && pp1->groups == pp2->groups &&
						strings_are_same (pp1->pattern, pp2->pattern)) {
					break;
				}
			}

			if (!tmp)
				return FILTER_CHANGED;
		}

		list1 = list1->next;
	}

	return changed;
}


void player_filter_new_defaults (void) {
	struct player_pattern *pp;
	GSList *list;

	sync_pattern_data ();

	filters[FILTER_PLAYER].changed =
		player_pattern_lists_compare (players, curplrs);

	for (list = curplrs; list; list = list->next) {
		pp = (struct player_pattern *) list->data;
		pp->dirty = TRUE;
	}

	for (list = players; list; list = list->next) {
		pp = (struct player_pattern *) list->data;
		if (!pp->dirty)
			free_player_pattern (pp);
	}
	g_slist_free (players);
	players = curplrs;
	curplrs = NULL;

	switch (filters[FILTER_PLAYER].changed) {

		case FILTER_CHANGED:
			filters[FILTER_PLAYER].last_changed = filter_time_inc();
			/* fall through */

		case FILTER_DATA_CHANGED:
			player_filter_save_patterns ();
			break;

		case FILTER_NOT_CHANGED:
		default:
			break;

	}
}


void player_filter_cfg_clean_up (void) {
	struct player_pattern *pp;
	GSList *list;

	if (curplrs) {
		for (list = curplrs; list; list = list->next) {
			pp = (struct player_pattern *) list->data;
			if (pp->dirty)
				free_player_pattern (pp);
		}
		g_slist_free (curplrs);
		curplrs = NULL;
	}
}


int player_filter_add_player (char *name, unsigned mask) {
	GSList *list;
	struct player_pattern *pp;
	char *pattern;
	int changed = FALSE;

	pattern = strdup_strip (name);

	if (!pattern)
		return FALSE;

	for (list = players; list; list = list->next) {
		pp = (struct player_pattern *) list->data;
		if (pp->mode == PATTERN_MODE_STRING && pp->pattern &&
				g_ascii_strcasecmp (pattern, pp->pattern) == 0) {
			break;
		}
	}

	if (!list) {
		pp = player_pattern_new (NULL);
		pp->pattern = pattern;
		pp->groups = 0;

		player_pattern_compile (pp);
		players = g_slist_append (players, pp);

		changed = TRUE;
	}

	mask &= PLAYER_GROUP_MASK;

	if ((pp->groups & mask) != mask) {
		pp->groups |= mask;

		filters[FILTER_PLAYER].last_changed = filter_time_inc();
		player_filter_save_patterns ();

		changed = TRUE;
	}

	return changed;
}


static GScannerConfig patterns_scanner_config = {
	.cset_skip_characters = " \t\n",
	.cset_identifier_first =
			G_CSET_a_2_z
			"_"
			G_CSET_A_2_Z,
	.cset_identifier_nth =
			G_CSET_a_2_z
			"_0123456789"
			G_CSET_A_2_Z
			G_CSET_LATINS
			G_CSET_LATINC,
	.cpair_comment_single = "#\n",
	.case_sensitive = TRUE,
	.skip_comment_multi = TRUE,
	.skip_comment_single = TRUE,
	.scan_comment_multi = FALSE,
	.scan_identifier = TRUE,
	.scan_identifier_1char = FALSE,
	.scan_identifier_NULL = FALSE,
	.scan_symbols = TRUE,
	.scan_binary = FALSE,
	.scan_octal = FALSE,
	.scan_float = FALSE,
	.scan_hex = TRUE,
	.scan_hex_dollar = FALSE,
	.scan_string_sq = FALSE,
	.scan_string_dq = TRUE,
	.numbers_2_int = TRUE,
	.int_2_float = FALSE,
	.identifier_2_string = TRUE,
	.char_2_token = TRUE,
	.symbol_2_token = TRUE,
};


static int skip_to_right_curly_bracket (GScanner *scanner) {
	int token;

	token = g_scanner_cur_token (scanner);

	while (1) {

		switch (token) {

			case G_TOKEN_EOF:
			case G_TOKEN_ERROR:
				return FALSE;

			case G_TOKEN_RIGHT_CURLY:
				return TRUE;

		}

		token = g_scanner_get_next_token (scanner);
	}
}


static int player_pattern_parse_statement (GScanner *scanner, char *fn) {
	struct player_pattern *pp;
	int token;

	token = g_scanner_get_next_token (scanner);

	switch (token) {

		case TOKEN_STRING:
		case TOKEN_SUBSTR:
		case TOKEN_REGEXP:

			pp = player_pattern_new (NULL);

			switch (token) {
				case TOKEN_REGEXP:
					pp->mode = PATTERN_MODE_REGEXP;
					break;

				case TOKEN_SUBSTR:
					pp->mode = PATTERN_MODE_SUBSTR;
					break;

				case TOKEN_STRING:
				default:
					pp->mode = PATTERN_MODE_STRING;
					break;
			}

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_LEFT_CURLY) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_STRING) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			pp->pattern = strdup_strip (scanner->value.v_string);

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_COMMA) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_INT) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			pp->groups = scanner->value.v_int & PLAYER_GROUP_MASK;

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_COMMA) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_STRING) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			pp->comment = strdup_strip (scanner->value.v_string);

			token = g_scanner_get_next_token (scanner);
			if (token != G_TOKEN_RIGHT_CURLY) {
				free_player_pattern (pp);
				return skip_to_right_curly_bracket (scanner);
			}

			player_pattern_compile (pp);
			players = g_slist_append (players, pp);
			filters[FILTER_PLAYER].changed = FILTER_CHANGED;
			break;

		case G_TOKEN_EOF:
			return FALSE;

		default:
			fprintf (stderr, "%s[%d:%d] parse error\n\n", fn, scanner->line,
					scanner->position);
			return skip_to_right_curly_bracket (scanner);
	}

	return TRUE;
}


static void player_filter_load_patterns (void) {
	char *fn;
	int fd;
	GScanner *scanner;
	int retval;
	int i;

	fn = file_in_dir (user_rcdir, PLAYERS_FILE);
	fd = open (fn, O_RDONLY);

	if (fd < 0) {
		g_free (fn);
		return;
	}

	scanner = g_scanner_new (&patterns_scanner_config);

	for (i = TOKEN_STRING; i <= TOKEN_REGEXP; i++) {
		g_scanner_scope_add_symbol (scanner, 0, mode_symbols[i - TOKEN_STRING], GINT_TO_POINTER(i));
	}

	g_scanner_input_file (scanner, fd);

	do {
		retval = player_pattern_parse_statement (scanner, fn);
	} while (retval);

	g_scanner_destroy (scanner);
	g_free (fn);
	close (fd);

	if (filters[FILTER_PLAYER].changed != FILTER_NOT_CHANGED)
		filters[FILTER_PLAYER].last_changed = filter_time_inc();
}


static void player_filter_save_patterns (void) {
	GSList *list;
	struct player_pattern *pp;
	FILE *f;
	char *fn;

	fn = file_in_dir (user_rcdir, PLAYERS_FILE);

	if (!players) {
		unlink (fn);
	}
	else {
		f = fopen (fn, "w");
		if (!f) {
			g_free (fn);
			return;
		}

		for (list = players; list; list = list->next) {
			pp = (struct player_pattern *) list->data;

			fprintf (f, "%s {\n  ", mode_symbols[pp->mode]);
			print_dq_string (f, pp->pattern);
			fprintf (f, ",\n  0x%02X,\n  ", pp->groups);
			print_dq_string (f, pp->comment);
			fprintf (f, "\n}\n\n");
		}

		fclose (f);
	}

	g_free (fn);
}


void player_filter_init (void) {
	player_filter_done ();
	player_filter_load_patterns ();
}


void player_filter_done (void) {
	g_slist_foreach (players, (GFunc) free_player_pattern, NULL);
	g_slist_free (players);
	players = NULL;
}

