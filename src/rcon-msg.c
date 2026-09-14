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

#include <string.h>

#include <glib.h>

#include "rcon-msg.h"

char *msg_terminate (char *msg, int size) {

	char *newmsg = NULL;
	int newsize = size;
	enum { nothing = 0, append_0 = 1, append_nl = 2 } what = nothing;

	if (!msg || size <= 0)
		return g_strdup("\n");

	if (size == 1) {
		if (msg[0] != '\0')
			what |= append_0;
		if (msg[0] != '\n')
			what |= append_nl;
	}
	else if (msg[size-1] != '\0') {
		// not null terminated, check for newline
		if (msg[size-1] != '\n')
			what |= append_nl;

		what |= append_0;
	}
	else if (msg[size-2] != '\n') {
		// null terminated but no newline
		what |= append_nl;
	}

	if (what & append_0)
		newsize++;
	if (what & append_nl)
		newsize++;

	newmsg = (char*)g_malloc(newsize);
	newmsg = strncpy(newmsg,msg,size);

	if ((what & append_0) || (what & append_nl)) {
		newmsg[newsize-1] = '\0';
	}
	if (what & append_nl) {
		newmsg[newsize-2] = '\n';
	}

	return newmsg;
}
