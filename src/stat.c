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

#include "gnuconfig.h"

#include <sys/types.h>	/* kill */
#include <stdio.h>	/* FILE, fopen, fclose, fprintf, ... */
#include <string.h>	/* strchr, strcmp, strlen, strcpy, memchr, strtok */
#include <stdlib.h>	/* strtol */
#include <unistd.h>	/* read, close, fork, pipe, exec, fcntl, _exit */
                        /* getpid, unlink, write */
#include <errno.h>      /* errno */
#include <fcntl.h>	/* fcntl */
#include <signal.h>	/* kill, signal... */
#include <time.h>	/* time */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */


#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "stat.h"
#include "utils.h"
#include "server.h"
#include "source.h"
#include "filter.h"
#include "dialogs.h"
#include "host.h"
#include "dns.h"
#include "debug.h"

static const char savage_master_header[5] = { 0x7E, 0x41, 0x03, 0x00, 0x00 };

static void stat_next (struct stat_job *job);
static void parse_qstat_record (struct stat_conn *conn);

typedef void (*server_unref_void)(void*);
typedef void (*userver_unref_void)(void*);

static int failed (char *name, char *arg) {
  fprintf (stderr, "%s(%s) failed: %s\n", name, (arg)? arg : "", 
                                                          g_strerror (errno));

  xqf_error("%s(%s) failed: %s\n", name, (arg)? arg : "", g_strerror (errno));
  
  return TRUE;
}


static void stat_free_conn (struct stat_conn *conn) {
  struct stat_job *job;

  if (!conn || !conn->job)
    return;

  debug (3, "stat_free_conn() -- Conn %lx", conn);

  job = conn->job;

  job->cons = g_slist_remove (job->cons, conn);

  if (conn->fd >= 0) {
    gdk_input_remove (conn->tag);
    close (conn->fd);
  }

  if (conn->pid > 0)
    kill (conn->pid, SIGTERM);

  if (conn->tmpfile) {
    unlink (conn->tmpfile);
    g_free (conn->tmpfile);
  }

  if (conn->buf)
    g_free (conn->buf);

  if (conn->strings)
    g_slist_free (conn->strings);

  if (conn->servers)
    server_list_free (conn->servers);

  if (conn->uservers)
    userver_list_free (conn->uservers);

  g_free (conn);
}


static void stat_update_masters (struct stat_job *job);
static void stat_master_update_done
				    (struct stat_conn *conn,
				     struct stat_job *job,
				     struct master *m,
				     enum master_state state);


// check if master output is in savage format, if yes parse it and return true.
// otherwise return false so the normal parse function can do the work
static gboolean parse_savage_master_output (struct stat_conn *conn)
{
  int res;
  struct stat_job *job = conn->job;

  conn->bufsize = 17;
  debug(3,"conn->first %d",conn->first);
  // check the signature of the first five bytes
  if(conn->first)
  {
    res = read (conn->fd, conn->buf + conn->pos, 5 - conn->pos);
    if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
	return TRUE;
      }
      failed ("read", NULL);
      stat_master_update_done (conn, job, conn->master, SOURCE_ERROR);
      stat_update_masters (job);
      return TRUE;
    }
    if (res == 0) {	/* EOF */
      stat_master_update_done (conn, job, conn->master, SOURCE_UP);
      stat_update_masters (job);
      return TRUE;
    }

    conn->pos += res;

    if(conn->pos < 5) // we need five bytes
      return TRUE;

    conn->first = 0;
    
    // we know we have five bytes, reset buffer for further processing
    conn->pos = 0;

    if(!memcmp(conn->buf,savage_master_header,5))
    {
      debug(3,"detected savage format");
      conn->is_savage = TRUE;
    }
    else
      return FALSE;
  }
  else if(!conn->is_savage)
    return FALSE;

  while(1)
  {
    size_t off = 0;
    res = read (conn->fd, conn->buf + conn->pos, conn->bufsize - conn->pos);
    if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
	return TRUE;
      }
      failed ("read", NULL);
      stat_master_update_done (conn, job, conn->master, SOURCE_ERROR);
      stat_update_masters (job);
      return TRUE;
    }
    if (res == 0) {	/* EOF */
      stat_master_update_done (conn, job, conn->master, SOURCE_UP);
      stat_update_masters (job);
      return TRUE;
    }

    conn->pos += res;

    // we always need six bytes
    for(off=0; off + 6 <= conn->pos; off+=6 )
    {
      struct server *s;
      struct host *h;
      enum server_type type = UNKNOWN_SERVER;
      unsigned char* ip = conn->buf+off;
      unsigned port = 0;
      struct in_addr in;
      port = (ip[5]<<8)+ip[4];
      if(!port) continue;
      if(!ip[0]) continue;
      debug(5,"%hhu.%hhu.%hhu.%hhu:%u",ip[0],ip[1],ip[2],ip[3],port);

      in.s_addr = 0;
      memcpy(&in.s_addr,ip,4);

      if(conn->master)
      {
	type = conn->master->type;
      }

      h = host_add_in (in);
      if (h) {						/* IP address */
	host_ref (h);
	if ((s = server_add (h, port, type)) != NULL) {

	  if(s->type != type )
	  {
	    server_free_info(s);
	    s->type = type;
	  }

	  conn->servers = g_slist_prepend (conn->servers, s);
	  server_ref(s);
	}
	host_unref (h);
      }
    }
    if(off >= conn->pos)
    {
      conn->pos=0;
    }
    else
    {
      memmove (conn->buf, conn->buf + conn->pos - conn->pos%6, conn->pos%6);
      conn->pos = conn->pos%6;
    }
  }
}

/**
  parse qstat output line str, in ip:port format. return true if
  successful, FALSE if server is down or timed out 

  str will be modified!!
 */
