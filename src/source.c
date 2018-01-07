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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>     /* strtol */
#include <string.h>     /* strchr, strcmp, strstr */
#include <sys/stat.h>   /* stat */
#include <unistd.h>     /* stat */
#include <sys/socket.h> /* inet_aton, inet_ntoa */
#include <netinet/in.h> /* inet_aton, inet_ntoa */
#include <arpa/inet.h>  /* inet_aton, inet_ntoa */
#include <errno.h>      /* errno */

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

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

static gboolean master_options_compare(QFMasterOptions* lhs, QFMasterOptions* rhs);
static QFMasterOptions parse_master_options(const char* str);
static void master_options_release(QFMasterOptions* o, gboolean doit);

struct master *favorites = NULL;
GSList *master_groups = NULL;

char* master_prefixes[MASTER_NUM_QUERY_TYPES] = {
	"master://",
	"gmaster://",
	"http://",
	"lan://",
	"file://",
	"gslist://",
};

char* master_designation[MASTER_NUM_QUERY_TYPES] = {
	N_("Standard"),
	N_("Gamespy"),
	N_("http"),
	N_("LAN"),
	N_("File"),
	"gslist",
};

static GSList *all_masters = NULL;


static void save_list (FILE *f, struct master *m) {
	GSList *srv;
	struct server *s;

	if (!m->servers)
		return;
	debug (6, "save_list() -- Saving Server List \"%s\"", m->name);

#ifdef DEBUG
	fprintf (stderr, "Saving server list \"%s\"...\n", m->name);
#endif

	if (m == favorites) {
		fprintf (f, "[%s]\n", m->name);
	}
	else {
		char* str = master_to_url(m);
		fputc('[', f);
		fputs(games[m->type].id, f);
		fputc(' ', f);
		fputs(str, f);
		fputs("]\n", f);
		g_free(str);
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


static struct master *find_master_server (char *addr, unsigned short port, char *str, QFMasterOptions* options) {
	GSList *list;
	struct master *m = NULL;
	struct in_addr ip;

	if (!addr || !port)
		return NULL;

	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;
		if (m->port && m->port == port)
		{
			if (m->hostname)
			{
				if (!g_ascii_strcasecmp (m->hostname, addr)) {
					if ((!str || // pre 0.9.4e list file
								!g_ascii_strcasecmp (games[m->type].id, str)) // list file in new format
							&& master_options_compare(&m->options, options))
						break;
				}
			}
			else
			{
				if (m->host && inet_aton (addr, &ip) && ip.s_addr == m->host->ip.s_addr) {
					if ((!str || // pre 0.9.4e list file
								!g_ascii_strcasecmp (games[m->type].id, str))
							&& master_options_compare(&m->options, options))
						break;
				}
			}
		}
		m = NULL;
	}

	return m;
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
	char *unified, *tmp;
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
	tmp = g_ascii_strdown (unified, -1);    /* g_ascii_strdown does not modify string in place */
	strcpy(unified, tmp);                   /* so recopy returned string at same address */
	g_free(tmp);

	strcpy (unified + (ptr2 - url), ptr3);

	while ((len = strlen (unified)) > 0 && unified[len - 1] == '/')
		unified[len - 1] = '\0';

#ifdef DEBUG
	fprintf (stderr, "URL unification %s -> %s\n", url, unified);
#endif

	return unified;
}


static int _compare_urls (const char *url1, const char *url2, gboolean withopts) {
	char *uni1;
	char *uni2;
	int res = 1;

	uni1 = unify_url (url1);
	uni2 = unify_url (url2);

	/*
	 *  It's error if any URL is NULL
	 *  Don't take care of situation when both of URLs are NULL
	 */

	if (uni1 != NULL && uni2 != NULL) {
		if (!withopts) {
			char* p = NULL;
			p = strchr(uni1, ';');
			if (p) *p='\0';
			p = strchr(uni2, ';');
			if (p) *p='\0';
		}
		res = strcmp (uni1, uni2);
	}

	if (uni1) g_free (uni1);
	if (uni2) g_free (uni2);
	return res;
}

#if 0
static int compare_urls (const char *url1, const char *url2) {
	return _compare_urls(url1, url2, TRUE);
}
#endif

static int compare_urls_noopts (const char *url1, const char *url2) {
	return _compare_urls(url1, url2, FALSE);
}

static struct master *find_master_url (char *url, QFMasterOptions* options) {
	GSList *list;
	struct master *m = NULL;

	if (!url)
		return NULL;

	debug(5, "%s %p", url, options);

	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;
		if (m->url && compare_urls_noopts (m->url, url) == 0
				&& master_options_compare(&m->options, options))
			break;
		else
			m = NULL;
	}

	return m;
}


/**
  @param str type string or favorites->name
  @param url url string, NULL for favorites
  */
static struct master *read_list_parse_master (char *str, char *url) {
	char *addr;
	unsigned short port;
	struct master *m = NULL;
	enum master_query_type query_type;
	QFMasterOptions options = { 0, NULL };
	char* optstr = NULL;

	if (favorites && !g_ascii_strcasecmp (str, favorites->name))
		return favorites;

	debug(5, "%s %s", str, url);

	query_type = get_master_query_type_from_address(str);

	optstr = strchr(str + strlen(master_prefixes[query_type]), ';');
	if (optstr) {
		*optstr++ = '\0';
		options = parse_master_options(optstr);
	}

	switch(query_type) {
		case MASTER_NATIVE:
		case MASTER_GAMESPY:
		case MASTER_LAN:
			if (parse_address (str + strlen(master_prefixes[query_type]), &addr, &port)) {
				m = find_master_server (addr, port, url, &options);
				g_free (addr);
			}
			break;
		case MASTER_HTTP:
		case MASTER_FILE:
		case MASTER_GSLIST:
			m = find_master_url (str, &options);
			break;
		case MASTER_NUM_QUERY_TYPES:
		case MASTER_INVALID_TYPE:
			break;
	}

	master_options_release(&options, TRUE);
	return m;
}

static gint server_sorting_helper (const struct server *s1,
		const struct server *s2) {
	if (s1 == s2)
		return 0;
	else if (s1<s2)
		return -1;
	else
		return 1;
}

