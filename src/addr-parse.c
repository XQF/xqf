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
 * parse_address() used to live in server.c, but it doesn't touch any of
 * that file's server/userver hash tables — it's pure "host[:port]" string
 * parsing. Split out so it (and its dependency on games[] via server.c's
 * other functions) doesn't have to be dragged in just to test it.
 */

#include <string.h>
#include <stdlib.h>

#include <glib.h>

#include "utils.h"
#include "server.h"

int parse_address (char *str, char **addr, unsigned short *port) {
	long tmp;
	char *ptr;
	char *endptr;

	if (!str || !addr || !port)
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

	/* malloc(strlen("addr:port") - strlen(":port") + strlen("\0")); */
	*addr = g_malloc (strlen(str) - strlen(ptr) + 1);
	strncpy (*addr, str, ptr - str);
	(*addr) [ptr - str] = '\0';

	if (hostname_is_valid (*addr))
		return TRUE;

	g_free (*addr);
	*addr = NULL;
	*port = 0;
	return FALSE;
}
