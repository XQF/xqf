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

#ifndef __QSTAT_H__
#define __QSTAT_H__

#include <unistd.h>	/* pid_t */

#include <gtk/gtk.h>

#include "xqf.h"
#include "dns.h"


#ifndef HTTP_HELPER
# define HTTP_HELPER		"wget -t 1 -T 20 -q -e robots=off -O -"
#endif


#define QSTAT_DELIM		'\t'
#define QSTAT_DELIM_STR		"\t"
#define QSTAT_MASTER_DELIM	' '

#define BUFFER_MINSIZE	1024
#define BUFFER_MAXSIZE	(128*1024)
#define BUFFER_TRESHOLD	64

// EVENT_* used to decide what sound to play
#define EVENT_REFRESH_SELECTED 1
#define EVENT_REFRESH          2
#define EVENT_UPDATE           3

struct stat_job;

struct stat_conn {
  pid_t	pid;
  int	fd;

  int	tag;
  GdkInputFunction input_callback;

  char	*buf;
  int	bufsize;
  int	pos;
  int 	lastnl;

  GSList *strings;	/* a list of strings in `buf' */

  GSList *servers;
  GSList *uservers;
  struct master *master;

  struct stat_job *job;

  char *tmpfile;
};

struct delayed_refresh {
  GSList *queued_servers;
  GSList *queued_hosts;
  GtkFunction refresh_handler;
  unsigned timeout_id;
};

struct stat_progress {
  int tasks;
  int done;
};

enum stat_state {
  STAT_RESOLVE_NAMES,
  STAT_UPDATE_SOURCE,
  STAT_REFRESH_SERVERS,
  STAT_RESOLVE_HOSTS
};

typedef void (*master_func) (struct stat_job *job, struct master *m);
typedef void (*server_func) (struct stat_job *job, struct server *s);
typedef void (*host_func)   (struct stat_job *job, struct host *h, 
                                                      enum dns_status status);
typedef void (*name_func)   (struct stat_job *job, struct userver *us,
                                                      enum dns_status status);
typedef void (*state_func)  (struct stat_job *job, enum stat_state state);
typedef void (*close_func)  (struct stat_job *job, int killed);

struct stat_job {
  GSList *masters;		/*  GSList <struct master *>   */
  GSList *servers;		/*  GSList <struct server *>   */
  GSList *hosts;		/*  GSList <struct host *>     */
  GSList *names;		/*  GSList <struct userver *>  */

  GSList *cons;			/* open connections */

  GSList *master_handlers;
  GSList *server_handlers;
  GSList *host_handlers;
  GSList *name_handlers;

  GSList *state_handlers;
  GSList *close_handlers;

  struct delayed_refresh delayed;
  struct stat_progress progress;

  int need_refresh;
  int need_redraw;

  int masters_to_resolve;
  int q2_masters;
  
  enum stat_state state;

  gpointer data;		/* arbitrary data */
};

extern	void parse_saved_server (GSList *strings);

extern	void stat_start (struct stat_job *job);
extern	void stat_stop (struct stat_job *job);

extern	struct stat_job *stat_job_create (GSList *masters, GSList *names,
                                              GSList *servers, GSList *hosts);
extern	void stat_job_free (struct stat_job *job);


#endif /* __QSTAT_H__ */