static void master_add_server (struct master *m, char *str, enum server_type type) {
	gchar *addr, *tmp;
	unsigned short port;
	struct host *h;
	struct server *s;
	struct userver *us;

	debug (6, "master_add_server() -- Master %lx", m);
	if (parse_address (str, &addr, &port)) {
		h = host_add (addr);
		if (h) {    /* IP address */
			host_ref (h);
			if ((s = server_add (h, port, type)) != NULL) {
				m->servers = server_list_prepend_ndp (m->servers, s);
				/* Since the server_add increments the ref count, and
				   server_list_prepend_ndp ups the ref_count, we should
				   unref it once because we are only keeping it in
				   one list after this function.
				   */
				server_unref (s);
			}
			host_unref (h);
		}
		else {      /* hostname */
			tmp = g_ascii_strdown (addr, -1);   /* g_ascii_strdown does not modify string in place */
			strcpy(addr, tmp);                  /* so recopy returned string at same address */
			g_free(tmp);
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

		if (token[0][0] == '[') {               // line is a master
			ch = strchr (token[0] + 1, ']');    // does token 0 have a ]?
			if (ch) {                           // it's a favorites or pre 0.9.4e lists file
				*ch = '\0';

				if (m && m->servers)
					m->servers = g_slist_reverse (m->servers);

				m = read_list_parse_master (token[0] + 1, NULL);
			}
			else {
				ch = strchr (token[1], ']');    // does token 1 have a ]?
				if (ch) {

					*ch = '\0';

					if (m && m->servers)
						m->servers = g_slist_reverse (m->servers);

					m = read_list_parse_master (token[1], token[0] + 1);    // master, type
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
	int line = 1;

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
			xqf_error("server record is too large in %s, line %d\n", realname, line);
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

			if (pos[0] == '\n') {   /* empty line */
				++line;
				parse_saved_server (strings);
				g_slist_free (strings);
				strings = NULL;
				pos = buf;
			}
			else {
				strings = g_slist_append (strings, pos);

				while (*pos) {
					if (*pos == '\n') {
						++line;
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


char* master_qstat_option(struct master* m) {
	g_return_val_if_fail(m!=NULL,NULL);

	if (m->_qstat_master_option)
		return m->_qstat_master_option;
	else
		return games[m->type].qstat_master_option;
}

void master_set_qstat_option(struct master* m, const char* opt) {
	g_return_if_fail(m!=NULL);
	g_free(m->_qstat_master_option);
	m->_qstat_master_option = opt?g_strdup(opt):NULL;
}

/** no deep copy! */
static QFMasterOptions parse_master_options(const char* str) {
	const char* semicolon = NULL;
	const char* val = NULL;
	QFMasterOptions options = {0, NULL};

	if (!str)
		return options;

	while (str && *str) {
		semicolon = strchr(str, ';');
		if (!semicolon) semicolon = str+strlen(str);

		// find next = before ;
		for (val=str; *val && *val != ';' && *val != '='; ++val);
		if (*val != '=')
			val = NULL;
		else
			++val;

		if (val && semicolon-val && !strncmp(str, "gsmtype", 7))
			options.gsmtype = g_strndup(val, semicolon-val);
		if (val && semicolon-val && !strncmp(str, "portadjust", 7))
			options.portadjust = atoi(val);

		if (*semicolon)
			str = semicolon+1;
		else
			str = semicolon;
	}

	return options;
}

static gboolean master_options_compare(QFMasterOptions* lhs, QFMasterOptions* rhs) {
	if (!lhs || !rhs)
		return FALSE;

	if (lhs->portadjust != rhs->portadjust)
		return FALSE;

	if (lhs->gsmtype && rhs->gsmtype && strcmp(lhs->gsmtype, rhs->gsmtype))
		return FALSE;

	return TRUE;
}

#if 0
char* master_to_string(QFMaster* m) {
	char buf[1024] = {0};

	switch(m->master_type) {
		case MASTER_NATIVE:
		case MASTER_GAMESPY:
		case MASTER_LAN:
			snprintf (buf, sizeof(buf), "[%s %s%s:%d",
					games[m->type].id,
					master_prefixes[m->master_type],
					(m->hostname)? m->hostname : inet_ntoa (m->host->ip),
					m->port
				 );
			break;
		case MASTER_HTTP:
		case MASTER_FILE:
		case MASTER_GSLIST:
			snprintf (buf, sizeof(buf), "[%s %s", games[m->type].id, m->url);
			break;
		case MASTER_NUM_QUERY_TYPES:
		case MASTER_INVALID_TYPE:
			break;
	}

	if (m->options.portadjust)
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), ";portadjust=%d", m->options.portadjust);
	if (m->options.gsmtype)
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), ";gsmtype=%s", m->options.gsmtype);

	snprintf(buf, sizeof(buf)-strlen(buf), "]\n");

	if (*buf)
		return g_strdup(buf);
	else
		return NULL;
}
#endif

char *master_to_url(QFMaster* m) {
	char *query_type;
	char *address;
	char buf[1024] = {0};

	if (m->master_type >= MASTER_NATIVE
			&& m->master_type < MASTER_NUM_QUERY_TYPES) {
		query_type = master_prefixes[m->master_type];
	}
	else
		return NULL;

	switch(m->master_type) {
		case MASTER_NATIVE:
		case MASTER_GAMESPY:
		case MASTER_LAN:
			if (m->hostname) {
				address = m->hostname;
			}
			else if (m->host) {
				address = inet_ntoa(m->host->ip);
			}
			else {
				xqf_error("master %s has neither hostname nor ip address", m->name);
				return NULL;
			}
			snprintf (buf, sizeof(buf), "%s%s:%d", query_type, address, m->port);
			break;
		case MASTER_HTTP:
		case MASTER_FILE:
		case MASTER_GSLIST:
			snprintf (buf, sizeof(buf), "%s", m->url);
			break;
		case MASTER_NUM_QUERY_TYPES:
		case MASTER_INVALID_TYPE:
			break;
	}

	if (m->options.portadjust)
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), ";portadjust=%d", m->options.portadjust);
	if (m->options.gsmtype)
		snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf), ";gsmtype=%s", m->options.gsmtype);

	if (*buf)
		return g_strdup(buf);
	else
		return NULL;
}


/** only free content, not pointer itself */
static void master_options_release(QFMasterOptions* o, gboolean doit) {
	if (!doit || !o) return;
	g_free(o->gsmtype);
}

struct master *add_master (char *path, char *name, enum server_type type, const char* qstat_query_arg, int user, int lookup_only) {
	char *addr = NULL;
	unsigned short port = 0;
	struct master *m = NULL;
	struct host *h = NULL;
	struct master *group = NULL;
	enum master_query_type query_type;
	char* optstr = NULL;
	QFMasterOptions options = {0, NULL};
	gboolean freeoptions = FALSE;

	debug(6,"%s,%s,%d,%d,%d",path,name,type,user,lookup_only);

	query_type = get_master_query_type_from_address(path);
	if (query_type == MASTER_INVALID_TYPE) {
		debug(1,"Invalid Master %s",path);
		return NULL;
	}

	optstr = strchr(path + strlen(master_prefixes[query_type]), ';');
	if (optstr) {
		*optstr++ = '\0';
		options = parse_master_options(optstr);
		freeoptions = TRUE;
	}

	switch(query_type) {
		case MASTER_NATIVE:
		case MASTER_GAMESPY:
		case MASTER_LAN:
			// check for valid hostname/ip
			if (parse_address (path + strlen(master_prefixes[query_type]), &addr, &port)) {
				// if no port was specified, add default master port if available or fail
				if (!port) {
					// use default_port instead of default_master_port for lan broadcasts
					if (query_type == MASTER_LAN) {
						port = games[type].default_port;
						/* no longer necessary as of qstat 2.5c
						// unreal needs one port higher
						if (!strcmp(games[type].qstat_option,"-uns")) {
						port++;
						type=GPS_SERVER;
						}
						*/
					}
					// do not use default for gamespy
					else if (query_type != MASTER_GAMESPY && games[type].default_master_port) {
						port = games[type].default_master_port;
					}
					else {
						master_options_release(&options, freeoptions);
						g_free (addr);
						// translator: %s == url, eg gmaster://bla.blub.org
						dialog_ok (NULL, _("You have to specify a port number for %s."),path);
						return NULL;
					}
				}

				m = find_master_server (addr, port, games[type].id, &options);

			}
			break;

		case MASTER_GSLIST:
			if (!options.gsmtype) {
				master_options_release(&options, freeoptions);
				dialog_ok (NULL, _("'gsmtype' options missing for master %s."), name);
				return NULL;
			}
		case MASTER_HTTP:
		case MASTER_FILE:
			m = find_master_url (path, &options);
			break;
		case MASTER_NUM_QUERY_TYPES:
		case MASTER_INVALID_TYPE:
			master_options_release(&options, freeoptions);
			return NULL;
	}

	if (lookup_only) {
		master_options_release(&options, freeoptions);
		g_free (addr);
		return m;
	}

	// we don't update qstat_query_arg for existing masters -- ln
	if (m) {
		if (user) { // Master renaming is forced by user
			g_free (m->name);
			m->name = g_strdup (name);
			m->user = TRUE;
			master_options_release(&m->options, TRUE);
			m->options = options;
			freeoptions = FALSE;
		}
		else { // Automatically rename masters that are not edited by user
			if (!m->user) {
				g_free (m->name);
				m->name = g_strdup (name);
				master_options_release(&m->options, TRUE);
				m->options = options;
				freeoptions = FALSE;
			}
		}
		master_options_release(&options, freeoptions);
		g_free (addr);
		return m;
	}

	// master was not known already, create new
	switch(query_type) {
		case MASTER_NATIVE:
		case MASTER_GAMESPY:
		case MASTER_LAN:
			m = create_master (name, type, FALSE);

			master_set_qstat_option(m, qstat_query_arg);

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
			break;
		case MASTER_HTTP:
		case MASTER_FILE:
		case MASTER_GSLIST:
			m = create_master (name, type, FALSE);
			m->url = g_strdup (path);
			break;
		case MASTER_NUM_QUERY_TYPES:
		case MASTER_INVALID_TYPE:
			master_options_release(&options, freeoptions);
			return NULL;
	}

	m->master_type = query_type;
	m->options = options;
	freeoptions = FALSE;

	if (m) {
		group = (struct master *) g_slist_nth_data (master_groups, type);
		group->masters = g_slist_append (group->masters, m);
		m->user = user;
	}

	master_options_release(&options, freeoptions);
	return m;
}

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

	host_unref (m->host);

	g_free (m->hostname);
	g_free (m->name);
	g_free (m->url);
	g_free (m->_qstat_master_option);
	g_free (m->options.gsmtype);

	server_list_free (m->servers);
	userver_list_free (m->uservers);

	g_free (m);
}


