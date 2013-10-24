/* XQF - Quake server browser and launcher
 * Functions for plugin script handing
 * Copyright (C) 2005 Ludwig Nussel <l-n@users.sourceforge.net>
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


#ifndef XQF_SCRIPTS_H
#define XQF_SCRIPTS_H

#include <gtk/gtk.h>

void scripts_add_dir(const char* dir);

void scripts_load();

GtkWidget *scripts_config_page();

gboolean check_script_prefs();

void save_script_prefs();

void script_action_start();
void script_action_quit();
void script_action_gamestart(struct game* g, struct server* s);
void script_action_gamequit(struct game* g, struct server* s);

#endif
