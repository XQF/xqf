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



/*
  server_new -- Create (malloc) a new server structure with
  all of the values set at zero except the reference count which 
  should be at one.
*/

static struct server *server_new (struct host *h, unsigned short port, 
				  enum server_type type) {
  struct server *server;

  if (port == 0 || type == UNKNOWN_SERVER)
    return NULL;

  server = g_malloc0 (sizeof (struct server));
  debug (6, "server_new() -- Server %lx", server);

  server_ref (server);

  server->host = h;
  host_ref (h); /* Increse the refernece count on the host struct */

  server->port = port;
  server->type = type;
  server->ping = -1;
  server->retries = -1;

  return server;
}


static struct userver *userver_new (const char *hostname, unsigned short port, 
                                                      enum server_type type) {
  struct userver *userver;

  if (port == 0 || type == UNKNOWN_SERVER)
    return NULL;

  userver = g_malloc0 (sizeof (struct userver));
  userver_ref (userver);  /* baa -- Needed? FIX ME */
  userver->hostname = g_strdup (hostname);
  userver->port = port;
  userver->type = type;

  return userver;
}



/*
  server_add -- See if a host/port is in our list.  If it is not
  then add one.  If it is then we increase the reference count.
*/
struct server *server_add (struct host *h, unsigned short port, 
			   enum server_type type) {
  struct server *server;
  GSList *ptr;
  int node;

  if (!h || type == UNKNOWN_SERVER)
    return NULL;

  if (port == 0)
    port = games[type].default_port;

  node = server_hash_func (h, port);
  
  debug (6, "server_add() -- Add/Get a server");

  if (!servers.nodes) {
    servers.nodes = g_malloc0 (sizeof (GSList *) * servers.num);
  }
  else {
    /* Go through each of the servers in the list (each node)
       and see if we have a matching (by reference) entry.
    */
    for (ptr = servers.nodes[node]; ptr; ptr = ptr->next) {
      server = (struct server *) ptr->data;
      if (server->host == h && server->port == port) {
	server_ref (server);   /* baa -- added Dec-27, 2000 CORE FIX? */
	return server;
      }
    }
  }

  server = server_new (h, port, type);
  if (server) {
    servers.nodes[node] = g_slist_prepend (servers.nodes[node], server);

    if (games[type].analyze_serverinfo)
      (*games[type].analyze_serverinfo) (server);
  }
  return server;
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

  if (s->gametype) { /* Added by baa */
    s->gametype = NULL;
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


void server_unref (struct server *server) {
  int node;

  if (!server)
    return;

  server->ref_count--;

  debug (6, "server_unref() -- Server %lx ref now at %d", 
	 server, server->ref_count);

  if (server->ref_count <= 0) {
    node = server_hash_func (server->host, server->port);
    servers.nodes[node] = g_slist_remove (servers.nodes[node], server);
    /*
      Oops, it seems that we were freeing the server info before
      freeing the host info.  Bad. To free the host info w/o a memory
      leak we need to do that before freeing the server info. --baa 
    */
    host_unref (server->host);
    server_free_info (server);
    g_free (server);
  }
}


void userver_unref (struct userver *s) {
  int node;

  if (!s)
    return;
  
  s->ref_count--;

  debug (6, "userver_unref() -- UServer %lx ref now at %d", 
	 s, s->ref_count);

  if (s->ref_count <= 0) {
    node = userver_hash_func (s->hostname, s->port);
    uservers.nodes[node] = g_slist_remove (uservers.nodes[node], s);
    if (s->s) server_unref (s->s);
    g_free (s->hostname);
    g_free (s);
  }
}


GSList *server_list_copy (GSList *list) {
  debug (3, "server_list_copy() -- list %ld copying all servers", list);
  g_slist_foreach (list, (GFunc) server_ref, NULL);
  return g_slist_copy (list);
}


GSList *userver_list_copy (GSList *list) {
  g_slist_foreach (list, (GFunc) userver_ref, NULL);
  return g_slist_copy (list);
}


GSList *server_list_append_list (GSList *list, GSList *server_list,
				 enum server_type type) {
  struct server *server;
  GSList *add = NULL;
  debug (6, "server_list_append_list() -- list %lx", list);
  while (server_list) {
    server = (struct server *) server_list->data;
    if (type == UNKNOWN_SERVER || type == server->type) {
      if (g_slist_find (list, server) == NULL) {
        add = g_slist_prepend (add, server);
        server_ref (server);
      }
    }
    server_list = g_slist_next (server_list);
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



/*
  all_servers -- Get a list of all servers.  This is called from 
  two functions source.c:free_masters and statistics.c:collect_statistics
  both of which call server_free_list afterwards.
*/
GSList *all_servers (void) {
  GSList *list = NULL;
  GSList *tmp;
  struct server *server;
  int i;

  if (!servers.nodes)
    return NULL;
  debug (6, "all_servers() -- Get all servers");
  for (i = 0; i < servers.num; i++) {
    for (tmp = servers.nodes[i]; tmp; tmp = tmp->next) {
      server = (struct server *) tmp->data;
      list = g_slist_prepend (list, server);
      server_ref (server);
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
  struct server *server;

  if (!us || !h) return NULL;
  if (us->s) return us->s;

  /* Since server_add increases the reference count for us,
     we do not do it here. */
  server = server_add (h, us->port, us->type);
  if (server) {
    us->s = server;
  }

  return server;
}


void uservers_to_servers (GSList **uservers, GSList **servers) {
  struct userver *us;
  GSList *tmp = *uservers;

  debug (6, "uservers_to_servers() -- uservers %lx  tmp %lx", uservers, tmp);
  while (tmp) {
    us = (struct userver *) tmp->data;
    debug (7, "uservers_to_servers() -- Check userver %lx", us);
    if (us->s) {
      debug (7, "uservers_to_servers() -- server %lx  userver %lx", us->s, us);
      *servers = server_list_append (*servers, us->s);

      *uservers = g_slist_remove (*uservers, us);
      userver_unref (us);

      tmp = *uservers;
      continue;
    }
    tmp = tmp->next;
  }
  debug (6, "uservers_to_servers() -- Done.");
}


/*
  server_lists_intersect() -- Find servers that are
  in two lists (by reference).  The first arg gets a list 
  of servers not in both lists.  The second list gets servers
  removed that are in both lists.
*/

void server_lists_intersect (GSList **list1, GSList **list2) {
  GSList *list3 = NULL;
  GSList *tmp;
  struct server *s;
  
  debug (6, "server_lists_intersect() -- ");
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