static int parse_master_output (char *str, struct stat_conn *conn) {
  char *token[8] = {0};
  int n = 0;
  char *endptr = NULL;
  enum server_type type = UNKNOWN_SERVER;
  char *addr = NULL;
  unsigned short port;
  struct server *s = NULL;
  struct userver *us = NULL;
  struct host *h = NULL;

  debug (6, "parse_master_output(%s,%p)",str,conn);
  n = tokenize_bychar (str, token, 8, QSTAT_DELIM);

  // UGLY HACK UGLY HACK UGLY HACK UGLY HACK
  // output from UT 2003 http server is formatted as
  // ip port gamespy_port
  // not the standard ip:port
  // If the master type is http and there are three columns
  // then it merges ip and port into one so it is handled
  // like other http master servers.
  // This is done by modifying token[0] which points somewhere
  // inside str so malloc/free is not necessary. It is guaranteed
  // that token[0] has enough space as there is at least a
  // whitespace character where the colon is placed now.
  if(conn->master->type == UT2_SERVER && conn->master->master_type == MASTER_HTTP && n == 3)
  {
    int off = strlen(token[0]);
    token[0][off]=':';
    strcpy(token[0]+off+1,token[1]);

    n=1;
  } 

  // output from broadcast, last line contains
  // <servertype> <bcastaddr> <number>
  // this line is skipped by n <= 3
  if(conn->master->master_type == MASTER_LAN)
  {
    if( n <= 3 )
      return TRUE;
    type = id2type (token[0]);
    n = 2; // we only need type and ip
  }
  else if (n >= 3) {

    /* Master address/status */

    strtol (token[2], &endptr, 10);	/* Is it a decimal number? */

    if (endptr == token[2]) {
      if (strcmp (token[2], "DOWN") == 0)
	conn->master->state = SOURCE_DOWN;
      else if (strcmp (token[2], "TIMEOUT") == 0)
	conn->master->state = SOURCE_TIMEOUT;
      else
	conn->master->state = SOURCE_ERROR;
      return FALSE;
    }
  }
  else if (n == 1) {

    n = tokenize_bychar (str, token, 8, QSTAT_MASTER_DELIM);

    switch (n) {

    case 1:
      type = conn->master->type;
      break;

    case 2:
      type = id2type (token[0]);
      break;

    default:
      return TRUE;

    }
  }
  else
  {
    debug(3,"parse_master_output() -- unkown string %s",str);
    return TRUE;
  }

    /* server address is in token[n - 1] */

    g_strchomp (token[n-1]);

    if (type != UNKNOWN_SERVER && parse_address (token[n - 1], &addr, &port)) {
      h = host_add (addr);
      if (h) {						/* IP address */
	host_ref (h);
	if ((s = server_add (h, port, type)) != NULL) {

// doesn't make any sense, doesn't it? -- ln
#if 0
	  if (conn->master && conn->master->type != UNKNOWN_SERVER && 
	                                      s->type != conn->master->type) {
	    s->type = conn->master->type;
	    server_free_info (s);
	  }
#else
	  if(s->type != type )
	  {
	    server_free_info(s);
	    s->type = type;
	  }
#endif
	  

	  /* 
	     o When the "conn" is freed, it will call the function
	     to free the list returned here
	  */
	  //XXX
//	  conn->servers = server_list_prepend (conn->servers, s);
	  conn->servers = g_slist_prepend (conn->servers, s);
	  server_ref(s);
	}
	host_unref (h);
      }
      else {						/* hostname */

	g_strdown (addr);
	if ((us = userver_add (addr, port, type)) != NULL)
	{
//	  conn->uservers = userver_list_add (conn->uservers, us);
	  conn->uservers = g_slist_prepend (conn->uservers, us);
	  userver_ref(us);
	}
      }
      g_free (addr);
    }

  return TRUE;
}


static void stat_master_input_callback (struct stat_conn *conn, int fd, 
                                                GdkInputCondition condition) {
  struct stat_job *job = conn->job;
  int first_used = 0;
  char *tmp;
  int res;

#warning ugly hack for savage, make master handling more generic!
  if(conn->master->type == SAS_SERVER && conn->master->master_type == MASTER_HTTP
	&& parse_savage_master_output(conn))
  {
    return;
  }

  debug_increase_indent();
  debug(3,"stat_master_input_callback(%p,%d,...)",conn,fd);

  while (1) {
    first_used = 0;

    if (conn->pos >= conn->bufsize - 1) {
      xqf_error ("server address string is too long\n");
      stat_master_update_done (conn, job, conn->master, SOURCE_ERROR);
      stat_update_masters (job);
      debug_decrease_indent();
      return;
    }

    res = read (fd, conn->buf + conn->pos, conn->bufsize - conn->pos);
    if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
	debug_decrease_indent();
	return;
      }
      failed ("read", NULL);
      stat_master_update_done (conn, job, conn->master, SOURCE_ERROR);
      stat_update_masters (job);
      debug_decrease_indent();
      return;
    }
    if (res == 0) {	/* EOF */
      debug(3,"stat_master_input_callback -- eof");
      stat_master_update_done (conn, job, conn->master, SOURCE_UP);
      stat_update_masters (job);
      debug_decrease_indent();
      return;
    }

    tmp = conn->buf + conn->pos;
    conn->pos += res;

    while (res && (tmp = memchr (tmp, '\n', res)) != NULL) {
      *tmp++ = '\0';

//      debug(0,"%s",conn->buf + first_used);

      if (!parse_master_output (conn->buf + first_used, conn)) {
	stat_master_update_done (conn, job, conn->master, conn->master->state);
	stat_update_masters (job);
	debug_decrease_indent();
	return;
      }

      first_used = tmp - conn->buf;
      res = conn->buf + conn->pos - tmp;
    }

    if (first_used > 0) {
      if (first_used != conn->pos) {
	memmove (conn->buf, conn->buf + first_used, conn->pos - first_used);
      }
      conn->pos = conn->pos - first_used;
    }
  }
  debug_decrease_indent();
}


static char **parse_serverinfo (char *token[], int n) {
  char **info;
  int size = 0;
  int i, j;
  char *ptr, *info_ptr;

  if (n == 0)
    return NULL;

  for (size = 0, i = 0; i < n; i++) {
    size += strlen (token[i]) + 1;
  }

  info = g_malloc (sizeof (char *) * (n + 1) * 2 + size);
  info_ptr = (char *) info + sizeof (char *) * (n + 1) * 2;

  for (i = 0, j = 0; i < n; i++) {
    ptr = strchr (token[i], '=');

    if (ptr)
      *ptr++ = '\0';

    info [j++] = strcpy (info_ptr, token[i]);
    info_ptr += strlen (token[i]) + 1;

    if (ptr) {
      info [j++] = strcpy (info_ptr, ptr);
      info_ptr += strlen (ptr) + 1;
    }
    else {
      info [j++] = NULL;
    }
  }

  info [j + 1] = info [j] = NULL;

  return info;
}


