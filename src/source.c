/* XQF - Quake server browser and launcher Copyright (C) 1998-2000 Roman
 * Pozlevich <roma@botik.ru>
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

#include "gnuconfig.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>	/* strtol */
#include <string.h>	/* strchr, strcmp, strstr */
#include <sys/stat.h>	/* stat */
#include <unistd.h>	/* stat */
#include <sys/socket.h>	/* inet_aton, inet_ntoa */
#include <netinet/in.h>	/* inet_aton, inet_ntoa */
#include <arpa/inet.h>	/* inet_aton, inet_ntoa */
#include <errno.h>	/* errno */

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "xqf-ui.h"
#include "source.h"
#include "server.h"
#include "host.h"
#include "pref.h"
#include "dialogs.h"
#include "game.h"
#include "utils.h"
#include "config.h"
#include "dns.h"
#include "zipped.h"
#include "stat.h"

struct master *favorites = NULL;
GSList *master_groups = NULL;

char* master_prefixes[MASTER_NUM_QUERY_TYPES] = {
	"master://",
	"gmaster://",
	"http://",
	"lan://"
};

char* master_designation[MASTER_NUM_QUERY_TYPES] = {
	N_("Standard"),
	N_("Gamespy"),
	N_("http"),
	N_("LAN")
};

static GSList *all_masters = NULL;


static void save_list (FILE *f, struct master *m) {
  GSList *srv;
  struct server *s;

  if (!m->servers)
    return;
  debug (6, "save_list() -- Saving Server List \"%s\"", m->name );

#ifdef DEBUG
  fprintf (stderr, "Saving server list \"%s\"...\n", m->name);
#endif

  if (m == favorites) {
    fprintf (f, "[%s]\n", m->name);
  }
  else {
    if (m->url) {
      fprintf (f, "[%s %s]\n", games[m->type].id, m->url);
    }
    else if (m->master_type == MASTER_GAMESPY)
    {
      fprintf (f, "[%s %s%s:%d]\n", games[m->type].id, master_prefixes[MASTER_GAMESPY],
        	(m->hostname)? m->hostname : inet_ntoa (m->host->ip), m->port);
    }
    else
    {
      fprintf (f, "[%s %s%s:%d]\n", games[m->type].id, master_prefixes[MASTER_NATIVE],
		(m->hostname)? m->hostname : inet_ntoa (m->host->ip), m->port);
    }
  }

  for (srv = m->servers; srv; srv = srv->next) {
    s = (struct server *) srv->data;
    fprintf (f, "%s %s:%d\n", games[s->type].id, inet_ntoa (s->host->ip), s->port);
  }

  fprintf (f, "\n");
}


void save_favorites (void) {
  FILE *f;
  char *realname;

  debug (6, "save_favorites() --");
  realname = file_in_dir (user_rcdir, FILENAME_FAVORITES);

  while (1) {
    f = fopen (realname, "w");

    if (!f) {
      if (dialog_yesno (NULL, 0, _("Retry"), _("Skip"), 
			_("Cannot write to file %s\nSystem Error: %s\n"), 
			realname, g_strerror (errno))) {
	continue;
      }
      g_free (realname);
      return;
    }

    break;
  }

  save_list (f, favorites);
  fclose (f);
  g_free (realname);
}


static void save_lists (GSList *list, const char *filename) {
  char *realname;
  struct zstream z;

  debug (6, "save_lists() -- %s", filename);
  realname = file_in_dir (user_rcdir, filename);

  if (!list) {
    zstream_unlink (realname);
    g_free (realname);
    return;
  }

  zstream_open_w (&z, realname);

  if (z.f) {
    while (list) {
      save_list (z.f, (struct master *) list->data);
      list = list->next;
    }
    zstream_close (&z);
  }

  g_free (realname);
}


static void save_server_info (const char *filename, GSList *servers) {
  char *realname;
  struct zstream z;
  struct server *s;

  realname = file_in_dir (user_rcdir, filename);

  if (!servers) {
    zstream_unlink (realname);
    g_free (realname);
    return;
  }

  zstream_open_w (&z, realname);

  if (z.f) {
    while (servers) {
      s = (struct server *) servers->data;
	
      if (s->ref_count > 0 && games[s->type].save_info)
	(*games[s->type].save_info) (z.f, s);

      servers = servers->next;
    }
    zstream_close (&z);
  }

  g_free (realname);
}


