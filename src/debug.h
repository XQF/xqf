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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEFAULT_DEBUG_LEVEL 0

#define debug(level,fmt,rest...) debug_int(__FILE__,__LINE__,__FUNCTION__,level,fmt,##rest)
void debug_int(const char* file, int line, const char* function, int level, const char* fmt, ...);
void debug_cmd(int, char *[], char *, ...);
void set_debug_level (int);
int get_debug_level (void);

int debug_increase_indent();
int debug_decrease_indent();

#endif
