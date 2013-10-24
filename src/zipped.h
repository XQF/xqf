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

#ifndef __ZIPPED_H__
#define __ZIPPED_H__


#include <sys/types.h>
#include <stdio.h>	/* FILE */


struct zstream {
  FILE *f;
  int is_pipe;
};


extern void zstream_open_r (struct zstream *z, const char *name);
extern void zstream_open_w (struct zstream *z, const char *name);
extern void zstream_close (struct zstream *z);
extern void zstream_unlink (const char *name);


#endif /* __ZIPPED_H__ */

