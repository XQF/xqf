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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include "utils.h"
#include "debug.h"

#define PRINT(fmt,rest...) \
  printf("%s: " fmt "\n", __FUNCTION__, ##rest)

#define ERROR(fmt,rest...) \
  PRINT(fmt ": %s", ##rest, strerror(errno))

typedef enum { tFile, tSymlink, tDirecory } FileType;

typedef struct
{
    FileType type;
    char name[PATH_MAX];
} File;

enum { MaxFiles = 255 };
static File* tempfiles[MaxFiles] = {0};
static int currentFile = 0;

#define BASEDIR "/tmp/xqf_test_util/"

static void popFile(int nr)
{
    for( ; nr > 0 && currentFile > 0 ; --nr)
    {
	File* entry = tempfiles[--currentFile];
	
	if(!entry)
	    continue;
	
	switch(entry->type)
	{
	    case tFile:
	    case tSymlink:
		if(unlink(entry->name) == -1)
		{
		    ERROR("unlink %s", entry->name);
		}
		break;
	    case tDirecory:
		if(rmdir(entry->name) == -1)
		{
		    ERROR("rmdir %s", entry->name);
		}
		break;
	}
	g_free(entry);
    }

    if(currentFile < 0)
	currentFile = 0;
}

static void cleanup()
{
    popFile(currentFile);
}

static const char* newFile(FileType type, const char* name)
{
    File* f = NULL;

    if(currentFile >= MaxFiles)
	exit(1);

    f = tempfiles[currentFile] = g_malloc0(sizeof(File));
    f->type = type;
    strcpy(f->name, BASEDIR);
    strcat(f->name, name);

    ++currentFile;

    return f->name;
}


static void doMkdir(const char* dir)
{
    dir = newFile(tDirecory, dir);
    if(mkdir(dir, 0755) == -1)
    {
	ERROR("mkdir %s", dir);
	exit(1);
    }
}

static void doSymlink(const char* oldpath, const char* newpath)
{
    newpath = newFile(tSymlink, newpath);
    if(symlink(oldpath, newpath) == -1)
    {
	ERROR("symlink %s -> %s", newpath, oldpath);
	exit(1);
    }
}

static void doFile(const char* name)
{
    int fd = -1;
    name = newFile(tFile, name);
    if((fd = open(name, O_CREAT|O_EXCL|O_RDWR, 0755)) == -1)
    {
	ERROR("open %s", name);
	exit(1);
    }
    close(fd);
}

void test_find_game_dir()
{
    const char* targets[] =
    {
	"westernq3",
	"WESTERNQ3",
	"triBALctf",
	"Q3UT2",
	"Q3UT3",
	"ABC",
	NULL
    };

    char* result = NULL;
    int type = 0;
    int i = 0;

    doMkdir("westernq3");
    doMkdir("WESTERNQ3");
    doMkdir("tribalctf");
    doMkdir("q3ut3");
    doSymlink("q3ut2", "Q3UT2");
    doSymlink("q3ut3", "Q3UT3");
    
    for(; targets[i]; ++i)
    {
	result = find_game_dir(BASEDIR, targets[i], &type);
	PRINT("search %s, result %s, type %d", targets[i], result, type);
	g_free(result);
    }

    popFile(6);
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

void test_resolve_path()
{
    char buf[PATH_MAX] = {0};
    char* oldpath = NULL;
    const char* files[] =
    {
	"binarypath",  // binary in $PATH
	"relative",    // relative symlink
	"absolute",    // absolute symlink
	"noexist",     // not existing
	BASEDIR "bin/binarypath",
	BASEDIR "bin/relative",
	BASEDIR "bin/absolute",
	BASEDIR "bin/noexist",
	BASEDIR "game/binary",
	"~/bin/bla",
	"~/bin/q3",
	NULL
    };
    int i = 0;

    doMkdir("game");
    doMkdir("bin");
    doFile("game/binary");
    doFile("bin/binarypath");
    doSymlink("../game/binary", "bin/relative");
    doSymlink(BASEDIR "game/binary", "bin/absolute");

    oldpath = getenv("PATH");
    if(!oldpath)
	exit(1);
    snprintf(buf, sizeof(buf), "%s%s:%s", BASEDIR, "bin", oldpath);
    setenv("PATH", buf, 1);

    for(; files[i]; ++i)
    {
	char* path = resolve_path(files[i]);
	PRINT("'%s' -> '%s'", files[i], path);
	g_free(path);
    }

    setenv("PATH", oldpath, 1);
}

int main (int argc, char* argv[])
{
    atexit(cleanup);

    doMkdir("");

    set_debug_level(0);

    test_find_game_dir();
    test_file_in_dir();
    test_resolve_path();

    exit(0);
}

// vim: sw=4