static char *builtin_masters_update_info[] = {
	/*
	 * known master servers that does not work anymore
	 *
	 */

	// does no longer work, removed 2018-01-07
	"DELETE WARSOWS master://eu.master.warsow.net:27950 warsow.net",
	"DELETE WARSOWS master://eu.master.warsow.gg:27950 warsow.gg",

	// does no longer work, removed 2015-06-27
	"DELETE NEXUIZS master://ghdigital.com ghdigital.com",
	"DELETE QWS master://192.246.40.37:27000 id Limbo",
	"DELETE QWS master://192.246.40.37:27002 id CTF",
	"DELETE QWS master://192.246.40.37:27003 id Team Fortress",
	"DELETE QWS master://192.246.40.37:27004 id Misc",
	"DELETE QWS master://192.246.40.37:27006 id Deathmatch",
	"DELETE T2S master://198.74.35.11:27999 Master 1",
	"DELETE T2S master://198.74.35.17:27999 Master 2",
	"DELETE T2S master://198.74.35.18:27999 Master 3",
	"DELETE WARSOWS master://ghdigital.com ghdigital.com",
	"DELETE WOS master://wolfmaster.idsoftware.com:27950 wolfmaster.idsoftware.com",
	"DELETE XONOTICS master://ghdigital.com ghdigital.com",

	// does no longer work, removed 2014-11-14
	"DELETE IOURTS master://master2.urbanterror.net:27900 master.urbanterror.net",
	"DELETE IOURTS master://master.urbanterror.net:27900 master.urbanterror.net",

	// does no longer work, removed 2014-11-03
	"DELETE A2S,-stma2s master://69.28.151.178:27011 Valve 1",
	"DELETE A2S,-stma2s master://steam1.steampowered.com:27011 Steam 1",
	"DELETE A2S,-stma2s master://steam2.steampowered.com:27011 Steam 2",
	"DELETE AMS http://simplembs.armygame.com/sparse.txt armygame.com",
	"DELETE EFS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=EF&hostport=1 multiplay.co.uk - Elite Force",
	"DELETE ETQWS http://etqw-ipgetter.demonware.net/ipgetter/ demonware",
	"DELETE HLA2S master://steam1.steampowered.com Steam 1",
	"DELETE HLS,-stm master://steam1.steampowered.com Steam 1",
	"DELETE HLS,-stm master://steam2.steampowered.com Steam 2",
	"DELETE MHS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=MOHAA&hostport=1 multiplay.co.uk - MOHAA",
	"DELETE NETP master://netpanzer.dyndns.org netpanzer.dyndns.org",
	"DELETE Q2S http://www.lithium.com/quake2/gamespy.txt Lithium",
	"DELETE Q2S:KP http://www.ogn.org:6666 OGN",
	"DELETE Q2S master://master.planetgloom.com gloom",
	"DELETE Q2S master://masterserver.exhale.de exhale.de",
	"DELETE SAS http://masterserver.savagedemo.s2games.com/gamelist.dat S2 Games",
	"DELETE SNS http://asp.planetquake.com/sinserverlist/servers.txt PlanetQuake",
	"DELETE UNS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT&hostport=1 multiplay.co.uk - UT",

	// does no longer work, removed 2013-10-27
	"DELETE Q3RALLYS master://master.q3alive.net master.q3alive.net",
	"DELETE Q3RALLYS master://master.q3rally.com master.q3rally.com",
	"DELETE SMOKINGUNSS master://master.q3alive.net master.q3alive.net"
	"DELETE SMOKINGUNSS master://master.q3alive.net master.q3alive.net",
	"DELETE SMOKINGUNSS master://soulserv.net:27950 soulserv.net",

	// not used anymore, removed 2013-10-27
	"DELETE Q3RALLYS master://master.ioquake3.org master.ioquake3.org",
	"DELETE REACTIONS master://master.ioquake3.org master.ioquake3.org",
	"DELETE SMOKINGUNSS master://master.ioquake3.org master.ioquake3.org",

	// does no longer work, removed 2005-09-07
	"DELETE AMS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=AA&hostport=1 multiplay.co.uk - Army Ops",
	"DELETE BF1942 http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=BF1942&hostport=1 multiplay.co.uk - BF1942",
	"DELETE CODS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=COD multiplay.co.uk - Call of Duty",
	"DELETE HLS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=HLS&hostport=1 multiplay.co.uk - Half-Life",
	"DELETE QS http://ironman.planetquake.com/serversqspy.txt Ironman",
	"DELETE SOF2S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=SOF2&hostport=1 multiplay.co.uk - SOF2",
	"DELETE UT2004S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K4D&hostport=1 multiplay.co.uk - UT2004 Demo",
	"DELETE UT2004S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K4&hostport=1 multiplay.co.uk - UT2004",
	"DELETE UT2S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K3&hostport=1 multiplay.co.uk - UT2003",
	"DELETE UT2S http://ut2003master.epicgames.com/serverlist/demo-all.txt Epic Demo",
	"DELETE UT2S http://ut2003master.epicgames.com/serverlist/full-all.txt Epic",

	// does no longer work, removed 2005-02-04
	"DELETE WOS master://wolf.idsoftware.com:27950 id",

	// does no longer work, removed 2004-11-26
	"DELETE UNS gmaster://unreal.epicgames.com:28900 Epic",

	// does no longer work, removed 2004-11-01
	"DELETE HLS master://half-life.east.won.net WON East",
	"DELETE HLS master://half-life.west.won.net WON West",

	// does no longer work, removed 2004-10-09
	"DELETE D3P http://www.gameaholic.com/servers/qspy-descent3 gameaholic.com",
	"DELETE D3P master://gt.pxo.net:3445 PXO",

	// does no longer work, removed 2004-09-26
	"DELETE Q2S master://master.quake.inet.fi:27900 iNET (Finland)",
	"DELETE Q2S master://q2master.gxp.de:27900 gXp (Germany)",
	"DELETE Q2S master://q2master.mondial.net.au:27900 Australia",
	"DELETE Q2S master://q2master.planetquake.com:27900 PlanetQuake",
	"DELETE Q2S master://qwmaster.barrysworld.com:27900 BarrysWorld (UK)",
	"DELETE Q2S master://satan.idsoftware.com:27900 id",
	"DELETE Q2S master://telefragged.com:27900 TeleFragged",
	"DELETE Q3S master://q3master.barrysworld.com:27950 BarrysWorld",
	"DELETE QWS master://santa.quakeforge.net:27000 QuakeForge",
	"DELETE UNS gmaster://utmaster.barrysworld.com:28909 BarrysWorld",

	// does no longer work, removed 2004-05-16
	"DELETE T2S master://198.74.32.54:27999 Master 1",
	"DELETE T2S master://198.74.32.55:27999 Master 2",
	"DELETE T2S master://211.233.86.203:28002 Master 3",

	// does no longer work, removed 2003-10-18
	"DELETE HWS master://santa.quakeforge.net:26900 QuakeForge",

	// does no longer work, removed 2002-11-05
	"DELETE QWS master://194.251.249.32:27000 iNET (Finland)",
	"DELETE QWS master://200.255.244.2:27000 Brazil",
	"DELETE QWS master://203.55.240.100:27000 Australia",
	"DELETE QWS master://204.182.161.2:27000 PlanetQuake",
	"DELETE QWS master://207.88.6.18:27000 FortressFest TF",
	"DELETE QWS master://master.quakeworld.ru:27000 Russia",
	"DELETE QWS master://qwmaster.barrysworld.com:27000 BarrysWorld (UK)",
	"DELETE QWS master://qwmaster.mondial.net.au:27000 Mondial Aussie",

	// does no longer work, removed 2002-10-23
	"DELETE T2S master://211.233.32.77:28002 Tribes2 Master",

	// does no longer work, removed 2002-01-05
	"DELETE T2S master://198.74.33.29:28002 NA West1",
	"DELETE T2S master://209.67.28.144:28002 NA East",
	"DELETE T2S master://209.67.28..145:27999 NA East",
	"DELETE T2S master://64.94.105.141:27999 NA West",
	"DELETE T2S master://64.94.105.145:28000 NA West",

	// does no longer work, removed 2001-12-27
	"DELETE Q3S master://q3.golsyd.net.au Australia",
	"DELETE Q3S master://q3master.splatterworld.de Germany",

	/*
	 * known master servers that work
	 *
	 */

	// added 2017-04-09
	"ADD CODUOS http://www.qtracker.com/server_list_details.php?game=callofdutyunitedoffensive qtracker.com",
	"ADD CODUOS master://codmaster.activision.com activision.com",
	"ADD COD2S http://www.qtracker.com/server_list_details.php?game=callofduty2 qtracker.com",
	"ADD COD2S master://cod2master.activision.com activision.com",
	"ADD COD4S http://www.qtracker.com/server_list_details.php?game=callofduty4 qtracker.com",
	"ADD COD4S master://cod4master.activision.com activision.com",

	// added 2017-03-03
	"ADD WOPS master://master.worldofpadman.com:27955 worldofpadman.com",
	"ADD WOPS master://master.worldofpadman.net:27955 worldofpadman.net",

	// added 2017-02-05
	"ADD JK3S master://master.jkhub.org:29060 jkhub.org",
	"ADD JK2S master://master.jkhub.org:28060 jkhub.org",

	// added 2015-08-21
	"ADD POSTAL2,-gsm,postal2 gmaster://master.333networks.com:28900 333networks.com",
	"ADD RUNESRV,-gsm,rune gmaster://master.333networks.com:28900 333networks.com",
	"ADD UNS,-gsm,ut gmaster://master.333networks.com:28900 333networks.com",
	"ADD UNS,-gsm,ut gmaster://master.errorist.tk:28900 errorist.tk",
	"ADD UNS,-gsm,ut gmaster://master.noccer.de:28900 noccer.de",

	// added 2015-08-20
	"ADD AMS,-gsm,armygame gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD BF1942,-gsm,bfield1942 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD MHS,-gsm,mohaa gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD Q2S:KP,-gsm,kingpin gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD RUNESRV,-gsm,rune gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD SMS,-gsm,serioussam gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD SMSSE,-gsm,serioussamse gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD UNS,-gsm,ut gmaster://gsm.qtracker.com:28900 qtracker.com (ut99)",
	"ADD UNS,-gsm,unreal gmaster://gsm.qtracker.com:28900 qtracker.com (unreal)",
	"ADD UT2004S,-gsm,ut2004 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD UT2S,-gsm,ut2 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"ADD SNS,-gsm,sin gmaster://gsm.qtracker.com:28900 qtracker.com",

	// added 2015-08-19
	"ADD ETLS master://master.etlegacy.com:27950 etlegacy.com",

	// added 2015-07-28
	"ADD EFS master://efmaster.tjps.eu:27953 tjps.eu",

	// added 2015-06-28
	"ADD ALIENARENAS http://www.qtracker.com/server_list_details.php?game=alienarena qtracker.com",
	"ADD AWS http://www.qtracker.com/server_list_details.php?game=quakeworld qtracker.com",
	"ADD CODS http://www.qtracker.com/server_list_details.php?game=callofduty qtracker.com",
	"ADD D3G http://www.qtracker.com/server_list_details.php?game=descent3 qtracker.com",
	"ADD DM3S http://www.qtracker.com/server_list_details.php?game=doom3 qtracker.com",
	"ADD EFS http://www.qtracker.com/server_list_details.php?game=eliteforce qtracker.com",
	"ADD ETQWS http://www.qtracker.com/server_list_details.php?game=enemyterritoryquakewars qtracker.com",
	"ADD ETLS http://www.qtracker.com/server_list_details.php?game=wolfensteinenemyterritory qtracker.com",
	"ADD H2S http://www.qtracker.com/server_list_details.php?game=hexen2 qtracker.com",
	"ADD HLS http://www.qtracker.com/server_list_details.php?game=halflife_won2 qtracker.com",
	"ADD HWS http://www.qtracker.com/server_list_details.php?game=hexenworld qtracker.com",
	"ADD IOURTS http://www.qtracker.com/server_list_details.php?game=urbanterror qtracker.com",
	"ADD JK2S http://www.qtracker.com/server_list_details.php?game=jediknight2 qtracker.com",
	"ADD JK3S http://www.qtracker.com/server_list_details.php?game=jediknightjediacademy qtracker.com",
	"ADD NEXUIZS http://www.qtracker.com/server_list_details.php?game=nexuiz qtracker.com",
	"ADD OPENARENAS http://www.qtracker.com/server_list_details.php?game=openarena qtracker.com",
	"ADD Q2S:HR http://www.qtracker.com/server_list_details.php?game=heretic2 qtracker.com",
	"ADD Q2S http://www.qtracker.com/server_list_details.php?game=quake2 qtracker.com",
	"ADD Q3S http://www.qtracker.com/server_list_details.php?game=quake3 qtracker.com",
	"ADD Q4S http://www.qtracker.com/server_list_details.php?game=quake4 qtracker.com",
	"ADD QS http://www.qtracker.com/server_list_details.php?game=quake qtracker.com",
	"ADD QWS http://www.qtracker.com/server_list_details.php?game=quakeworld qtracker.com",
	"ADD SFS http://www.qtracker.com/server_list_details.php?game=soldieroffortune qtracker.com",
	"ADD SOF2S http://www.qtracker.com/server_list_details.php?game=soldieroffortune2 qtracker.com",
	"ADD T2S http://www.qtracker.com/server_list_details.php?game=tribes2 qtracker.com",
	"ADD TREMFUSIONS http://www.qtracker.com/server_list_details.php?game=tremulous qtracker.com",
	"ADD TREMULOUSS http://www.qtracker.com/server_list_details.php?game=tremulous qtracker.com",
	"ADD WARSOWS http://www.qtracker.com/server_list_details.php?game=warsow qtracker.com",
	"ADD WOETS http://www.qtracker.com/server_list_details.php?game=wolfensteinenemyterritory qtracker.com",
	"ADD WOPS http://www.qtracker.com/server_list_details.php?game=worldofpadman qtracker.com",
	"ADD WOS http://www.qtracker.com/server_list_details.php?game=returntocastlewolfenstein qtracker.com",
	"ADD XONOTICS http://www.qtracker.com/server_list_details.php?game=xonotic qtracker.com",

	// added 2015-06-27
	"ADD H2S http://www.gameaholic.com/servers/qspy-hexen2 gameaholic.com",
	"ADD HWS http://www.gameaholic.com/servers/qspy-hexenworld gameaholic.com",
	"ADD Q2S master://master.quakeservers.net:27900 quakeservers.net",
	"ADD Q3S master://dctalk.no-ip.info:27950 dctalk.no-ip.info",
	"ADD Q3S master://master0.excessiveplus.net:27950 excessiveplus.net",
	"ADD QWS http://www.gameaholic.com/servers/qspy-quakeworld gameaholic.com",
	"ADD QWS master://master.quakeservers.net:27000 quakeservers.net",
	"ADD QWS master://qwmaster.fodquake.net:27000 fodquake.net",
	"ADD T2S http://t2.plugh.us/t2/serverlist.php t2.plugh.us",
	"ADD T2S http://master.tribesnext.com/list tribesnext.com",
	"ADD UNS http://www.gameaholic.com/servers/qspy-unreal gameaholic.com",
	"ADD UNS gmaster://unreal.epicgames.com:28900 epicgames.com",
	"ADD WARSOWS master://excalibur.nvg.ntnu.no:27950 nvg.ntnu.no",
	"ADD WOS master://dpmaster.deathmask.net:27950 deathmask.net",
	"ADD WOS master://wolfmaster.s4ndmod.com:27950 s4ndmod.com",

	// added 2015-06-22
	"ADD A2S,-stma2s master://hl2master.steampowered.com:27011 steampowered.com",

	// added 2015-06-22
	"ADD HLS master://master3.won2.steamlessproject.nl:27010 steamlessproject.nl",

	// added 2014-11-14
	"ADD IOURTS master://master2.urbanterror.info:27900 urbanterror.info #2",
	"ADD Q3S master://master3.idsoftware.com:27950 idsoftware.com", // same as monster.idsoftware.com
	"ADD Q3S master://master.quake3arena.com:27950 quake3arena.com",

	// added 2014-11-06
	"ADD Q3S master://master.maverickservers.com:27950 maverickservers.com",

	// added 2014-10-30
	"ADD IOURTS master://master.urbanterror.info:27900 urbanterror.info",

	// added 2014-10-23
	"ADD JK2S master://masterjk2.ravensoft.com:28060 ravensoft.com",
	"ADD JK2S master://master.ouned.de:28060 ouned.de",

	// added 2014-10-14
	"ADD TURTLEARENAS master://dpmaster.deathmask.net:27950 deathmask.net",
	"ADD TURTLEARENAS master://master.ioquake3.org:27950 ioquake3.org",

	// added 2014-09-30
	"ADD ETLS master://etmaster.idsoftware.com:27950 idsoftware.com",

	// added 2014-09-28
	"ADD TREMFUSIONS master://master.tremulous.net:30710 tremulous.net",
	"ADD TREMULOUSGPPS master://master.tremulous.net:30700 tremulous.net",
	"ADD TREMULOUSS master://master.tremulous.net:30710 tremulous.net",

	// added 2013-11-10
	"ADD WOPS master://master.worldofpadman.com:27955 worldofpadman.com",

	// added 2013-10-31
	"ADD XONOTICS master://dpmaster.tchr.no tchr.no",

	// added 2013-10-27
	"ADD ALIENARENAS master://master2.corservers.com:27900 corservers.com #2",
	"ADD ALIENARENAS master://master.corservers.com:27900 corservers.com",
	"ADD OPENARENAS master://master.ioquake3.org:27950 ioquake3.org",
	"ADD Q3S master://master.ioquake3.org:27950 ioquake3.org",
	"ADD REACTIONS master://master.rq3.com:27950 rq3.com",

	// added 2013-10-26
	"ADD SMOKINGUNSS master://master.smokin-guns.org:27950 smokin-guns.org",
	"ADD SMOKINGUNSS master://parttimegeeks.net:27950 parttimegeeks.net",
	"ADD UNVANQUISHEDS master://unvanquished.net:27950 unvanquished.net",

	// added 2013-10-25
	"ADD XONOTICS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2013-10-10
	"ADD ZEQ2LITES master://master.ioquake3.org:27950 ioquake3.org",

	// added 2008-09-13
	"ADD OPENARENAS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2008-01-15
	"ADD OTTDS master://master.openttd.org openttd.org",

	// added 2007-01-15
	"ADD Q3S master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-11-01
	"ADD WARSOWS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-10-07
	"ADD Q4S master://q4master.idsoftware.com idsoftware.com",

	// added 2005-09-07
	"ADD SAS http://masterserver.savage.s2games.com/gamelist_full.dat s2games.com",

	// added 2005-06-04
	"ADD NEXUIZS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-03-24
	"ADD UT2004S master://ut2004master2.epicgames.com:28902 epicgames.com #2",

	// added 2004-10-17
	"ADD UT2004S master://ut2004master1.epicgames.com:28902 epicgames.com",

	// added 2004-10-14
	"ADD D3G http://d3.descent.cx/d3cxraw.d3?terse=y descent.cx",

	// added 2004-09-26
	"ADD Q2S master://netdome.biz netdome.biz",

	// added 2004-08-07
	"ADD DM3S master://idnet.ua-corp.com:27650 ua-corp.com",

	// added 2004-06-19
	"ADD JK3S master://masterjk3.ravensoft.com:29060 ravensoft.com",

	// added 2003-11-18
	"ADD CODS master://cod01.activision.com activision.com",

	// added 2003-04-24
	"ADD WOETS master://etmaster.idsoftware.com:27950 idsoftware.com",

	// added 2002-11-05
	"ADD QWS master://qwmaster.ocrana.de:27000 ocrana.de",

	// added 2002-09-29
	"ADD SOF2S master://master.sof2.ravensoft.com:20110 ravensoft.com",

	// added 2002-05-20
	"ADD EFS master://master.stef1.ravensoft.com:27953 ravensoft.com",

	// added 2001-12-27
	"ADD EFS http://www.gameaholic.com/servers/qspy-startrekeliteforce gameaholic.com",
	"ADD Q3S http://www.gameaholic.com/servers/qspy-quake3 gameaholic.com",

	// added 2001-10-12
	"ADD RUNESRV http://www.gameaholic.com/servers/qspy-rune gameaholic.com",

	// added 2001-09-28
	"ADD SFS http://www.gameaholic.com/servers/qspy-soldieroffortune gameaholic.com",

	// added 2000-11-05
	"ADD Q2S:HR http://www.gameaholic.com/servers/qspy-heretic2 gameaholic.com",
	"ADD Q2S http://www.gameaholic.com/servers/qspy-quake2 gameaholic.com",
	"ADD Q2S:KP http://www.gameaholic.com/servers/qspy-kingpin gameaholic.com",
	"ADD QS http://www.gameaholic.com/servers/qspy-quake gameaholic.com",
	"ADD SNS http://www.gameaholic.com/servers/qspy-sin gameaholic.com",

	// lan servers
	"ADD ALIENARENAS lan://255.255.255.255 LAN",
	"ADD AMS lan://255.255.255.255 LAN",
	"ADD CODS lan://255.255.255.255 LAN",
	"ADD CODUOS lan://255.255.255.255 LAN",
	"ADD COD2S lan://255.255.255.255 LAN",
	"ADD COD4S lan://255.255.255.255 LAN",
	"ADD DM3S lan://255.255.255.255 LAN",
	"ADD EFS lan://255.255.255.255 LAN",
	"ADD ETLS lan://255.255.255.255 LAN",
	"ADD ETQWS lan://255.255.255.255 LAN",
	"ADD HLS lan://255.255.255.255 LAN",
	"ADD IOURTS lan://255.255.255.255:50724 LAN",
	"ADD JK2S lan://255.255.255.255 LAN",
	"ADD JK3S lan://255.255.255.255 LAN",
	"ADD MHS lan://255.255.255.255 LAN",
	"ADD NEXUIZS lan://255.255.255.255 LAN",
	"ADD OPENARENAS lan://255.255.255.255 LAN",
	"ADD OTTDS lan://255.255.255.255 LAN",
	"ADD POSTAL2 lan://255.255.255.255 LAN",
	"ADD Q2S lan://255.255.255.255 LAN",
	"ADD Q3RALLYS lan://255.255.255.255 LAN",
	"ADD Q3S lan://255.255.255.255 LAN",
	"ADD Q4S lan://255.255.255.255 LAN",
	"ADD QS lan://255.255.255.255 LAN",
	"ADD QWS lan://255.255.255.255 LAN",
	"ADD REACTIONS lan://255.255.255.255 LAN",
	"ADD RUNESRV lan://255.255.255.255 LAN",
	"ADD SFS lan://255.255.255.255 LAN",
	"ADD SMOKINGUNSS lan://255.255.255.255 LAN",
	"ADD T2S lan://255.255.255.255 LAN",
	"ADD TREMFUSIONS lan://255.255.255.255 LAN",
	"ADD TREMULOUSGPPS lan://255.255.255.255 LAN",
	"ADD TREMULOUSS lan://255.255.255.255 LAN",
	"ADD TURTLEARENAS lan://255.255.255.255 LAN",
	"ADD UNS lan://255.255.255.255 LAN",
	"ADD UNVANQUISHEDS lan://255.255.255.255 LAN",
	"ADD UT2004S lan://255.255.255.255 LAN",
	"ADD UT2S lan://255.255.255.255 LAN",
	"ADD WARSOWS lan://255.255.255.255 LAN",
	"ADD WOETS lan://255.255.255.255 LAN",
	"ADD WOPS lan://255.255.255.255 LAN",
	"ADD WOS lan://255.255.255.255 LAN",
	"ADD XONOTICS lan://255.255.255.255 LAN",
	"ADD ZEQ2LITES lan://255.255.255.255 LAN",

	NULL
};