static struct master *find_master_server (char *addr, unsigned short port, char *str) {
  GSList *list;
  struct master *m;
  struct in_addr ip;

  if (!addr || !port)
    return NULL;

  for (list = all_masters; list; list = list->next) {
    m = (struct master *) list->data;
    if (m->port && m->port == port) 
    {
      if (m->hostname) 
      {
	if (!g_strcasecmp (m->hostname, addr))
	{
          if (!str) // pre 0.9.4e list file
            return m;
	  if (!g_strcasecmp (games[m->type].id, str)) // list file in new format
	    return m;
	}
      }
      else 
      {
	if (m->host && inet_aton (addr, &ip) && ip.s_addr == m->host->ip.s_addr) {
          if (!str) // pre 0.9.4e list file
            return m;
	  if (!g_strcasecmp (games[m->type].id, str))
  	    return m;
	}
      }
    }
  }

  return NULL;
}


/*
 *  This is internal function to make compare_urls() function work.
 *  Don't use it for any other purposes.  
 *
 *  Unification is
 *    1) lower case protocol and hostname
 *    2) remove port reference if it's standard port for the protocol
 *    3) remove trailing slashes from the end of the file location
 */

static char *unify_url (const char *url) {
  char *unified;
  char *ptr, *ptr2, *ptr3;
  long port;
  int len;

  if (!url)
    return NULL;

  ptr = strstr (url, "://");
  if (ptr == NULL)
    return NULL;

  ptr += 3;

  if (*ptr == '\0')
    return NULL;

  unified = g_malloc (strlen (url) + 1);

  ptr2 = strchr (ptr, ':');
  if (!ptr2) {
    ptr2 = strchr (ptr, '/');
    if (!ptr2)
      ptr2 = ptr + strlen (ptr);
    ptr3 = ptr2;
  }
  else {
    port = strtol (ptr2 + 1, &ptr3, 10);
    if (port != 80)
      ptr2 = ptr3;
  }

  strncpy (unified, url, ptr2 - url);
  unified[ptr2 - url] = '\0';
  g_strdown (unified);

  strcpy (unified + (ptr2 - url), ptr3);

  while ( (len = strlen (unified)) > 0 && unified[len - 1] == '/' )
    unified[len - 1] = '\0';

#ifdef DEBUG
  fprintf (stderr, "URL unification %s -> %s\n", url, unified);
#endif

  return unified;
}


static int compare_urls (const char *url1, const char *url2) {
  char *uni1;
  char *uni2;
  int res = 1;

  uni1 = unify_url (url1);
  uni2 = unify_url (url2);

  /*
   *  It's error if any URL is NULL
   *  Don't take care of situation when both of URLs are NULL
   */

  if (uni1 != NULL && uni2 != NULL)
    res = strcmp (uni1, uni2);

  if (uni1) g_free (uni1);
  if (uni2) g_free (uni2);
  return res;
}


static struct master *find_master_url (char *url) {
  GSList *list;
  struct master *m;

  if (!url)
    return NULL;

  for (list = all_masters; list; list = list->next) {
    m = (struct master *) list->data;
    if (m->url && compare_urls (m->url, url) == 0)
      return m;
  }

  return NULL;
}


static struct master *read_list_parse_master (char *str, char *str2) {
  char *addr;
  unsigned short port;
  struct master *m;

  if (favorites && !g_strcasecmp (str, favorites->name))
    return favorites;

  if (g_strncasecmp (str, master_prefixes[MASTER_NATIVE], strlen(master_prefixes[MASTER_NATIVE])) == 0) {
    if (parse_address (str + strlen(master_prefixes[MASTER_NATIVE]), &addr, &port)) {
      m = find_master_server (addr, port, str2);
      g_free (addr);
      return m;
    }
  }

  if (g_strncasecmp (str, master_prefixes[MASTER_GAMESPY], strlen (master_prefixes[MASTER_GAMESPY])) == 0) {
    if (parse_address (str + strlen(master_prefixes[MASTER_GAMESPY]), &addr, &port)) {
      m = find_master_server (addr, port, str2);
      g_free (addr);
      return m;
    }
  }

  if (g_strncasecmp (str, master_prefixes[MASTER_HTTP], strlen(master_prefixes[MASTER_HTTP])) == 0) {
    m = find_master_url (str);
    return m;
  }

