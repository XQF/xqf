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

#ifndef __SRV_LIST_H__
#define __SRV_LIST_H__

#include <gtk/gtk.h>

#include "xqf.h"


extern	GSList *qw_colors_pixmap_cache;
extern	GSList *server_pixmap_cache;

extern	void assemble_server_address (char *buf, int size, struct server *s);

extern	void player_clist_set_server (struct server *s);
extern	void player_clist_redraw (void);

extern	void server_clist_sync_selection (void);
extern	int server_clist_refresh_server (struct server *s);

extern	void server_clist_select_one (int row);
extern	GSList *server_clist_selected_servers (void);
extern	GSList *server_clist_all_servers (void);

extern	void server_clist_selection_visible (void);

extern	void server_clist_show_hostname (struct host *h);
extern	void server_clist_redraw (void);

extern	void server_clist_set_list (GSList *servers);
extern	void server_clist_build_filtered (GSList *servers, int update);


#endif /* __SRV_LIST_H__ */
