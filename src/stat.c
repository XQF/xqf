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

#include <sys/types.h>  /* kill */
#include <stdio.h>      /* FILE, fopen, fclose, fprintf, ... */
#include <string.h>     /* strchr, strcmp, strlen, strcpy, memchr, strtok */
#include <stdlib.h>     /* strtol */
#include <unistd.h>     /* read, close, fork, pipe, exec, fcntl, _exit, getpid, unlink, write */
#include <fcntl.h>      /* fcntl */
#include <errno.h>      /* errno */
#include <signal.h>     /* kill, signal... */
#include <time.h>       /* time */
#include <sys/socket.h> /* inet_ntoa */
#include <netinet/in.h> /* inet_ntoa */
#include <arpa/inet.h>  /* inet_ntoa */

#include <glib.h>
#include <glib/gi18n.h>

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

static void stat_next (struct stat_job *job);
static void parse_qstat_record (struct stat_conn *conn);

static GSList* stat_buffer_to_strings(gchar buffer[], gsize bufsize);

typedef void (*server_unref_void)(void*);
typedef void (*userver_unref_void)(void*);

static int failed (gchar *name, gchar *arg) {
	fprintf (stderr, "%s(%s) failed\n", name, (arg)? arg : "");

	xqf_error("%s(%s) failed\n", name, (arg)? arg : "");

	return TRUE;
}


static void stat_free_conn (struct stat_conn *conn) {
	struct stat_job *job;

	if (!conn || !conn->job) {
		return;
	}

	debug (3, "stat_free_conn() -- Conn %lx", conn);

	job = conn->job;

	job->cons = g_slist_remove (job->cons, conn);

	if (conn->fd >= 0) {
		// FIXME GError
		g_io_channel_shutdown(conn->chan, TRUE, NULL);
		g_io_channel_unref(conn->chan);
		conn->chan = 0;

		g_source_remove (conn->tag);
		conn->tag = 0;

		close (conn->fd);
	}

	if (conn->pid > 0) {
		kill (conn->pid, SIGTERM);
	}

	if (conn->tmpfile) {
		unlink (conn->tmpfile);
		g_free (conn->tmpfile);
	}

	if (conn->buf) {
		g_free (conn->buf);
	}

	if (conn->strings) {
		g_slist_free (conn->strings);
	}

	if (conn->servers) {
		server_list_free (conn->servers);
	}

	if (conn->uservers) {
		userver_list_free (conn->uservers);
	}

	g_free (conn);
}


static void stat_update_masters (struct stat_job *job);
static void stat_master_update_done(
			struct stat_conn *conn,
			 struct stat_job *job,
			 struct master *m,
			 enum master_state state);

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
	gchar *addr = NULL;
	gchar *tmp;
	unsigned short port;
	struct server *s = NULL;
	struct userver *us = NULL;
	struct host *h = NULL;

	debug (6, "parse_master_output(%s,%p)",str,conn);
	n = tokenize_bychar (str, token, 8, QSTAT_DELIM);

	if (conn->master->master_type == MASTER_HTTP) {
		// HACK: UGLY HACK UGLY HACK UGLY HACK UGLY HACK
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
		if (conn->master->type == UT2_SERVER && n == 3) {
			int off = strlen(token[0]);
			token[0][off]=':';
			strcpy(token[0]+off+1,token[1]);

			n=1;
		}

		// HACK: UGLY HACK UGLY HACK UGLY HACK UGLY HACK
		// gameaholic has '#' comments
		// hambloch.com has '::' comments and empty lines
		// hostname always starts with a letter or a number
		// ignore everything else
		if ( (token[0][0] < '0' || token[0][0] > '9') &&
			(token[0][0] < 'a' || token[0][0] > 'z') ) {
			return TRUE;
		}
	}

	// output from broadcast, last line contains
	// <servertype> <bcastaddr> <number>
	// this line is skipped by n <= 3
	if (conn->master->master_type == MASTER_LAN) {
		if (n <= 3) {
			return TRUE;
		}

		// never do that: qstat returns the qstat_str, not the game id
		// for example we use -cods for COD:UO that will return CODS
		// instead of CODUOS and XQF will mark the server for the
		// wrong game, never trust qstat, always trust XQF
		// type = id2type (token[0]);

		type = conn->master->type;

		n = 2; // we only need type and ip
	}
	else if (n >= 3) {

		/* Master address/status */

		strtol (token[2], &endptr, 10); /* Is it a decimal number? */

		if (endptr == token[2]) {
			if (strcmp (token[2], "DOWN") == 0) {
				debug(3, "parse_master_output() -- master down");
				conn->master->state = SOURCE_DOWN;
			}
			else if (strcmp (token[2], "TIMEOUT") == 0) {
				debug(3, "parse_master_output() -- master timeout");
				conn->master->state = SOURCE_TIMEOUT;
			}
			else /* if (strcmp (token[2], "ERROR") == 0) */ {
				const char *error_msg;

				if (n >= 4) {
					error_msg = token[3];
				} else {
					error_msg = _("Unknown error parsing master server address.");
				}

				debug(3, "parse_master_output() -- master error (%s)", error_msg);
				conn->master->state = SOURCE_ERROR;
				dialog_ok (NULL, "%s", error_msg);
			}
			return FALSE;
		}
	}
	else if (n == 1) {

		n = tokenize_bychar (str, token, 8, QSTAT_MASTER_DELIM);

		switch (n) {

			// line in the form:
			// <hostname>:<port>
			// expected to be found in both http list and file list
			case 1:
				type = conn->master->type;
				break;

			// line in the form:
			// <type>\0<hostname>:<port>
			// expected to be found in qstat output
			case 2:
				// never do that: qstat returns the qstat_str, not the game id
				// for example we use -cods for COD:UO that will return CODS
				// instead of CODUOS and XQF will mark the server for the
				// wrong game, never trust qstat, always trust XQF
				// type = id2type (token[0]);

				type = conn->master->type;
				break;

			// this is not expected at all
			default:
				debug(3, "parse_master_output() -- bad string %s",str);
				return TRUE;

		}
	}
	else {
		debug(3, "parse_master_output() -- unknown string %s",str);
		return TRUE;
	}

	/* server address is in token[n - 1] */

	g_strchomp (token[n-1]);

	if (type != UNKNOWN_SERVER && parse_address (token[n - 1], &addr, &port)) {
		port += conn->master->options.portadjust;
		h = host_add (addr);
		if (h) {    /* IP address */
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
				if (s->type != type) {
					server_free_info(s);
					s->type = type;
				}
#endif

				if (conn->master->server_query_type != UNKNOWN_SERVER) {
					s->server_query_type = conn->master->server_query_type;
				}

				/*
				   When the "conn" is freed, it will call the function
				   to free the list returned here
				*/
				// XXX:
				//	  conn->servers = server_list_prepend (conn->servers, s);
				conn->servers = g_slist_prepend (conn->servers, s);
				server_ref(s);
			}
			host_unref (h);
		}
		else {  /* hostname */

			tmp = g_ascii_strdown (addr, -1);   /* g_ascii_strdown does not modify string in place */
			strcpy(addr, tmp);
			g_free(tmp);

			port += conn->master->options.portadjust;
			if ((us = userver_add (addr, port, type)) != NULL) {

				if (conn->master->server_query_type != UNKNOWN_SERVER) {
					us->server_query_type = conn->master->server_query_type;
				}

				// conn->uservers = userver_list_add (conn->uservers, us);
				conn->uservers = g_slist_prepend (conn->uservers, us);
				userver_ref(us);
			}
		}
		g_free (addr);
	}

	return TRUE;
}


