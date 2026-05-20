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

#include <glib.h>
#include <glib/gi18n.h>

#include "game.h"
#include "xqf-ui.h"
#include "history.h"
#include "dialogs.h"
#include "utils.h"
#include "config.h"
#include "addserver.h"
#include "srv-prop.h"
#include "pref.h"

static struct history *server_history = NULL;

static char *enter_server_result;
static enum server_type *server_type;
static GtkWidget *server_combo;


static void server_combo_activate_callback (GtkWidget *widget, gpointer data) {
	enter_server_result = strdup_strip (gtk_editable_get_text (
				GTK_EDITABLE (combo_get_entry (server_combo))));
	history_add (server_history, enter_server_result);

	config_set_string ("/" CONFIG_FILE "/Add Server/game",
			type2id (*server_type));
}


static void select_server_type_callback (GtkWidget *widget,
		enum server_type type) {
	*server_type = type;
}

/** dialog to prompt user for server type and address
 * @param type pointer where to store the selected type. if UNKNOWN_SERVER,
 *             preset type will be read from hitory file
 * @param addr preset string value for address field, NULL for nothing
 * @returns address string or NULL if user pressed cancel. string must be freed
 */
char *add_server_dialog (enum server_type *type, const char* addr) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *option_menu;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;

	g_return_val_if_fail(type != NULL, NULL);

	enter_server_result = NULL;

	if (*type == UNKNOWN_SERVER) {
		char *typestr;

		// Get last game type added (stored in server_combo_activate_callback)
		typestr = config_get_string ("/" CONFIG_FILE "/Add Server/game");

		if (typestr) {
			*type = id2type (typestr);
			g_free (typestr);
		}
		else {
			*type = 0; // Set to first game
		}
	}

	server_type = type;

	window = dialog_create_modal_transient_window (_("Add Server"), TRUE, FALSE, NULL);
	main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child (GTK_WINDOW (window), main_vbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	xqf_widget_set_margin_all (hbox, 16);
	gtk_box_append (GTK_BOX (main_vbox), hbox);

	/* Server Entry */

	label = gtk_label_new (_("Server:"));
	gtk_box_append (GTK_BOX (hbox), label);
	gtk_widget_set_visible (label, TRUE);

	server_combo = gtk_entry_new ();
	gtk_widget_set_size_request (server_combo, 200, -1);
	gtk_box_append (GTK_BOX (hbox), server_combo);
	gtk_entry_set_max_length (GTK_ENTRY (server_combo), 128);
	g_signal_connect (
			G_OBJECT (server_combo), "activate",
			G_CALLBACK (server_combo_activate_callback), NULL);
	g_signal_connect_swapped (
			G_OBJECT (server_combo), "activate",
			G_CALLBACK (gtk_window_destroy), GTK_WINDOW (window));

	gtk_widget_grab_focus (server_combo);
	gtk_widget_set_visible (server_combo, TRUE);

	combo_set_vals (server_combo, server_history->items, addr);

	/* Server Type Option Menu */

	option_menu = create_server_type_menu (*type,
			create_server_type_menu_filter_configured,
			G_CALLBACK(select_server_type_callback));

	gtk_box_append (GTK_BOX (hbox), option_menu);
	gtk_widget_set_visible (option_menu, TRUE);

	gtk_widget_set_visible (hbox, TRUE);

	/* Separator */

	gtk_box_append (GTK_BOX (main_vbox),
	                gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

	/* Buttons */

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	xqf_widget_set_margin_all (hbox, 8);
	gtk_box_append (GTK_BOX (main_vbox), hbox);

	/* Cancel Button */

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_box_append (GTK_BOX (hbox), button);
	gtk_widget_set_size_request (button, 80, -1);
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_window_destroy), GTK_WINDOW (window));
	gtk_widget_set_visible (button, TRUE);

	/* OK Button */

	button = gtk_button_new_with_label ("OK");
	gtk_box_append (GTK_BOX (hbox), button);
	gtk_widget_set_size_request (button, 80, -1);
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (server_combo_activate_callback),
			G_OBJECT (combo_get_entry (server_combo)));
	g_signal_connect_swapped (G_OBJECT (button), "clicked",
			G_CALLBACK (gtk_window_destroy), GTK_WINDOW (window));
	gtk_widget_set_visible (button, TRUE);

	gtk_widget_set_visible (hbox, TRUE);

	gtk_widget_set_visible (main_vbox, TRUE);
	gtk_widget_set_visible (window, TRUE);

	dialog_run_modal (window);

	unregister_window (window);

	return enter_server_result;
}


void add_server_init (void) {
	server_history = history_new (_("Add Server"));
}


void add_server_done (void) {
	if (server_history) {
		history_free (server_history);
		server_history = NULL;
	}
}


