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

#ifndef __MENUS_H__
#define __MENUS_H__


#include <gtk/gtk.h>


enum menuitem_type {
  MENU_END = 0,
  MENU_ITEM,
  MENU_CHECK_ITEM,
  MENU_RADIO_ITEM,
  MENU_SEPARATOR,
  MENU_TEAROFF,
  MENU_BRANCH,
  MENU_LAST_BRANCH
};

struct menuitem {
  enum menuitem_type type;

  const char *label;

  guint accel_key;
  GdkModifierType accel_mods;

  GtkSignalFunc	callback;
  gpointer user_data;

  GtkWidget **widget;
};


extern	GtkWidget *create_menu (const struct menuitem *items, 
                                                  GtkAccelGroup *accel_group);

extern	GtkWidget *create_menubar (const struct menuitem *items, 
                                                  GtkAccelGroup *accel_group);

#endif /* __MENUS_H__ */


