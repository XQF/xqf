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

#include <sys/types.h>
#include <string.h>	/* strlen, strstr, strcpy */

#include "i18n.h"
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
static void master_check_master_addr_prefix()
{
  char *pos;
  char *master_addr;

  master_addr= gtk_entry_get_text(GTK_ENTRY (GTK_COMBO
	(master_addr_combo)->entry));

  if(!master_addr|| !strlen(master_addr))
  {
	  if(current_master_query_type==MASTER_LAN)
	  {
		  char *txt =
			  g_strdup_printf("%s%s",
				master_prefixes[current_master_query_type],
				"255.255.255.255");
		  gtk_entry_set_text(
		      GTK_ENTRY( GTK_COMBO( master_addr_combo)->entry), txt);
	  }
	  return;
  }
  
  if (g_strncasecmp(master_addr, master_prefixes[current_master_query_type],
      strlen(master_prefixes[current_master_query_type])))
  {
    pos = lowcasestrstr(master_addr,"://");
    if(!pos)
    {
      pos = master_addr;
    }
    else
    {
      // +"://"
      pos+=3;
    }
    master_addr =
      g_strconcat(master_prefixes[current_master_query_type],pos,NULL);
    gtk_entry_set_text(GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry),master_addr);
  }
}

static void master_okbutton_callback (GtkWidget *widget, GtkWidget* window)
{
  master_check_master_addr_prefix();

  master_addr_result = strdup_strip (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry)));
  master_name_result = strdup_strip (gtk_entry_get_text (
                               GTK_ENTRY (GTK_COMBO (master_name_combo)->entry)));

  config_set_string ("/" CONFIG_FILE "/Add Master/game", 
                                                      type2id (master_type));
  
  if(!master_addr_result || !master_name_result)
  {
    dialog_ok (NULL, _("You have to specify a name and an address."));
    return;
  }

  master_to_add = add_master (master_addr_result, master_name_result, master_type, TRUE, FALSE);
  if(!master_to_add)
  {
    dialog_ok (NULL, _("Master address \"%s\" is not valid."),
	master_addr_result);
  }
  else
  {

    if (master_addr_result)
      history_add (master_history_addr, master_addr_result);
    if (master_name_result)
      history_add (master_history_name, master_name_result);

    gtk_widget_destroy(window);
  }
}

static void select_master_type_callback (GtkWidget *widget, enum server_type type)
{
  master_type = type;
  gtk_widget_set_state (master_query_type_radios[MASTER_NATIVE], GTK_STATE_NORMAL);
  if(!games[type].default_master_port)
  {
    gtk_widget_set_sensitive
      (GTK_WIDGET(master_query_type_radios[MASTER_NATIVE]),FALSE);
   if(current_master_query_type==MASTER_NATIVE)
   {
     gtk_toggle_button_set_active
       (GTK_TOGGLE_BUTTON(master_query_type_radios[MASTER_GAMESPY]),TRUE);
   }
  }
  else
  {
    gtk_widget_set_sensitive
      (GTK_WIDGET(master_query_type_radios[MASTER_NATIVE]),TRUE);
  }
}

static void master_type_radio_callback (GtkWidget *widget, enum master_query_type type)
{
  char* master_name;

  current_master_query_type = type;
  master_check_master_addr_prefix();

  master_name = gtk_entry_get_text(GTK_ENTRY (GTK_COMBO
		      (master_name_combo)->entry));

  if(current_master_query_type==MASTER_LAN
      && (!master_name || !strlen(master_name)))
  {
    gtk_entry_set_text(
      GTK_ENTRY( GTK_COMBO( master_name_combo)->entry), _("LAN"));
  }
}

static void master_activate_radio_for_type( enum master_query_type type )
{
  if( type < MASTER_NATIVE || type >= MASTER_NUM_QUERY_TYPES )
    type=MASTER_NATIVE;

  if(master_query_type_radios[type])
  {
    gtk_toggle_button_set_active
      (GTK_TOGGLE_BUTTON(master_query_type_radios[type]),TRUE);
  }
}

static void master_address_from_history_selected_callback (GtkWidget *widget,
    gpointer data)
{
  char* str = gtk_entry_get_text( GTK_ENTRY (GTK_COMBO (master_addr_combo)->entry));
  enum master_query_type type = get_master_query_type_from_address(str);
  master_activate_radio_for_type(type);
}

static GtkWidget *create_master_type_menu (enum server_type type) {
  GtkWidget *menu;
  GtkWidget *menu_item;
  GtkWidget *first_menu_item = NULL;
  int i;
  int j = 0;
   
  menu = gtk_menu_new ();

  for (i = 0; i < GAMES_TOTAL; i++) {

    // Skip a game if it's not configured and show only configured is enabled
    if (!games[i].cmd && default_show_only_configured_games)
      continue;

    if (j == type) {
      // Store first valid game menu item for gtk_menu_item_activate below
      menu_item = first_menu_item = gtk_menu_item_new ();
    }
    else
      menu_item = gtk_menu_item_new ();
    
    j++;

    menu_item = gtk_menu_item_new ();
    gtk_menu_append (GTK_MENU (menu), menu_item);

    gtk_container_add (GTK_CONTAINER (menu_item), game_pixmap_with_label (i));

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                     GTK_SIGNAL_FUNC (select_master_type_callback), (gpointer) i);

    gtk_widget_show (menu_item);
  }

  // initiates callback to set servertype to first configured game
  gtk_menu_item_activate (GTK_MENU_ITEM (first_menu_item)); 

  return menu;
}

