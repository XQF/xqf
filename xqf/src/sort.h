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

#ifndef __SORT_H__
#define __SORT_H__

#include "xqf.h"


enum ssort_mode {
  SORT_SERVER_NAME = 0,
  SORT_SERVER_ADDRESS,
  SORT_SERVER_PING,
  SORT_SERVER_TO,
  SORT_SERVER_PRIVATE,
  SORT_SERVER_PLAYERS,
  SORT_SERVER_MAP,
  SORT_SERVER_GAME,
  SORT_SERVER_MOD
};

enum psort_mode {
  SORT_PLAYER_NAME = 0,
  SORT_PLAYER_FRAGS,
  SORT_PLAYER_COLOR,
  SORT_PLAYER_SKIN,
  SORT_PLAYER_PING,
  SORT_PLAYER_TIME
};

enum isort_mode {
  SORT_INFO_RULE = 0,
  SORT_INFO_VALUE
};


extern	int compare_servers (const struct server *a, const struct server *b,
                                                        enum ssort_mode mode);
extern	int compare_players (const struct player *a, const struct player *b,
                                                        enum psort_mode mode);
extern	int compare_srvinfo (const char **i1, const char **i2,
                                                        enum isort_mode mode);


#endif /* __SORT_H__ */

