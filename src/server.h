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

#ifndef __SERVER_H__
#define __SERVER_H__

#include <glib.h>

#include "xqf.h"


extern	struct server *server_add (struct host *h, unsigned short port, 
                                                       enum server_type type);
extern	struct userver *userver_add (const char *name, unsigned short port,
                                                       enum server_type type);
extern	void server_free_info (struct server *s);

extern	void server_unref (struct server *s);
extern	void userver_unref (struct userver *s);

extern	GSList *server_list_copy (GSList *list);
extern	GSList *userver_list_copy (GSList *list);

extern	GSList *server_list_append_list (GSList *list, GSList *servers,
                                                       enum server_type type);
extern	GSList *userver_list_append_list (GSList *list, GSList *uservers,
                                                       enum server_type type);

extern	int servers_total (void);
extern	int uservers_total (void);
extern	GSList *all_servers (void);

extern	int parse_address (char *str, char **addr, unsigned short *port);

extern	struct server *userver_set_host (struct userver *s, struct host *h);
extern	void uservers_to_servers (GSList **uservers, GSList **servers);

extern	void server_lists_intersect (GSList **list1, GSList **list2);

extern	void server_list_fprintf (FILE *f, GSList *servers);
extern	void userver_list_fprintf (FILE *f, GSList *uservers);


static inline void server_ref (struct server *s) {
  if (s) s->ref_count++;
}

static inline void userver_ref (struct userver *us) {
  if (us) us->ref_count++;
}

static inline GSList *server_list_prepend (GSList *list, struct server *s) {
  if (g_slist_find (list, s) == NULL) {
    list = g_slist_prepend (list, s);
    server_ref (s);
  }
  return list;
}

static inline GSList *server_list_append (GSList *list, struct server *s) {
  if (g_slist_find (list, s) == NULL) {
    list = g_slist_append (list, s);
    server_ref (s);
  }
  return list;
}

static inline GSList *server_list_remove (GSList *list, struct server *s) {
  if (g_slist_find (list, s)) {
    list = g_slist_remove (list, s);
    server_unref (s);
  }
  return list;
}

static inline GSList *userver_list_add (GSList *list, struct userver *us) {
  if (g_slist_find (list, us) == NULL) {
    list = g_slist_prepend (list, us);
    userver_ref (us);
  }
  return list;
}

static inline GSList *userver_list_remove (GSList *list, struct userver *us) {
  if (g_slist_find (list, us)) {
    list = g_slist_remove (list, us);
    userver_unref (us);
  }
  return list;
}

static inline void server_list_free (GSList *list) {
  if (list) {
    g_slist_foreach (list, (GFunc) server_unref, NULL);
    g_slist_free (list);
  }
}

static inline void userver_list_free (GSList *list) {
  if (list) {
    g_slist_foreach (list, (GFunc) userver_unref, NULL);
    g_slist_free (list);
  }
}


#endif /* __SERVER_H__ */
