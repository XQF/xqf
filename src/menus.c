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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "menus.h"


static void create_menu_recursive (GtkWidget *menu, 
			      const struct menuitem *items, 
			      GtkAccelGroup *accel_group) {
  GtkWidget *menu_item;
  GtkWidget *label;
  guint ac_key;

  while (items->type != MENU_END) {

    switch (items->type) {

    case MENU_ITEM:
    case MENU_CHECK_ITEM:
    case MENU_RADIO_ITEM:
    case MENU_BRANCH:
    case MENU_LAST_BRANCH:

      switch (items->type) {

      case MENU_RADIO_ITEM:
	menu_item = NULL;	/* Not Implemented */
	break;

      case MENU_CHECK_ITEM:
	menu_item = gtk_check_menu_item_new ();
	gtk_check_menu_item_set_show_toggle (
                                       GTK_CHECK_MENU_ITEM (menu_item), TRUE);
	break;

      default:
	menu_item = gtk_menu_item_new ();
	break;

      }

      label = gtk_widget_new (GTK_TYPE_ACCEL_LABEL,
			      "GtkWidget::visible", TRUE,
			      "GtkWidget::parent", menu_item,
			      "GtkAccelLabel::accel_widget", menu_item,
			      "GtkMisc::xalign", 0.0,
			      NULL);

      ac_key = gtk_label_parse_uline (GTK_LABEL (label), items->label);

      if (accel_group && ac_key != GDK_VoidSymbol) {
	if (GTK_IS_MENU_BAR (menu)) {
	  gtk_widget_add_accelerator (menu_item, "activate_item", accel_group,
				      ac_key, GDK_MOD1_MASK, GTK_ACCEL_LOCKED);
	}
	if (GTK_IS_MENU (menu)) {
	  gtk_widget_add_accelerator (menu_item, "activate_item",
                          gtk_menu_ensure_uline_accel_group (GTK_MENU (menu)),
                                                 ac_key, 0, GTK_ACCEL_LOCKED);
	}	
      }

      if (items->type != MENU_BRANCH && items->type != MENU_LAST_BRANCH) {
	if (items->callback) {
	  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
 	                 GTK_SIGNAL_FUNC (items->callback), items->user_data);
	}

	if (accel_group && items->accel_key) {
	  gtk_widget_add_accelerator (menu_item, "activate", accel_group,
                      items->accel_key, items->accel_mods, GTK_ACCEL_VISIBLE);
	}
      }
      else {	/* Branch */
	if (items->type == MENU_LAST_BRANCH)
	  gtk_menu_item_right_justify (GTK_MENU_ITEM (menu_item));

	gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item),
  			         create_menu (items->user_data, accel_group));
      }

      if (GTK_IS_MENU_BAR (menu))
	gtk_menu_bar_append (GTK_MENU_BAR (menu), menu_item);
      else 
	gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);

      if (items->widget)
	*items->widget = menu_item;

      break;

    case MENU_SEPARATOR:
      menu_item = gtk_menu_item_new ();
      gtk_widget_set_sensitive (menu_item, FALSE);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
      break;

    case MENU_TEAROFF:
      menu_item = gtk_tearoff_menu_item_new ();
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
      break;

    default:
      break;
    }

    items++;
  }
}


GtkWidget *create_menu (const struct menuitem *items, 
                                                 GtkAccelGroup *accel_group) {
  GtkWidget *menu;

  menu = gtk_menu_new ();

  create_menu_recursive (menu, items, accel_group);

  return menu;
}


GtkWidget *create_menubar (const struct menuitem *items, 
                                                 GtkAccelGroup *accel_group) {
  GtkWidget *menubar;
  menubar = gtk_menu_bar_new ();

  create_menu_recursive (menubar, items, accel_group);

  return menubar;
}