/* the function should return FALSE if the event source should be removed */
static gboolean stat_master_input_callback (GIOChannel *chan, GIOCondition condition, struct stat_conn *conn) {
	struct stat_job *job = conn->job;
	gchar *buf = g_malloc(sizeof(gchar*) * BUFFER_MINSIZE);
	gsize res = 0;
	GError *err = NULL;
	GIOStatus status;
	GSList *strings, *current;

	debug_increase_indent();
	debug(3, "stat_master_input_callback(%p,%d,...)",conn,chan);

	/* return FALSE when there is nothing (more) to do */
	while (TRUE) {
		status = g_io_channel_read_chars(chan, buf, BUFFER_MINSIZE, &res, &err);

		conn->bufsize += res;

		if (conn->buf == NULL) {
			conn->buf = g_malloc(sizeof(gchar*) * conn->bufsize);
		}
		else {
			conn->buf = g_realloc(conn->buf, sizeof(gchar*) * conn->bufsize);
		}

		strncpy(conn->buf + (conn->bufsize - res), buf, res);

		if (status == G_IO_STATUS_EOF) {
			gboolean unsuccessful = FALSE;

			debug(3, "stat_master_input_callback -- eof");
			debug(6, "conn->buf: [%d]", buf);

			strings = stat_buffer_to_strings(conn->buf, conn->bufsize);
			current = strings;

			while (current) {
				if (strlen(current->data)) {
					debug(6, "parse_master_output: [%s]", current->data);
					if (!parse_master_output(current->data, conn)) {
						unsuccessful = TRUE;
					}
				}
				current = current->next;
			}

			if (unsuccessful) {
				stat_master_update_done (conn, job, conn->master, conn->master->state);
			} else {
				stat_master_update_done (conn, job, conn->master, SOURCE_UP);
			}

			stat_update_masters (job);
			debug_decrease_indent();
			g_free(buf);
			return FALSE;
		}
		else if (status == G_IO_STATUS_AGAIN) {
			debug(3, "stat_master_input_callback -- unavailable");
			debug_decrease_indent();
			g_free(buf);
			return TRUE;
		}
		else if (status == G_IO_STATUS_ERROR) {
			debug(3, "stat_master_input_callback -- error");
			debug(6, "conn->buf: [%d]", buf);
			failed ("read", NULL);

			stat_master_update_done (conn, job, conn->master, SOURCE_ERROR);
			stat_update_masters (job);
			debug_decrease_indent();
			g_free(buf);
			return FALSE;
		}

		/* G_IO_STATUS_NORMAL */
		debug(3, "stat_master_input_callback -- chars read");
		/* loop */
	}
	// infinite loop end, next lines are never read
}


