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
#include <string.h>	/* strlen, strstr, strcpy */

#include <gtk/gtk.h>

#include "game.h"
#include "history.h"
#include "dialogs.h"
#include "utils.h"
#include "config.h"
#include "source.h"
#include "addmaster.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

static struct history *master_history_addr = NULL;
static struct history *master_history_name = NULL;

static char *master_addr_result;
static char *master_name_result;
static enum server_type *master_type;

static GtkWidget *master_addr_combo;
static GtkWidget *master_name_combo;


static void master_combo_activate_callback (GtkWidget *widget, gpointer data) {
  char *str;

  master_addr_result = strdup_strip (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry)));
  master_name_result = strdup_strip (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (master_name_combo)->entry)));

  config_set_string ("/" CONFIG_FILE "/Add Master/game", 
                                                      type2id (*master_type));

  if (master_addr_result)
    history_add (master_history_addr, master_addr_result);
  if (master_name_result)
    history_add (master_history_name, master_name_result);

  if (master_name_result == NULL || master_addr_result == NULL) {
    if (master_name_result) {
      g_free (master_name_result);
      master_name_result = NULL;
    }
    if (master_addr_result) {
      g_free (master_addr_result);
      master_addr_result = NULL;
    }
    return;
  }

  /* No prefix? Add "master://" */

  if (strstr (master_addr_result, "://") == NULL) {
    str = g_malloc (strlen (master_addr_result) + sizeof (PREFIX_MASTER));
    strcpy (str, PREFIX_MASTER);
    strcpy (str + sizeof (PREFIX_MASTER) - 1, master_addr_result);
    g_free (master_addr_result);
    master_addr_result = str;
  }
}


static void select_master_type_callback (GtkWidget *widget, 
					                  enum server_type type) {
  *master_type = type;
}


static GtkWidget *create_master_type_menu (void) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  int i;
 
  menu = gtk_menu_new ();

  for (i = 0; i < GAMES_TOTAL; i++) {
    menu_item = gtk_menu_item_new ();
    gtk_menu_append (GTK_MENU (menu), menu_item);

    gtk_container_add (GTK_CONTAINER (menu_item), game_pixmap_with_label (i));

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                     GTK_SIGNAL_FUNC (select_master_type_callback), (gpointer) i);

    gtk_widget_show (menu_item);
  }

  return menu;
}


char *add_master_dialog (enum server_type *type, char **desc) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *table;
  GtkWidget *option_menu;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  GtkWidget *hseparator;
  char *typestr;

  master_name_result = NULL;
  master_addr_result = NULL;
  master_type = type;

  typestr = config_get_string ("/" CONFIG_FILE "/Add Master/game");
  if (typestr) {
    *type = id2type (typestr);
    g_free (typestr);
  }
  else {
    *type = QW_SERVER;
  }

  window = dialog_create_modal_transient_window (_("Add Master"), 
                                                           TRUE, FALSE, NULL);
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
  gtk_signal_connect (
                   GTK_OBJECT (GTK_COMBO (master_name_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (master_combo_activate_callback), NULL);
  gtk_signal_connect_object (
                   GTK_OBJECT (GTK_COMBO (master_name_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));

  GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_name_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_name_combo)->button, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (GTK_COMBO (master_name_combo)->entry);

  gtk_widget_show (master_name_combo);
 
  if (master_history_name->items)
    combo_set_vals (master_name_combo, master_history_name->items, "");

  /* Master Type Option Menu */

  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                                                  create_master_type_menu ());
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), *type);
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
  gtk_signal_connect (
                   GTK_OBJECT (GTK_COMBO (master_addr_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (master_combo_activate_callback), NULL);
  gtk_signal_connect_object (
                   GTK_OBJECT (GTK_COMBO (master_addr_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));

  GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_addr_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_addr_combo)->button, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (GTK_COMBO (master_addr_combo)->entry);
  gtk_widget_show (master_addr_combo);

  if (master_history_addr->items)
    combo_set_vals (master_addr_combo, master_history_addr->items, "");
  
  gtk_widget_show (table);
  
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
		             GTK_SIGNAL_FUNC (master_combo_activate_callback),
			     GTK_OBJECT (GTK_COMBO (master_name_combo)->entry));
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

  *desc = master_name_result;
  return master_addr_result;
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

