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
#include <string.h>	/* strcmp */

#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"	/* for deprecated functions */
#include "pref.h"
#include "utils.h"
#include "server.h"
#include "host.h"
#include "sort.h"


static inline int compare_strings (const char *str1, const char *str2) {
  int res;

  if (str1) {
    if (str2) {
      res = g_strcasecmp (str1, str2);
      if (!res)
	res = strcmp (str1, str2);
    }
    else {
      res = 1;
    }
  }
  else {
    res = (str2)? -1 : 0;
  }
  return res;
}


int compare_servers (const struct server *s1, const struct server *s2,
                                                       enum ssort_mode mode) {
  int res;

  switch (mode) {

  case SORT_SERVER_NAME:
    res = compare_strings (s1->name, s2->name);
    break;

  case SORT_SERVER_ADDRESS:
    if (show_hostnames) {

      if (s1->host->name && !s2->host->name) {
	res = -1;
	break;
      }

      if (s2->host->name && !s1->host->name) {
	res = 1;
	break;
      }

      if (s1->host->name && s2->host->name) {
	res = compare_strings (s1->host->name, s2->host->name);
	if (res == 0)
	  res = s1->port - s2->port;
	break;
      }

    }

    if (s1->host->ip.s_addr == s2->host->ip.s_addr) {
      res = s1->port - s2->port;
    }
    else {
      res = (g_ntohl (s1->host->ip.s_addr) > g_ntohl (s2->host->ip.s_addr))? 
                                                                       1 : -1;
    }
    break;

  case SORT_SERVER_MAP:
    res = compare_strings (s1->map, s2->map);
    break;

  case SORT_SERVER_GAME:
    res = compare_strings (s1->game, s2->game);
    break;

  case SORT_SERVER_GAMETYPE:
    res = compare_strings (s1->gametype, s2->gametype);
    break;
    
  case SORT_SERVER_PRIVATE:
    if( (s1->flags & SERVER_PASSWORD ) && ( s2->flags & SERVER_PASSWORD ))
      res = 0;
    else if (s1->flags & SERVER_PASSWORD )
      res = 1;
    else if (s2->flags & SERVER_PASSWORD )
      res = -1;
    else
      res = 0;
    break;
    
  case SORT_SERVER_PLAYERS:
    res = s1->curplayers - s2->curplayers;
    if (!res) {
      res = s1->maxplayers - s2->maxplayers;
    }
    break;

  case SORT_SERVER_PING:
    if (s1->ping >= 0) {
      if (s2->ping >= 0) {
	res = s1->ping - s2->ping;
	if (!res) 
	  res = s1->retries - s2->retries;
      }
      else {
	res = -1;
      }
    }
    else {
      res = (s2->ping >= 0)? 1 : 0;
    }
    break;

  case SORT_SERVER_TO:
    if (s1->retries >= 0) {
      if (s2->retries >= 0) {
	res = s1->retries - s2->retries;
	if (!res) 
	  res = s1->ping - s2->ping;
      }
      else {
	res = -1;
      }
    }
    else {
      res = (s2->retries >= 0)? 1 : 0;
    }
    break;

  default:
    res = 0;
    break;
  }

  return res;
}


int compare_players (const struct player *p1, const struct player *p2,
                                                       enum psort_mode mode) {
  int res;

  switch (mode) {

  case SORT_PLAYER_FRAGS:
    res = p1->frags - p2->frags;
    break;

  case SORT_PLAYER_NAME:
    res = compare_strings (p1->name, p2->name);
    break;

  case SORT_PLAYER_TIME:
    res = p1->time - p2->time;
    break;

  case SORT_PLAYER_COLOR:
    res = p1->pants - p2->pants;
    if (!res) {
      res = p1->shirt - p2->shirt;
    }
    break;

  case SORT_PLAYER_PING:
    res = p1->ping - p2->ping;
    break;

  case SORT_PLAYER_SKIN:
    res = compare_strings (p1->skin, p2->skin);
    break;

  default:
    res = 0;
    break;
  }

  return res;
}


int compare_srvinfo (const char **i1, const char **i2, enum isort_mode mode) {
  int res;

  switch (mode) {

  case SORT_INFO_RULE:
    res = compare_strings (i1[0], i2[0]);
    break;

  case SORT_INFO_VALUE:
    res = compare_strings (i1[1], i2[1]);
    break;

  default:
    res = 0;
    break;
  }

  return res;
}


