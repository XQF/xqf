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

#ifndef __PSEARCH_H__
#define __PSEARCH_H__


#define	PSEARCH_HISTORY_MAXITEMS	10

enum psearch_mode { 
  PSEARCH_MODE_STRING = 0,
  PSEARCH_MODE_SUBSTR = 1,
  PSEARCH_MODE_REGEXP = 2
};

struct psearch_data {
  char *pattern;
  enum psearch_mode mode;
  void *data;
};

extern	int find_player_dialog (void);

extern	void psearch_init (void);
extern	void psearch_done (void);

extern	int psearch_data_is_empty (void);
extern	char *psearch_lookup_pattern (void);

extern	void find_player (int find_next);


#endif /* __PSEARCH_H__ */

