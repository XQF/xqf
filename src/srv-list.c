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
#include <sys/socket.h> /* inet_ntoa */
#include <netinet/in.h> /* inet_ntoa */
#include <arpa/inet.h>  /* inet_ntoa */
#include <string.h>     /* strncpy */

#include "xqf-ui.h"
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
#include "srv-prop.h"   /* pulp */
#include "country-filter.h"
#include "xqf-lists.h"
#include "xqf-server-item.h"
#include "xqf-player-item.h"


GSList *qw_colors_pixmap_cache = NULL;
GSList *server_pixmap_cache = NULL;


static int sync_selection_blocked = FALSE;


void assemble_server_address (char *buf, int size, const struct server *s) {
	if (show_default_port || games[s->type].default_port != s->port) {
		g_snprintf (buf, size, "%s:%d", (show_hostnames && s->host->name)?
				s->host->name : inet_ntoa (s->host->ip),
				s->port);
	}
	else {
		strncpy (buf, (show_hostnames && s->host->name)?
				s->host->name : inet_ntoa (s->host->ip), size - 1);
	}
}


/* ------------------------------------------------------------------ */
/* player_list_set_server — populate player_store from s->players     */
/* ------------------------------------------------------------------ */

void player_list_set_server (struct server *s) {
	GSList *plist;

	g_list_store_remove_all (player_store);

	if (!s || !s->players)
		return;

	for (plist = s->players; plist; plist = plist->next) {
		struct player *p = (struct player *) plist->data;
		XqfPlayerItem *item = xqf_player_item_new (p, s);
		g_list_store_append (player_store, item);
		g_object_unref (item);
	}
}


void player_list_redraw (void) {
	if (!cur_server)
		return;
	/* Re-populate player store to force factory rebind. */
	player_list_set_server (cur_server);
}


/* ------------------------------------------------------------------ */
/* server_list_sync_selection — read GtkMultiSelection, update state  */
/* ------------------------------------------------------------------ */

void server_list_sync_selection (void) {
	if (!server_selection || sync_selection_blocked)
		return;

	debug (7, "server_list_sync_selection() --");

	GtkBitset *sel = gtk_selection_model_get_selection (server_selection);
	guint n_sel = gtk_bitset_get_size (sel);

	if (cur_server) {
		debug (7, "server_list_sync_selection() -- unref server %lx", cur_server);
		server_unref (cur_server);
		cur_server = NULL;
	}

	if (n_sel == 1) {
		GtkBitsetIter iter;
		guint pos;
		gtk_bitset_iter_init_first (&iter, sel, &pos);
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_selection), pos));
		cur_server = xqf_server_item_get (item);
		server_ref (cur_server);
		g_object_unref (item);
	}

	gtk_bitset_unref (sel);

	player_list_set_server (cur_server);
	srvinf_treeview_set_server (cur_server);
	set_widgets_sensitivity (builder);
}


/* ------------------------------------------------------------------ */
/* server_list_refresh_server — add / update / remove one server      */
/* ------------------------------------------------------------------ */

/* Search server_store (unsorted) for a matching server pointer. */
static guint
server_store_find_in_store (struct server *s)
{
	guint n = g_list_model_get_n_items (G_LIST_MODEL (server_store));
	for (guint i = 0; i < n; i++) {
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_store), i));
		struct server *ss = xqf_server_item_get (item);
		g_object_unref (item);
		if (ss == s) return i;
	}
	return G_MAXUINT;
}

/* Force GtkColumnView to rebind the item at position pos in server_store. */
static void
server_store_rebind (guint pos)
{
	XqfServerItem *old = XQF_SERVER_ITEM (
		g_list_model_get_item (G_LIST_MODEL (server_store), pos));
	struct server *s = xqf_server_item_get (old);
	g_object_unref (old);
	XqfServerItem *fresh = xqf_server_item_new (s);
	gpointer add[1] = { fresh };
	g_list_store_splice (server_store, pos, 1, add, 1);
	g_object_unref (fresh);
}

int server_list_refresh_server (struct server *s) {
	debug (6, "server_list_refresh_server() -- Server %lx", s);

	apply_filters (cur_filter | FILTER_PLAYER_MASK, s);

	guint pos = server_store_find_in_store (s);

	if (pos != G_MAXUINT) {
		debug (6, "server_list_refresh_server() -- Server %lx is at store pos %u", s, pos);
		if (default_refresh_sorts && (s->filters & cur_filter) != cur_filter) {
			g_list_store_remove (server_store, pos);
			return FALSE;
		} else {
			server_store_rebind (pos);
			return TRUE;
		}
	} else {
		if ((s->filters & cur_filter) == cur_filter) {
			debug (6, "server_list_refresh_server() -- Server %lx needs to be added.", s);
			XqfServerItem *item = xqf_server_item_new (s);
			g_list_store_append (server_store, item);
			g_object_unref (item);
			return TRUE;
		}
	}

	return FALSE;
}