  return NULL;
}


static void master_add_server (struct master *m, char *str, 
                                                      enum server_type type) {
  char *addr;
  unsigned short port;
  struct host *h;
  struct server *s;
  struct userver *us;

  debug (6, "master_add_server() -- Master %lx", m);
  if (parse_address (str, &addr, &port)) {
    h = host_add (addr);
    if (h) {						/* IP address */
      host_ref (h);
      if ((s = server_add (h, port, type)) != NULL) {
	m->servers = server_list_prepend (m->servers, s);
	/* Since the server_add increments the ref count, and 
	   server_list_prepend ups the ref_count, we should
	   unref it once because we are only keeping it in
	   one list after this function.
	*/
	server_unref (s);
      }
      host_unref (h);
    }
    else {						/* hostname */
      g_strdown (addr);
      if ((us = userver_add (addr, port, type)) != NULL)
	m->uservers = userver_list_add (m->uservers, us);
    }
    g_free (addr);
  }
}


static void read_lists (const char *filename) {
  struct zstream z;
  char *realname;
  char buf[4096];
  char *ch;
  struct master *m = NULL;
  char *token[8];
  int n;
  enum server_type type;

  realname = file_in_dir (user_rcdir, filename);
  zstream_open_r (&z, realname);

  if (z.f == NULL) {
    g_free (realname);
    return;
  }

  while (fgets (buf, 4096, z.f)) {
    n = tokenize (buf, token, 8, " \t\n\r");
    if (n < 1)
      continue;

    if (token[0][0] == '[') {  // line is a master
      ch = strchr (token[0] + 1, ']'); // does token 0 have a ]?
      if (ch) {	// it's a favorites or pre 0.9.4e lists file
       *ch = '\0';

      if (m && m->servers)
        m->servers = g_slist_reverse (m->servers);

      m = read_list_parse_master (token[0] + 1, NULL);
    }
    else {
      ch = strchr (token[1], ']'); // does token 1 have a ]?
      if (ch) {

      *ch = '\0';

      if (m && m->servers)
      m->servers = g_slist_reverse (m->servers);

      m = read_list_parse_master (token[1], token[0] + 1); // master, type
      }
     }
    }
    else {
      if (!m || n < 2)
	continue;

      type = id2type (token[0]);

      if (type != UNKNOWN_SERVER)
	master_add_server (m, token[1], type);
    }
  }

  if (m && m->servers)
    m->servers = g_slist_reverse (m->servers);

  zstream_close (&z);
  g_free (realname);
}


static void read_server_info (const char *filename) {
  struct zstream z;
  char *realname;
  char *buf;
  char *pos;
  GSList *strings;

  debug(3,"read_server_info(%s)",filename);

  realname = file_in_dir (user_rcdir, filename);
  zstream_open_r (&z, realname);

  if (z.f == NULL) {
    g_free (realname);
    return;
  }

  buf = g_malloc (1024*16);
  pos = buf;
  strings = NULL;

  while (1) {
    if (pos - buf > 1024*16 - 16) {
      fprintf (stderr, "server record is too large\n");
      g_slist_free (strings);
      break;
    }

    if (!fgets (pos, 1024*16 - (pos - buf), z.f)) {

      /* EOF || Error */

      if (strings) {
	parse_saved_server (strings);
	g_slist_free (strings);
      }
      break;

    }
    else {

      if (pos[0] == '\n') {		/* empty line */
	parse_saved_server (strings);
	g_slist_free (strings);
	strings = NULL;
	pos = buf;
      }
      else {
	strings = g_slist_append (strings, pos);

	while (*pos) {
	  if (*pos == '\n') {
	    *pos = '\0';
	    break;
	  }
	  pos++;
	}
	pos++;
      }

    }
  }

  g_free (buf);

  zstream_close (&z);
  g_free (realname);
}


static void read_favorites_old_format (char *fn) {
  FILE *f;
  char buf[1024];
  char *token[3];
  int n;
  enum server_type type;

  f = fopen (fn, "r");
  if (!f)
    return;

  while (fgets (buf, 1024, f)) {
    n = tokenize_bychar (buf, token, 3, '|');
    if (n < 2)
      continue;

    type = id2type (token[0]);

    if (type != UNKNOWN_SERVER)
      master_add_server (favorites, token[1], type);
  }

  if (favorites->servers)
    favorites->servers = g_slist_reverse (favorites->servers);

  fclose (f);
}


