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

/*
 * Pure server-filter evaluation logic: no GTK widget construction or
 * dialog code, so this can be linked into tests/test_filter.c on its own.
 * filter.c keeps the dialog UI and calls into these via filter-eval.h.
 */

#include <string.h>

#include <glib.h>

#include "filter-eval.h"
#include "utils.h"

/* Declared in full in pref.h, which we avoid including here so this file
 * stays GTK-dialog-free; test binaries provide this global themselves. */
extern int serverlist_countbots;

GArray       *server_filters;
unsigned int  current_server_filter;


struct server_filter_vars *server_filter_vars_new (void)
{
	struct server_filter_vars *f = g_malloc (sizeof (struct server_filter_vars));
	if (!f) return NULL;

	f->filter_retries = 2;
	f->filter_ping = 9999;
	f->filter_not_full = 0;
	f->filter_not_empty = 0;
	f->filter_no_cheats = 0;
	f->filter_no_password = 0;
	f->filter_name = NULL;
	f->game_contains = NULL;
	f->version_contains = NULL;
	f->game_type = NULL;
	f->map_contains = NULL;
	f->server_name_contains = NULL;
#ifdef USE_GEOIP
	f->countries = g_array_new (FALSE, FALSE, sizeof (int));
#endif

	return f;
}


void server_filter_vars_free (struct server_filter_vars *v)
{
	if (!v) return;

	g_free (v->filter_name);
	g_free (v->game_contains);
	g_free (v->version_contains);
	g_free (v->map_contains);
	g_free (v->server_name_contains);
	g_free (v->game_type);
#ifdef USE_GEOIP
	g_array_free (v->countries, TRUE);
	v->countries = NULL;
#endif
}


/* deep copy of server_filter_vars */
struct server_filter_vars *server_filter_vars_copy (struct server_filter_vars *v)
{
	struct server_filter_vars *f;
#ifdef USE_GEOIP
	unsigned i;
#endif

	if (!v) return NULL;

	f = server_filter_vars_new ();
	if (!f) return NULL;

	f->filter_retries       = v->filter_retries;
	f->filter_ping          = v->filter_ping;
	f->filter_not_full      = v->filter_not_full;
	f->filter_not_empty     = v->filter_not_empty;
	f->filter_no_cheats     = v->filter_no_cheats;
	f->filter_no_password   = v->filter_no_password;
	f->filter_name          = g_strdup (v->filter_name);
	f->game_contains        = g_strdup (v->game_contains);
	f->version_contains     = g_strdup (v->version_contains);
	f->game_type            = g_strdup (v->game_type);
	f->map_contains         = g_strdup (v->map_contains);
	f->server_name_contains = g_strdup (v->server_name_contains);
#ifdef USE_GEOIP
	for (i = 0; i < v->countries->len; i++)
		g_array_append_val (f->countries, g_array_index (v->countries, int, i));
#endif

	return f;
}


/*
 * This applies a filter's attributes to a server entry and returns true if
 * it passes the filter or false if not.
 */
int server_pass_filter (struct server *s)
{
	char **info_ptr;
	struct server_filter_vars *filter;
	int players = s->curplayers;

	/* Filter Zero is No Filter */
	if (current_server_filter == 0) { return TRUE; }

	filter = g_array_index (server_filters, struct server_filter_vars *, current_server_filter - 1);

	if (s->ping == -1)  /* no information */
		return FALSE;

	if (s->retries >= filter->filter_retries)
		return FALSE;

	if (s->ping >= filter->filter_ping)
		return FALSE;

	if (serverlist_countbots && s->curbots <= players)
		players -= s->curbots;

	if (filter->filter_not_full && (players >= s->maxplayers))
		return FALSE;

	if (filter->filter_not_empty && (players == 0))
		return FALSE;

	if (filter->filter_no_cheats && ((s->flags & SERVER_CHEATS) != 0))
		return FALSE;

	if (filter->filter_no_password && ((s->flags & SERVER_PASSWORD) != 0))
		return FALSE;

	if (filter->game_contains && *filter->game_contains) {
		if (!s->game)
			return FALSE;
		else if (!lowcasestrstr (s->game, filter->game_contains))
			return FALSE;
	}

	if (filter->game_type && *filter->game_type) {
		if (!s->gametype)
			return FALSE;
		else if (!lowcasestrstr (s->gametype, filter->game_type))
			return FALSE;
	}

	if (filter->map_contains && *filter->map_contains) {
		if (!s->map)
			return FALSE;
		else if (!lowcasestrstr (s->map, filter->map_contains))
			return FALSE;
	}

	if (filter->version_contains && *filter->version_contains) {
		const char *version = NULL;
		/* Filter for the version */
		for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
			if (strcmp (*info_ptr, "version") == 0) {
				version = info_ptr[1];
			}
		}
		if (!version) {
			return FALSE;
		}
		else if (!lowcasestrstr (version, filter->version_contains)) {
			return FALSE;
		}
	}   /* end version check */

#ifdef USE_GEOIP
	if (filter->countries->len > 0) {
		gboolean have_country = FALSE;
		unsigned i;

		if (!s->country_id) {
			return FALSE;
		}
		else {
			for (i = 0; i < filter->countries->len; ++i) {
				if (g_array_index (filter->countries, int, i) == s->country_id) {
					have_country = TRUE;
				}
			}

			if (!have_country) {
				return FALSE;
			}
		}
	}
#endif

	if (filter->server_name_contains && *filter->server_name_contains) {
		if (!s->name) {
			return FALSE;
		}
		else if (!lowcasestrstr (s->name, filter->server_name_contains)) {
			return FALSE;
		}
	}

	return TRUE;
}


/* QUICK FILTER */

static char *quick_filter_token[8];
static char  quick_filter_str[512] = {0};

int quick_filter (struct server *s)
{
	unsigned i;
	size_t max = sizeof (quick_filter_token) / sizeof (quick_filter_token[0]);

	if (!s || !*quick_filter_str) return TRUE;

	for (i = 0; i < max && quick_filter_token[i]; ++i) {
		if (s->map && strstr (s->map, quick_filter_token[i]))
			continue;

		if (s->game && lowcasestrstr (s->game, quick_filter_token[i]))
			continue;

		if (s->gametype && lowcasestrstr (s->gametype, quick_filter_token[i]))
			continue;

		if (s->name && lowcasestrstr (s->name, quick_filter_token[i]))
			continue;

		if (s->host && s->host->name && lowcasestrstr (s->host->name, quick_filter_token[i]))
			continue;

		{
			gboolean match = FALSE;
			char **info_ptr;
			for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
				if (lowcasestrstr (info_ptr[1], quick_filter_token[i])) {
					match = TRUE;
					break;
				}
			}
			if (!match)
				return FALSE;
		}
	}

	return TRUE;
}


void filter_quick_set (const char *str)
{
	if (str) {
		unsigned num;
		size_t max = sizeof (quick_filter_token) / sizeof (quick_filter_token[0]);
		strncpy (quick_filter_str, str, sizeof (quick_filter_str) - 1);
		num = tokenize (quick_filter_str, quick_filter_token, max, " ");
		if (num < max)
			quick_filter_token[num] = NULL;
	}
	else {
		quick_filter_str[0] = '\0';
		quick_filter_token[0] = NULL;
	}
}


const char *filter_quick_get (void)
{
	if (!*quick_filter_token)
		return NULL;
	return quick_filter_str;
}


void filter_quick_unset (void)
{
	quick_filter_str[0] = '\0';
	quick_filter_token[0] = NULL;
}

/* /QUICK FILTER */