static char *builtin_gslist_masters_update_info[] = {
	// GameSpy was shutdown, removed 2015-06-22
	"DELETE QS gslist://master.gamespy.com;gsmtype=quake1 Gslist",
	"DELETE QWS gslist://master.gamespy.com;gsmtype=quakeworld Gslist",
	"DELETE Q2S gslist://master.gamespy.com;gsmtype=quake2 Gslist",
	"DELETE Q3S gslist://master.gamespy.com;gsmtype=quake3 Gslist",
	"DELETE Q4S gslist://master.gamespy.com;gsmtype=quake4 Gslist",
	"DELETE WOS gslist://master.gamespy.com;gsmtype=rtcw Gslist",
	"DELETE WOETS gslist://master.gamespy.com;gsmtype=rtcwet Gslist",
	"DELETE DM3S gslist://master.gamespy.com;gsmtype=doom3 Gslist",
	"DELETE RUNESRV gslist://master.gamespy.com;portadjust=-1;gsmtype=rune Gslist",
	"DELETE UNS gslist://master.gamespy.com;portadjust=-1;gsmtype=ut Gslist",
	"DELETE UT2004S gslist://master.gamespy.com;portadjust=-10;gsmtype=ut2004 Gslist",
	"DELETE UT2004S gslist://master.gamespy.com;portadjust=-10;gsmtype=ut2004d Gslist (Demo)",
	"DELETE POSTAL2 gslist://master.gamespy.com;portadjust=-1;gsmtype=postal2 Gslist",
	"DELETE POSTAL2 gslist://master.gamespy.com;portadjust=-1;gsmtype=postal2d Gslist (Demo)",
	"DELETE AMS gslist://master.gamespy.com;portadjust=-1;gsmtype=armygame Gslist",
	"DELETE D3G gslist://master.gamespy.com;gsmtype=descent3 Gslist",
	"DELETE SOF2S gslist://master.gamespy.com;gsmtype=sof2 Gslist",

	// no fixed port offset
	// "ADD GPS gslist://master.gamespy.com;gsmtype=mohaa Gslist",

	// not compatible with linux version
	// "ADD SMS gslist://master.gamespy.com;portadjust=-1;gsmtype=serioussam Gslist",
	// "ADD SMSSE gslist://master.gamespy.com;portadjust=-1;gsmtype=serioussamse Gslist",

	NULL
};