static void compat_convert_favorites (void) {
  struct stat statbuf;
  char *fn;

  fn = file_in_dir (user_rcdir, FILENAME_FAVORITES);
  if (stat (fn, &statbuf) == 0) {
    g_free (fn);
    return;
  }
  g_free (fn);

  fn = file_in_dir (user_rcdir, "Favorites");
  read_favorites_old_format (fn);
  g_free (fn);
}


static struct master *create_master (char *name, enum server_type type, 
                                                                  int group) {
  struct master *m;

  m = g_malloc0 (sizeof (struct master));
  m->name = g_strdup (name);
  m->type = type;
  m->state = SOURCE_NA;
  m->isgroup = group;

  if (!group)
    all_masters = g_slist_append (all_masters, m);

  return m;
}


struct master *add_master (char *path, char *name, enum server_type type, 
                                                  int user, int lookup_only) {
  char *addr = NULL;
  unsigned short port = 0;
  struct master *m = NULL;
  struct host *h = NULL;
  struct master *group = NULL;
  enum master_query_type query_type;

  debug(6,"add_master(%s,%s,%d,%d,%d)",path,name,type,user,lookup_only);

  query_type = get_master_query_type_from_address(path);
  if( query_type == MASTER_INVALID_TYPE )
  {
    debug(1,"Invalid Master %s",path);
    return NULL;
  }

  if( query_type == MASTER_NATIVE || query_type == MASTER_GAMESPY  || query_type == MASTER_LAN)
  {
    // check for valid hostname/ip
    if (parse_address (path + strlen(master_prefixes[query_type]), &addr, &port))
    {
      // if no port was specified, add default master port if available or fail
      if (!port)
      {
	// use default_port instead of default_master_port for lan broadcasts
	if( query_type == MASTER_LAN )
	{
	  port = games[type].default_port;
	  // unreal needs one port higher
	  if(!strcmp(games[type].qstat_option,"-uns"))
	  {
	    port++;
	    type=GPS_SERVER;
	  }
	}
	// do not use default for gamespy
	else if (query_type != MASTER_GAMESPY && games[type].default_master_port)
	{
	  port = games[type].default_master_port;
	}
	else
	{
	  g_free (addr);
	  // translator: %s == url, eg gmaster://bla.blub.org
	  dialog_ok (NULL, _("You have to specify a port number for %s."),path);
	  return NULL;
	}
      }

      m = find_master_server (addr, port, games[type].id);

    }
  }
  else if( query_type == MASTER_HTTP )
  {
      m = find_master_url (path);
  }

  if (lookup_only)
  {
    g_free (addr);
    return m;
  }

  if (m)
  {
    if (user)
    { // Master renaming is forced by user
      g_free (m->name);
      m->name = g_strdup (name);
      m->user = TRUE;
    }
    else
    { // Automatically rename masters that are not edited by user
      if (!m->user) {
	g_free (m->name);
	m->name = g_strdup (name);
      }
    }
    g_free (addr);
    return m;
  }
  
  // master was not known already, create new
  if( query_type == MASTER_NATIVE || query_type == MASTER_GAMESPY || query_type == MASTER_LAN)
  {
    m = create_master (name, type, FALSE);

    h = host_add (addr);
    if (h) {
      m->host = h;
      host_ref (h);
      g_free (addr);
      addr = NULL;
    }
    else {
      m->hostname = addr;
    }
    m->port = port;
  }
  else if( query_type == MASTER_HTTP )
  {
    m = create_master (name, type, FALSE);
    m->url = g_strdup (path);
  }
  else return NULL;
  
  m->master_type = query_type;

  if (m) {
    group = (struct master *) g_slist_nth_data (master_groups, type);
    group->masters = g_slist_append (group->masters, m);
    m->user = user;
  }
  
  return m;
}



