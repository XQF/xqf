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
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */
#include <string.h>	/* strncpy */

#include <gtk/gtk.h>

#include "xqf.h"
#include "game.h"
#include "sort.h"
#include "skin.h"
#include "filter.h"
#include "utils.h"
#include "pref.h"
#include "host.h"
#include "server.h"
#include "source.h"
#include "stat.h"
#include "psearch.h"
#include "dialogs.h"
#include "pixmaps.h"
#include "srv-info.h"
#include "srv-list.h"


GSList *qw_colors_pixmap_cache = NULL;
GSList *server_pixmap_cache = NULL;


static int sync_selection_blocked = FALSE;


static void get_server_pixmap (GtkWidget *window, struct server *s,
                          GSList **cache, GdkPixmap **pix, GdkBitmap **mask) {
  unsigned key;

  if (!s || !window || !buddy_pix[1].pix || !q_pix.pix) {
    *pix = NULL;
    *mask = NULL;
    return;
  }

  key = ((s->flags & PLAYER_GROUP_MASK) << 8) + s->type;

  if (cache) {
    pixmap_cache_lookup (*cache, pix, mask, key);
    if (*pix)
      return;
  }

  create_server_pixmap (window, games[s->type].pix, 
                                     s->flags & PLAYER_GROUP_MASK, pix, mask);
  if (cache)
    pixmap_cache_add (cache, *pix, *mask, key);
}


void assemble_server_address (char *buf, int size, struct server *s) {
  if (show_default_port || games[s->type].default_port != s->port) {
    g_snprintf (buf, size, "%s:%d", (show_hostnames && s->host->name)? 
                                    s->host->name : inet_ntoa (s->host->ip), 
                                                                     s->port);
  }
  else {
    strncpy (buf, (show_hostnames && s->host->name)? 
                               s->host->name : inet_ntoa (s->host->ip), size);
  }
}


static int server_clist_refresh_row (struct server *s, int row) {
  GdkPixmap *server_pixmap;
  GdkBitmap *server_pixmask;
  char *text[9];
  char buf1[256], buf2[32], buf3[32], buf4[32];
  char *retries;
  struct pixmap *retries_pix = NULL;
  int col;

  text[0] = NULL;

  assemble_server_address (buf1, 256, s);
  text[1] = buf1;

  if (s->ping >= 0) {
    g_snprintf (buf2, 32, "%d", (s->ping > MAX_PING)? MAX_PING : s->ping);
    text[2] = buf2;
  }
  else {
    text[2] = "n/a";
  }

  retries_pix = &server_status[0];

  if (s->retries >= 0) {
    if (s->ping == MAX_PING + 1) {
      retries = "D";	/* DOWN */
      retries_pix = &server_status[2];
    }
    else if (s->ping == MAX_PING) {
      retries = "T";	/* TIMEOUT */
      retries_pix = &server_status[3];
    }
    else {
      g_snprintf (buf3, 32, "%d", s->retries);	/* UP */
      retries = buf3;
      retries_pix = &server_status[1];
    }
  }
  else {
    retries = "?";
    retries_pix = &server_status[0];
  }

  if (xqf_start_time > s->refreshed)
    retries_pix = &server_status[0];

  text[3] = text[4] = NULL;

  g_snprintf (buf4, 32, "%d/%d", s->curplayers, s->maxplayers);
  text[5] = (!s->curplayers)? buf4 : NULL;

  text[6] = (s->map) ?  s->map : NULL;
  text[7] = (s->game)? s->game : NULL;
  text[8] = (s->gametype) ? s->gametype : NULL;

  if (row < 0) {
    row = gtk_clist_append (server_clist, text);
  }
  else {
    for (col = 1; col < 8; col++) {
      gtk_clist_set_text (server_clist, row, col, text[col]);
    }
  }

  gtk_clist_set_pixtext (server_clist, row, 3, retries, 2,
                                         retries_pix->pix, retries_pix->mask);
#if 0
  if (s->curplayers) {
    gtk_clist_set_pixtext (server_clist, row, 4, buf4, 2,
     (s->curplayers >= s->maxplayers)? man_red_pix.pix : man_black_pix.pix,
     (s->curplayers >= s->maxplayers)? man_red_pix.mask : man_black_pix.mask);
  }
#endif
  if (s->curplayers >= s->maxplayers)
    gtk_clist_set_pixtext (server_clist, row, 5, buf4, 2,
                           man_red_pix.pix, man_red_pix.mask );

  else if ( (s->curplayers + s->private_client ) >= s->maxplayers)
    gtk_clist_set_pixtext (server_clist, row, 5, buf4, 2,
                           man_yellow_pix.pix, man_yellow_pix.mask );
  
  else if (s->curplayers)
    gtk_clist_set_pixtext (server_clist, row, 5, buf4, 2,
                           man_black_pix.pix, man_black_pix.mask );
  
    

  /* Show if the server is private or not */
  if (s->flags & SERVER_PASSWORD ) {
    gtk_clist_set_pixtext (server_clist, row, 4, "", 0,
                           locked_pix.pix, locked_pix.mask );
  } else {
    gtk_clist_set_text (server_clist, row, 4, "" );
  }

  get_server_pixmap (main_window, s, &server_pixmap_cache, &server_pixmap, 
                                                             &server_pixmask);
  gtk_clist_set_pixtext (server_clist, row, 0, 
                   (s->name)? s->name : "", 2, server_pixmap, server_pixmask);

  gdk_pixmap_unref (server_pixmap);
  gdk_bitmap_unref (server_pixmask);

  return row;
}


