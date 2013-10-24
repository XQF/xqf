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

#ifndef __HOST_H__
#define __HOST_H__

#include <gtk/gtk.h>


#define  HOST_CACHE_MAX_AGE	(7 * 24 * 60 * 60)	/* one week */
#define  HOST_CACHE_MAX_SIZE	5000


extern	int hosts_total (void);
extern	GSList *all_hosts (void);
extern	struct host *host_add (const char *address);
extern  struct host *host_add_in (struct in_addr addr);
extern	void host_unref (struct host *h);

extern	GSList *merge_host_to_resolve  (GSList *hosts, struct host *h);
extern	GSList *merge_hosts_to_resolve (GSList *hosts, GSList *servers);

extern	void host_cache_save (void);
extern	void host_cache_load (void);
extern	void host_cache_clear (void);

static inline void host_ref (struct host *h) {
  if (h) {
    h->ref_count++;
  }
}

static inline GSList *host_list_add (GSList *list, struct host *h) {
  if (g_slist_find (list, h) == NULL) {
    list = g_slist_prepend (list, h);
    host_ref (h);
  }
  return list;
}

static inline GSList *host_list_remove (GSList *list, struct host *h) {
  if (g_slist_find (list, h)) {
    list = g_slist_remove (list, h);
    host_unref (h);
  }
  return list;
}

static inline void host_list_free (GSList *list) {
  if (list) {
    g_slist_foreach (list, (GFunc) host_unref, NULL);
    g_slist_free (list);
  }
}


#endif /* __HOST_H__ */