#if 0
  if (g_strncasecmp (path, master_prefixes[MASTER_NATIVE],
			  strlen(master_prefixes[MASTER_NATIVE])) == 0) {
    if (parse_address (path + strlen(master_prefixes[MASTER_NATIVE]), &addr, &port)) {

      if (!port) {
	if (games[type].default_master_port) {
	  port = games[type].default_master_port;
	}
	else {
	  g_free (addr);
	  return NULL;
	}
      }

      m = find_master_server (addr, port, games[type].id);

      if (lookup_only) {
	g_free (addr);
	return m;
      }

      if (m) {
	if (user) {
	  /* Master renaming is forced by user */
	  g_free (m->name);
	  m->name = g_strdup (name);
	  m->user = TRUE;
	}
	else {
	  /* Automatically rename masters that are not edited by user */
	  if (!m->user) {
	    g_free (m->name);
	    m->name = g_strdup (name);
	  }
	}
	g_free (addr);
	return m;
      }
      else
       {
	m = create_master (name, type, FALSE);

        m->master_type = MASTER_NATIVE; // Regular master

	h = host_add (addr);
	if (h) {
	  m->host = h;
	  host_ref (h);
	  g_free (addr);
	}
	else {
	  m->hostname = addr;
	}
	m->port = port;
      }

    }
  }

  else if (g_strncasecmp (path, master_prefixes[MASTER_GAMESPY],
			  strlen(master_prefixes[MASTER_GAMESPY])) == 0) {
    if (parse_address (path + strlen(master_prefixes[MASTER_GAMESPY]), &addr, &port)) {

      if (!port) {
	if (games[type].default_master_port) {
	  port = games[type].default_master_port;
	}
	else {
	  g_free (addr);
	  return NULL;
	}
      }

      m = find_master_server (addr, port, games[type].id);

      if (lookup_only) {
	g_free (addr);
	return m;
      }

      if (m) {
	if (user) {
	  /* Master renaming is forced by user */
	  g_free (m->name);
	  m->name = g_strdup (name);
	  m->user = TRUE;
	}
	else {
	  /* Automatically rename masters that are not edited by user */
	  if (!m->user) {
	    g_free (m->name);
	    m->name = g_strdup (name);
	  }
	}
	g_free (addr);
	return m;
      }
      else 
      {
	m = create_master (name, type, FALSE);

        m->master_type = MASTER_GAMESPY; // Gamespy master

	h = host_add (addr);
	if (h) {
	  m->host = h;
	  host_ref (h);
	  g_free (addr);
	}
	else {
	  m->hostname = addr;
	}
	m->port = port;
      }

    }
  }

  else {
    if (g_strncasecmp (path, master_prefixes[MASTER_HTTP],
			    strlen(master_prefixes[MASTER_HTTP])) == 0) {
      m = find_master_url (path);

      if (lookup_only) {
	return m;
      }

      if (m) {
	if (user) {
	  /* Master renaming is forced by user */
	  g_free (m->name);
	  m->name = g_strdup (name);
	  m->user = TRUE;
	}
	else {
	  /* Automatically rename masters that are not edited by user */
	  if (!m->user) {
	    g_free (m->name);
	    m->name = g_strdup (name);
	  }
	}
	return m;
      }
      else {
	m = create_master (name, type, FALSE);
	m->url = g_strdup (path);
      }
    }
  }
  if (m) {
    group = (struct master *) g_slist_nth_data (master_groups, type);
    group->masters = g_slist_append (group->masters, m);
    m->user = user;
  }

  return m;
}
#endif


void free_master (struct master *m) {
  struct master *group;

  if (!m)
    return;

  if (m->isgroup) {
    while (m->masters)
      free_master ((struct master *) m->masters->data);
  }
  else {
    if (m->type != UNKNOWN_SERVER) {
      group = (struct master *) g_slist_nth_data (master_groups, m->type);
      group->masters = g_slist_remove (group->masters, m);
    }
    all_masters = g_slist_remove (all_masters, m);
  }

  if (m->host)     host_unref (m->host);
  if (m->hostname) g_free (m->hostname);
  if (m->name)     g_free (m->name);
  if (m->url)      g_free (m->url);

  server_list_free (m->servers);
  userver_list_free (m->uservers);

  g_free (m);
}


