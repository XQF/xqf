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

#ifndef __HISTORY_H__
#define __HISTORY_H__

#include <glib.h>


#define	HISTORY_DEFAULT_MAX_ITEMS	16

struct history {
  GList *items;
  char *id;
  int size;
};

extern	struct history *history_new (char *id);
extern	char *history_add (struct history *h, char *str);
extern  void history_free (struct history *h);


#endif /* __HISTORY_H__ */


