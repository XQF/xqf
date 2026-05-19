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

static GListStore *pattern_store = NULL;
static GtkSingleSelection *pattern_sel_model = NULL;
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
static void pattern_list_update_row (struct player_pattern *pp, int row);


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


static GObject *make_pattern_item (struct player_pattern *pp) {
	GObject *obj = g_object_new (G_TYPE_OBJECT, NULL);
	for (int i = 0; i < 3; i++) {
		char key[8];
		g_snprintf (key, sizeof (key), "grp%d", i);
		GdkTexture *t = (pp->groups & (1u << i)) ? group_pix[i].texture : NULL;
		g_object_set_data_full (obj, key,
				t ? g_object_ref (t) : NULL,
				t ? (GDestroyNotify) g_object_unref : NULL);
	}
	char mode_buf[64];
	const char *mode_str = _(mode_symbols[pp->mode]);
	if (pp->error) {
		g_snprintf (mode_buf, sizeof (mode_buf), "(!) %s", mode_str);
		mode_str = mode_buf;
	}
	g_object_set_data_full (obj, "mode", g_strdup (mode_str), g_free);
	g_object_set_data_full (obj, "pattern",
			g_strdup (pp->pattern ? pp->pattern : ""), g_free);
	return obj;
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

		gtk_editable_set_text (GTK_EDITABLE (pattern_entry),
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

		gtk_editable_set_text (GTK_EDITABLE (pattern_entry), "");
		gtk_editable_set_editable (GTK_EDITABLE (pattern_entry), FALSE);

		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_STRING], FALSE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_SUBSTR], FALSE);
		gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_REGEXP], FALSE);

		gtk_widget_set_sensitive (delete_button, FALSE);
		gtk_widget_set_sensitive (up_button, FALSE);
		gtk_widget_set_sensitive (down_button, FALSE);
	}
}


static void pattern_list_update_row (struct player_pattern *pp, int row) {
	if (row < 0 || !pattern_store) return;
	GObject *obj = make_pattern_item (pp);
	gpointer arr[1] = { obj };
	g_list_store_splice (pattern_store, (guint) row, 1, arr, 1);
	g_object_unref (obj);
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

	pattern = strdup_strip (gtk_editable_get_text (GTK_EDITABLE (pattern_entry)));
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
	GObject *obj = make_pattern_item (pp);
	if (row < 0) {
		g_list_store_append (pattern_store, obj);
		row = (int) g_list_model_get_n_items (G_LIST_MODEL (pattern_store)) - 1;
	} else {
		g_list_store_insert (pattern_store, (guint) row, obj);
	}
	g_object_unref (obj);
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
		pattern_list_update_row (pp, row);
	}
}



static void on_grp_cell_click (GtkGestureClick *gesture, int n_press,
		double x, double y, gpointer data) {
	(void) n_press; (void) x; (void) y; (void) data;
	GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));
	GtkListItem *li = g_object_get_data (G_OBJECT (widget), "list-item");
	int col_idx = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "col-idx"));
	if (!li) return;
	guint row = gtk_list_item_get_position (li);
	if (row == GTK_INVALID_LIST_POSITION) return;
	pattern_set_groups ((int) row, col_idx, TRUE);
}


static void pattern_list_selection_changed (GtkSingleSelection *sel,
		GParamSpec *pspec, gpointer data) {
	(void) pspec; (void) data;
	sync_pattern_data ();
	guint pos = gtk_single_selection_get_selected (sel);
	if (pos == GTK_INVALID_LIST_POSITION || pos >= (guint) g_slist_length (curplrs))
		current_row = -1;
	else
		current_row = (int) pos;
	pattern_list_sync_selection ();
}


static void new_pattern_callback (GtkWidget *widget, gpointer data) {
	(void) widget; (void) data;
	struct player_pattern *pp = player_pattern_new (NULL);
	int row;

	if (current_row >= 0) {
		row = current_row;
		current_row++;
	} else {
		row = 0;
	}

	curplrs = g_slist_insert (curplrs, pp, row);
	pattern_list_insert_row (pp, row);
	gtk_single_selection_set_selected (pattern_sel_model, (guint) row);
	gtk_widget_grab_focus (pattern_entry);
}