static char *builtin_masters_update_info[] = {

  "ADD QS http://www.gameaholic.com/servers/qspy-quake Gameaholic.Com",
  "ADD QS http://ironman.planetquake.com/serversqspy.txt Ironman",

  "ADD QWS master://192.246.40.37:27000 id Limbo",
  "ADD QWS master://192.246.40.37:27002 id CTF",
  "ADD QWS master://192.246.40.37:27003 id Team Fortress",
  "ADD QWS master://192.246.40.37:27004 id Misc",
  "ADD QWS master://192.246.40.37:27006 id Deathmatch",
  "ADD QWS master://204.182.161.2:27000 PlanetQuake",
  "ADD QWS master://203.55.240.100:27000 Australia",
  "ADD QWS master://200.255.244.2:27000 Brazil",
  "ADD QWS master://qwmaster.ocrana.de:27000 Germany",
  "ADD QWS master://207.88.6.18:27000 FortressFest TF",
  "ADD QWS master://master.quakeworld.ru:27000 Russia",
  "ADD QWS master://194.251.249.32:27000 iNET (Finland)",
  "ADD QWS master://qwmaster.mondial.net.au:27000 Mondial Aussie",
  "ADD QWS master://qwmaster.barrysworld.com:27000 BarrysWorld (UK)",

  "ADD Q2S master://satan.idsoftware.com:27900 id",
  "ADD Q2S master://q2master.planetquake.com:27900 PlanetQuake", 
  "ADD Q2S master://telefragged.com:27900 TeleFragged",
  "ADD Q2S master://qwmaster.barrysworld.com:27900 BarrysWorld (UK)",
  "ADD Q2S master://master.quake.inet.fi:27900 iNET (Finland)",
  "ADD Q2S master://q2master.mondial.net.au:27900 Australia",
  "ADD Q2S master://q2master.gxp.de:27900 gXp (Germany)",
  "ADD Q2S http://www.gameaholic.com/servers/qspy-quake2 Gameaholic.Com",
  "ADD Q2S http://www.lithium.com/quake2/gamespy.txt Lithium",

  "ADD Q3S master://master3.idsoftware.com id",
//  "ADD Q3S master://q3master.splatterworld.de Germany",
//  "ADD Q3S master://q3.golsyd.net.au Australia",
  "ADD Q3S master://q3master.barrysworld.com:27950 BarrysWorld",
  "ADD Q3S http://www.gameaholic.com/servers/qspy-quake3 Gameaholic.com",

  "ADD SNS http://www.gameaholic.com/servers/qspy-sin Gameaholic.Com",
  "ADD SNS http://asp.planetquake.com/sinserverlist/servers.txt PlanetQuake",

  "ADD HLS master://half-life.east.won.net WON East",
  "ADD HLS master://half-life.west.won.net WON West",

  "ADD Q2S:KP http://www.gameaholic.com/servers/qspy-kingpin Gameaholic.Com",
  "ADD Q2S:KP http://www.ogn.org:6666 OGN",

  "ADD Q2S:HR http://www.gameaholic.com/servers/qspy-heretic2 Gameaholic.Com",

  "ADD UNS gmaster://unreal.epicgames.com:28900 Epic",
  "ADD UNS gmaster://utmaster.barrysworld.com:28909 BarrysWorld",

  "ADD T2S master://211.233.32.77:28002 Tribes2 Master",

  "ADD WOS master://wolf.idsoftware.com:27950 id",

  "ADD EFS http://www.gameaholic.com/servers/qspy-startrekeliteforce Gameaholic.com",

  "ADD D3P master://gt.pxo.net:3445 PXO",
  "ADD D3P http://www.gameaholic.com/servers/qspy-descent3 gameaholic.com",

  "ADD SFS http://www.gameaholic.com/servers/qspy-soldieroffortune gameaholic.com",
  "ADD GPS http://www.gameaholic.com/servers/qspy-noonelivesforever Noone Lives Forevere - gameaholic.com",
  "ADD GPS http://www.gameaholic.com/servers/qspy-rallymasters Rally Masters - gameaholic.com",
  "ADD GPS http://www.gameaholic.com/servers/qspy-turok2 Turok 2 - gameaholic.com",
  "ADD GPS http://www.gameaholic.com/servers/qspy-shogo Shogo - gameaholic.com",

  "ADD RUNESRV http://www.gameaholic.com/servers/qspy-rune gameaholic.com",

  NULL
};


/*
 *   Format:  Action ServerType URL Description
 */