static char **parse_serverinfo (char *token[], int n) {
	char **info;
	int size = 0;
	int i, j;
	char *ptr, *info_ptr;

	if (n == 0) {
		return NULL;
	}

	for (size = 0, i = 0; i < n; i++) {
		size += strlen (token[i]) + 1;
	}

	info = g_malloc (sizeof (char *) * (n + 1) * 2 + size);
	info_ptr = (char *) info + sizeof (char *) * (n + 1) * 2;

	for (i = 0, j = 0; i < n; i++) {
		ptr = strchr (token[i], '=');

		if (ptr) {
			*ptr++ = '\0';
		}

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


static struct server *parse_server (char *token[], int n, time_t refreshed, int saved) {
	struct host *h;
	struct server *server;
	enum server_type type;
	char *addr, *pipe;
	unsigned short port;

	if (n < 3) {
		return NULL;
	}

	pipe = strchr(token[0], '|');

	if (pipe != NULL) {
		// DESTRUCTIVE
		*pipe = '\0';
	}

	type = id2type (token[0]);

	if (type == UNKNOWN_SERVER) {
		return NULL;
	}

	if (!parse_address (token[1], &addr, &port)) {
		return NULL;
	}

	h = host_add (addr);
	g_free (addr);
	if (!h) {
		return NULL;
	}

	server = server_add (h, port, type);

	if (pipe != NULL) {
		server->server_query_type = id2type(++pipe);
	}

	debug (6, "server %lx retrieved", server);

	server->flt_mask &= ~FILTER_SERVER_MASK;

	server->refreshed = refreshed;

	if (n == 3) {
		if (strcmp (token[2], "DOWN") == 0) {
			server->retries = MAX_RETRIES;
			server->ping = MAX_PING + 1;    /* DOWN */
		}
		else {
			server->retries = MAX_RETRIES;
			server->ping = MAX_PING;        /* TIMEOUT */
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
		if (server->type == QW_SERVER || type == QW_SERVER) {
			server->type = type;        /* the real type of server (QW <-> Q2) */
		}

		if (games[server->type].parse_server) {
			/*
			   We have a function to parse the server information,
			   so first we free all of the data elements of this
			   structure but not the structure itself.  This
			   is because the *_analyse functions should assign values
			   to each of the elemets.
			*/
			server_free_info (server);

			server->retries = MAX_RETRIES;
			server->ping = MAX_PING;    /* TIMEOUT */

			/* Actually parse the info with the given function.  The mapping
			   and functions are in game.c
			*/
			(*games[server->type].parse_server) (token, n, server);

			if (server->ping > MAX_PING) {
				if (!saved || server->ping != MAX_PING + 1) {
					server->ping = MAX_PING;
				}
			}

			if (server->retries > maxretries) {
				server->retries = maxretries;
			}
		}

		if (server->name && !g_utf8_validate(server->name,-1,NULL)) {
			// don't care about conversion errors
			char* convd = g_convert(server->name,-1,"UTF-8","iso-8859-1",NULL,NULL,NULL);
			if (convd) {
				g_free(server->name);
				server->name = convd;
			}
		}
	}

	if (!saved && server->ping<MAX_PING) {
		server->last_answer = server->refreshed;
	}

	return server;
}

/** teamnames must be static storage, pointers to it will not be freed */
static void q3parseteams(struct server* s,
		const unsigned numteams,
		char** playerteamrules,	// which rule holds the numbers that indicate which player is in what team
		char** teamnames,	// default names for team
		char** teamnamesrules)	// which rules hold the team names
{
	// bitmask of players for each team
	long* teams = NULL;
	char **info_ptr = NULL;
	unsigned i = 0, n = 0;
	unsigned team = numteams;
	GSList *plist = NULL;
	struct player *p  = NULL;
	char** teamnames_fromrules = NULL;

	if (!s || !playerteamrules || numteams<2) return;

	teams = g_malloc0(sizeof(long)*numteams);
	if (!teams) return;

	teamnames_fromrules = g_malloc0(sizeof(char*)*numteams);
	if (!teamnames_fromrules) return;

	for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
		for (i = 0; i < numteams; ++i) {
			if (strcmp (*info_ptr, playerteamrules[i]) == 0) {
				team = i;
				break;
			}
			if (teamnamesrules && teamnamesrules[i] &&strcmp (*info_ptr, teamnamesrules[i]) == 0) {
				teamnames_fromrules[i]=info_ptr[1];
			}
		}

		if (team != numteams) {
			char* e = NULL;
			char* p = info_ptr[1];
			for (;; p = e, e = NULL) {
				long pnr = strtol(p,&e,10);
				if (p == e || (e && !*e)) {
					break;
				}
				if (pnr != LONG_MIN && pnr != LONG_MAX
						&& pnr <= (long)(sizeof(teams[team])*8)
						&& pnr > 0) {
					teams[team] |= 2<<(pnr-1);
				}
			}
			team = numteams;
		}
	}
	for (plist = s->players, n = 0 ; plist ; plist=plist->next, ++n) {
		for (team=0;team != numteams; ++team) {
			if (teams[team]&(2<<n)) {
				p = plist->data;
				if (teamnames_fromrules[team]) {
					p->model = teamnames_fromrules[team];
				}
				else {
					p->model = teamnames[team];
				}
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
		if (games[s->type].analyze_serverinfo) {
			(*games[s->type].analyze_serverinfo) (s);
		}
		return;
	}

	n = tokenize_bychar ((char *) strings->data, token, 256, QSTAT_DELIM);
	if (n == 0) {
		return;
	}

	s->info = parse_serverinfo (token, n);

	strings = strings->next;

	if (strings && games[s->type].parse_player) {
		while (strings) {
			n = tokenize_bychar ((char *) strings->data, token, 256, QSTAT_DELIM);
			p = (*games[s->type].parse_player) (token, n, s);
			if (!p) /* error, try to recover */
				return;

			plist = g_slist_prepend (plist, p);

			strings = strings->next;
		}
		s->players = g_slist_reverse (plist);
	}

	if (games[s->type].analyze_serverinfo) (*games[s->type].analyze_serverinfo) (s);

	if (s->type == WO_SERVER || s->type == WOET_SERVER) {
		static char* teamnames[2] = { N_("Allies"), N_("Axis") };
		static char* playerteamrules[2] = { "Players_Allies", "Players_Axis" };
		q3parseteams(s,2,playerteamrules,teamnames,NULL);
	}
	else if (s->type == Q3_SERVER) {
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

	if (!conn || !conn->strings) {
		return; /* error, try to recover */
	}

	job = conn->job;

	/* Debug before tokenizing. */
	debug (4, "parse_qstat_record() -- Conn %lx: %s", conn, conn->strings->data);

	n = tokenize_bychar ((char *) conn->strings->data, token, 16, QSTAT_DELIM);
	if (n < 3) {
		return;     /* error, try to recover */
	}

	if (n >= 3 && token[0][0] == '\0' && token[1][0] == '\0' && strcmp(token[2], "ERROR") == 0) {
		const char *error_msg;

		if (n >= 4) {
			error_msg = token[3];
		} else {
			error_msg = _("Unknown error parsing qstat server record.");
		}

		dialog_ok (NULL, "%s", error_msg);
		return;
	}

	server = parse_server (token, n, time (NULL), FALSE);
	if (server) {
		job->need_redraw = TRUE;

		/*
		   o The list here is freed when the job is freed.
		   o The parse_server call above increments the ref
		   count as does the prepend below. So we need to decrement it
		   one just for fun.
		*/
		job->delayed.queued_servers = server_list_prepend (job->delayed.queued_servers, server);
		job->progress.done++;
		server_unref (server);

		debug (6, "Server %lx in delayed list %lx.", server, job->delayed.queued_servers);

		parse_qstat_record_part2 (conn->strings->next, server);

		for (list = job->server_handlers; list; list = list->next) {
			(* (server_func) list->data) (job, server);
		}
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

	if (!strings || !strings->next) {
		return;
	}

	// first line contains seconds of last refresh and last answer
	refreshdata = g_strsplit(strings->data," ",2);

	if (!refreshdata || !refreshdata[0]) {
		debug(0,"refreshdata empty");
		return;
	}

	refreshed = strtoul (refreshdata[0], &endptr, 10);

	if (endptr == refreshdata[0])       /* It's not a number */
		return;

	if (refreshdata[1]) // post 0.9.10 format
	{
		last_answer = strtoul (refreshdata[1], &endptr, 10);

		if (endptr == refreshdata[1])   /* It's not a number */
			return;
	}

	g_strfreev(refreshdata);
	refreshdata = NULL;

	strings = strings->next;

	if (!strings) return;

	n = tokenize_bychar ((char *) strings->data, token, 16, QSTAT_DELIM);
	if (n < 3) {
		return;
	}

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


static void stat_servers_update_done (struct stat_conn *conn) {
	debug (3, "stat_servers_update_done() -- Conn %lx  server list %lx", conn, conn->job->servers);
	server_list_free (conn->job->servers);
	conn->job->servers = NULL;
	stat_free_conn (conn);
}

static gchar asciification(gchar c) {
			/* FIXME: workaround to replace non ascii character from binary buffer
			 * because qstat uses ISO-8859-1 encoding for raw ouput
			 */

			if (c >= '\0' && c <= '~') {
				return c;
			}
			else {
				return '_';
			}
}

static GSList* stat_buffer_to_strings(gchar buffer[], gsize bufsize) {
	GSList *strings = NULL;
	gsize token_mul = 1;
	gchar *token = g_malloc(token_mul * BUFFER_MINSIZE * sizeof(gchar*));
	gsize i, last;

	for (i = 0, last = 0; i < bufsize; i++) {
		if (i - last == (token_mul * BUFFER_MINSIZE)) {
			debug(6, "token too short: extending token size from %d to %d", (token_mul * BUFFER_MINSIZE), ((token_mul+1) * BUFFER_MINSIZE));
			token_mul++;
			token = g_realloc(token, (token_mul * BUFFER_MINSIZE * sizeof(gchar*)));
		}

		if (buffer[i] == '\n' || buffer[i] == '\0' || i == bufsize - 1) {
			token[i - last] = '\0';

			debug(6, "stat_buffer_to_strings - token: %s", token);

			strings = g_slist_append(strings, strdup(token));
			last = i + 1;
		}
		else {
			/* FIXME: workaround to replace non ascii character from binary buffer */
			token[i - last] = asciification(buffer[i]);
		}
	}
	g_free(token);
	return strings;
}

/*
   stat_servers_input_callback -- as data is returned from the qstat
   process, this gets called.  Sometimes there are multiple lines
   so the results have to be looped over.
*/
static gboolean stat_servers_input_callback (GIOChannel *chan, GIOCondition condition, struct stat_conn *conn) {
	struct stat_job *job = conn->job;
	gchar *buf = g_malloc(sizeof(gchar*) * BUFFER_MINSIZE);
	gsize res = 0;
	GError *err = NULL;
	GIOStatus status;
	GSList *strings, *current;

	/* return FALSE when there is nothing (more) to do */
	debug (3, "stat_servers_input_callback() -- Conn %lx", conn);
	while (TRUE) {
		status = g_io_channel_read_chars(chan, buf, BUFFER_MINSIZE, &res, &err);

		conn->bufsize += res;

		if (conn->buf == NULL) {
			conn->buf = g_malloc(sizeof(gchar*) * conn->bufsize);
		}
		else {
			conn->buf = g_realloc(conn->buf, sizeof(gchar*) * conn->bufsize);
		}

		strncpy(conn->buf + (conn->bufsize - res), buf, res);

		if (status == G_IO_STATUS_EOF) {
			debug(3, "stat_servers_input_callback -- eof");
			debug(6, "conn->buf: [%d]", buf);
			debug(3, "Conn %ld  Sub Process Done with server list %lx", conn, conn->job->servers);

			strings = stat_buffer_to_strings(conn->buf, conn->bufsize);
			current = strings;

			conn->strings = NULL;

			while (current) {
				while (current && strlen(current->data)) {
					conn->strings = g_slist_append(conn->strings, strdup(current->data));
					current = current->next;
				}
				if (conn->strings) {
					parse_qstat_record (conn);
					g_slist_free (conn->strings);
					conn->strings = NULL;
				}
				if (current) {
					current = current->next;
				}
			}
			g_slist_free(strings);

			stat_servers_update_done (conn);
			stat_next (job);
			g_free(buf);
			return FALSE;
		}
		else if (status == G_IO_STATUS_AGAIN) {
			debug(3, "stat_servers_input_callback -- unavailable");
			g_free(buf);
			return TRUE;
		}
		else if (status == G_IO_STATUS_ERROR) {
			debug(3, "stat_servers_input_callback -- error");
			debug(6, "conn->buf: [%d]", buf);
			failed ("read", NULL);

			stat_servers_update_done (conn);
			stat_next (job);
			g_free(buf);
			return TRUE;
		}

		/* G_IO_STATUS_NORMAL */
		debug(3, "stat_servers_input_callback -- chars read");
		/* loop	*/
	}
	// infinite loop end, next lines are never read
}

/**
  return connection to local file
*/
static struct stat_conn *new_file_conn (struct stat_job *job, const char* file, GIOFunc input_callback, struct master *m) {
	struct stat_conn *conn;
	char *file2;

	int fd = -1;

	file2 = expand_tilde(file);

	fd = open(file2,O_RDONLY);
	if (fd == -1) {
		perror(__FUNCTION__);
		return NULL;
	}

	conn = g_malloc (sizeof (struct stat_conn));
	if (!conn) {
		return NULL;
	}

	conn->buf = g_malloc (BUFFER_MINSIZE);
	conn->bufsize = BUFFER_MINSIZE;
	conn->tmpfile = NULL;
	conn->pid = 0;
	conn->fd = fd;
	conn->chan = g_io_channel_unix_new (conn->fd);

	/* FIXME: workaround to read the channel as binary (and exclude non ascii character elsewhere)
	 * because qstat uses ISO-8859-1 encoding for raw ouput
	 */

	g_io_channel_set_encoding(conn->chan, NULL, NULL);

	conn->pos = 0;
	conn->lastnl = 0;

	conn->strings = NULL;
	conn->servers = NULL;
	conn->uservers = NULL;

	conn->master = m;

	conn->job = job;
	job->cons = g_slist_prepend (job->cons, conn);

	conn->tag = g_io_add_watch (conn->chan, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI, input_callback, conn);
	conn->input_callback = input_callback;

	if (file2) {
		g_free(file2);
	}

	return conn;
}

/*
   start_qstat -- Fork and run qstat with the given command line
   options. Returns a "conn?"
*/
static struct stat_conn *start_qstat (struct stat_job *job, char *argv[], GIOFunc input_callback, struct master *m) {

	struct stat_conn *conn;
	pid_t pid;
	int pipefds[2];

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

	if (pid) {  /* parent */
		close (pipefds[1]);

		if (set_nonblock (pipefds[0]) == -1) {
			failed("fcntl", NULL);
		}

		conn = g_malloc (sizeof (struct stat_conn));

		conn->buf = NULL;
		conn->bufsize = 0;
		conn->tmpfile = NULL;
		conn->pid = pid;
		conn->fd = pipefds[0];
		conn->chan = g_io_channel_unix_new (conn->fd);

		/* FIXME: workaround to read the channel as binary (and exclude non ascii character elsewhere)
		 * because qstat uses ISO-8859-1 encoding for raw ouput
		 */

		g_io_channel_set_encoding(conn->chan, NULL, NULL);

		conn->pos = 0;
		conn->lastnl = 0;

		conn->strings = NULL;
		conn->servers = NULL;
		conn->uservers = NULL;

		conn->master = m;

		conn->job = job;
		job->cons = g_slist_prepend (job->cons, conn);

		conn->tag = g_io_add_watch (conn->chan, G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_PRI, input_callback, conn);
		conn->input_callback = input_callback;
	}
	else {  /* child */
		close (pipefds[0]);
		dup2 (pipefds[1], STDOUT_FILENO);
		close (pipefds[1]);

		execvp (argv[0], argv);

		failed ("execvp", argv[0]);

		fprintf (stdout, QSTAT_DELIM_STR QSTAT_DELIM_STR "ERROR" QSTAT_DELIM_STR);
		fprintf (stdout, _("%s command not found."), argv[0]);
		fprintf (stdout, "\n");
		fflush (stdout);

		_exit (1);
	}

	return conn;
}


static void stat_close (struct stat_job *job, int killed) {
	GSList *tmp;

	dns_set_callback (NULL, NULL);
	dns_cancel_requests ();
	debug (3, "stat_close() -- Job %lx  Killed? %d", job, killed);

	while (job->cons) {
		stat_free_conn ((struct stat_conn *) job->cons->data);
	}

	if (job->delayed.refresh_handler) {
		g_source_remove (job->delayed.timeout_id);
	}

	for (tmp = job->close_handlers; tmp; tmp = tmp->next) {
		(* (close_func) tmp->data) (job, killed);
	}

	stat_job_free (job);

	if (event_type == EVENT_REFRESH_SELECTED) {
		debug (1, "refresh selected done\n");
	}
	if (event_type == EVENT_REFRESH) {
		debug (1, "refresh done.\n");
		play_sound(sound_refresh_done, 0);
	}
	if (event_type == EVENT_UPDATE) {
		debug (1, "update done.\n");
		play_sound(sound_update_done, 0);
	}
	event_type=0;
}


static const char delim[] = " \t\n\r";


static struct stat_conn *stat_update_master_qstat (struct stat_job *job, struct master *m) {
	char *argv[16];
	int argi = 0;
	char buf1[64];
	char* arg_type = NULL;
	char buf2[64];
	char buf_rawarg[] = { QSTAT_DELIM, '\0' };
	struct stat_conn *conn = NULL;
	char *cmd = NULL;
	char *file = NULL;
	char srcport[12] = {0};

	short startprog = 1;

	debug (3, "stat_update_master_qstat(%p,%p)", job, m);
	if (!m) {
		return NULL;
	}

	if (m->url) {
		if (m->master_type == MASTER_GSLIST) {
			int ret = 0;
			startprog = 1;

			while (current_server_filter > 0 && (cur_filter & FILTER_SERVER_MASK)) {
				struct server_filter_vars* filter =
					g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1);
				size_t bufsize = 2048;
				char* pos;

				if (!filter) {
					xqf_error("filter is NULL");
					return NULL;
				}

				pos = arg_type = g_new0(char, bufsize);
				if (filter->game_contains&&*filter->game_contains) {
					ret = snprintf(pos, bufsize,"(gametype LIKE '%%%s%%')", filter->game_contains);
					if (ret == -1) {
						break;
					}
					pos += ret;
					bufsize -= ret;
				}

				if (filter->map_contains&&*filter->map_contains) {
					if (pos != arg_type) {
						ret = snprintf(pos, bufsize," AND ");
						if (ret == -1) {
							break;
						}
						pos += ret;
						bufsize -= ret;
					}

					ret = snprintf(pos, bufsize,"(mapname LIKE '%%%s%%')", filter->map_contains);
					if (ret == -1) {
						break;
					}
					pos += ret;
					bufsize -= ret;
				}

				if (filter->server_name_contains&&*filter->server_name_contains) {
					if (pos != arg_type) {
						ret = snprintf(pos, bufsize," AND ");
						if (ret == -1) {
							break;
						}
						pos += ret;
						bufsize -= ret;
					}

					ret = snprintf(pos, bufsize,"(hostname LIKE '%%%s%%')", filter->server_name_contains);
					if (ret == -1) {
						break;
					}
					pos += ret;
					bufsize -= ret;
				}

				if (filter->filter_not_empty) {
					if (pos != arg_type) {
						ret = snprintf(pos, bufsize," AND ");
						if (ret == -1) {
							break;
						}
						pos += ret;
						bufsize -= ret;
					}

					ret = snprintf(pos, bufsize,"(numplayers > 0)");
					if (ret == -1) {
						break;
					}
					pos += ret;
					bufsize -= ret;
				}

				if (filter->filter_not_full) {
					if (pos != arg_type) {
						ret = snprintf(pos, bufsize," AND ");
						if (ret == -1) {
							break;
						}
						pos += ret;
						bufsize -= ret;
					}

					ret = snprintf(pos, bufsize,"(numplayers < maxplayers)");
					if (ret == -1) {
						break;
					}
					pos += ret;
					bufsize -= ret;
				}

				break;
			}

			if (ret == -1) {
				g_free(arg_type);
				arg_type = NULL;
			}


			argv[argi++] = "gslist";
			argv[argi++] = "-q";
			argv[argi++] = "-o";
			argv[argi++] = "5";
			if (arg_type) {
				argv[argi++] = "-f";
				argv[argi++] = arg_type;
				debug(3, "%s", arg_type);
			}
			argv[argi++] = "-N";
			argv[argi++] = m->options.gsmtype;
			argv[argi] = NULL;
		}
		else if (!strncmp(m->url,master_prefixes[MASTER_FILE],strlen(master_prefixes[MASTER_FILE]))) {
			startprog = 0;
			file=strdup_strip(m->url + strlen(master_prefixes[MASTER_FILE]));
		}
		// if MASTER_HTTP
		else {
			cmd = strdup_strip (HTTP_HELPER);

			argv[argi++] = strtok (cmd, delim);
			while ((argv[argi] = strtok (NULL, delim)) != NULL) {
				argi++;
			}

			argv[argi++] = m->url;
			argv[argi] = NULL;
		}
	}
	else {

		if (m->master_type!=MASTER_LAN && m->master_type != MASTER_GAMESPY && !master_qstat_option(m)) {
			return NULL;
		}

		argv[argi++] = QSTAT_EXEC;
		argv[argi++] = "-errors";

		if (access(qstat_configfile, R_OK) == 0) {
			argv[argi++] = "-cfg";
			argv[argi++] = qstat_configfile;
		}

		if (qstat_srcport_low) {
			snprintf(srcport, sizeof(srcport), "%hu-%hu", qstat_srcport_low, qstat_srcport_high);
			argv[argi++] = "-srcport";
			argv[argi++] = srcport;
		}

		if (qstat_srcip) {
			argv[argi++] = "-srcip";
			argv[argi++] = qstat_srcip;
		}

		argv[argi++] = "-raw";
		argv[argi++] = buf_rawarg;

		argv[argi++] = "-retry";
		argv[argi++] = buf1;
		g_snprintf (buf1, 64, "%d", maxretries + 2);

		if (m->master_type == MASTER_LAN) {
			debug (3, "stat_update_master_qstat() -- MASTER_LAN");
			arg_type = g_strdup_printf("%s,outfile", games[m->type].qstat_option);
		}
		else if (m->master_type == MASTER_GAMESPY && !master_qstat_option(m)) {
			arg_type = g_strdup_printf("-gsm,%s,outfile", games[m->type].qstat_str);
		}
		// add master arguments
		else if (games[m->type].flags & GAME_MASTER_QUAKE3) {
			// TODO: master protocol should be server specific
			const char* masterprotocol = game_get_attribute(m->type,"masterprotocol");

			if (masterprotocol && !strcmp(masterprotocol, "auto")) {
				masterprotocol = game_get_attribute(m->type, "_masterprotocol");
			}
			if (masterprotocol) {
				arg_type = g_strdup_printf("%s,%s,outfile", master_qstat_option(m),masterprotocol);
			}
			else {
				xqf_warning("GAME_MASTER_QUAKE3 flag set, but no protocol specified");
				arg_type = g_strdup_printf("%s,outfile", master_qstat_option(m));
			}
		}
		else if ((games[m->type].flags & GAME_MASTER_STEAM) && current_server_filter > 0 && (cur_filter & FILTER_SERVER_MASK)) {
			struct server_filter_vars* filter =
				g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1);

			if (!filter) {
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
		else if (m->type == UT2004_SERVER) {
			GString* str = NULL;
			const char* cdkey = game_get_attribute(m->type,"cdkey");
			if (!cdkey) {
				xqf_error(_("UT2004 CD Key not found, cannot query master '%s'.\n"
							"Make sure the working directory is set correctly."), m->name);
				goto out;
			}

			str = g_string_new(NULL);
			g_string_sprintf(str, "%s,outfile,cdkey=%s", master_qstat_option(m), cdkey);

			if (current_server_filter > 0 && (cur_filter & FILTER_SERVER_MASK)) {
				struct server_filter_vars* filter = g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1);

				if (filter) {
					GString* status = g_string_new(NULL);

					if (filter->filter_not_empty) {
						status = g_string_append(status, "notempty");
					}

					if (filter->filter_not_full) {
						status = g_string_append(status, "notfull");
					}

					if (filter->filter_no_password) {
						status = g_string_append(status, "nopassword");
					}

					if (filter->game_contains&&*filter->game_contains) {
						g_string_sprintfa(str, ",gametype=%s", filter->game_contains);
					}

					if (status->str && *status->str) {
						g_string_sprintfa(str, ",status=%s", status->str);
					}
					g_string_free(status, TRUE);
				}
			}
			else {
				str = g_string_append(str, ",status=nostandard");
			}

			arg_type = str->str;
			g_string_free(str, FALSE);
		}
		else {
			arg_type = g_strdup_printf ("%s,outfile", master_qstat_option(m));
		}

		argv[argi++] = arg_type;

		argv[argi++] = buf2;
		g_snprintf (buf2, 64, "%s%s:%d,-", m->master_type == MASTER_LAN?"+":"" ,inet_ntoa (m->host->ip), m->port);

		argv[argi] = NULL;

	}   /*  if (m->url)  */

	if (argi >= sizeof(argv)) {
		xqf_error("FIXME: argi too big, stack corrupt");
	}

	if (startprog) {
		if (get_debug_level() > 3){
			char **argptr = argv;
			fprintf (stderr, "stat_update_master_qstat: EXEC> ");
			while (*argptr) {
				fprintf (stderr, "%s ", *argptr++);
			}
			fprintf (stderr, "\n");
		}

		conn = start_qstat (job, argv, (GIOFunc) stat_master_input_callback, m);
	}
	else if (file) {
		conn = new_file_conn (job, file, (GIOFunc) stat_master_input_callback, m);
		g_free (file);
	}

out:
	g_free (cmd);
	g_free (arg_type);

	return conn;
}

#define MAX_SERVERS_IN_CMDLINE 16

static struct stat_conn *stat_open_conn_qstat (struct stat_job *job) {
	struct server *s = NULL;
	char *argv[16 + MAX_SERVERS_IN_CMDLINE*2];
	int argi = 0;
	char buf[64 + MAX_SERVERS_IN_CMDLINE*64];
	int bufi = 0;
	char *fn = NULL;
	GSList *tmp;
	struct stat_conn *conn;
	char *qstat_query_option;
	char *qstat_query_str;
	char srcport[12] = {0};

	if (!job->servers) {
		return NULL;
	}

	/*
	   The g_slist_reverse does not allocate any new
	   lists or memory.  However, it means that job->servers
	   will point to a different member.
	*/
	debug (6, "stat_open_conn_qstat() -- server list was %lx", job->servers);
	job->servers = g_slist_reverse (job->servers);
	debug (6, "stat_open_conn_qstat() -- server list now %lx", job->servers);

	argv[argi++] = QSTAT_EXEC;
	argv[argi++] = "-errors";

	if (access(qstat_configfile, R_OK) == 0) {
		argv[argi++] = "-cfg";
		argv[argi++] = qstat_configfile;
	}

	if (qstat_srcport_low) {
		snprintf(srcport, sizeof(srcport), "%hu-%hu", qstat_srcport_low, qstat_srcport_high);
		argv[argi++] = "-srcport";
		argv[argi++] = srcport;
	}

	if (qstat_srcip) {
		argv[argi++] = "-srcip";
		argv[argi++] = qstat_srcip;
	}

	argv[argi++] = "-maxsimultaneous";
	argv[argi++] = &buf[bufi];
	bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%d", maxsimultaneous);
	argv[argi++] = "-retry";
	argv[argi++] = &buf[bufi];
	bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%d", maxretries);

	argv[argi++] = "-raw";
	argv[argi++] = &buf[bufi];
	bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%c", QSTAT_DELIM);

	argv[argi++] = "-R";

	argv[argi++] = "-P";

	argv[argi++] = "-carets";

	if (g_slist_length (job->servers) <= MAX_SERVERS_IN_CMDLINE) {
		for (tmp = job->servers; tmp; tmp = tmp->next) {
			s = (struct server *) tmp->data;

			if (s->server_query_type != UNKNOWN_SERVER) {
				qstat_query_option = games[s->server_query_type].qstat_option;
			}
			else {
				qstat_query_option = games[s->type].qstat_option;
			}

			argv[argi++] = qstat_query_option;

			argv[argi++] = &buf[bufi];
			bufi += 1 + g_snprintf (&buf[bufi], sizeof (buf) - bufi, "%s:%d", inet_ntoa (s->host->ip), s->port);
		}
	}
	else {
		int fd = -1;
		FILE* f = NULL;

		argv[argi++] = "-f";
		fn = &buf[bufi];
		bufi += 1 + g_snprintf(fn, sizeof (buf) - bufi, "%s/xqf-qstat.XXXXXX", g_get_tmp_dir ());

		argv[argi++] = fn;

		fd = mkstemp(fn);

		if (fd < 0 || !(f = fdopen(fd, "w"))) {
			dialog_ok (NULL, _("Failed to create a temporary file %s: %s"), fn, strerror(errno));
			return NULL;
		}

		for (tmp =job->servers; tmp; tmp = tmp->next) {
			s = (struct server *) tmp->data;

			if (s->server_query_type != UNKNOWN_SERVER) {
				qstat_query_str = games[s->server_query_type].qstat_str;
			}
			else {
				qstat_query_str = games[s->type].qstat_str;
			}

			fprintf (f, "%s %s:%d\n", qstat_query_str, inet_ntoa (s->host->ip), s->port);
		}

		fclose (f);
	}

	argv[argi] = NULL;

	if (argi >= sizeof(argv)) {
		xqf_error("FIXME: argi too big, stack corrupt");
	}

	conn = start_qstat (job, argv, (GIOFunc) stat_servers_input_callback, NULL);
	if (conn && fn) {
		conn->tmpfile = g_strdup (fn);
	}

	return conn;
}


// compare pointers
static gint compare_ptr (gconstpointer s1, gconstpointer s2) {
	if (s1 == s2) {
		return 0;
	}
	else if (s1 < s2) {
		return -1;
	}
	else {
		return 1;
	}
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

		if (default_refresh_on_update) {
			job->need_refresh = TRUE;
		}
	}

	if (conn) {
		stat_free_conn (conn);
	}

	job->need_redraw = TRUE;
	/*
	   job->delayed.queued_servers = server_list_append_list (job->delayed.queued_servers, m->servers, UNKNOWN_SERVER);

	   job->servers = server_list_append_list (job->servers, m->servers, UNKNOWN_SERVER);
	   job->names = userver_list_append_list (job->names, m->uservers, UNKNOWN_SERVER);
	*/
	for (tmp = m->servers; tmp; tmp = tmp->next) {
		struct server* s = tmp->data;
		job->delayed.queued_servers = g_slist_prepend(job->delayed.queued_servers, s);
		job->servers = g_slist_prepend(job->servers, s);
		server_ref(s);
		server_ref(s);
	}
	for (tmp = m->uservers; tmp; tmp = tmp->next) {
		struct userver *us = tmp->data;
		job->names = g_slist_prepend(job->names,us);
		userver_ref(us);
	}

	job->delayed.queued_servers = slist_sort_remove_dups(job->delayed.queued_servers, compare_ptr,(server_unref_void)server_unref);

	job->servers = slist_sort_remove_dups(job->servers,compare_ptr,(server_unref_void)server_unref);

	job->names = slist_sort_remove_dups(job->names,compare_ptr,(userver_unref_void)userver_unref);

	debug (3, "stat_master_update_done -- job->master_handlers = %p",job->master_handlers);
	for (tmp = job->master_handlers; tmp; tmp = tmp->next) {
		(* (master_func) tmp->data) (job, m);
	}

	job->progress.done++;

	if (m->type == Q2_SERVER && !m->url) {
		job->q2_masters--;
	}
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

			if (!stat_update_master_qstat (job, m)) {
				stat_master_update_done (NULL, job, m, SOURCE_ERROR);
			}
			else {
				freecons--;
			}

			continue;
		}

		tmp = tmp->next;
	}

	if (job->masters == NULL && job->cons == NULL) {
		stat_next (job);
	}
}


static void stat_master_resolved_callback (char *id, struct host *h, enum dns_status status, void *data) {
	struct stat_job *job = (struct stat_job *) data;
	struct master *m;
	GSList *list;
	enum master_state state;

	debug (3, "stat_master_resolved_callback(%s,%p,%d,%p)", id, h,status,data);

	if (!job || !id) {
		return;
	}

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

				if (status == DNS_STATUS_TIMEOUT || status == DNS_STATUS_ERROR) {
					state = SOURCE_TIMEOUT;
				}
				else {  /* DNS_STATUS_NOTFOUND, etc... */
					state = SOURCE_ERROR;
				}

				stat_master_update_done (NULL, job, m, state);
				continue;
			}
		}
		list = list->next;
	}

	stat_update_masters (job);
}