static struct server *parse_server (char *token[], int n, time_t refreshed,
                                                                  int saved) {
  struct host *h;
  struct server *server;
  enum server_type type;
  char *addr;
  unsigned short port;

  if (n < 3)
    return NULL;

  type = id2type (token[0]);

  if (type == UNKNOWN_SERVER)
    return NULL;

  if (!parse_address (token[1], &addr, &port))
    return NULL;

  h = host_add (addr);
  g_free (addr);
  if (!h)
    return NULL;

  server = server_add (h, port, type);

  debug (6, "server %lx retreived", server);

  server->flt_mask &= ~FILTER_SERVER_MASK;

  server->refreshed = refreshed;

  if (n == 3) {
    if (strcmp (token[2], "DOWN") == 0) {
      server->retries = MAX_RETRIES;
      server->ping = MAX_PING + 1;	/* DOWN */
    }
    else {
      server->retries = MAX_RETRIES;
      server->ping = MAX_PING;	/* TIMEOUT */
    }

    if (server->players) {
      g_slist_foreach (server->players, (GFunc) g_free, NULL);
      g_slist_free (server->players);
      server->players = NULL;
      server->flags &= ~PLAYER_GROUP_MASK;
      server->flt_mask &= ~FILTER_PLAYER_MASK;
    }
    server->curplayers = 0;
    server->curbots = 0;

  }
  else {
    /* We did get some information */
    if (server->type == QW_SERVER || type == QW_SERVER)
      server->type = type;	/* the real type of server (QW <-> Q2) */

    if (games[server->type].parse_server) {
      /* 
	 We have a function to parse the server information,
	 so first we free all of the data elements of this
	 structure but not the structure its self.  This
	 is because the *_analyse functions should assign values
	 to each of the elemets.  
      */
      server_free_info (server);

      server->retries = MAX_RETRIES;
      server->ping = MAX_PING;	/* TIMEOUT */

      /* Actually parse the info with the given function.  The mapping
	 and functions are in game.c  
      */
      (*games[server->type].parse_server) (token, n, server);

      if (server->ping > MAX_PING) {
	if (!saved || server->ping != MAX_PING + 1)
	  server->ping = MAX_PING;
      }

      if (server->retries > maxretries)
	server->retries = maxretries;
    }

#ifdef USE_GTK2
    if(server->name && !g_utf8_validate(server->name,-1,NULL))
    {
      // don't care about conversion errors
      char* convd = g_convert(server->name,-1,"UTF-8","iso-8859-1",NULL,NULL,NULL);
      if(convd)
      {
	g_free(server->name);
	server->name = convd;
      }
    }
#endif
  }

  if(!saved && server->ping<MAX_PING)
  {
    server->last_answer = server->refreshed;
  }

  return server;
}

/** teamnames must be static storage, pointers to it will not be freed */
static void q3parseteams(struct server* s,
    const unsigned numteams,
    char** playerteamrules,// which rule holds the numbers that indicate which player is in what team
    char** teamnames,      // default names for team
    char** teamnamesrules) // which rules hold the team names
{
    // bitmask of players for each team
    long* teams = NULL;
    char **info_ptr = NULL;
    unsigned i = 0, n = 0;
    unsigned team = numteams;
    GSList *plist = NULL;
    struct player *p  = NULL;
    char** teamnames_fromrules = NULL;

    if(!s || !playerteamrules || numteams<2) return;

    teams = g_malloc0(sizeof(long)*numteams);
    if(!teams) return;

    teamnames_fromrules = g_malloc0(sizeof(char*)*numteams);
    if(!teamnames_fromrules) return;

    for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2)
    {
      for(i = 0; i < numteams; ++i)
      {
	if (strcmp (*info_ptr, playerteamrules[i]) == 0)
	{
	  team = i;
	  break;
	}
	if (teamnamesrules && teamnamesrules[i] &&strcmp (*info_ptr, teamnamesrules[i]) == 0)
	{
	  teamnames_fromrules[i]=info_ptr[1];
	}
      }

      if(team != numteams )
      {
	char* e = NULL;
	char* p = info_ptr[1];
	for(;; p = e, e = NULL)
	{
	  long pnr = strtol(p,&e,10);
	  if(p == e || (e && !*e))
	    break;
	  if(pnr != LONG_MIN && pnr != LONG_MAX
	      && pnr <= (long)(sizeof(teams[team])*8)
	      && pnr > 0)
	  {
	    teams[team] |= 2<<(pnr-1);
	  }
	}
	team = numteams;
      }
    }
    for(plist = s->players, n = 0 ; plist ; plist=plist->next, ++n)
    {
      for(team=0;team != numteams; ++team)
      {
	if(teams[team]&(2<<n))
	{
	  p = plist->data;
	  if(teamnames_fromrules[team])
	    p->model = teamnames_fromrules[team];
	  else
	    p->model = teamnames[team];
	}
      }
    }

    g_free(teams);
    g_free(teamnames_fromrules);
}

static void parse_qstat_record_part2 (GSList *strings, struct server *s) {
  int n;
  char *token[256];
  struct player *p;
  GSList *plist = NULL;

  if (!strings) {
    if (games[s->type].analyze_serverinfo)
      (*games[s->type].analyze_serverinfo) (s);
    return;
  }

  n = tokenize_bychar ((char *) strings->data, token, 256, QSTAT_DELIM);
  if (n == 0)
    return;

  s->info = parse_serverinfo (token, n);

  if (games[s->type].analyze_serverinfo)
    (*games[s->type].analyze_serverinfo) (s);

  strings = strings->next;

  if (strings && games[s->type].parse_player) {
    while (strings) {
      n = tokenize_bychar ((char *) strings->data, token, 256, QSTAT_DELIM);
      p = (*games[s->type].parse_player) (token, n, s);
      if (!p)		/* error, try to recover */
	return;

      plist = g_slist_prepend (plist, p);

      strings = strings->next;
    }
    s->players = g_slist_reverse (plist);
  }

  if(s->type == WO_SERVER || s->type == WOET_SERVER)
  {
    static char* teamnames[2] = { N_("Allies"), N_("Axis") };
    static char* playerteamrules[2] = { "Players_Allies", "Players_Axis" };
    q3parseteams(s,2,playerteamrules,teamnames,NULL);
  }
  else if(s->type == Q3_SERVER)
  {
    static char* teamnames[2] = { N_("Red"), N_("Blue") };
    static char* playerteamrules[2] = { "Players_Red", "Players_Blue" };
    static char* teamnamesrules[2] = { "g_TeamRed", "g_TeamBlue" };
    q3parseteams(s,2,playerteamrules,teamnames,teamnamesrules);
  }

}

