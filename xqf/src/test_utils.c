/* XQF - Quake server browser and launcher
 * test utility functions
 * Copyright (C) 2004 Ludwig Nussel <l-n@users.sourceforge.net>
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

#include "gnuconfig.h"
#include <stdio.h>
#include "utils.h"
#include "debug.h"

#define PRINT(fmt,rest...) \
  printf("%s: " fmt "\n", __FUNCTION__, ##rest)

void test_find_game_dir()
{
    const char* basedir = "/usr/local/games/quake3";
    const char* targets[] =
    {
	"westernq3",
	"WESTERNQ3",
	"triBALctf",
	"Q3UT2",
	"ABC",
	NULL
    };

    char* result = NULL;
    int type = 0;
    int i = 0;

    for(; targets[i]; ++i)
    {
	result = find_game_dir(basedir, targets[i], &type);
	PRINT("search %s, result %s, type %d", targets[i], result, type);
	g_free(result);
    }
}

void test_file_in_dir()
{
    const char* dirs[] =
    {
	"/a", "b",
	"/a/", "b",
	"/a", "",
	"/a/", "",
	"/a", NULL,
	"/a/", NULL,
	"", "b",
	"/", "b",
	NULL, "b",
	NULL, NULL,
    };
    int i = 0;

    for(; dirs[i] || dirs[i+1]; i+=2)
    {
	char* dir = file_in_dir(dirs[i], dirs[i+1]);
	PRINT("'%s' + '%s' = '%s'", dirs[i], dirs[i+1], dir);
	g_free(dir);
    }
}

int main (int argc, char* argv[])
{
    set_debug_level(0);

    test_find_game_dir();
    test_file_in_dir();

    return 0;
}

// vim: sw=4