static void delete_pattern_callback (GtkWidget *widget, gpointer data) {
	(void) widget; (void) data;
	debug(5,"delete_pattern_callback(widget=%x,data=%x)",widget,data);

	if (current_row < 0)
		return;

	GSList *link = g_slist_nth (curplrs, current_row);
	struct player_pattern *pp = (struct player_pattern *) link->data;

	curplrs = g_slist_remove_link (curplrs, link);
	if (pp->dirty)
		free_player_pattern (pp);

	int row = current_row;
	current_row = -1;

	g_list_store_remove (pattern_store, (guint) row);
	pattern_list_sync_selection ();
}


static void move_up_down_pattern_callback (GtkWidget *widget, int dir) {
	int row = current_row;

	debug(5,"move_up_down_pattern_callback(widget=%x, dir=%d) row=%d",widget,dir,row);

	if ((dir != -1 || row <= 0) &&
			(dir != 1  || row < 0 || row == (int) g_slist_length (curplrs) - 1))
		return;

	GSList *link_a = g_slist_nth (curplrs, row);
	GSList *link_b = g_slist_nth (curplrs, row + dir);
	gpointer tmp = link_a->data;
	link_a->data = link_b->data;
	link_b->data = tmp;

	pattern_list_update_row ((struct player_pattern *) link_a->data, row);
	pattern_list_update_row ((struct player_pattern *) link_b->data, row + dir);

	current_row = row + dir;
#if GTK_CHECK_VERSION(4, 12, 0)
	gtk_column_view_scroll_to (GTK_COLUMN_VIEW (pattern_list),
			(guint) current_row, NULL, GTK_LIST_SCROLL_NONE, NULL);
#endif
	gtk_single_selection_set_selected (pattern_sel_model, (guint) current_row);
}


static void grp_col_setup_cb (GtkListItemFactory *factory, GtkListItem *list_item,
		gpointer col_idx_ptr) {
	(void) factory;
	GtkWidget *image = gtk_image_new ();
	gtk_widget_set_halign (image, GTK_ALIGN_CENTER);
	gtk_widget_set_valign (image, GTK_ALIGN_CENTER);
	gtk_widget_set_visible (image, TRUE);

	GtkGesture *click = gtk_gesture_click_new ();
	gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (click), 1);
	g_object_set_data (G_OBJECT (image), "list-item", list_item);
	g_object_set_data (G_OBJECT (image), "col-idx", col_idx_ptr);
	g_signal_connect (click, "pressed", G_CALLBACK (on_grp_cell_click), NULL);
	gtk_widget_add_controller (image, GTK_EVENT_CONTROLLER (click));

	gtk_list_item_set_child (list_item, image);
}

static void grp_col_bind_cb (GtkListItemFactory *factory, GtkListItem *list_item,
		gpointer col_idx_ptr) {
	(void) factory;
	int col_idx = GPOINTER_TO_INT (col_idx_ptr);
	char key[8];
	g_snprintf (key, sizeof (key), "grp%d", col_idx);
	GtkWidget *image = gtk_list_item_get_child (list_item);
	GObject *obj = gtk_list_item_get_item (list_item);
	GdkTexture *texture = g_object_get_data (obj, key);
	gtk_image_set_from_paintable (GTK_IMAGE (image),
			texture ? GDK_PAINTABLE (texture) : NULL);
}

static void text_col_setup_cb (GtkListItemFactory *factory, GtkListItem *list_item,
		gpointer data) {
	(void) factory; (void) data;
	GtkWidget *label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
	gtk_widget_set_visible (label, TRUE);
	gtk_list_item_set_child (list_item, label);
}