static void parse_qstat_record (struct stat_conn *conn) {
  struct server *server;
  char *token[16];
  int n;
  GSList *list;
  struct stat_job *job;

  if (!conn || !conn->strings)
    return;	/* error, try to recover */

  job = conn->job;

  /* Debug before tokenizing. */
  debug (4, "parse_qstat_record() -- Conn %lx: %s", conn, conn->strings->data);

  n = tokenize_bychar ((char *) conn->strings->data, token, 16, QSTAT_DELIM);
  if (n < 3)
    return;     /* error, try to recover */

  server = parse_server (token, n, time (NULL), FALSE);
  if (server) {
    job->need_redraw = TRUE;

    /*
      o The list here is freed when the job is freed.
      o The parse_server call above increments the ref
      count as does the prepend below. So we need to decrement it
      one just for fun.
    */
    job->delayed.queued_servers = 
      server_list_prepend (job->delayed.queued_servers, server);
    job->progress.done++;
    server_unref (server);

    debug (6, "Server %lx in delayed list %lx.", server, job->delayed.queued_servers);

    parse_qstat_record_part2 (conn->strings->next, server);
    
    for (list = job->server_handlers; list; list = list->next)
      (* (server_func) list->data) (job, server);
  }
}


void parse_saved_server (GSList *strings) {
  struct server *server;
  char *token[16];
  int n;
  time_t refreshed = 0;
  time_t last_answer = 0;
  char *endptr;
  char** refreshdata = NULL;

  if (!strings || !strings->next)
    return;

  // first line contains seconds of last refresh and last answer
  refreshdata = g_strsplit(strings->data," ",2);

  if(!refreshdata || !refreshdata[0])
  {
    debug(0,"refreshdata empty");
    return;
  }

  refreshed = strtoul (refreshdata[0], &endptr, 10);

  if (endptr == refreshdata[0])	/* It's not a number */
    return;

  if(refreshdata[1]) // post 0.9.10 format
  {
    last_answer = strtoul (refreshdata[1], &endptr, 10);

    if (endptr == refreshdata[1])	/* It's not a number */
      return;
  }

  g_strfreev(refreshdata);
  refreshdata = NULL;

  strings = strings->next;

  if(!strings) return;

  n = tokenize_bychar ((char *) strings->data, token, 16, QSTAT_DELIM);
  if (n < 3)
    return;

  server = parse_server (token, n, refreshed, TRUE);
  // unref newly created server since it is already referenced once
  // if it was in lists.gz. If it was not already referenced it will
  // be freed and does not stay stay around in memory. This way old
  // servers will not pile up in srvinfo.gz
  server = server_unref(server);

  if (server) {
    server_ref (server);
    parse_qstat_record_part2 (strings->next, server);
    server->last_answer=last_answer;
    server_unref (server);
  }
}


static void adjust_pointers (GSList *list, gpointer new, gpointer old) {
  while (list) {
    list->data = (gpointer) ((char *)new + ((char *)list->data - (char *)old));
    list = list->next;
  }
}


static void stat_servers_update_done (struct stat_conn *conn) {
  debug (3, "stat_servers_update_done() -- Conn %lx  server list %lx", conn, conn->job->servers);
  server_list_free (conn->job->servers);
  conn->job->servers = NULL;
  stat_free_conn (conn);
}



/* 
   stat_server_input_callback -- as data is returned from the qstat
   process, this gets called.  Sometimes there are multiple lines
   so the results have to be looped over.
*/
static void stat_servers_input_callback (struct stat_conn *conn, int fd, 
                                                GdkInputCondition condition) {
  struct stat_job *job = conn->job;
  int first_used = 0;
  int blocked = FALSE;
  char *tmp;
  int res;
  /* debug (3, "stat_servers_input_callback() -- Conn %lx", conn); */
  while (1) {
    first_used = 0;
    blocked = FALSE;

    if (conn->bufsize - conn->pos < BUFFER_TRESHOLD) {
      if (conn->bufsize >= BUFFER_MAXSIZE) {
	fprintf (stderr, "server record is too large\n");
	stat_servers_update_done (conn);
	stat_next (job);
	return;
      }
      conn->bufsize += conn->bufsize;
      tmp = conn->buf;
      conn->buf = g_realloc (conn->buf, conn->bufsize);
      adjust_pointers (conn->strings, conn->buf, tmp);
    }

    res = read (fd, conn->buf + conn->pos, conn->bufsize - conn->pos);
    if (res < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	return;
      failed ("read", NULL);
      stat_servers_update_done (conn);
      stat_next (job);
      return;
    }
    if (res == 0) {	/* EOF */
      debug (3, "Conn %ld  Sub Process Done with server list %lx", conn, conn->job->servers);
      stat_servers_update_done (conn);
      stat_next (job);
      return;
    }

    tmp = conn->buf + conn->pos;
    conn->pos += res;

    while (res && (tmp = memchr (tmp, '\n', res)) != NULL) {
      *tmp++ = '\0';

      if (conn->buf[conn->lastnl] == '\0') {
	blocked = TRUE;
	gdk_input_remove (conn->tag);

	parse_qstat_record (conn);

	g_slist_free (conn->strings);
	conn->strings = NULL;

	first_used = conn->lastnl + 1;
      }
      else {
	conn->strings = g_slist_append (conn->strings, 
                                                    conn->buf + conn->lastnl);
      }

      conn->lastnl = tmp - conn->buf;
      res = conn->buf + conn->pos - tmp;
    }

    if (first_used) {
      if (first_used == conn->pos) {
	conn->pos = 0;
	conn->lastnl = 0;
      }
      else {
	tmp = conn->buf + first_used;
	g_memmove (conn->buf, conn->buf + first_used, conn->pos - first_used);
	conn->pos -= first_used;
	conn->lastnl -= first_used;
	adjust_pointers (conn->strings, conn->buf, tmp);
      }
    }

    if (blocked) {
      conn->tag = gdk_input_add (conn->fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION,
                                                  conn->input_callback, conn);
    }

  }
}


static void set_nonblock (int fd) {
  int flags;

  flags = fcntl (fd, F_GETFL, 0);
  if (flags < 0 || fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    failed ("fcntl", NULL);
    return;
  }
}


/**
  return connection to local file
 */
static struct stat_conn *new_file_conn (struct stat_job *job, const char* file, 
                          GdkInputFunction input_callback, struct master *m)
{
    struct stat_conn *conn;
    char *file2;
    
    int fd = -1;

    file2 = expand_tilde(file);
        
    fd = open(file2,O_RDONLY);
    if(fd == -1)
    {
	perror(__FUNCTION__);
	return NULL;
    }

    conn = g_malloc (sizeof (struct stat_conn));
    if(!conn)
	return NULL;

    conn->buf = g_malloc (BUFFER_MINSIZE);
    conn->bufsize = BUFFER_MINSIZE;
    conn->tmpfile = NULL;
    conn->pid = 0;
    conn->fd = fd; 
    conn->pos = 0;
    conn->lastnl = 0;
    conn->first = TRUE;
    conn->is_savage = FALSE;

    conn->strings = NULL;
    conn->servers = NULL;
    conn->uservers = NULL;

    conn->master = m;

    conn->job = job;
    job->cons = g_slist_prepend (job->cons, conn);

    conn->tag = gdk_input_add (conn->fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, 
				     (GdkInputFunction) input_callback, conn);
    conn->input_callback = (GdkInputFunction) input_callback;

    if (file2)
      g_free(file2);

    return conn;
}

