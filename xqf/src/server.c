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
#include <stdio.h>	/* fprintf */
#include <string.h>	/* strchr, strncpy */
#include <stdlib.h>	/* strtol */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */

#include <glib.h>

#include "xqf.h"
#include "game.h"
#include "host.h"
#include "utils.h"
#include "server.h"


struct server_hash {
  int num;
  GSList **nodes;
};

static struct server_hash servers = { 251, NULL };
static struct server_hash uservers = { 17, NULL };


static int server_hash_func (const struct host *h, unsigned short port) {
  unsigned char *ptr;

  if (!h)
    return 0;

  ptr = (char *) &h->ip.s_addr;
  return (ptr[0] + (ptr[1] << 2) + (ptr[2] << 4) + (ptr[3] << 6) + port) % 
                                                                  servers.num;
}


static int userver_hash_func (const char *hostname, unsigned short port) {
  int sum = 0;

  while (*hostname) {
    sum += *hostname;
    hostname++;
  }
  return (sum + port) % uservers.num;
}


static struct server *server_new (struct host *h, unsigned short port, 
                                                      enum server_type type) {
  struct server *s;

  if (port == 0 || type == UNKNOWN_SERVER)
    return NULL;

  s = g_malloc0 (sizeof (struct server));

  s->host = h;
  host_ref (h);

  s->port = port;
  s->type = type;
  s->ping = -1;
  s->retries = -1;

  return s;
}


static struct userver *userver_new (const char *hostname, unsigned short port, 
                                                      enum server_type type) {
  struct userver *s;

  if (port == 0 || type == UNKNOWN_SERVER)
    return NULL;

  s = g_malloc0 (sizeof (struct userver));

  s->hostname = g_strdup (hostname);
  s->port = port;
  s->type = type;

  return s;
}


struct server *server_add (struct host *h, unsigned short port, 
                                                      enum server_type type) {
  struct server *s;
  GSList *ptr;
  int node;

  if (!h || type == UNKNOWN_SERVER)
    return NULL;

  if (port == 0)
    port = games[type].default_port;

  node = server_hash_func (h, port);

  if (!servers.nodes) {
    servers.nodes = g_malloc0 (sizeof (GSList *) * servers.num);
  }
  else {
    for (ptr = servers.nodes[node]; ptr; ptr = ptr->next) {
      s = (struct server *) ptr->data;
      if (s->host == h && s->port == port) {
	return s;
      }
    }
  }

  s = server_new (h, port, type);
  if (s) {
    servers.nodes[node] = g_slist_prepend (servers.nodes[node], s);

    if (games[type].analyze_serverinfo)
      (*games[type].analyze_serverinfo) (s);
  }
  return s;
}


struct userver *userver_add (const char *hostname, unsigned short port,
                                                      enum server_type type) {
  GSList *ptr;
  struct userver *s;
  int node;

  if (!hostname || type == UNKNOWN_SERVER)
    return NULL;

  if (port == 0)
    port = games[type].default_port;

  node = userver_hash_func (hostname, port);

  if (!uservers.nodes) {
    uservers.nodes = g_malloc0 (sizeof (GSList *) * uservers.num);
  }
  else {
    for (ptr = uservers.nodes[node]; ptr; ptr = ptr->next) {
      s = (struct userver *) ptr->data;
      if (strcmp (s->hostname, hostname) == 0 && s->port == port)
	return s;
    }
  }

  s = userver_new (hostname, port, type);
  if (s)
    uservers.nodes[node] = g_slist_prepend (uservers.nodes[node], s);
  return s;
}


void server_free_info (struct server *s) {
  if (s->name) {
    g_free (s->name);
    s->name = NULL;
  }
  if (s->map) {
    g_free (s->map);
    s->map  = NULL;
  }
  if (s->info) {
    g_free (s->info);
    s->info = NULL;
    s->game = NULL;
  }
  if (s->players) {
    g_slist_foreach (s->players, (GFunc) g_free, NULL);
    g_slist_free (s->players);
    s->players = NULL;
  }

  s->flags = 0;
  s->maxplayers = 0;
  s->curplayers = 0;

  s->filters = 0;
  s->flt_mask = 0;
  s->flt_last = 0;
}


void server_unref (struct server *s) {
  int node;

  if (!s)
    return;

  s->ref_count--;

  if (s->ref_count <= 0) {
    node = server_hash_func (s->host, s->port);
    servers.nodes[node] = g_slist_remove (servers.nodes[node], s);
    server_free_info (s);
    host_unref (s->host);
    g_free (s);
  }
}


void userver_unref (struct userver *s) {
  int node;

  if (!s)
    return;

  s->ref_count--;

  if (s->ref_count <= 0) {
    node = userver_hash_func (s->hostname, s->port);
    uservers.nodes[node] = g_slist_remove (uservers.nodes[node], s);
    if (s->s) server_unref (s->s);
    g_free (s->hostname);
    g_free (s);
  }
}


GSList *server_list_copy (GSList *list) {
  g_slist_foreach (list, (GFunc) server_ref, NULL);
  return g_slist_copy (list);
}


GSList *userver_list_copy (GSList *list) {
  g_slist_foreach (list, (GFunc) userver_ref, NULL);
  return g_slist_copy (list);
}