/** parse type of the form <TYPE>[,QSTAT_MASTER_OPION]
 * DESTRUCTIVE
 * @return qstat_master_option or NULL
 */
char* master_id2type(char* token, enum server_type* type) {
	char* ret = NULL;
	char* coma = strchr(token, ',');

	if (coma) {
		*coma = '\0';
		ret = ++coma;
	}

	*type = id2type(token);

	return ret;
}

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
		char* qstat_query_arg = master_id2type (token[1], &type);
		if (type != UNKNOWN_SERVER) {

			if (g_ascii_strcasecmp (token[0], ACTION_ADD) == 0) {
				m = add_master (token[2], token[3], type, qstat_query_arg, FALSE, FALSE);
				if (m && source_ctree != NULL)
					source_ctree_add_master (source_ctree, m);
			}
			else if (g_ascii_strcasecmp (token[0], ACTION_DELETE) == 0) {
				m = add_master (token[2], token[3], type, qstat_query_arg, FALSE, TRUE);
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

void update_master_gslist_builtin (void) {
	char **ptr;

	for (ptr = builtin_gslist_masters_update_info; *ptr; ptr++)
		update_master_list_action (*ptr);
}

// Refresh list of games and masters in left pane
void refresh_source_list (void) {
	GtkCTreeNode *node;
	GSList *list;
	struct master *m;
	struct master *group = NULL;

	// Re-add masters to the list, or remove them if game is not
	// configured and set to show only configured games.
	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;

		if (m == favorites || m->isgroup)
			continue;

		// Look for existing entry in ctree so we only re-add if needed,
		// otherwise everthing gets re-expanded
		node = gtk_ctree_find_by_row_data (GTK_CTREE (source_ctree), NULL, m);

		// If it's not there check to see if we should add it
		if (!node) // It's not in the list so maybe we should add it
		{
			if (!default_show_only_configured_games || games[m->type].cmd)
				source_ctree_add_master (source_ctree, m);
		}
		else // It's in the list so maybe we should delete it
			if (default_show_only_configured_games && !games[m->type].cmd)
				source_ctree_delete_master (source_ctree, m);
	}
	// Remove master group if set to show only configured games.
	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;

		if (m == favorites)
			continue;

		if (default_show_only_configured_games && !games[m->type].cmd) {
			group = (struct master *) g_slist_nth_data (master_groups, m->type);
			source_ctree_remove_master_group (source_ctree, group);
		}
	}
}

