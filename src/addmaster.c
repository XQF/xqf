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
#include <string.h>     /* strlen, strstr, strcpy */

#include <glib.h>
#include <glib/gi18n.h>

#include "game.h"
#include "xqf-ui.h"
#include "history.h"
#include "dialogs.h"
#include "utils.h"
#include "config.h"
#include "source.h"
#include "addmaster.h"
#include "srv-prop.h"
#include "pref.h"

static struct history *master_history_addr;
static struct history *master_history_name;

static char *master_addr_result;
static char *master_name_result;
static enum server_type master_type;

static struct master *master_to_add;

// currently active radio button
static enum master_query_type current_master_query_type = MASTER_NATIVE;

static GtkWidget *master_addr_combo;
static GtkWidget *master_name_combo;
static GtkWidget *master_query_type_radios[MASTER_NUM_QUERY_TYPES];

// get text from master address entry, check if prefix matches radio buttons,
// modify and write back if needed
static void master_check_master_addr_prefix(void) {
	const gchar *pos;
	const gchar *master_addr;
	gchar *master_tmp_addr;

	master_addr= gtk_entry_get_text(GTK_ENTRY (GTK_COMBO(master_addr_combo)->entry));

	// Replace up to :// with master type selected from radio buttons
	if (g_ascii_strncasecmp(master_addr, master_prefixes[current_master_query_type],
				strlen(master_prefixes[current_master_query_type]))) {
		pos = lowcasestrstr(master_addr,"://");
		if (!pos) {
			pos = master_addr;
		}
		else {
			// +"://"
			pos+=3;
		}

		// Add lan://255.255.255.255 if user picks LAN and has not already entered an address
		if (current_master_query_type == MASTER_LAN && (strlen(master_addr) <= (size_t)(pos - master_addr))) {
			char *txt = g_strdup_printf("%s%s", master_prefixes[current_master_query_type], "255.255.255.255");
			gtk_entry_set_text(
					GTK_ENTRY(GTK_COMBO(master_addr_combo)->entry), txt);
			g_free(txt);
		}

		// Otherwise, just change the master type (xxx://)
		else {
			master_tmp_addr = g_strconcat(master_prefixes[current_master_query_type], pos, NULL);
			gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(master_addr_combo)->entry), master_tmp_addr);
			g_free(master_tmp_addr);
		}
	}
}

static void master_okbutton_callback (GtkWidget *widget, GtkWidget* window) {
	master_check_master_addr_prefix();

	master_addr_result = strdup_strip (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry)));
	master_name_result = strdup_strip (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (master_name_combo)->entry)));

	config_set_string ("/" CONFIG_FILE "/Add Master/game", type2id (master_type));

	if (!master_addr_result || !master_name_result) {
		dialog_ok (NULL, _("You have to specify a name and an address."));
		return;
	}

	// FIXME: gui does not allow to set server_query_type (defaults here as UNKNOWN_SERVER)
	master_to_add = add_master (master_addr_result, master_name_result, master_type, UNKNOWN_SERVER, NULL, TRUE, FALSE);
	if (!master_to_add) {
		dialog_ok (NULL, _("Master address \"%s\" is not valid."),
				master_addr_result);
	}
	else {

		if (master_addr_result)
			history_add (master_history_addr, master_addr_result);
		if (master_name_result)
			history_add (master_history_name, master_name_result);

		gtk_widget_destroy(window);
	}
}

static void select_master_type_callback (GtkWidget *widget, enum server_type type) {
	if (!master_query_type_radios[MASTER_NATIVE])
		return;

	master_type = type;
	gtk_widget_set_state (master_query_type_radios[MASTER_NATIVE], GTK_STATE_NORMAL);
	if (!games[type].default_master_port) {
		gtk_widget_set_sensitive
			(GTK_WIDGET(master_query_type_radios[MASTER_NATIVE]),FALSE);
		if (current_master_query_type == MASTER_NATIVE) {
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(master_query_type_radios[MASTER_GAMESPY]),TRUE);
		}
	}
	else {
		gtk_widget_set_sensitive
			(GTK_WIDGET(master_query_type_radios[MASTER_NATIVE]),TRUE);
	}
}

