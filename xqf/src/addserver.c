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

#include "gnuconfig.h"

#include "i18n.h"
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
  enter_server_result = strdup_strip (gtk_entry_get_text (
                                GTK_ENTRY (GTK_COMBO (server_combo)->entry)));
  history_add (server_history, enter_server_result);

  config_set_string ("/" CONFIG_FILE "/Add Server/game", 
                                                      type2id (*server_type));
}


static void select_server_type_callback (GtkWidget *widget, 
  					                  enum server_type type) {
  *server_type = type;
}


static GtkWidget *create_server_type_menu (void) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  GtkWidget *first_menu_item = NULL;
  int i;
  int j=0;
  
  menu = gtk_menu_new ();

  for (i = 0; i < GAMES_TOTAL; i++) {

    // Skip a game if it's not configured and show only configured is enabled
    if (!games[i].cmd && default_show_only_configured_games)
      continue;
  
    if (j == 0) {
      menu_item = first_menu_item = gtk_menu_item_new ();
      j++;
    }
    else
      menu_item = gtk_menu_item_new ();
      
    gtk_menu_append (GTK_MENU (menu), menu_item);

    gtk_container_add (GTK_CONTAINER (menu_item), game_pixmap_with_label (i));

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                 GTK_SIGNAL_FUNC (select_server_type_callback), (gpointer) i);

    gtk_widget_show (menu_item);
  }
  
  // initiates callback to set servertype to first configured game
  gtk_menu_item_activate (GTK_MENU_ITEM (first_menu_item)); 
  
  return menu;
}


char *add_server_dialog (enum server_type *type) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *option_menu;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hseparator;
  char *typestr;

  enter_server_result = NULL;
  server_type = type;
 
  typestr = config_get_string ("/" CONFIG_FILE "/Add Server/game");
 
  if (typestr) {
    *type = id2type (typestr);
    g_free (typestr);
  }
  else {
    *type = QW_SERVER;
  }

  window = dialog_create_modal_transient_window (_("Add Server"), 
                                                           TRUE, FALSE, NULL);
  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 16);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  /* Server Entry */

  label = gtk_label_new (_("Server:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  server_combo = gtk_combo_new ();
  gtk_widget_set_usize (server_combo, 200, -1);
  gtk_box_pack_start (GTK_BOX (hbox), server_combo, TRUE, TRUE, 0);
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (server_combo)->entry), 128);
  gtk_combo_set_case_sensitive (GTK_COMBO (server_combo), TRUE);
  gtk_combo_set_use_arrows_always (GTK_COMBO (server_combo), TRUE);
  gtk_combo_disable_activate (GTK_COMBO (server_combo));
  gtk_signal_connect (
                   GTK_OBJECT (GTK_COMBO (server_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (server_combo_activate_callback), NULL);
  gtk_signal_connect_object (
                   GTK_OBJECT (GTK_COMBO (server_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));

  GTK_WIDGET_SET_FLAGS (GTK_COMBO (server_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (server_combo)->button, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (GTK_COMBO (server_combo)->entry);
  gtk_widget_show (server_combo);

  if (server_history->items)
    combo_set_vals (server_combo, server_history->items, "");

  /* Server Type Option Menu */

  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                                                  create_server_type_menu ());
  // Set default type passed to add_server_dialog
  //gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), *type);
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), 0);
  gtk_widget_show (option_menu);

  gtk_widget_show (hbox);

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
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  /* OK Button */

  button = gtk_button_new_with_label ("OK");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
		             GTK_SIGNAL_FUNC (server_combo_activate_callback),
			     GTK_OBJECT (GTK_COMBO (server_combo)->entry));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  gtk_widget_show (main_vbox);
  gtk_widget_show (window);

  gtk_main ();

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