static int player_clist_refresh_row (struct server *s, struct player *p, 
                                                                    int row) {
  GdkPixmap *qw_colors_pixmap = NULL;
  char *text[6];
  char buf1[32], buf2[32], buf3[32], buf4[32];
  char skin_buf[128];
  int col;

  if ((games[s->type].flags & GAME_QUAKE1_PLAYER_COLORS) != 0)
    allocate_quake_player_colors (main_window->window);

  text[0] = text[1] = text[2] = text[3] = text[4] = text[5] = NULL;

  if (p->name && (p->flags & PLAYER_GROUP_MASK) == 0)
    text[0] = p->name;

  g_snprintf (buf1, 32, "%d", p->frags);
  text[1] = buf1;

  if (p->model && p->skin) {
    g_snprintf (skin_buf, 128, "%s/%s", p->model, p->skin);
    text[3] = skin_buf;
  }
  else {
    if (p->model)
      text[3] = p->model;
    if (p->skin)
      text[3] = p->skin;
  }

  if (p->ping >= 0) {
    g_snprintf (buf3, 32, "%d", p->ping);
    text[4] = buf3;
  }

  if (p->time >= 0) {
    g_snprintf (buf4, 32, "%02d:%02d", p->time / 60 / 60, p->time / 60 % 60);
    text[5] = buf4;
  }

  if (row < 0) {
    row = gtk_clist_append (player_clist, text);
  }
  else {
    for (col = 1; col < 7; col++) {
      gtk_clist_set_text (player_clist, row, col, text[col]);
    }
  }

  if ((games[s->type].flags & GAME_QUAKE1_PLAYER_COLORS) != 0) {
    qw_colors_pixmap = qw_colors_pixmap_create (main_window, 
                                 p->shirt, p->pants, &qw_colors_pixmap_cache);

    g_snprintf (buf2, 32, "%d:%d", p->shirt, p->pants);
    gtk_clist_set_pixtext (player_clist, row, 2, buf2, 2, 
                                                      qw_colors_pixmap, NULL);
    gdk_pixmap_unref (qw_colors_pixmap);
  }

  if ((p->flags & PLAYER_GROUP_MASK) == 0) {
    gtk_clist_set_text (player_clist, row, 0, (p->name)? p->name : "");
    gtk_clist_set_shift (player_clist, row, 0, 0, 
                                         pixmap_width (buddy_pix[1].pix) + 2);
  }
  else {
    ensure_buddy_pix (main_window, p->flags & PLAYER_GROUP_MASK);
    gtk_clist_set_shift (player_clist, row, 0, 0, 0);
    gtk_clist_set_pixtext (player_clist, row, 0, 
                                (p->name)? p->name : "", 2,
                                buddy_pix[p->flags & PLAYER_GROUP_MASK].pix, 
			        buddy_pix[p->flags & PLAYER_GROUP_MASK].mask);
  }

  return row;
}


void player_clist_set_server (struct server *s) {
  GSList *plist;
  struct player *p;
  int row;

  if (!s || !s->players) {
    if (player_clist->rows > 0)
      gtk_clist_clear (player_clist);
    return;
  }

  gtk_clist_freeze (player_clist);
  gtk_clist_clear (player_clist);

  for (plist = s->players; plist; plist = plist->next) {
    p = (struct player *) plist->data;
    row = player_clist_refresh_row (s, p, -1);
    gtk_clist_set_row_data (player_clist, row, p);
  }

  gtk_clist_sort (player_clist);
  gtk_clist_thaw (player_clist);

  pixmap_cache_clear (&qw_colors_pixmap_cache, 10);
}