char *master2url( struct master *m )
{
  char *query_type;
  char *address;
  char *result = NULL;

  if ( m->master_type >= MASTER_NATIVE
      && m->master_type < MASTER_NUM_QUERY_TYPES )
  {
    query_type = master_prefixes[m->master_type];
  }
  else
    return NULL;

  switch(m->master_type)
  {
    case MASTER_NATIVE:
    case MASTER_GAMESPY:
    case MASTER_LAN:
      if(m->host)
      {
	address = inet_ntoa(m->host->ip);
      }
      else
      {
	address = m->hostname;
      }
      result = g_strdup_printf("%s%s:%d",query_type,address,m->port);
      break;
    case MASTER_HTTP:
    case MASTER_FILE:
      result = strdup(m->url);
      break;
    case MASTER_NUM_QUERY_TYPES:
    case MASTER_INVALID_TYPE:
      break;
  }

  return result;
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

  int j;
  int menu_type = 0;

  master_name_result = NULL;
  master_addr_result = NULL;
  current_master_query_type = MASTER_NATIVE;

  master_to_edit = NULL;
  master_to_add = NULL;
  
  for (i=MASTER_NATIVE;i<MASTER_NUM_QUERY_TYPES;i++)
    master_query_type_radios[i]=NULL;

  master_to_edit = m;

  if(master_to_edit)
  {
    current_master_query_type = master_to_edit->master_type;
    master_type = master_to_edit->type;
  }
  else
  {
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

  if (default_show_only_configured_games) {
    // Find last configured game in list and set menu_type for menu position
    for (j = 0; j < GAMES_TOTAL; j++) {
      if (games[j].cmd) {
        if (j == master_type)
          break;
        menu_type++;
      }
    }
    if (j == GAMES_TOTAL) {
      // Game not found propbably because last added game is no longer configured.
      // Look for first configured game and use that.
      for (j = 0; j < GAMES_TOTAL; j++) {
        if (games[j].cmd) {
          master_type = j;
          menu_type = 0;
          break;
        }
      }
    }
  }
  else
    menu_type = master_type;
  
  if (master_to_edit)
  {
    windowtitle=_("Rename Master");
  }
  else
  {
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
  gtk_signal_connect(
                   GTK_OBJECT (GTK_COMBO (master_name_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (master_okbutton_callback), GTK_OBJECT (window));

  GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_name_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_name_combo)->button, GTK_CAN_FOCUS);
  gtk_widget_grab_focus (GTK_COMBO (master_name_combo)->entry);

  gtk_widget_show (master_name_combo);
 
  if (master_history_name->items)
    combo_set_vals (master_name_combo, master_history_name->items, "");

  if(master_to_edit)
  {
    gtk_entry_set_text(GTK_ENTRY (GTK_COMBO
	  (master_name_combo)->entry),strdup(master_to_edit->name));
  }


  /* Master Type Option Menu */

  option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (option_menu), 
                                                  create_master_type_menu (menu_type));
  gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), menu_type);
  
  if(master_to_edit)
  {
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
  gtk_signal_connect (
                   GTK_OBJECT (GTK_COMBO (master_addr_combo)->entry), "activate",
                   GTK_SIGNAL_FUNC (master_okbutton_callback), GTK_OBJECT (window));
  gtk_signal_connect (
                   GTK_OBJECT (GTK_COMBO (master_addr_combo)->list),
		   "selection-changed",
                   GTK_SIGNAL_FUNC
		   (master_address_from_history_selected_callback),NULL);

  GTK_WIDGET_SET_FLAGS (GTK_COMBO (master_addr_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (master_addr_combo)->button, GTK_CAN_FOCUS);
//  gtk_widget_grab_focus (GTK_COMBO (master_addr_combo)->entry);
  
  gtk_widget_show (master_addr_combo);

  if (master_history_addr->items)
    combo_set_vals (master_addr_combo, master_history_addr->items, "");

  if(master_to_edit)
  {
    gtk_entry_set_text(GTK_ENTRY (GTK_COMBO
	  (master_addr_combo)->entry),master2url(master_to_edit));
    gtk_widget_set_state (master_addr_combo, GTK_STATE_NORMAL);
    gtk_widget_set_sensitive (GTK_WIDGET(master_addr_combo),FALSE);
  }
  
  gtk_widget_show (table);

  /* query type */
  hbox = gtk_hbox_new (TRUE, 8);
  for (i=MASTER_NATIVE;i<MASTER_NUM_QUERY_TYPES;i++)
  {
    master_query_type_radios[i] =
		    gtk_radio_button_new_with_label_from_widget(
			i==MASTER_NATIVE?NULL:GTK_RADIO_BUTTON(master_query_type_radios[MASTER_NATIVE]),
			_(master_designation[i]));
    if(master_to_edit)
    {
      gtk_widget_set_sensitive (GTK_WIDGET(master_query_type_radios[i]),FALSE);
    }
    gtk_signal_connect(GTK_OBJECT (master_query_type_radios[i]), "toggled",
                   GTK_SIGNAL_FUNC (master_type_radio_callback), (gpointer)i);

    gtk_widget_show (master_query_type_radios[i]);
    gtk_box_pack_start (GTK_BOX (hbox),master_query_type_radios[i], FALSE, FALSE, 0);
  }
  if(master_to_edit)
  {
    master_activate_radio_for_type(current_master_query_type);
  }
  else if(!games[master_type].default_master_port &&
		    current_master_query_type == MASTER_NATIVE)
  {
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
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  /* OK Button */

  button = gtk_button_new_with_label ("OK");
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
      GTK_SIGNAL_FUNC(master_okbutton_callback), window);
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

