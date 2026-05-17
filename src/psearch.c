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
#include <string.h>     /* strcmp */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "xqf-lists.h"
#include "xqf-player-item.h"
#include "xqf-server-item.h"
#include "srv-list.h"
#include "srv-prop.h"
#include "dialogs.h"
#include "sort.h"
#include "utils.h"
#include "history.h"
#include "config.h"
#include "psearch.h"

#define REGCOMP_FLAGS (REG_EXTENDED | REG_NOSUB | REG_ICASE)


static struct psearch_data psearch = { NULL, PSEARCH_MODE_SUBSTR, NULL };

static struct history *psearch_history = NULL;

static GtkWidget *psearch_combo;
static GtkWidget *mode_buttons[3];

static int psearch_new_pattern;

static const char *mode_names[3] = {
	N_("Exact Match"),
	N_("Substring"),
	N_("Regular Expression")
};


static int psearch_test_player (struct player *p) {
	return
		((psearch.mode == PSEARCH_MODE_STRING &&
		  g_ascii_strcasecmp (p->name, psearch.data) == 0) ||
		 (psearch.mode == PSEARCH_MODE_SUBSTR &&
		  lowcasestrstr (p->name, psearch.data)) ||
		 (psearch.mode == PSEARCH_MODE_REGEXP &&
		  regexec ((regex_t *) psearch.data, p->name, 0, NULL, 0) == 0));
}


static void psearch_free_pattern_data (void) {
	if (psearch.data) {
		if (psearch.mode == PSEARCH_MODE_REGEXP) {
			regfree ((regex_t *) psearch.data);
		}
		g_free (psearch.data);
		psearch.data = NULL;
	}
}


static void psearch_free_pattern (void) {
	if (psearch.pattern) {
		g_free (psearch.pattern);
		psearch.pattern = NULL;
	}
	psearch_free_pattern_data ();
}


static char *get_regerror (int errcode, regex_t *compiled) {
	size_t length;
	char *buffer;

	length = regerror (errcode, compiled, NULL, 0);
	buffer = g_malloc (length);
	regerror (errcode, compiled, buffer, length);
	return buffer;
}


static int psearch_compile_pattern (void) {
	char *error;
	int res;

	psearch_free_pattern_data ();

	if (psearch.pattern && psearch.pattern[0]) {
		switch (psearch.mode) {

			case PSEARCH_MODE_REGEXP:
				psearch.data = g_malloc (sizeof (regex_t));

				res = regcomp ((regex_t *) psearch.data, psearch.pattern, REGCOMP_FLAGS);
				if (res) {
					error = get_regerror (res, (regex_t *) psearch.data);
					psearch_free_pattern ();

					dialog_ok (_("XQF: Error"),
							_("Regular Expression Error!\n\n%s\n\n%s."),
							psearch.pattern, error);
					g_free (error);
					return FALSE;
				}
				break;

			case PSEARCH_MODE_STRING:
			case PSEARCH_MODE_SUBSTR:
			default:
				psearch.data = g_ascii_strdown(psearch.pattern, -1);    /* g_ascii_strdown does implicit strndup */
				break;

		}

		return TRUE;
	}

	return FALSE;
}


static void psearch_combo_activate_callback (GtkWidget *widget,
		gpointer data) {
	psearch_free_pattern ();

	if (gtk_check_button_get_active (GTK_CHECK_BUTTON (mode_buttons[PSEARCH_MODE_STRING])))
		psearch.mode = PSEARCH_MODE_STRING;
	else if (gtk_check_button_get_active (GTK_CHECK_BUTTON (mode_buttons[PSEARCH_MODE_SUBSTR])))
		psearch.mode = PSEARCH_MODE_SUBSTR;
	else
		psearch.mode = PSEARCH_MODE_REGEXP;

	config_set_int ("/" CONFIG_FILE "/Find Player/mode", psearch.mode);

	psearch.pattern = strdup_strip (
			gtk_entry_get_text (GTK_ENTRY (combo_get_entry (psearch_combo))));

	if (psearch.pattern && psearch.pattern[0]) {
		history_add (psearch_history, psearch.pattern);
		psearch_new_pattern = psearch_compile_pattern ();
	}
}