static void master_type_radio_callback(GtkWidget *widget, enum master_query_type type) {
	const gchar* master_name;

	// This gets called when a button is made inactive AND when it's made active
	// so only do this if it's set to active
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (master_query_type_radios[type]))) {
		current_master_query_type = type;
		master_check_master_addr_prefix();

		master_name = gtk_entry_get_text(GTK_ENTRY (GTK_COMBO (master_name_combo)->entry));

		if (current_master_query_type == MASTER_LAN
				&& (!master_name || !strlen(master_name))) {
			gtk_entry_set_text(
					GTK_ENTRY(GTK_COMBO(master_name_combo)->entry), _("LAN"));
		}
	}
}
static void master_activate_radio_for_type(enum master_query_type type) {
	if (type < MASTER_NATIVE || type >= MASTER_NUM_QUERY_TYPES)
		type=MASTER_NATIVE;

	if (master_query_type_radios[type]) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(master_query_type_radios[type]),TRUE);
	}
}

static void master_address_from_history_selected_callback (GtkWidget *widget, gpointer data) {
	const gchar* str = gtk_entry_get_text(GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry));
	enum master_query_type type = get_master_query_type_from_address(str);
	master_activate_radio_for_type(type);
}


struct master *add_master_dialog (struct master *m) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *table;
	GtkWidget *option_menu;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *hseparator;
	char *typestr;
	enum master_query_type i;
	struct master *master_to_edit;
	char *windowtitle;

	master_name_result = NULL;
	master_addr_result = NULL;
	current_master_query_type = MASTER_NATIVE;

	master_to_edit = NULL;
	master_to_add = NULL;

	for (i=MASTER_NATIVE;i<MASTER_NUM_QUERY_TYPES;i++)
		master_query_type_radios[i]=NULL;

	master_to_edit = m;

	if (master_to_edit) {
		current_master_query_type = master_to_edit->master_type;
		master_type = master_to_edit->type;
	}
	else {
		// Get last game type added (stored in master_okbutton_callback)
		typestr = config_get_string ("/" CONFIG_FILE "/Add Master/game");
		if (typestr) {
			master_type = id2type (typestr);
			g_free (typestr);
		}
		else {
			master_type = QW_SERVER;
		}
	}

	if (master_to_edit) {
		windowtitle=_("Rename Master");
	}
	else {
		windowtitle=_("Add Master");
	}
	window = dialog_create_modal_transient_window(windowtitle, TRUE, FALSE, NULL);
	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 16);
	gtk_box_pack_start (GTK_BOX (main_vbox), table, FALSE, FALSE, 0);

	/* Master Name (Description) */

	label = gtk_label_new (_("Master Name"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 0, 1);

	master_name_combo = gtk_combo_new ();
	gtk_widget_set_usize (master_name_combo, 200, -1);
	gtk_box_pack_start (GTK_BOX (hbox), master_name_combo, TRUE, TRUE, 0);
	gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (master_name_combo)->entry), 256);
	gtk_combo_set_case_sensitive (GTK_COMBO (master_name_combo), TRUE);
	gtk_combo_set_use_arrows_always (GTK_COMBO (master_name_combo), TRUE);
	gtk_combo_disable_activate (GTK_COMBO (master_name_combo));
	g_signal_connect(
			G_OBJECT (GTK_COMBO (master_name_combo)->entry), "activate",
			G_CALLBACK (master_okbutton_callback), G_OBJECT (window));

	GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_name_combo)->entry, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_name_combo)->button, GTK_CAN_FOCUS);
	gtk_widget_grab_focus (GTK_COMBO (master_name_combo)->entry);

	gtk_widget_show (master_name_combo);

	if (master_history_name->items)
		combo_set_vals (master_name_combo, master_history_name->items, "");

	if (master_to_edit) {
		gtk_entry_set_text(GTK_ENTRY (GTK_COMBO (master_name_combo)->entry), master_to_edit->name);
	}


	/* Master Type Option Menu */

	option_menu = create_server_type_menu (master_type, create_server_type_menu_filter_configured, G_CALLBACK(select_master_type_callback));

	gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);

	if (master_to_edit) {
		gtk_widget_set_state (option_menu, GTK_STATE_NORMAL);
		gtk_widget_set_sensitive (GTK_WIDGET(option_menu),FALSE);
	}

	gtk_widget_show (option_menu);

	gtk_widget_show (hbox);

	/* Master Address */

	label = gtk_label_new (_("Master Address"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
			GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);

	master_addr_combo = gtk_combo_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), master_addr_combo, 1, 2, 1, 2);
	gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry), 4096);
	gtk_combo_set_case_sensitive (GTK_COMBO (master_addr_combo), TRUE);
	gtk_combo_set_use_arrows_always (GTK_COMBO (master_addr_combo), TRUE);
	gtk_combo_disable_activate (GTK_COMBO (master_addr_combo));
	g_signal_connect (
			G_OBJECT (GTK_COMBO (master_addr_combo)->entry), "activate",
			G_CALLBACK (master_okbutton_callback), G_OBJECT (window));
	g_signal_connect (
			G_OBJECT (GTK_COMBO (master_addr_combo)->list),
			"selection-changed",
			G_CALLBACK
			(master_address_from_history_selected_callback),NULL);

	GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_addr_combo)->entry, GTK_CAN_FOCUS);
	GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_addr_combo)->button, GTK_CAN_FOCUS);
	// gtk_widget_grab_focus (GTK_COMBO (master_addr_combo)->entry);

	gtk_widget_show (master_addr_combo);

	if (master_history_addr->items)
		combo_set_vals (master_addr_combo, master_history_addr->items, "");

	if (master_to_edit) {
		char* url = master_to_url(master_to_edit);
		gtk_entry_set_text(GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry), url);
		gtk_widget_set_state (master_addr_combo, GTK_STATE_NORMAL);
		gtk_widget_set_sensitive (GTK_WIDGET(master_addr_combo),FALSE);
		g_free(url);
	}

	gtk_widget_show (table);

	/* query type */
	hbox = gtk_hbox_new (TRUE, 8);
	for (i=MASTER_NATIVE;i<MASTER_NUM_QUERY_TYPES;i++) {
		master_query_type_radios[i] =
			gtk_radio_button_new_with_label_from_widget(
					i == MASTER_NATIVE?NULL:GTK_RADIO_BUTTON(master_query_type_radios[MASTER_NATIVE]),
					_(master_designation[i]));
		if (master_to_edit) {
			gtk_widget_set_sensitive (GTK_WIDGET(master_query_type_radios[i]),FALSE);
		}
		g_signal_connect(G_OBJECT (master_query_type_radios[i]), "toggled",
				G_CALLBACK (master_type_radio_callback), (gpointer)i);

		gtk_widget_show (master_query_type_radios[i]);
		gtk_box_pack_start (GTK_BOX (hbox),master_query_type_radios[i], FALSE, FALSE, 0);
	}
	if (master_to_edit) {
		master_activate_radio_for_type(current_master_query_type);
	}
	else if (!games[master_type].default_master_port &&
			current_master_query_type == MASTER_NATIVE) {
		gtk_widget_set_state (master_query_type_radios[MASTER_NATIVE], GTK_STATE_NORMAL);
		gtk_widget_set_sensitive
			(GTK_WIDGET(master_query_type_radios[MASTER_NATIVE]),FALSE);
		gtk_toggle_button_set_active
			(GTK_TOGGLE_BUTTON(master_query_type_radios[MASTER_GAMESPY]),TRUE);
	}

	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	/* Separator */

	hseparator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (main_vbox), hseparator, FALSE, FALSE, 0);
	gtk_widget_show (hseparator);

	/* Buttons */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	/* Cancel Button */

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_set_usize (button, 80, -1);
	g_signal_connect_swapped (G_OBJECT (button), "clicked", G_CALLBACK (gtk_widget_destroy), window);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_show (button);

	/* OK Button */

	button = gtk_button_new_with_label ("OK");
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_set_usize (button, 80, -1);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK(master_okbutton_callback), window);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);
	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);

	return master_to_add;
}


void add_master_init (void) {
	master_history_addr = history_new ("Add Master Address");
	master_history_name = history_new ("Add Master Name");
}


void add_master_done (void) {
	if (master_history_addr) {
		history_free (master_history_addr);
		master_history_addr = NULL;
	}
	if (master_history_name) {
		history_free (master_history_name);
		master_history_name = NULL;
	}
}