/*
  start_qstat -- Fork and run qstat with the given command line
  options.  Returns a "conn?"
*/
static struct stat_conn *start_qstat (struct stat_job *job, char *argv[], 
                          GdkInputFunction input_callback, struct master *m) {
 
  struct stat_conn *conn;
  pid_t pid;
  int pipefds[2];
  const char error_msg[] = QSTAT_DELIM_STR 
                           QSTAT_DELIM_STR 
                           "ERROR" QSTAT_DELIM_STR 
                           "command not found\n";

  debug (3, "start_qstat() -- Job %lx  Setting up/forking pipes to qstat", job);
  debug_cmd (3, argv, "start_qstat() -- Job %lx", job);

  if (pipe (pipefds) < 0) {
    failed ("pipe", NULL);
    return NULL;
  }

  pid = fork ();
  if (pid < (pid_t) 0) {
    failed ("fork", NULL);
    return NULL;
  }

  if (pid) {	/* parent */
    close (pipefds[1]);

    set_nonblock (pipefds[0]);

    conn = g_malloc (sizeof (struct stat_conn));
    conn->buf = g_malloc (BUFFER_MINSIZE);
    conn->bufsize = BUFFER_MINSIZE;
    conn->tmpfile = NULL;
    conn->pid = pid;
    conn->fd = pipefds[0];
    conn->pos = 0;
    conn->lastnl = 0;
    conn->first = TRUE;
    conn->is_savage = FALSE;

    conn->strings = NULL;
    conn->servers = NULL;
    conn->uservers = NULL;

    conn->master = m;

    conn->job = job;
    job->cons = g_slist_prepend (job->cons, conn);

    conn->tag = gdk_input_add (conn->fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, 
                                     (GdkInputFunction) input_callback, conn);
    conn->input_callback = (GdkInputFunction) input_callback;
  }
  else {	/* child */
    close (pipefds[0]);
    dup2 (pipefds[1], 1);
    close (pipefds[1]);

    execvp (argv[0], argv);

    failed ("execvp", argv[0]);

    write (1, error_msg, sizeof (error_msg) - 1);

    _exit (1);
  }

  return conn;
}


static void stat_close (struct stat_job *job, int killed) {
  GSList *tmp;

  dns_set_callback (NULL, NULL);
  dns_cancel_requests ();
  debug (3, "stat_close() -- Job %lx  Killed? %d", job, killed);

  while (job->cons)
    stat_free_conn ((struct stat_conn *) job->cons->data);

  if (job->delayed.refresh_handler)
    gtk_timeout_remove (job->delayed.timeout_id);

  for (tmp = job->close_handlers; tmp; tmp = tmp->next)
    (* (close_func) tmp->data) (job, killed);

  stat_job_free (job);

  if(event_type == EVENT_REFRESH_SELECTED) {
    debug (1, "refresh selected done\n");
  }
  if(event_type == EVENT_REFRESH) {
    debug (1, "refresh done.\n");
    play_sound(sound_refresh_done, 0);
  }
  if(event_type == EVENT_UPDATE) {
    debug (1, "update done.\n");
    play_sound(sound_update_done, 0);
  }
  event_type=0;
}


static const char delim[] = " \t\n\r";


static struct stat_conn *stat_update_master_qstat (struct stat_job *job, 
                                                           struct master *m) {
  char *argv[16];
  int argi = 0;
  char buf1[64];
  char* arg_type = NULL;
  char buf3[64];
  char buf_rawarg[] = { QSTAT_DELIM, '\0' };
  struct stat_conn *conn = NULL;
  char *cmd = NULL;
  char *file = NULL;

  short startprog = 1;

  debug (3, "stat_update_master_qstat(%p,%p)", job, m);
  if (!m)
    return NULL;

  if (m->url) {

    if(!strncmp(m->url,master_prefixes[MASTER_FILE],strlen(master_prefixes[MASTER_FILE])))
    {
      startprog = 0;
      file=strdup_strip(m->url + strlen(master_prefixes[MASTER_FILE]));
    }
    else
    {
      cmd = strdup_strip (HTTP_HELPER);

      argv[argi++] = strtok (cmd, delim);
      while ((argv[argi] = strtok (NULL, delim)) != NULL)
	argi++;

      argv[argi++] = m->url;
      argv[argi] = NULL;
    }
  }
  else {
    
    if (m->master_type!=MASTER_LAN && !master_qstat_option(m))
      return NULL;

    argv[argi++] = QSTAT_EXEC;
    argv[argi++] = "-errors";

    if( access(qstat_configfile, R_OK) == 0 )
    {
	argv[argi++] = "-cfg";
	argv[argi++] = qstat_configfile;
    }

    argv[argi++] = "-raw";
    argv[argi++] = buf_rawarg;

    argv[argi++] = "-retry";
    argv[argi++] = buf1;
    g_snprintf (buf1, 64, "%d", maxretries + 2);

    if(m->master_type==MASTER_LAN)
    {
      debug (3, "stat_update_master_qstat() -- MASTER_LAN");
      arg_type = g_strdup_printf("%s,outfile", games[m->type].qstat_option);
    }
    else if (m->master_type == MASTER_GAMESPY)
    {
    	arg_type = g_strdup_printf("-gsm,%s,outfile", games[m->type].qstat_str);
    }
    // add master arguments
    else if( games[m->type].flags & GAME_QUAKE3_MASTERPROTOCOL )
    {
      // TODO: master protocol should be server specific
      char* masterprotocol = g_datalist_get_data(&games[m->type].games_data,"masterprotocol");

      if(masterprotocol)
	arg_type = g_strdup_printf("%s,%s,outfile", master_qstat_option(m),masterprotocol);
      else
      {
	xqf_warning("GAME_QUAKE3_MASTERPROTOCOL flag set, but no protocol specified");
	arg_type = g_strdup_printf("%s,outfile", master_qstat_option(m));
      }
    }
    else if( m->type == HL_SERVER && current_server_filter > 0 && (cur_filter & FILTER_SERVER_MASK))
    {
      struct server_filter_vars* filter =
	g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1);

      if(!filter)
      {
	xqf_error("filter is NULL");
	return NULL;
      }

      arg_type = g_strconcat(
	  master_qstat_option(m),
	  ",outfile",
	  (filter->game_contains&&*filter->game_contains)?",game=":"",
	  (filter->game_contains&&*filter->game_contains)?filter->game_contains:"",
	  (filter->map_contains&&*filter->map_contains)?",map=":"",
	  (filter->map_contains&&*filter->map_contains)?filter->map_contains:"",
	  (filter->filter_not_empty||filter->filter_not_full)?",status=":"",
	  filter->filter_not_empty?"notempty":"",
	  (filter->filter_not_empty&&filter->filter_not_full)?":":"",
	  filter->filter_not_full?"notfull":"",
	  NULL);
    }
    else
    {
      arg_type = g_strdup_printf ("%s,outfile", master_qstat_option(m));
    }

    
    argv[argi++] = arg_type;

    argv[argi++] = buf3;
    g_snprintf (buf3, 64, "%s%s:%d,-", m->master_type==MASTER_LAN?"+":"" ,inet_ntoa (m->host->ip), m->port);

    argv[argi] = NULL;

  }	/*  if (m->url)  */

  if(argi >= sizeof(argv))
    xqf_error("FIXME: argi too big, stack corrupt");

  if(startprog)
  {
    if (get_debug_level() > 3){
      char **argptr = argv;
      fprintf (stderr, "stat_update_master_qstat: EXEC> ");
      while (*argptr)
	fprintf (stderr, "%s ", *argptr++);
      fprintf (stderr, "\n");
    }

    conn = start_qstat (job, argv, 
			      (GdkInputFunction) stat_master_input_callback, m);
  }
  else if(file)
  {
    conn = new_file_conn (job, file, (GdkInputFunction) stat_master_input_callback, m);
    g_free (file);
  }
  
  g_free (cmd);
  g_free (arg_type);

  return conn;
}


