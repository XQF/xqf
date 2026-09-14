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

#ifndef __PLAYER_FILTER_EVAL_H__
#define __PLAYER_FILTER_EVAL_H__

/*
 * Pure player-filter evaluation logic and pattern data model, split out of
 * flt-player.c so it can be linked into a test binary
 * (tests/test_player_filter.c) without also pulling in flt-player.c's GTK
 * dialog code. flt-player.c still owns and reads/writes `players`/`curplrs`
 * for its pattern-editing UI.
 */

#include "flt-player.h"

/* GSList<struct player_pattern *> — players holds the active, applied
 * pattern set; curplrs is the dialog's working copy, NULL when not open. */
extern GSList *players;
extern GSList *curplrs;

extern struct player_pattern *player_pattern_new (struct player_pattern *src);
extern void                   player_pattern_compile (struct player_pattern *pp);
extern void                   free_player_pattern_compiled_data (struct player_pattern *pp);
extern void                   free_player_pattern (struct player_pattern *pp);

#endif /* __PLAYER_FILTER_EVAL_H__ */