void update_master_list_web (void) {
	char **ptr;

	for (ptr = builtin_masters_update_info; *ptr; ptr++)
		update_master_list_action (*ptr);
}


static void load_master_list (void) {
	struct master *m;
	enum server_type type;
	char* qstat_query_arg = NULL;
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
			tmp = strrchr (token[0], ',');
			if (tmp) {
				user = (g_ascii_strcasecmp (tmp+1, "USER") == 0);
				if (user)
					*tmp = '\0';
			}
			qstat_query_arg = master_id2type (token[0], &type);

			if (type != UNKNOWN_SERVER) {

				/*  Use 'user' mode to insert master to the master list
				 *  and fix m->user after that.
				 */

				m = add_master (token[1], token[2], type, qstat_query_arg, TRUE, FALSE);
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
	char typeid[128] = {0};
	char *confstr;
	char *str;
	int n = 0;

	config_clean_section ("/" CONFIG_FILE "/Master List");
	config_push_prefix ("/" CONFIG_FILE "/Master List");

	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;

		if (m == favorites || m->isgroup)
			continue;

		g_snprintf (conf, 64, "master%d", n);

		g_snprintf (typeid, sizeof(typeid), "%s%s%s%s",
				type2id (m->type),
				m->_qstat_master_option?",":"",
				m->_qstat_master_option?m->_qstat_master_option:"",
				m->user?",USER":"");

		str = master_to_url(m);
		confstr = g_strjoin (" ", typeid, str, m->name, NULL);

		config_set_string (conf, confstr);

		g_free (str);
		g_free (confstr);
		n++;
	}

	config_pop_prefix ();
}