#define MAX_SERVERS_IN_CMDLINE	8

static struct stat_conn *stat_open_conn_qstat (struct stat_job *job) {
  struct server *s = NULL;
  char *argv[16 + MAX_SERVERS_IN_CMDLINE*2];
  int argi = 0;
  char buf[64 + MAX_SERVERS_IN_CMDLINE*64];
  int bufi = 0;
  char *fn = NULL;
  GSList *tmp;
  struct stat_conn *conn;

  if (!job->servers)
    return NULL;

  /*
    The g_slist_reverse does not allocate any new 
    lists or memory.  However, it means that job->servers
    will point to a different member.
  */
  debug (6, "stat_open_conn_qstat() -- server list was %lx", job->servers );
  job->servers = g_slist_reverse (job->servers);
  debug (6, "stat_open_conn_qstat() -- server list now %lx", job->servers );

  argv[argi++] = QSTAT_EXEC;
  argv[argi++] = "-errors";
  
  if( access(qstat_configfile, R_OK) == 0 )
  {
    argv[argi++] = "-cfg";
    argv[argi++] = qstat_configfile;
  }


  argv[argi++] = "-maxsimultaneous";
  argv[argi++] = &buf[bufi];
  bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%d", 
                                                             maxsimultaneous);
  argv[argi++] = "-retry";
  argv[argi++] = &buf[bufi];
  bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%d", maxretries);
  
  argv[argi++] = "-raw";
  argv[argi++] = &buf[bufi];
  bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%c", QSTAT_DELIM);

  argv[argi++] = "-R";

  argv[argi++] = "-P";

  if (g_slist_length (job->servers) <= MAX_SERVERS_IN_CMDLINE) {
    for (tmp = job->servers; tmp; tmp = tmp->next) {
      s = (struct server *) tmp->data;

      if (games[s->type].qstat_option)
	argv[argi++] = games[s->type].qstat_option;

      argv[argi++] = &buf[bufi];
      bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%s:%d",
                                            inet_ntoa (s->host->ip), s->port);
    }
  }
  else
  {
    int fd = -1;
    FILE* f = NULL;

    argv[argi++] = "-f";
    fn = &buf[bufi];
    bufi += 1 + g_snprintf(fn, sizeof (buf) - bufi,
		  "%s/xqf-qstat.XXXXXX", g_get_tmp_dir ());

    argv[argi++] = fn;
    
    fd = mkstemp(fn);

    if (fd < 0 || !(f = fdopen(fd, "w"))) {
      dialog_ok (NULL, _("Failed to create a temporary file %s: %s"), fn, strerror(errno));
      return NULL;
    }

    for (tmp =job-> servers; tmp; tmp = tmp->next) {
      s = (struct server *) tmp->data;
      fprintf (f, "%s %s:%d\n", games[s->type].qstat_str,
                                            inet_ntoa (s->host->ip), s->port);
    }

    fclose (f);
  }

  argv[argi] = NULL;

  if(argi >= sizeof(argv))
    xqf_error("FIXME: argi too big, stack corrupt");

  conn = start_qstat (job, argv, 
                        (GdkInputFunction) stat_servers_input_callback, NULL);
  if (conn && fn)
    conn->tmpfile = g_strdup (fn);

  return conn;
}


// compare pointers
static gint compare_ptr (gconstpointer s1, gconstpointer s2)
{
  if(s1==s2)
    return 0;
  else if(s1<s2)
    return -1;
  else
    return 1;
}

