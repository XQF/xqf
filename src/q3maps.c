/* XQF - Quake server browser and launcher
 * Functions for finding installed q3 maps
 * Copyright (C) 2002 Ludwig Nussel <l-n@users.sourceforge.net>
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
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>

#include "debug.h"
#include "zip/unzip.h"

/** open zip file and insert all contained .bsp into maphash */
static void findq3maps_zip(const char* path, GHashTable* maphash)
{
    unzFile f;
    int ret;

    if(!path || !maphash)
	return;

    f=unzOpen(path);
    if(!f)
    {
	debug(0,"error opening zip file %s",path);
	return;
    }

    for (ret = unzGoToFirstFile(f); ret == UNZ_OK; ret = unzGoToNextFile(f))
    {
	enum { bufsize = 256 };
	char buf[bufsize] = {0};
	if(unzGetCurrentFileInfo(f,NULL,buf,bufsize,NULL,0,NULL,0) == UNZ_OK)
	{
	    if(!g_strncasecmp(buf,"maps/",5)
		&& !g_strcasecmp(buf+strlen(buf)-4,".bsp"))
	    {
		// s#maps/(.*)\.bsp#\1#
		char* mapname=g_strndup(buf+5,strlen(buf)-4-5);
		if(g_hash_table_lookup(maphash,mapname))
		{
		    g_free(mapname);
		}
		else
		{
		    g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
		}
	    }
	}
    }

    if(unzClose(f) != UNZ_OK)
    {
	debug(0,"couldn't close file %s",path);
    }
}

/**
 * find all .pk3 files one level under startdir, call findq3maps_zip to insert
 * maps into maphash
 */
void findq3maps_dir(GHashTable* maphash, const char* startdir)
{
    DIR* dir = NULL;
    struct dirent* dire = NULL;
    char* curdir = NULL;
    GSList* dirstack = NULL;
    int maxlevel = 1;
    char* mod = NULL;	
    typedef struct
    {
	char* name;
	int level;
    } DirStackEntry;

    DirStackEntry* dse;
    
    if(!startdir || !maphash)
	return;

    dse = g_new0(DirStackEntry,1);
    dse->name=g_strdup(startdir);

    dirstack = g_slist_prepend(dirstack,dse);

    while(dirstack)
    {
	GSList* current = dirstack;
	dirstack = g_slist_remove_link(dirstack,dirstack);

	dse = current->data;
	curdir = dse->name;
	if(dse->level == 1)
	{
	    if(mod) g_free(mod);
	    mod=g_strdup(g_basename(curdir));
	}

	dir = opendir(curdir);
	if(!dir)
	{
	    perror(curdir);
	    g_free(curdir);
	    g_free(dse);
	    continue;
	}

	while((dire = readdir(dir)))
	{
	    char* name = dire->d_name;
	    struct stat statbuf;

	    if(!strcmp(name,".") || !strcmp(name,".."))
		continue;

	    name = g_strconcat(curdir,"/",name,NULL);
	    if(stat(name,&statbuf))
	    {
		perror(name);
		g_free(name);
		continue;
	    }
	    if(S_ISDIR(statbuf.st_mode))
	    {
		if(maxlevel>=0 && dse->level<maxlevel)
		{
		    DirStackEntry* tmpdse = g_new0(DirStackEntry,1);
		    tmpdse->name=name;
		    tmpdse->level=dse->level+1;
		    dirstack = g_slist_prepend(dirstack,tmpdse);
		}
		else
		{
		    g_free(name);
		}
	    }
	    else
	    {
		if(strlen(dire->d_name)>4
		    && !g_strcasecmp(dire->d_name+strlen(dire->d_name)-4,".pk3"))
		{
		    findq3maps_zip(name,maphash);
		}
		g_free(name);
	    }
	}

	closedir(dir);
	g_free(curdir);
	g_free(dse);
    }
    if(mod)
	g_free(mod);
}

// case insensitive compare for hash
static gint maphashmapsequal(gconstpointer m1, gconstpointer m2)
{
    return (g_strcasecmp(m1,m2)==0);
}

static void maphashforeachfunc(char* key, gpointer value, gpointer user_data)
{
    printf("%s ",key);
}

static gboolean maphashforeachremovefunc(gpointer key, gpointer value, gpointer user_data)
{
    g_free(key);
    return TRUE;
}

static void q3_print_maps(GHashTable* maphash)
{
    g_hash_table_foreach(maphash, (GHFunc) maphashforeachfunc, NULL);
    printf("\n");
}

/** free all keys and destroy maphash */
void q3_clear_maps(GHashTable* maphash)
{
    if(!maphash)
	return;
    g_hash_table_foreach_remove(maphash, maphashforeachremovefunc, NULL);
    g_hash_table_destroy(maphash);
}

/** create map hash */
GHashTable* q3_init_maphash()
{
    return g_hash_table_new(g_str_hash,maphashmapsequal);
}

/** return true if mapname is contained in maphash, false otherwise */
gboolean q3_lookup_map(GHashTable* maphash, const char* mapname)
{
    if(g_hash_table_lookup(maphash,mapname))
	return TRUE;
    return FALSE;
}

#if 0
int main (void)
{
    const char q3dirname[]="/usr/local/games/quake3";
    GHashTable* maphash = NULL;

    maphash = q3_init_maps();

    q3_addmaps(maphash,q3dirname);
    q3_addmaps(maphash,"/home/madmax/.q3a");

    q3_print_maps(maphash);

    printf("%d\n",q3_has_map(maphash,"blub"));
    printf("%d\n",q3_has_map(maphash,"q3dm1"));

    q3_clear_maps(maphash);

    return 0;
}
#endif