GSList *server_list_append_list (GSList *list, GSList *servers,
                                                      enum server_type type) {
  struct server *s;
  GSList *add = NULL;

  while (servers) {
    s = (struct server *) servers->data;
    if (type == UNKNOWN_SERVER || type == s->type) {
      if (g_slist_find (list, s) == NULL) {
        add = g_slist_prepend (add, s);
        server_ref (s);
      }
    }
    servers = g_slist_next (servers);
  }
  
  return g_slist_concat (list, g_slist_reverse (add));
}


GSList *userver_list_append_list (GSList *list, GSList *uservers,
                                                      enum server_type type) {
  struct userver *us;
  GSList *add = NULL;

  while (uservers) {
    us = (struct userver *) uservers->data;
    if (type == UNKNOWN_SERVER || type == us->type) {
      if (g_slist_find (list, us) == NULL) {
        add = g_slist_prepend (add, us);
        userver_ref (us);
      }
    }
    uservers = g_slist_next (uservers);
  }
  
  return g_slist_concat (list, g_slist_reverse (add));
}


int servers_total (void) {
  int i;
  int size = 0;

  if (!servers.nodes)
    return 0;

  for (i = 0; i < servers.num; i++)
    size += g_slist_length (servers.nodes[i]);

  return size;
}


int uservers_total (void) {
  int i;
  int size = 0;

  if (!uservers.nodes)
    return 0;

  for (i = 0; i < uservers.num; i++)
    size += g_slist_length (uservers.nodes[i]);

  return size;
}


GSList *all_servers (void) {
  GSList *list = NULL;
  GSList *tmp;
  struct server *s;
  int i;

  if (!servers.nodes)
    return NULL;

  for (i = 0; i < servers.num; i++) {
    for (tmp = servers.nodes[i]; tmp; tmp = tmp->next) {
      s = (struct server *) tmp->data;
      list = g_slist_prepend (list, s);
      server_ref (s);
    }
  }

  return list;
}


int parse_address (char *str, char **addr, unsigned short *port) {
  long tmp;
  char *ptr;
  char *endptr;

  if(!str || !addr || !port)
    return FALSE;

  *port = 0;
  *addr = NULL;

  ptr = strchr (str, ':');
  if (!ptr) {
    if (hostname_is_valid (str)) {
      *addr = g_strdup (str);
      return TRUE;
    }
    return FALSE;
  }

  if (ptr == str)
    return FALSE;

  tmp = strtol (ptr + 1, &endptr, 10);

  if (*endptr != '\0' || tmp <= 0 || tmp > 65535)
    return FALSE;

  *port = (unsigned short) tmp;

  *addr = g_malloc (ptr - str + 1);
  strncpy (*addr, str, ptr - str);
  (*addr) [ptr - str] = '\0';

  if (hostname_is_valid (*addr))
    return TRUE;

  g_free (*addr);
  *addr = NULL;
  *port = 0;
  return FALSE;
}


struct server *userver_set_host (struct userver *us, struct host *h) {
  struct server *s;

  if (!us || !h) return NULL;
  if (us->s) return us->s;

  s = server_add (h, us->port, us->type);
  if (s) {
    us->s = s;
    server_ref (s);
  }

  return s;
}


void uservers_to_servers (GSList **uservers, GSList **servers) {
  struct userver *us;
  GSList *tmp = *uservers;

  while (tmp) {
    us = (struct userver *) tmp->data;
    if (us->s) {
      *servers = server_list_append (*servers, us->s);

      *uservers = g_slist_remove (*uservers, us);
      userver_unref (us);

      tmp = *uservers;
      continue;
    }
    tmp = tmp->next;
  }
}


void server_lists_intersect (GSList **list1, GSList **list2) {
  GSList *list3 = NULL;
  GSList *tmp;
  struct server *s;

  for (tmp = *list1; tmp; tmp = tmp->next) {
    s = (struct server *) tmp->data;
    if (g_slist_find (*list2, s)) {
      *list2 = g_slist_remove (*list2, s);
      server_unref (s);
    }
    else {
      list3 = g_slist_prepend (list3, s);
      server_ref (s);
    }
  }

  server_list_free (*list1);
  *list1 = list3;
}


void server_list_fprintf (FILE *f, GSList *servers) {
  struct server *s;
  int i;

  for (i = 0; servers; i++) {
    s = (struct server *) servers->data;
    fprintf (f, "%d> [%s:%d](%d) %s \"%s\"\n", i, inet_ntoa (s->host->ip), 
                           s->port, s->ref_count, games[s->type].id, s->name);
    servers = servers->next;
  }
}


void userver_list_fprintf (FILE *f, GSList *uservers) {
  struct userver *us;
  int i;

  for (i = 0; uservers; i++) {
    us = (struct userver *) uservers->data;
    fprintf (f, "%d> [%s:%d](%d) %s\n", i, us->hostname, us->port, 
                                           us->ref_count, games[us->type].id);
    if (us->s) {
      fprintf (f, "  --> [%s:%d](%d) %s \"%s\"\n", 
                     inet_ntoa (us->s->host->ip), us->s->port, 
                     us->s->ref_count, games[us->type].id, us->s->name);
    }
      
    uservers = uservers->next;
  }
}