static void stat_master_update_done (struct stat_conn *conn,
				     struct stat_job *job,
				     struct master *m,
				     enum master_state state) {
  GSList *tmp;

  m->state = state;

  debug (1, "stat_master_update_done(%s) -- status %d\n", 
                                (conn)? conn->master->name : "(null)", state);

  if (state == SOURCE_UP && conn) {
    debug (3, "stat_master_update_done -- state == SOURCE_UP && conn");
    server_list_free (m->servers);
//    m->servers = g_slist_reverse (conn->servers);
    m->servers = slist_sort_remove_dups(conn->servers,compare_ptr,(server_unref_void)server_unref);
    conn->servers = NULL;

    userver_list_free (m->uservers);
//    m->uservers = g_slist_reverse (conn->uservers);
    m->uservers = slist_sort_remove_dups(conn->uservers,compare_ptr,(userver_unref_void)userver_unref);
    conn->uservers = NULL;

    if (default_refresh_on_update)
      job->need_refresh = TRUE;
  }

  if (conn)
    stat_free_conn (conn);

  job->need_redraw = TRUE;
  /*
  job->delayed.queued_servers = server_list_append_list (
                     job->delayed.queued_servers, m->servers, UNKNOWN_SERVER);

  job->servers = server_list_append_list (job->servers, m->servers,
                                                              UNKNOWN_SERVER);
  job->names = userver_list_append_list (job->names, m->uservers,
                                                              UNKNOWN_SERVER);
  */
  for (tmp = m->servers; tmp; tmp = tmp->next)
  {
    struct server* s = tmp->data;
    job->delayed.queued_servers = g_slist_prepend(job->delayed.queued_servers, s);
    job->servers = g_slist_prepend(job->servers, s);
    server_ref(s);
    server_ref(s);
  }
  for (tmp = m->uservers; tmp; tmp = tmp->next)
  {
    struct userver *us = tmp->data;
    job->names = g_slist_prepend(job->names,us);
    userver_ref(us);
  }

  job->delayed.queued_servers = slist_sort_remove_dups(job->delayed.queued_servers,
      compare_ptr,(server_unref_void)server_unref);
  
  job->servers = slist_sort_remove_dups(job->servers,compare_ptr,(server_unref_void)server_unref);
  
  job->names = slist_sort_remove_dups(job->names,compare_ptr,(userver_unref_void)userver_unref);

  debug (3, "stat_master_update_done -- job->master_handlers = %p",job->master_handlers);
  for (tmp = job->master_handlers; tmp; tmp = tmp->next)
    (* (master_func) tmp->data) (job, m);

  job->progress.done++;

  if (m->type == Q2_SERVER && !m->url)
    job->q2_masters--;
}


static void stat_update_masters (struct stat_job *job) {
  struct master *m;
  GSList *tmp;
  int freecons;

  freecons = maxsimultaneous - job->masters_to_resolve - 
                                                   g_slist_length (job->cons);

  debug (3, "stat_update_masters(%p) -- freecons: %d", job,freecons);
  tmp = job->masters;

  while (tmp && freecons > 0) {
    m = (struct master *) tmp->data;

    if (m->host || m->url) {

      if (m->type == Q2_SERVER && !m->url) {

	/*
         *  QStat binds himself to fixed port (26500) to access Q2 masters.
	 *  It's impossible to run several QStat programs concurently for
         *  Q2 masters. 
	 */

	if (job->q2_masters > 0) {
	  tmp = tmp->next;
	  continue;
	}
	job->q2_masters++;
      }

      tmp = job->masters = g_slist_remove (job->masters, m);

      if (!stat_update_master_qstat (job, m))
	stat_master_update_done (NULL, job, m, SOURCE_ERROR);
      else
	freecons--;

      continue;
    }

    tmp = tmp->next;
  }

  if (job->masters == NULL && job->cons == NULL)
    stat_next (job);
}


static void stat_master_resolved_callback (char *id, struct host *h,
                                         enum dns_status status, void *data) {
  struct stat_job *job = (struct stat_job *) data;
  struct master *m;
  GSList *list;
  enum master_state state;

  debug (3, "stat_master_resolved_callback(%s,%p,%d,%p)", id, h,status,data);

  if (!job || !id)
    return;

  job->masters_to_resolve--;
  job->progress.done++;
                                               
  list = job->masters;

  while (list) {
    m = (struct master *) list->data;

    if (m->host == NULL && m->hostname && strcmp (m->hostname, id) == 0) {
      if (h) {
	m->host = h;
	host_ref (h);
      }
      else {
	list = job->masters = g_slist_remove (job->masters, m);

        if (status == DNS_STATUS_TIMEOUT || status == DNS_STATUS_ERROR)
	  state = SOURCE_TIMEOUT;
	else				/* DNS_STATUS_NOTFOUND, etc... */
	  state = SOURCE_ERROR;

	stat_master_update_done (NULL, job, m, state);
	continue;
      }
    }
    list = list->next;
  }

  stat_update_masters (job);
}


// ip for hostname resolved
static void stat_name_resolved_callback (char *id, struct host *h,
                                         enum dns_status status, void *data) {
  struct stat_job *job = (struct stat_job *) data;
  struct userver *us;
  GSList *list;
  GSList *tmp;

  debug (6, "%s,%p,%d,%p",id,h,status,data);

  if (!job || !id)
    return;

  list = job->names;
  while (list) {
    us = (struct userver *) list->data;
    if (strcmp (us->hostname, id) == 0) {
      if (h)
	userver_set_host (us, h);

      /* automatically add it to the list of servers to refresh */

      if (us->s) {

	if (us->s->type != us->type) {
	  us->s->type = us->type;
	  server_free_info (us->s);
	}

	/* 
	   o When the job is freed, the list will
	   be freed as well. This will take care of
	   the reference counting.
	*/
	job->servers = server_list_prepend (job->servers, us->s);
      }

      for (tmp = job->name_handlers; tmp; tmp = tmp->next)
	(* (name_func) tmp->data) (job, us, status);

      // TODO optimizable, no need to start from start of list. on the other
      // hand, it's unlikely that the list is big anyway...
      list = job->names = g_slist_remove (job->names, us);
      userver_unref (us);
      continue;
    }
    list = list->next;
  }

  if (job->names == NULL) {
    dns_set_callback (NULL, NULL);
    stat_next (job);
  }
}


static void stat_host_resolved_callback (char *id, struct host *h,
                                         enum dns_status status, void *data) {
  struct stat_job *job = (struct stat_job *) data;
  GSList *tmp;

  if (!job || !id || !h)
    return;

  if (status == DNS_STATUS_OK) {
    job->need_redraw = TRUE;
    job->delayed.queued_hosts = host_list_add (job->delayed.queued_hosts, h);
    job->progress.done++;
  }

  for (tmp = job->host_handlers; tmp; tmp = tmp->next)
    (* (host_func) tmp->data) (job, h, status);

  job->hosts = g_slist_remove (job->hosts, h);
  host_unref (h);

  if (job->hosts == NULL) {
    dns_set_callback (NULL, NULL);
    stat_next (job);
  }
}


static void change_state (struct stat_job *job, enum stat_state state) {
  GSList *tmp;

  for (tmp = job->state_handlers; tmp; tmp = tmp->next)
    (* (state_func) tmp->data) (job, state);
}


static void move_q2masters_to_top (GSList **list) {
  GSList *q2masters = NULL;
  GSList *tmp = *list;
  struct master *m;

  while (tmp) {
    m = (struct master *) tmp->data;
    tmp = tmp->next;

    if (!m->url && m->type == Q2_SERVER) {
      *list = g_slist_remove (*list, m);
      q2masters = g_slist_append (q2masters, m);
    }
  }

  *list = g_slist_concat (q2masters, *list);
}

