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

#ifndef __FLT_PLAYER_H__
#define __FLT_PLAYER_H__

#include <sys/types.h>
#include <regex.h>

#include <gtk/gtk.h>

#include "xqf.h"


enum pattern_mode { 
  PATTERN_MODE_STRING = 0,
  PATTERN_MODE_SUBSTR = 1,
  PATTERN_MODE_REGEXP = 2
};

struct player_pattern {
  char *pattern;
  enum pattern_mode mode;
  unsigned groups;
  char *comment;

  char *error;
  void *data;

  int dirty;
};

extern	int player_filter (struct server *s);

extern	void player_filter_page (GtkWidget *notebook);

extern	void player_filter_new_defaults (void);
extern	void player_filter_cfg_clean_up (void);

extern	int player_filter_add_player (char *name, unsigned mask);

extern	void player_filter_init (void);
extern	void player_filter_done (void);


#endif /* __FLT_PLAYER_H__ */