// ip for hostname resolved
static void stat_name_resolved_callback (char *id, struct host *h, enum dns_status status, void *data) {
	struct stat_job *job = (struct stat_job *) data;
	struct userver *us;
	GSList *list;
	GSList *tmp;

	debug (6, "%s,%p,%d,%p",id,h,status,data);

	if (!job || !id) {
		return;
	}

	list = job->names;
	while (list) {
		us = (struct userver *) list->data;
		if (strcmp (us->hostname, id) == 0) {
			if (h) {
				userver_set_host (us, h);
			}

			/* automatically add it to the list of servers to refresh */

			if (us->s) {

				if (us->s->type != us->type) {
					us->s->type = us->type;
					server_free_info (us->s);
				}

				us->s->server_query_type = us->server_query_type;

				/*
				   o When the job is freed, the list will
				   be freed as well. This will take care of
				   the reference counting.
				*/
				job->servers = server_list_prepend (job->servers, us->s);
			}

			for (tmp = job->name_handlers; tmp; tmp = tmp->next) {
				(* (name_func) tmp->data) (job, us, status);
			}

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


static void stat_host_resolved_callback (char *id, struct host *h, enum dns_status status, void *data) {
	struct stat_job *job = (struct stat_job *) data;
	GSList *tmp;

	if (!job || !id || !h) {
		return;
	}

	if (status == DNS_STATUS_OK) {
		job->need_redraw = TRUE;
		job->delayed.queued_hosts = host_list_add (job->delayed.queued_hosts, h);
		job->progress.done++;
	}

	for (tmp = job->host_handlers; tmp; tmp = tmp->next) {
		(* (host_func) tmp->data) (job, h, status);
	}

	job->hosts = g_slist_remove (job->hosts, h);
	host_unref (h);

	if (job->hosts == NULL) {
		dns_set_callback (NULL, NULL);
		stat_next (job);
	}
}


static void change_state (struct stat_job *job, enum stat_state state) {
	GSList *tmp;

	for (tmp = job->state_handlers; tmp; tmp = tmp->next) {
		(* (state_func) tmp->data) (job, state);
	}
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

static void stat_next_masters (struct stat_job *job) {
	GSList *list = NULL;
	GSList *tmp = NULL;
	GSList *hostnames = NULL;
	struct master *m = NULL;

	debug_increase_indent();
	debug (3, "Job %lx  Have job->masters", job);
	job->state = STAT_UPDATE_SOURCE;

	move_q2masters_to_top (&job->masters);

	// store all unresolved hostnames in hostnames
	for (list = job->masters; list; list = list->next) {
		m = (struct master *) list->data;

		if (!m->host && m->hostname) {
			for (tmp = hostnames; tmp; tmp = tmp->next) {
				if (strcmp (tmp->data, m->hostname) == 0) {
					break;
				}
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

		for (list = hostnames; list; list = list->next) {
			dns_lookup ((char *) list->data);
		}

		g_slist_free (hostnames);
		hostnames = NULL;
	}

	stat_update_masters (job);
	debug_decrease_indent();
	return;
}

static void stat_next_names (struct stat_job *job) {
	GSList *list = NULL;
	GSList *tmp = NULL;
	GSList *hostnames = NULL;
	struct userver *us = NULL;

	debug (3, "Job %lx  job->names", job);
	job->state = STAT_RESOLVE_NAMES;

	for (list = job->names; list; list = list->next) {
		us = (struct userver *) list->data;

		for (tmp = hostnames; tmp; tmp = tmp->next) {
			if (strcmp (tmp->data, us->hostname) == 0) {
				break;
			}
		}
		if (tmp == NULL) {
			hostnames = g_slist_prepend (hostnames, us->hostname);
		}
	}

	dns_set_callback (stat_name_resolved_callback, job);
	job->progress.tasks = g_slist_length (hostnames);
	change_state (job, STAT_RESOLVE_NAMES);

	for (list = hostnames; list; list = list->next) {
		dns_lookup ((char *) list->data);
	}

	g_slist_free (hostnames);
	hostnames = NULL;

	return;
}

static void stat_next_servers (struct stat_job *job) {
	debug (3, "Servers:  Job %lx  server list %lx", job, job->servers);
	if (!job->need_refresh) {
		stat_close (job, FALSE);
		return;
	}

	job->state = STAT_REFRESH_SERVERS;

	if (default_resolve_on_update) {
		job->hosts = merge_hosts_to_resolve (job->hosts, job->servers);
	}

	job->progress.tasks = g_slist_length (job->servers);
	change_state (job, STAT_REFRESH_SERVERS);

	if (!stat_open_conn_qstat (job)) {
		/* It's very bad, stop everything. */
		xqf_error ("Error! Could not stat_open_conn_qstat()");
		stat_close (job, TRUE);
	}
	return;
}

static void stat_next_hosts (struct stat_job *job) {
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

static void stat_next (struct stat_job *job) {
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
		job->delayed.timeout_id = g_timeout_add (1000, job->delayed.refresh_handler, job);
	}

	stat_next (job);
	debug_decrease_indent();
}


void stat_stop (struct stat_job *job) {
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

	if (job->delayed.queued_servers) {
		server_list_free (job->delayed.queued_servers);
	}

	if (job->delayed.queued_hosts) {
		host_list_free (job->delayed.queued_hosts);
	}

#ifdef DEBUG
	if (job->data) {
		fprintf (stderr, "stat.c: stat_job_free() -- \'data\' must be NULL!\n");
	}
#endif

	g_free (job);
}
