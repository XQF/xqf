/* XQF - Quake server browser and launcher
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
.*
.*
.* trayicon.c:
.*
 *  Copyright (C) Jochen Baier <email@jochen-baier.de>
.*
.* based on:
.*
.*   eggtrayicon.c api
.*
 *   Copyright (C) Anders Carlsson <andersca@gnu.org>
.*
.*   magic transparent KDE icon:
.*
.*   Copyright (C) Jochen Baier <email@jochen-baier.de>
.*
 */


#ifndef _system_tray_h_
#define _system_tray_h_

#include <gtk/gtk.h>

extern void tray_init (GtkWidget * window) ;
extern void tray_done (void);
extern void tray_delete_event_hook (void);
extern void tray_icon_set_tooltip  (gchar *tip);
extern gboolean tray_icon_work(void);
extern void tray_icon_stop_animation (void);
extern void tray_icon_start_animation (void);

#endif /*_system_tray_h_*/