void player_clist_redraw (void) {
  struct player *p;
  int i;

#ifdef DEBUG
  fprintf (stderr, "player_clist_redraw()\n");
#endif

  if (!cur_server || player_clist->rows == 0)
    return;

  gtk_clist_freeze (player_clist);

  for (i = 0; i < player_clist->rows; i++) {
    p = (struct player *) gtk_clist_get_row_data (player_clist, i);
    player_clist_refresh_row (cur_server, p, i);
  }

  gtk_clist_thaw (player_clist);

  pixmap_cache_clear (&qw_colors_pixmap_cache, 10);
}


void server_clist_sync_selection (void) {
  GList *selection;

  if (!sync_selection_blocked) {
    selection = server_clist->selection;

    if (cur_server) {
      server_unref (cur_server);
      cur_server = NULL;
    }

    if (selection && !selection->next) {
      cur_server = (struct server *) gtk_clist_get_row_data (
                                         server_clist, (int) selection->data);
      server_ref (cur_server);
    }

    player_clist_set_server (cur_server);
    srvinf_ctree_set_server (cur_server);

    set_widgets_sensitivity ();
  }
}


int server_clist_refresh_server (struct server *s) {
  int row;

  debug (6, "server_clist_refresh_server() -- Server %lx", s );

  apply_filters (cur_filter | FILTER_PLAYER_MASK, s);

  row = gtk_clist_find_row_from_data (server_clist, s);

  if (row >= 0) {
    debug (6, "server_clist_refresh_server() -- Server %lx is at row %d", s, row );

    if (default_refresh_sorts && (s->filters & cur_filter) != cur_filter) {
      gtk_clist_remove (server_clist, row);
      return FALSE;
    }
    else {
      server_clist_refresh_row (s, row);
      return TRUE;
    }

  }
  else {
    if ((s->filters & cur_filter) == cur_filter) {
      row = server_clist_refresh_row (s, -1);
      gtk_clist_set_row_data_full (server_clist, row, s, 
                                             (GtkDestroyNotify) server_unref);
      server_ref (s);
      return TRUE;
    }
  }

  return FALSE;
}


void server_clist_select_one (int row) {
  sync_selection_blocked = TRUE;
  gtk_clist_unselect_all (server_clist);
  sync_selection_blocked = FALSE;

  gtk_clist_select_row (server_clist, row, -1);
}


GSList *server_clist_selected_servers (void) {
  GList *rows = server_clist->selection;
  GSList *list = NULL;
  struct server *s;

  debug (6, "server_clist_selected_servers() --");
  while (rows) {
    s = (struct server *) gtk_clist_get_row_data (
                                              server_clist, (int) rows->data);
    list = server_list_prepend (list, s);
    rows = rows->next;
  }

  return list;
}

/*
  server_clist_all_servers -- Return all servers that are in the server 
  clist widget. It returns a new list.  Note that the prepend function
  adds one to the reference count.
*/
GSList *server_clist_all_servers (void) {
  GSList *list = NULL;
  struct server *server;
  int row;

  debug (6, "server_clist_all_servers() -- ");
  for (row = 0; row < server_clist->rows; row++) {
    server = (struct server *) gtk_clist_get_row_data (server_clist, row);
    list = server_list_prepend (list, server);
  }
  debug (7, "server_clist_all_servers() -- Return list %lx", list);
  return g_slist_reverse (list);
}


void server_clist_selection_visible (void) {
  GList *rows = server_clist->selection;
  GList *row;
  GtkVisibility vis;
  int min;

  if (!rows)
    return;

  for (row = rows; row; row = row->next) {
    vis = gtk_clist_row_is_visible (server_clist, (int) row->data);
    if (vis == GTK_VISIBILITY_FULL)
      return;
  }

  min = (int) rows->data;
  for (row = rows->next; row; row = row->next) {
    if (min > (int) row->data)
      min = (int) row->data;
  }

  if (rows->next)	/* if (g_list_length (rows) > 1) */
    gtk_clist_moveto (server_clist, min, 0, 0.2, 0.0);
  else 
    gtk_clist_moveto (server_clist, min, 0, 0.5, 0.0);
}