int find_player_dialog (void) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	int i;

	psearch_new_pattern = FALSE;

	window = dialog_create_modal_transient_window (_("Find Player"),
			TRUE, FALSE, NULL);
	main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 16);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	/* Pattern Entry */

	label = gtk_label_new (_("Find Player:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	/* ComboBox */

	psearch_combo = gtk_combo_box_text_new_with_entry ();
	gtk_entry_set_max_length (GTK_ENTRY (combo_get_entry (psearch_combo)), 128);
	gtk_widget_set_size_request (GTK_WIDGET (combo_get_entry (psearch_combo)), 160, -1);
	g_signal_connect (G_OBJECT (combo_get_entry (psearch_combo)),
			"activate", G_CALLBACK (psearch_combo_activate_callback), NULL);
	g_signal_connect_swapped (G_OBJECT (combo_get_entry (psearch_combo)),
			"activate", G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
	gtk_box_pack_start (GTK_BOX (hbox), psearch_combo, TRUE, TRUE, 0);
	gtk_widget_grab_focus (GTK_WIDGET (combo_get_entry (psearch_combo)));
	gtk_widget_show (psearch_combo);

	if (psearch_history->items) {
		combo_set_vals (psearch_combo, psearch_history->items, "");

		gtk_combo_box_set_active (GTK_COMBO_BOX (psearch_combo), 0);
		gtk_editable_select_region (GTK_EDITABLE (combo_get_entry (psearch_combo)), 0, -1);
	}

	/* OK Button */

	button = gtk_button_new_with_label (_("OK"));
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (psearch_combo_activate_callback),
			G_OBJECT (psearch_combo));
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	/* Cancel Button */

	button = gtk_button_new_with_label (_("Cancel"));
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	/* Mode Buttons */

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	for (i = 0; i < 3; i++) {
		mode_buttons[i] = gtk_check_button_new_with_label (_(mode_names[i]));
		if (i > 0)
			gtk_check_button_set_group (GTK_CHECK_BUTTON (mode_buttons[i]), GTK_CHECK_BUTTON (mode_buttons[0]));
		gtk_box_pack_start (GTK_BOX (hbox), mode_buttons[i], FALSE, FALSE, 0);
		gtk_widget_show (mode_buttons[i]);
	}

	gtk_check_button_set_active (
			GTK_CHECK_BUTTON (mode_buttons[psearch.mode]), TRUE);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);
	gtk_widget_show (window);

	dialog_run_modal (window);

	unregister_window (window);

	return psearch_new_pattern;
}


void psearch_init (void) {
	psearch_history = history_new ("Player Search");
	psearch.mode = config_get_int ("/" CONFIG_FILE "/Find Player/mode=1");
}


void psearch_done (void) {
	if (psearch.pattern) {
		psearch_free_pattern ();
	}
	if (psearch_history) {
		history_free (psearch_history);
		psearch_history = NULL;
	}
}


int psearch_data_is_empty (void) {
	return (psearch.data == NULL);
}


char *psearch_lookup_pattern (void) {
	char *pat;
	int len;

	if (!psearch.pattern || !psearch.pattern[0])
		return NULL;

	len = strlen (psearch.pattern) + 2 + strlen (mode_names[psearch.mode]) + 2;
	pat = g_malloc (len);

	g_snprintf (pat, len, "%s (%s)", psearch.pattern, mode_names[psearch.mode]);

	return pat;
}



static int server_has_player (struct server *s) {
	GSList *plist;
	struct player *p;

	for (plist = s->players; plist; plist = plist->next) {
		p = (struct player *) plist->data;
		if (psearch_test_player (p))
			return TRUE;
	}
	return FALSE;
}


static int psearch_next_player (guint start) {
	if (!player_selection) return FALSE;

	guint n = g_list_model_get_n_items (G_LIST_MODEL (player_selection));
	for (guint i = start; i < n; i++) {
		XqfPlayerItem *pi = XQF_PLAYER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (player_selection), i));
		struct player *p = xqf_player_item_get (pi);
		gboolean match = psearch_test_player (p);
		g_object_unref (pi);

		if (match) {
			gtk_selection_model_select_item (player_selection, i, TRUE);
			gtk_widget_activate_action (player_view, "list.scroll-to-item", "u", i);
			return TRUE;
		}
	}

	return FALSE;
}


void find_player (int find_next) {
	if (!server_selection) return;

	guint srv_n = g_list_model_get_n_items (G_LIST_MODEL (server_selection));
	guint srv_start = 0;

	if (find_next && cur_server) {
		/* Find the position of cur_server in the sorted view */
		for (guint i = 0; i < srv_n; i++) {
			XqfServerItem *si = XQF_SERVER_ITEM (
				g_list_model_get_item (G_LIST_MODEL (server_selection), i));
			struct server *s = xqf_server_item_get (si);
			g_object_unref (si);
			if (s == cur_server) {
				/* Try to continue from the next player in this server first */
				guint pl_start = 0;
				if (player_selection) {
					GtkBitset *pl_sel = gtk_selection_model_get_selection (player_selection);
					guint pl_min = gtk_bitset_get_minimum (pl_sel);
					if (pl_min != G_MAXUINT)
						pl_start = pl_min + 1;
					gtk_bitset_unref (pl_sel);
				}
				if (psearch_next_player (pl_start))
					return;
				srv_start = i + 1;
				break;
			}
		}
	}

	for (guint i = srv_start; i < srv_n; i++) {
		XqfServerItem *si = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_selection), i));
		struct server *s = xqf_server_item_get (si);
		g_object_unref (si);

		if (server_has_player (s)) {
			server_list_select_one ((int) i);
			psearch_next_player (0);
			return;
		}
	}

	if (!find_next || !cur_server) {
		dialog_ok (NULL, _("Player not found."));
		reset_main_status_bar(builder);
	}
	else {
		if (dialog_yesno (_("XQF: End of server list reached"), 0, _("Yes"), _("No"),
					_("Continue search from beginning?"))) {
			find_player (FALSE);
		}
		else
			reset_main_status_bar(builder);
	}
}