static void update_master_list_action (const char *action) {
  char *str;
  char *token[4];
  int n;
  enum server_type type;
  struct master *m;

  str = strdup_strip (action);

  if (!str || !*str)
    return;

  n = tokenize (str, token, 4, " \t\n\r");

  if (n == 4) {
    type = id2type (token[1]);
    if (type != UNKNOWN_SERVER) {

      if (g_strcasecmp (token[0], ACTION_ADD) == 0) {
	m = add_master (token[2], token[3], type, FALSE, FALSE);
	if (m && source_ctree != NULL)
	  source_ctree_add_master (source_ctree, m);
      }
      else if (g_strcasecmp (token[0], ACTION_DELETE) == 0) {
	m = add_master (token[2], token[3], type, FALSE, TRUE);
	if (m) {
	  if (source_ctree != NULL)
	    source_ctree_delete_master (source_ctree, m);
	  free_master (m);
	}
      }

    }
  }

  g_free (str);
}


void update_master_list_builtin (void) {
  char **ptr;

  for (ptr = builtin_masters_update_info; *ptr; ptr++)
    update_master_list_action (*ptr);
}

void update_master_list_web (void) {
  char **ptr;

  for (ptr = builtin_masters_update_info; *ptr; ptr++)
    update_master_list_action (*ptr);
}


static void load_master_list (void) {
  struct master *m;
  enum server_type type;
  char conf[64];
  char *token[3];
  char *str;
  char *tmp;
  int user;
  int i = 0;
  int n;

  config_push_prefix ("/" CONFIG_FILE "/Master List");

  while (1) {
    g_snprintf (conf, 64, "master%d", i);
    str = config_get_string (conf);

    if (!str)
      break;

    n = tokenize (str, token, 3, " \t\n\r");

    if (n == 3) {

      user = FALSE;
      tmp = strchr (token[0], ',');
      if (tmp) {
	*tmp++ = '\0';
	user = (g_strcasecmp (tmp, "USER") == 0);
      }
      type = id2type (token[0]);

      if (type != UNKNOWN_SERVER) {

	/*  Use 'user' mode to insert master to the master list
	 *  and fix m->user after that.
	 */

	m = add_master (token[1], token[2], type, TRUE, FALSE);
	if (m)
	  m->user = user;
      }

    }

    g_free (str);
    i++;
  }

  config_pop_prefix ();
}


static void save_master_list (void) {
  GSList *list;
  struct master *m;
  char conf[64];
  const char *typeid;
  char typeid_buf[64];
  char *confstr;
  char *addr;
  int n = 0;

  config_clean_section ("/" CONFIG_FILE "/Master List");
  config_push_prefix ("/" CONFIG_FILE "/Master List");

  for (list = all_masters; list; list = list->next) {
    m = (struct master *) list->data;

    if (m == favorites || m->isgroup)
      continue;

    g_snprintf (conf, 64, "master%d", n);

    if (m->user) {
      g_snprintf (typeid_buf, 64, "%s,USER", type2id (m->type));
      typeid = typeid_buf;
    }
    else {
      typeid = type2id (m->type);
    }

    if (m->url) {
      confstr = g_strjoin (" ", typeid, m->url, m->name, NULL);
    }
    else {
      addr = g_strdup_printf ("%s%s:%d", master_prefixes[m->master_type],
        	       (m->hostname)? m->hostname : inet_ntoa (m->host->ip), m->port);

      confstr = g_strjoin (" ", typeid, addr, m->name, NULL);
      g_free (addr);
    }

    config_set_string (conf, confstr);
    g_free (confstr);
    n++;
  }

  config_pop_prefix ();
}


void init_masters (int update) {
  struct master *m;
  int i;

  favorites = create_master (N_("Favorites"), UNKNOWN_SERVER, FALSE);

  for (i = 0; i < GAMES_TOTAL; i++) {
    m = create_master (games[i].name, i, TRUE);
    master_groups = g_slist_append (master_groups, m);
  }

  load_master_list ();

  if (update) {
    update_master_list_builtin ();
    config_sync ();
  }

  compat_convert_favorites ();

  read_lists (FILENAME_FAVORITES);
  read_lists (FILENAME_LISTS);

  read_server_info (FILENAME_SRVINFO);
}