static void text_col_bind_cb (GtkListItemFactory *factory, GtkListItem *list_item,
		gpointer key) {
	(void) factory;
	GtkWidget *label = gtk_list_item_get_child (list_item);
	GObject *obj = gtk_list_item_get_item (list_item);
	const char *text = g_object_get_data (obj, (const char *) key);
	gtk_label_set_text (GTK_LABEL (label), text ? text : "");
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
	gtk_box_append (GTK_BOX (vbox), frame);

	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
	gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
	xqf_widget_set_margin_all (grid, 6);
	gtk_frame_set_child (GTK_FRAME (frame), grid);

	/* Pattern Entry */

	label = gtk_label_new (_("Pattern"));
	gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
	gtk_widget_set_visible (label, TRUE);

	pattern_entry = gtk_entry_new ();
	gtk_entry_set_max_length (GTK_ENTRY (pattern_entry), 256);
	gtk_grid_attach (GTK_GRID (grid), pattern_entry, 1, 0, 1, 1);
	gtk_widget_set_hexpand (pattern_entry, TRUE);
	g_signal_connect (pattern_entry, "activate",
			G_CALLBACK (sync_pattern_data), NULL);
	g_signal_connect (pattern_entry, "changed",
			G_CALLBACK (sync_pattern_data), NULL);
	gtk_widget_set_visible (pattern_entry, TRUE);

	/* Mode Buttons */

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_grid_attach (GTK_GRID (grid), hbox, 1, 1, 1, 1);
	gtk_widget_set_hexpand (hbox, TRUE);

	for (i = 0; i < 3; i++) {
		mode_buttons[i] = gtk_check_button_new_with_label (_(mode_names[i]));
		if (i > 0)
			gtk_check_button_set_group (GTK_CHECK_BUTTON (mode_buttons[i]), GTK_CHECK_BUTTON (mode_buttons[0]));

		g_signal_connect (mode_buttons[i], "toggled", G_CALLBACK (sync_pattern_data), NULL);

		gtk_box_append (GTK_BOX (hbox), mode_buttons[i]);
		gtk_widget_set_visible (mode_buttons[i], TRUE);
	}

	gtk_widget_set_visible (hbox, TRUE);

	gtk_widget_set_visible (grid, TRUE);
	gtk_widget_set_visible (frame, TRUE);

	/* Comment */

	label = gtk_label_new (_("Pattern Comment"));
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_box_append (GTK_BOX (vbox), label);
	gtk_widget_set_visible (label, TRUE);

	comment_text_buffer = gtk_text_buffer_new (NULL);
	comment_text = gtk_text_view_new_with_buffer (comment_text_buffer);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (comment_text), GTK_WRAP_WORD_CHAR);
	g_signal_connect (comment_text_buffer, "changed",
			G_CALLBACK (sync_pattern_data), NULL);
	gtk_widget_set_visible (comment_text, TRUE);

	scrollwin = gtk_scrolled_window_new();
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
	                                GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_widget_set_size_request (scrollwin, -1, 80);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), comment_text);
	gtk_widget_set_visible (scrollwin, TRUE);

	GtkWidget *comment_frame = gtk_frame_new (NULL);
	gtk_frame_set_child (GTK_FRAME (comment_frame), scrollwin);
	gtk_box_append (GTK_BOX (vbox), comment_frame);
	gtk_widget_set_visible (comment_frame, TRUE);

	gtk_widget_set_visible (vbox, TRUE);

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
	GtkWidget *button;
	GtkWidget *peditor;
	int i;

	page_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	xqf_widget_set_margin_all (page_hbox, 8);

	label = gtk_label_new (_("Player Filter"));
	gtk_widget_set_visible (label, TRUE);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_hbox, label);

	/* Pattern list (GtkColumnView) */

	scrollwin = gtk_scrolled_window_new();
	gtk_box_append (GTK_BOX (page_hbox), scrollwin);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	pattern_store = g_list_store_new (G_TYPE_OBJECT);
	pattern_sel_model = GTK_SINGLE_SELECTION (
			gtk_single_selection_new (G_LIST_MODEL (pattern_store)));
	gtk_single_selection_set_autoselect (pattern_sel_model, FALSE);
	gtk_single_selection_set_can_unselect (pattern_sel_model, TRUE);

	pattern_list = gtk_column_view_new (GTK_SELECTION_MODEL (pattern_sel_model));
	gtk_widget_set_size_request (pattern_list, 260, 200);
	gtk_column_view_set_reorderable (GTK_COLUMN_VIEW (pattern_list), FALSE);
	gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (pattern_list), TRUE);

	for (i = 0; i < 3; i++) {
		GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
		g_signal_connect (factory, "setup", G_CALLBACK (grp_col_setup_cb), GINT_TO_POINTER (i));
		g_signal_connect (factory, "bind",  G_CALLBACK (grp_col_bind_cb),  GINT_TO_POINTER (i));

		GtkColumnViewColumn *col = gtk_column_view_column_new ("", factory);
		gtk_column_view_column_set_resizable (col, FALSE);
		gtk_column_view_column_set_fixed_width (col, pixmap_width (&group_pix[i]) + 8);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (pattern_list), col);
		g_object_unref (col);
		g_object_unref (factory);
	}

	{
		GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
		g_signal_connect (factory, "setup", G_CALLBACK (text_col_setup_cb), NULL);
		g_signal_connect (factory, "bind",  G_CALLBACK (text_col_bind_cb),  (gpointer) "mode");
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("Mode"), factory);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_column_set_fixed_width (col, 55);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (pattern_list), col);
		g_object_unref (col);
		g_object_unref (factory);

		factory = gtk_signal_list_item_factory_new ();
		g_signal_connect (factory, "setup", G_CALLBACK (text_col_setup_cb), NULL);
		g_signal_connect (factory, "bind",  G_CALLBACK (text_col_bind_cb),  (gpointer) "pattern");
		col = gtk_column_view_column_new (_("Pattern"), factory);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (pattern_list), col);
		g_object_unref (col);
		g_object_unref (factory);
	}

	g_signal_connect (pattern_sel_model, "notify::selected",
			G_CALLBACK (pattern_list_selection_changed), NULL);

	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), pattern_list);
	gtk_widget_set_visible (pattern_list, TRUE);
	gtk_widget_set_visible (scrollwin, TRUE);

	/* Buttons */

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_box_append (GTK_BOX (page_hbox), vbox);

	button = gtk_button_new_with_label (_("New"));
	gtk_box_append (GTK_BOX (vbox), button);
	g_signal_connect (button, "clicked", G_CALLBACK (new_pattern_callback), NULL);
	gtk_widget_set_visible (button, TRUE);

	delete_button = gtk_button_new_with_label (_("Delete"));
	gtk_box_append (GTK_BOX (vbox), delete_button);
	g_signal_connect (delete_button, "clicked", G_CALLBACK (delete_pattern_callback), NULL);
	gtk_widget_set_visible (delete_button, TRUE);

	vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_set_halign (vbox2, GTK_ALIGN_FILL);
	gtk_widget_set_valign (vbox2, GTK_ALIGN_CENTER);
	gtk_box_append (GTK_BOX (vbox), vbox2);

	up_button = gtk_button_new_with_label (_("Up"));
	gtk_box_append (GTK_BOX (vbox2), up_button);
	g_signal_connect (up_button, "clicked", G_CALLBACK (move_up_down_pattern_callback), (void *) -1);
	gtk_widget_set_visible (up_button, TRUE);

	down_button = gtk_button_new_with_label (_("Down"));
	gtk_box_append (GTK_BOX (vbox2), down_button);
	g_signal_connect (down_button, "clicked", G_CALLBACK (move_up_down_pattern_callback), (void *) 1);
	gtk_widget_set_visible (down_button, TRUE);

	gtk_widget_set_visible (vbox2, TRUE);
	gtk_widget_set_visible (vbox, TRUE);

	/* Pattern Editor */

	peditor = player_filter_pattern_editor ();
	gtk_box_append (GTK_BOX (page_hbox), peditor);

	gtk_widget_set_visible (page_hbox, TRUE);

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

