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


#include <stdio.h>
#include <stdarg.h>
#include "debug.h"

static int indent_level;

void debug(int level, char *fmt, ...)
{
  va_list argp;
  int i;
  if( level > debug_level ) return;
  for (i=0;i<indent_level;i++)
  {
    fprintf(stderr, " ");
  }
  fprintf(stderr, "debug(%d): ", level);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
}


void debug_cmd(int level, char *argv[], char *fmt, ...)
{
  va_list argp;
  int i;
  if( level > debug_level ) return;
  fprintf(stderr, "debug(%d): ", level);
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "  EXEC> ");
  for (i = 0; argv[i]; ++i)
    fprintf (stderr, "%s ", argv[i]);
  fprintf(stderr, "\n");
}


void set_debug_level (int level)
{
  debug_level = level;
}

int get_debug_level (void)
{
  return (debug_level);
}

int debug_increase_indent()
{
	indent_level++;
	return indent_level;
}

int debug_decrease_indent()
{
	if(indent_level>0)indent_level--;
	return indent_level;
}