void free_masters (void) {
  GSList *tmp;

  debug (6, "free_masters() --");
  save_favorites ();

  if (default_save_lists) {
    tmp = g_slist_copy (all_masters);
    tmp = g_slist_remove (tmp, favorites);
    save_lists (tmp, FILENAME_LISTS);
    g_slist_free (tmp);

    if (default_save_srvinfo) {
      tmp = all_servers (); /* free done in two lines */
      save_server_info (FILENAME_SRVINFO, tmp);
      server_list_free (tmp);
    }
  }
  else {
    save_lists (NULL, FILENAME_LISTS);
    if (default_save_srvinfo) {
      save_server_info (FILENAME_SRVINFO, favorites->servers);
    }
  }

  save_master_list ();

  free_master (favorites);
  favorites  = NULL;

  g_slist_foreach (master_groups, (GFunc) free_master, NULL);
  g_slist_free (master_groups);
  master_groups = NULL;
}


static inline GSList *master_list_add_sigle (GSList *list, struct master *m) {
  if (g_slist_find (list, m) == NULL)
    list = g_slist_append (list, m);
  return list;
}


static void master_list_add (GSList **masters, GSList **servers, 
                                        GSList **uservers, struct master *m) {
  GSList *tmp;

  debug (6, "master_list_add() -- master '%s'", m->name);
  if (m->isgroup || m == favorites) {
    if (servers) {
      *servers = server_list_append_list (*servers, favorites->servers, 
					                             m->type);
    }
    if (uservers) {
      *uservers = userver_list_append_list (*uservers, favorites->uservers,
                                                                     m->type);
    }
  }

  if (masters) {
    if (m->isgroup) {
      for (tmp = m->masters; tmp; tmp = tmp->next)
	*masters =  master_list_add_sigle (*masters, 
                                                 (struct master *) tmp->data);
    }
    else {
      if (m != favorites)
	*masters = master_list_add_sigle (*masters, m);
    }
  }
}


void collate_server_lists (GSList *masters, GSList **servers, 
                                                          GSList **uservers) {
  struct master *m;
  GSList *tmp;

  for (tmp = masters; tmp; tmp = tmp->next) {
    m = (struct master *) tmp->data;
    if (servers) {
      *servers = server_list_append_list (*servers, m->servers, 
					                      UNKNOWN_SERVER);
    }
    if (uservers) {
      *uservers = userver_list_append_list (*uservers, m->uservers, 
                                                              UNKNOWN_SERVER);
    }
  }
}


void master_selection_to_lists (GSList *list, GSList **masters, 
                                        GSList **servers, GSList **uservers) {
  struct master *m;
  GSList *tmp;
  debug (6, "master_selection_to_lists() --");
  for (tmp = list; tmp; tmp = tmp->next) {
    m = (struct master *) tmp->data;
    uservers_to_servers (&m->uservers, &m->servers);
    master_list_add (masters, servers, uservers, m);
  }
}


int source_has_masters_to_update (GSList *source) {
  GSList *tmp;
  struct master *m;

  for (tmp = source; tmp; tmp = tmp->next) {
    m = (struct master *) tmp->data;
    if (m->isgroup) {
      if (source_has_masters_to_update (m->masters))
	return TRUE;
    }
    else {
      if (m != favorites) 
	return TRUE;
    }
  }
  return FALSE;
}


int source_has_masters_to_delete (GSList *source) {
  GSList *tmp;
  struct master *m;

  for (tmp = source; tmp; tmp = tmp->next) {
    m = (struct master *) tmp->data;
    if (m != favorites && !m->isgroup)
      return TRUE;
  }
  return FALSE;
}


GSList *references_to_server (struct server *s) {
  GSList *list;
  GSList *res = NULL;
  struct master *m;

  for (list = all_masters; list; list = list->next) {
    m = (struct master *) list->data;
    if (g_slist_find (m->servers, s))
      res = g_slist_append (res, m);
  }

  return res;
}

enum master_query_type get_master_query_type_from_address(char* address)
{
  enum master_query_type type;
  // check for known master prefix
  debug(6,"get_master_query_type_from_address(%s)",address);
  for (type=MASTER_NATIVE;type<MASTER_NUM_QUERY_TYPES;type++)
  {
    if(!g_strncasecmp( address, master_prefixes[type],
	strlen(master_prefixes[type])))
    {
      debug(6,"get_master_query_type_from_address: found %s",master_prefixes[type]);
      return type;
    }
  }
  // only accept if there is no :// part at all
  if(lowcasestrstr(address,"://"))
  {
    debug(6,"get_master_query_type_from_address: invalid");
    return MASTER_INVALID_TYPE;
  }
  else
  {
    debug(6,"get_master_query_type_from_address: default native");
    return MASTER_NATIVE;
  }
}
