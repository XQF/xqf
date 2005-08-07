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
#include <sys/socket.h>	/* inet_aton, inet_ntoa */
#include <netinet/in.h> /* inet_aton, inet_ntoa */
#include <arpa/inet.h>	/* inet_aton, inet_ntoa */
#include <time.h>	/* time */
#include <stdlib.h>	/* strtoul */

#include <gtk/gtk.h>

#include "xqf.h"
#include "pref.h"
#include "utils.h"
#include "host.h"
#include "zipped.h"


struct host_hash {
  int num;
  GSList **nodes;
};

static struct host_hash hosts = { 251, NULL };

static GSList *host_cache = NULL;


static int host_hash_func (const struct in_addr *ip) {
  unsigned char *ptr;

  if (!ip)
    return 0;

  ptr = (char *) &ip->s_addr;
  return (ptr[0] + (ptr[1] << 2) + (ptr[2] << 4) + (ptr[3] << 6)) % hosts.num;
}

static gint host_sorting_helper (const struct host *h1, 
				      const struct host *h2) {
  if(h1==h2)
    return 0;
  else if(h1<h2)
    return -1;
  else
    return 1;
}


int hosts_total (void) {
  int i;
  int total = 0;

  if (!hosts.nodes)
    return 0;

  for (i = 0; i < hosts.num; i++) {
    total += g_slist_length (hosts.nodes[i]);
  }
  return total;
}


GSList *all_hosts (void) {
  GSList *list = NULL;
  GSList *tmp;
  struct host *h;
  int i;

  if (!hosts.nodes)
    return NULL;

  for (i = 0; i < hosts.num; i++) {
    for (tmp = hosts.nodes[i]; tmp; tmp = tmp->next) {
      h = (struct host *) tmp->data;
      list = g_slist_prepend (list, h);
      host_ref (h);
    }
  }

  return list;
}


struct host *host_add (const char *address) {
  struct in_addr addr;

  if (!address || !*address || !inet_aton (address, &addr))
    return NULL;

  return host_add_in(addr);
}

struct host *host_add_in (struct in_addr addr) {
  struct host *h;
  GSList *ptr;
  int node;
  int i;

  node = host_hash_func (&addr);

  if (!hosts.nodes) {
    hosts.nodes = g_malloc (sizeof (GSList *) * hosts.num);
    for (i = 0; i < hosts.num; i++)
      hosts.nodes[i] = NULL;
  }
  else {
    for (ptr = hosts.nodes[node]; ptr; ptr = ptr->next) {
      h = (struct host *) ptr->data;
      if (h->ip.s_addr == addr.s_addr)
	return h;
    }
  }

  h = g_malloc0 (sizeof (struct host));
  h->ip = addr;

  hosts.nodes[node] = g_slist_prepend (hosts.nodes[node], h);
  return h;
}


void host_unref (struct host *h) {
  int node;

  if (!h || !hosts.nodes)
    return;

  h->ref_count--;

  if (h->ref_count <= 0) {
    node = host_hash_func (&h->ip);
    hosts.nodes[node] = g_slist_remove (hosts.nodes[node], h);

    if (h->name) g_free (h->name);
    g_free (h);
  }
}

/*
GSList *merge_host_to_resolve (GSList *hosts, struct host *h) {

  if (h) {
    if (h->refreshed == 0 || h->refreshed >= time (NULL) + HOST_CACHE_MAX_AGE)
      hosts = host_list_add (hosts, h);
  }

  return hosts;
}
*/

GSList *merge_hosts_to_resolve (GSList *hosts, GSList *servers) {
  struct host *h;
  time_t curtime = time (NULL);

  for (; servers; servers = servers->next) {
    h = ((struct server *) servers->data)->host;
    if (h->refreshed == 0 || h->refreshed >= curtime + HOST_CACHE_MAX_AGE)
    {
//      hosts = host_list_add (hosts, h);
      hosts = g_slist_prepend (hosts, h);
      host_ref(h);
    }
  }
  
  hosts = slist_sort_remove_dups(hosts,(GCompareFunc)host_sorting_helper,(void(*)(void*))host_unref);

  return hosts;
}


static void host_cache_save_list (FILE *f, GSList *hosts) {
  struct host *h;

  while (hosts) {
    h = (struct host *) hosts->data;

    if (h->refreshed && h->name)
      fprintf (f, "%lu %s %s\n", h->refreshed, inet_ntoa (h->ip), h->name);

    hosts = hosts->next;
  }
}

static GSList *host_cache_read_list (FILE *f) {
  GSList *hosts = NULL;
  char buf[1024];
  char *token[4];
  int n;
  struct host *h;
  time_t refreshed;
  char *endptr;

  while (fgets (buf, 1024, f)) {
    n = tokenize (buf, token, 4, " \t\n\r");
    if (n == 3) {
      refreshed = strtoul (token[0], &endptr, 10);
      if (endptr == token[0])	/* It's not a number */
	continue;

      h = host_add (token[1]);

      if (h) {
	if (h->name)
	  g_free (h->name);

	h->name = g_strdup (token[2]);
	h->refreshed = refreshed;

//	hosts = host_list_add (hosts, h);
	hosts = g_slist_prepend (hosts, h);
	host_ref(h);
      }
    }
  }

  hosts = slist_sort_remove_dups(hosts,(GCompareFunc)host_sorting_helper,(void(*)(void*))host_unref);

  return hosts;
}


static int host_cache_sorting_helper (const struct host *h1, 
				      const struct host *h2) {
  return (h1->refreshed > h2->refreshed)? -1 : 1;
}


static GSList *hosts_to_cache (void) {
  GSList *hosts = NULL;
  GSList *allhosts;
  GSList *tmp;
  struct host *h;
  time_t curtime = time (NULL);

  allhosts = all_hosts ();

  for (tmp = allhosts; tmp; tmp = tmp->next) {
    h = (struct host *) tmp->data;
    if (h->name && h->refreshed && 
                              h->refreshed < curtime + HOST_CACHE_MAX_AGE*2) {
      hosts = g_slist_prepend (hosts, h);
      host_ref (h);
    }
  }

  host_list_free (allhosts);

  hosts = g_slist_sort (hosts, (GCompareFunc) host_cache_sorting_helper);

  if (g_slist_length (hosts) > HOST_CACHE_MAX_SIZE) {
    tmp = g_slist_nth (hosts, HOST_CACHE_MAX_SIZE - 1);
    host_list_free (tmp->next);
    tmp->next = NULL;
  }

  return hosts;
}


void host_cache_save (void) {
  GSList *hosts;
  char *hostcache;
  struct zstream z;

  hosts = hosts_to_cache ();
  host_cache_clear ();
  host_cache = hosts;

  hostcache = file_in_dir (user_rcdir, HOSTCACHE_FILE);

  if (host_cache) {
    zstream_open_w (&z, hostcache);
    if (z.f) {
      host_cache_save_list (z.f, host_cache);
      zstream_close (&z);
    }
  }
  else {
    zstream_unlink (hostcache);
  }

  g_free (hostcache);
}


void host_cache_load (void) {
  char *hostcache;
  struct zstream z;

  hostcache = file_in_dir (user_rcdir, HOSTCACHE_FILE);
  zstream_open_r (&z, hostcache);

  if (z.f) {
    host_cache_clear ();
    host_cache = host_cache_read_list (z.f);
    zstream_close (&z);
  }

  g_free (hostcache);
}


void host_cache_clear (void) {
  if (host_cache) {
    host_list_free (host_cache);
    host_cache = NULL;
  }
}

