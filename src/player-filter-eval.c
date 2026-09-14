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
 * Pure player-filter evaluation logic: no GTK widget construction or
 * dialog code, so this can be linked into tests/test_player_filter.c on
 * its own. flt-player.c keeps the pattern-editing dialog UI.
 */

#include <regex.h>
#include <string.h>

#include <glib.h>

#include "player-filter-eval.h"
#include "utils.h"

#define REGCOMP_FLAGS (REG_EXTENDED | REG_NOSUB | REG_ICASE)

GSList *players = NULL;
GSList *curplrs = NULL;


static char *get_regerror (int errcode, regex_t *compiled) {
	size_t length;
	char *buffer;

	length = regerror (errcode, compiled, NULL, 0);
	buffer = g_malloc (length);
	regerror (errcode, compiled, buffer, length);
	return buffer;
}


void free_player_pattern_compiled_data (struct player_pattern *pp) {

	if (!pp)
		return;

	if (pp->error) {
		g_free (pp->error);
		pp->error = NULL;
	}

	if (pp->data) {
		if (pp->mode == PATTERN_MODE_REGEXP) {
			regfree ((regex_t *) pp->data);
		}
		g_free (pp->data);
		pp->data = NULL;
	}
}


void free_player_pattern (struct player_pattern *pp) {
	free_player_pattern_compiled_data (pp);
	if (pp->pattern) g_free (pp->pattern);
	if (pp->comment) g_free (pp->comment);
	g_free (pp);
}


void player_pattern_compile (struct player_pattern *pp) {
	int res;

	free_player_pattern_compiled_data (pp);

	if (pp->pattern && pp->pattern[0]) {

		switch (pp->mode) {

			case PATTERN_MODE_REGEXP:
				pp->data = g_malloc (sizeof (regex_t));

				res = regcomp ((regex_t *) pp->data, pp->pattern, REGCOMP_FLAGS);
				if (res) {
					pp->error = get_regerror (res, (regex_t *) pp->data);
					regfree ((regex_t *) pp->data);
					g_free (pp->data);
					pp->data = NULL;
				}
				break;

			case PATTERN_MODE_STRING:
			case PATTERN_MODE_SUBSTR:
			default:
				pp->data = g_ascii_strdown(pp->pattern, -1);    /* g_ascii_strdown does implicit strndup */
				break;

		}

	}
}


struct player_pattern *player_pattern_new (struct player_pattern *src) {
	struct player_pattern *pp;

	pp = g_malloc0 (sizeof (struct player_pattern));

	if (src) {
		pp->mode    = src->mode;
		pp->pattern = g_strdup (src->pattern);
		pp->comment = g_strdup (src->comment);
		pp->groups  = src->groups;
	}
	else {
		pp->mode    = PATTERN_MODE_STRING;
		pp->groups  = PLAYER_GROUP_RED;
	}

	pp->dirty = TRUE;
	return pp;
}


int player_filter (struct server *s) {
	/* The 'vars' is ignored in this function, however, since we need it
	   for applying the server filter, we will put it in the arguments. */

	GSList *list;
	GSList *plist;
	struct player_pattern *pp;
	struct player *p;

	s->flags &= ~PLAYER_GROUP_MASK;

	if (!s->players)
		return FALSE;

	for (plist = s->players; plist; plist = plist->next) {
		p = (struct player *) plist->data;
		p->flags &= ~PLAYER_GROUP_MASK;
	}

	if (!players)
		return FALSE;

	for (list = players; list; list = list->next) {
		pp = (struct player_pattern *) list->data;

		for (plist = s->players; plist; plist = plist->next) {
			p = (struct player *) plist->data;

			if (pp->data && !pp->error) {
				if ((pp->mode == PATTERN_MODE_STRING &&
						g_ascii_strcasecmp (p->name, pp->data) == 0) ||
						(pp->mode == PATTERN_MODE_SUBSTR &&
						 lowcasestrstr (p->name, pp->data)) ||
						(pp->mode == PATTERN_MODE_REGEXP &&
						 regexec ((regex_t *) pp->data, p->name, 0, NULL, 0) == 0)) {

					p->flags |= pp->groups;
					s->flags |= pp->groups;
				}
			}
		}
	}

	return ((s->flags & PLAYER_GROUP_MASK) != 0)? TRUE : FALSE;
}