void server_clist_show_hostname (struct host *h) {
  struct server *s;
  char buf[256];
  int row;

  if (!h->name)
    return;

  /* Freezing and sorting should be done in upper-level function */

  for (row = 0; row < server_clist->rows; row++) {
    s = (struct server *) gtk_clist_get_row_data (server_clist, row);
    if (s->host == h) {
      assemble_server_address (buf, 256, s);
      gtk_clist_set_text (server_clist, row, 1, buf);
    }
  }
}


void server_clist_redraw (void) {
  struct server *s;
  char buf[256];
  int row;

  gtk_clist_freeze (server_clist);

  for (row = 0; row < server_clist->rows; row++) {
    s = (struct server *) gtk_clist_get_row_data (server_clist, row);
    assemble_server_address (buf, 256, s);
    gtk_clist_set_text (server_clist, row, 1, buf);
  }

  if (server_clist->sort_column == SORT_SERVER_ADDRESS)
    gtk_clist_sort (server_clist);

  gtk_clist_thaw (server_clist);
}


void server_clist_set_list (GSList *servers) {
  GSList *list;
  struct server *server;
  int row;
  GSList *filtered;

  filtered = build_filtered_list (cur_filter, servers);

  gtk_clist_freeze (server_clist);
  gtk_clist_clear (server_clist);

  if (filtered) {

    for (list = filtered; list; list = list->next) {
      server = (struct server *) list->data;
      row = server_clist_refresh_row (server, -1);
      gtk_clist_set_row_data_full (server_clist, row, server, 
				   (GtkDestroyNotify) server_unref);
      /*
	Because the build_filtered_list add a reference count
	 to the server structure and because we are going
	 to free that list, we DO NOT need to add a reference 
	 count to the server structure.
      */
      /* server_ref (server); */
    }

    server_list_free (filtered);

    gtk_clist_sort (server_clist);
  }

  gtk_clist_thaw (server_clist);

  pixmap_cache_clear (&server_pixmap_cache, 8);

  server_clist_sync_selection ();
}


void server_clist_build_filtered (GSList *server_list, int update) {
  /* This gets called whenever a user clicks the filter button */

  GSList *delete;
  GSList *add;
  GSList *tmp;
  struct server *server;
  int row;

  debug (6, "server_clist_build_filterd() -- Update? %d", update);
  delete = server_clist_all_servers ();
  add = build_filtered_list (cur_filter, server_list); /* in filter.c */

  server_lists_intersect (&delete, &add);

  gtk_clist_freeze (server_clist);

  /*
    If there are entries in the server clist, we want to first remove
    them.  Be sure to decrement the reference count as we will
    be adding them back in later.
  */
  if (delete) {
    debug (7, "server_clist_build_filtered() -- Got Delete %lx", delete);
    for (tmp = delete; tmp; tmp = tmp->next) {
      server = (struct server *) tmp->data;
      row = gtk_clist_find_row_from_data (server_clist, server);
      if (row >= 0) {
	gtk_clist_remove (server_clist, row);
	debug (3, "server_clist_build_filtered() -- Delete server %lx", server);
      }
      /* 
	 Allways unref the server because the list returned
	 to us had the count incremented by prepend.
      */
    }
    server_list_free (delete);
  }


  if (update) {
    for (row = 0; row < server_clist->rows; row++) {
      server = (struct server *) gtk_clist_get_row_data (server_clist, row);
      server_clist_refresh_row (server, row);
    }
  }

  /* 
     Now we want to add all of the servers that we
     got from the build_fiter_list function. Note that
     the said function adds one to the reference count
     for us and since we free that list after we put
     the data in the clist, we do NOT need to 
     do a reference count in this fuction. --baa
  */
  if (add) {
    for (tmp = add; tmp; tmp = tmp->next) {
      server = (struct server *) tmp->data;
      row = server_clist_refresh_row (server, -1);
      gtk_clist_set_row_data_full (server_clist, row, server, 
				   (GtkDestroyNotify) server_unref);
      /*server_ref (server);*/
    }
    server_list_free (add);
  }

  // prevent segfault on some systems if server list is empty due to a player
  // filter that finds no servers that match by clearing the list if rows is 0.
  // Fix by 'slashdev'
  if(server_clist->rows==0)
    gtk_clist_clear(server_clist);

  gtk_clist_sort (server_clist);
  server_clist_selection_visible ();

  gtk_clist_thaw (server_clist);
}