static void stat_next_masters (struct stat_job *job)
{
  GSList *list = NULL;
  GSList *tmp = NULL;
  GSList *hostnames = NULL;
  struct master *m = NULL;

  debug_increase_indent();
  debug (3, "Job %lx  Have job->masters", job );
  job->state = STAT_UPDATE_SOURCE;

  move_q2masters_to_top (&job->masters);

  // store all unresolved hostnames in hostnames
  for (list = job->masters; list; list = list->next) {
    m = (struct master *) list->data;

    if (!m->host && m->hostname) {
      for (tmp = hostnames; tmp; tmp = tmp->next) {
	if (strcmp (tmp->data, m->hostname) == 0)
	  break;
      }
      if (tmp == NULL) {
	hostnames = g_slist_prepend (hostnames, m->hostname);
      }
    }
  }

  job->masters_to_resolve = g_slist_length (hostnames);
  job->progress.tasks = -1;
  change_state (job, STAT_UPDATE_SOURCE);

  if (hostnames) {
    dns_set_callback (stat_master_resolved_callback, job);

    for (list = hostnames; list; list = list->next)
      dns_lookup ((char *) list->data);

    g_slist_free (hostnames);
    hostnames = NULL;
  }

  stat_update_masters (job);
  debug_decrease_indent();
  return;
}

static void stat_next_names (struct stat_job *job)
{
  GSList *list = NULL;
  GSList *tmp = NULL;
  GSList *hostnames = NULL;
  struct userver *us = NULL;

  debug (3, "Job %lx  job->names", job );
  job->state = STAT_RESOLVE_NAMES;

  for (list = job->names; list; list = list->next) {
    us = (struct userver *) list->data;

    for (tmp = hostnames; tmp; tmp = tmp->next) {
      if (strcmp (tmp->data, us->hostname) == 0)
	break;
    }
    if (tmp == NULL) {
      hostnames = g_slist_prepend (hostnames, us->hostname);
    }
  }

  dns_set_callback (stat_name_resolved_callback, job);
  job->progress.tasks = g_slist_length (hostnames);
  change_state (job, STAT_RESOLVE_NAMES);
  
  for (list = hostnames; list; list = list->next)
    dns_lookup ((char *) list->data);

  g_slist_free (hostnames);
  hostnames = NULL;

  return;
}

static void stat_next_servers (struct stat_job *job)
{
  debug (3, "Servers:  Job %lx  server list %lx", job, job->servers );
  if (!job->need_refresh) {
    stat_close (job, FALSE);
    return;
  }

  job->state = STAT_REFRESH_SERVERS;

  if (default_resolve_on_update)
    job->hosts = merge_hosts_to_resolve (job->hosts, job->servers);

  job->progress.tasks = g_slist_length (job->servers);
  change_state (job, STAT_REFRESH_SERVERS);

  if (!stat_open_conn_qstat (job)) {

    /* It's very bad, stop everything. */
    xqf_error ("Error! Could not stat_open_conn_qstat()");
    stat_close (job, TRUE);
  }
  return;
}

static void stat_next_hosts (struct stat_job *job)
{
  GSList *list = NULL;
  struct host *h = NULL;

  debug (3, "Job %p job->hosts", job);

  job->state = STAT_RESOLVE_HOSTS;

  dns_set_callback (stat_host_resolved_callback, job);

  job->progress.tasks = g_slist_length (job->hosts);
  change_state (job, STAT_RESOLVE_HOSTS);

  for (list = job->hosts; list; list = list->next) {
    h = (struct host *) list->data;
    if (h) dns_lookup (inet_ntoa (h->ip));
  }

  return;
}

static void stat_next (struct stat_job *job)
{
  debug_increase_indent();
  debug (3, "Job %p",job);
  job->progress.done = 0;

  if (job->masters) {
    stat_next_masters(job);
  }
  else if (job->names) {
    stat_next_names(job);
  }
  else if (job->servers) {
    stat_next_servers(job);
  }
  else if (job->hosts) {
    stat_next_hosts(job);
  }
  else {
    debug (3, "Job %p Done, Closing the job...", job);
    stat_close (job, FALSE);
  }
  debug_decrease_indent();
}

void stat_start (struct stat_job *job) {

  debug_increase_indent();
  debug (3, "Job %p", job);
  if (job->delayed.refresh_handler) {
    job->delayed.timeout_id = gtk_timeout_add (1000, 
                                           job->delayed.refresh_handler, job);
  }

  stat_next (job);
  debug_decrease_indent();
}


void stat_stop (struct stat_job *job)
{
  debug (3, "Job %lx", job);
  stat_close (job, TRUE);
}


struct stat_job *stat_job_create (GSList *masters, GSList *names, 
                                             GSList *servers, GSList *hosts) {
  struct stat_job *job;

  job = g_malloc (sizeof (struct stat_job));
  debug (3, "New Job %lx  Server List %lx", job, servers);
  job->masters = masters;
  job->hosts   = hosts;
  job->servers = servers;
  job->names   = names;

  job->cons = NULL;

  job->master_handlers = NULL;
  job->server_handlers = NULL;
  job->host_handlers   = NULL;
  job->name_handlers   = NULL;

  job->state_handlers  = NULL;
  job->close_handlers  = NULL;

  job->delayed.queued_servers = NULL;
  job->delayed.queued_hosts = NULL;
  job->delayed.refresh_handler = NULL;
  job->delayed.timeout_id = 0;

  job->progress.tasks = -1;
  job->progress.done = 0;

  job->need_refresh = (masters && default_refresh_on_update) ||
                                             (!masters && (servers || names));
  job->need_redraw = FALSE;

  job->masters_to_resolve = 0;
  job->q2_masters = 0;

  job->data    = NULL;

  return job;
}


void stat_job_free (struct stat_job *job) {

  debug (3, "Job %lx  server list %lx", job, job->servers);
  if (job->masters) g_slist_free (job->masters);
  if (job->servers) server_list_free (job->servers);
  if (job->hosts)   host_list_free (job->hosts);
  if (job->names)   userver_list_free (job->names);

  g_slist_free (job->master_handlers);
  g_slist_free (job->server_handlers);
  g_slist_free (job->host_handlers);
  g_slist_free (job->name_handlers);

  g_slist_free (job->state_handlers);
  g_slist_free (job->close_handlers);

  if (job->delayed.queued_servers)
    server_list_free (job->delayed.queued_servers);

  if (job->delayed.queued_hosts)
    host_list_free (job->delayed.queued_hosts);

#ifdef DEBUG
  if (job->data) {
    fprintf (stderr, "stat.c: stat_job_free() -- \'data\' must be NULL!\n");
  }
#endif

  g_free (job);
}