void init_masters (int update) {
	typedef void (*server_unref_void)(void*);
	struct master *m;
	struct master *m2;
	int i;
	GSList *list;

	favorites = create_master (N_("Favorites"), UNKNOWN_SERVER, FALSE);

	for (i = 0; i < GAMES_TOTAL; i++) {
		m = create_master (games[i].name, i, TRUE);
		master_groups = g_slist_append (master_groups, m);
	}

	load_master_list ();

	if (update) {
		update_master_list_builtin ();
		if (have_gslist_masters())
			update_master_gslist_builtin();
		config_sync ();
	}

	compat_convert_favorites ();

	read_lists (FILENAME_FAVORITES);
	debug (2, "starting to read server list");
	read_lists (FILENAME_LISTS);
	debug (2, "finished reading server list");

	// Go through all servers, sort them and then remove duplicates
	// master_add_server now uses server_list_prepend_ndp which does not
	// search for duplicates while adding servers.  This is much faster.
	// Because of this, duplicate checking must be done here.
	// Only working on servers, not uservers.  Lists file only contains IPs.
	debug (2, "Searching for duplicate server entries");

	// for each master
	for (list = all_masters; list; list = list->next) {
		m2 = (struct master *) list->data;
		if (!m2)
			continue;
		debug (2, "  Working on master: %s",m2->name);

		m2->servers = slist_sort_remove_dups(m2->servers,(GCompareFunc)server_sorting_helper,(server_unref_void)server_unref);

	}

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

enum master_query_type get_master_query_type_from_address(const gchar* address) {
	enum master_query_type type;
	// check for known master prefix
	debug(6,"get_master_query_type_from_address(%s)",address);
	for (type=MASTER_NATIVE;type<MASTER_NUM_QUERY_TYPES;type++) {
		if (!g_ascii_strncasecmp(address, master_prefixes[type],
					strlen(master_prefixes[type]))) {
			debug(6,"get_master_query_type_from_address: found %s",master_prefixes[type]);
			return type;
		}
	}
	// only accept if there is no :// part at all
	if (lowcasestrstr(address,"://")) {
		debug(6,"get_master_query_type_from_address: invalid");
		return MASTER_INVALID_TYPE;
	}
	else {
		debug(6,"get_master_query_type_from_address: default native");
		return MASTER_NATIVE;
	}
}

gboolean have_gslist_masters() {
	GSList *list;
	struct master *m;

	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;
		if (m->master_type == MASTER_GSLIST)
			return TRUE;
	}

	return FALSE;
}

gboolean have_gslist_installed() {
	char* gslist = find_file_in_path("gslist");
	g_free(gslist);
	return (gslist != NULL);
}

void master_remove_server(struct master* m, struct server* s) {
	if (!m || !s)
		return;

	m->servers = server_list_remove(m->servers, s);
}

void server_remove_from_all(struct server *s) {
	GSList *list;
	struct master *m;

	for (list = all_masters; list; list = list->next) {
		m = (struct master *) list->data;

		if (m == favorites)
			continue;

		m->servers = server_list_remove(m->servers, s);
	}
}
