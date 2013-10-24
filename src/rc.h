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

#ifndef __RC_H__
#define __RC_H__


#include <gtk/gtk.h>


enum keyword_type {
  KEYWORD_INT,
  KEYWORD_BOOL,
  KEYWORD_STRING
};

struct keyword {
  char *name;
  enum keyword_type required;
  char *config;
};


#define	BUFFER_SIZE	1024

#define STATE_SPACE	0
#define STATE_TOKEN	1
#define STATE_INT	2
#define STATE_STRING	3
#define STATE_COMMENT	4

#define TOKEN_EOF	0
#define TOKEN_KEYWORD	1
#define TOKEN_INT	2
#define TOKEN_STRING	3
#define TOKEN_COMMA	4
#define TOKEN_EOL	5


extern	int rc_parse (void);
extern  int rc_save (void);
extern  int rc_check_dir (void);


#endif /* __RC_H__ */