/* ------------------------------------------------------------------ */
/* server_list_select_one — select item at sorted position row        */
/* ------------------------------------------------------------------ */

void server_list_select_one (int row) {
	if (!server_selection || row < 0)
		return;
	debug (7, "server_list_select_one() -- Row %d", row);
	gtk_selection_model_select_item (server_selection, (guint) row, TRUE);
}


/* ------------------------------------------------------------------ */
/* Enumerate selected / all servers                                     */
/* ------------------------------------------------------------------ */

GSList *server_list_selected_servers (void) {
	GSList *list = NULL;
	if (!server_selection) return NULL;

	GtkBitset *sel = gtk_selection_model_get_selection (server_selection);
	GtkBitsetIter iter;
	guint pos;
	gboolean has = gtk_bitset_iter_init_first (&iter, sel, &pos);
	while (has) {
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_selection), pos));
		list = server_list_prepend (list, xqf_server_item_get (item));
		g_object_unref (item);
		has = gtk_bitset_iter_next (&iter, &pos);
	}
	gtk_bitset_unref (sel);
	return g_slist_reverse (list);
}

GSList *server_list_get_n_servers (int amount) {
	GSList *list = NULL;
	if (!server_store) return NULL;
	guint n = g_list_model_get_n_items (G_LIST_MODEL (server_store));
	for (guint i = 0; i < n && (int) i < amount; i++) {
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_store), i));
		list = server_list_prepend (list, xqf_server_item_get (item));
		g_object_unref (item);
	}
	return g_slist_reverse (list);
}

GSList *server_list_all_servers (void) {
	GSList *list = NULL;
	if (!server_store) return NULL;
	guint n = g_list_model_get_n_items (G_LIST_MODEL (server_store));
	debug (6, "start");
	for (guint i = 0; i < n; i++) {
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_store), i));
		list = server_list_prepend (list, xqf_server_item_get (item));
		g_object_unref (item);
	}
	debug (6, "Return list %lx", list);
	return g_slist_reverse (list);
}


/* ------------------------------------------------------------------ */
/* server_list_selection_visible — scroll to show selection           */
/* ------------------------------------------------------------------ */

void server_list_selection_visible (void) {
	/* TODO: use gtk_column_view_scroll_to() (GTK 4.12+) */
}


/* ------------------------------------------------------------------ */
/* server_list_show_hostname / server_list_redraw                    */
/* Force GtkColumnView to rebind updated rows.                         */
/* ------------------------------------------------------------------ */

void server_list_show_hostname (struct host *h) {
	if (!h->name || !server_store) return;
	guint n = g_list_model_get_n_items (G_LIST_MODEL (server_store));
	for (guint i = 0; i < n; i++) {
		XqfServerItem *item = XQF_SERVER_ITEM (
			g_list_model_get_item (G_LIST_MODEL (server_store), i));
		struct server *s = xqf_server_item_get (item);
		gboolean match = (s->host == h);
		g_object_unref (item);
		if (match)
			server_store_rebind (i);
	}
}

void server_list_redraw (void) {
	if (!server_store) return;
	debug (7, "server_list_redraw() --");
	guint n = g_list_model_get_n_items (G_LIST_MODEL (server_store));
	for (guint i = 0; i < n; i++)
		server_store_rebind (i);
}


/* ------------------------------------------------------------------ */
/* server_list_set_list — clear and repopulate from filtered list     */
/* ------------------------------------------------------------------ */

void server_list_set_list (GSList *servers) {
	GSList *filtered;

	debug (7, "server_list_set_list() -- list %lx", servers);

	filtered = build_filtered_list (cur_filter, servers);

	g_list_store_remove_all (server_store);

	for (GSList *l = filtered; l; l = l->next) {
		struct server *s = (struct server *) l->data;
		XqfServerItem *item = xqf_server_item_new (s);
		g_list_store_append (server_store, item);
		g_object_unref (item);
	}

	server_list_free (filtered);
	pixmap_cache_clear (&server_pixmap_cache, 8);

	server_list_sync_selection ();
	debug (7, "server_list_set_list() -- Done.");
}


/* ------------------------------------------------------------------ */
/* server_list_build_filtered — rebuild list, restore selection       */
/* ------------------------------------------------------------------ */

void server_list_build_filtered (GSList *server_list, int update) {
	(void) update;
	debug (3, "server_list_build_filtered()");

	struct server *saved = cur_server;
	if (saved) server_ref (saved);

	server_list_set_list (server_list);

	if (saved) {
		guint pos = server_store_find (saved);
		server_unref (saved);
		if (pos != G_MAXUINT)
			server_list_select_one ((int) pos);
	}

	server_list_selection_visible ();
}
