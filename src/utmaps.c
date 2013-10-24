/* XQF - Quake server browser and launcher
 * Functions for finding installed ut/ut2/rune maps
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


/**
 * find all files with specified suffix up to two levels under startdir, insert
 * maps into maphash
 */
void findutmaps_dir(GHashTable* maphash, const char* startdir, const char* suffix)
{
    DIR* dir = NULL;
    struct dirent* dire = NULL;
    char* curdir = NULL;
    GSList* dirstack = NULL;
    int maxlevel = 2;
    char* mod = NULL;	
    typedef struct
    {
	char* name;
	int level;
    } DirStackEntry;

    DirStackEntry* dse;
    
    if(!startdir || !maphash || !suffix)
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
		if(strlen(dire->d_name)>strlen(suffix)
			&& !g_strcasecmp(dire->d_name+strlen(dire->d_name)-strlen(suffix),suffix))
		{
		    // s#(.*)suffix#\1#
		    char* mapname = g_strndup(dire->d_name,strlen(dire->d_name)-strlen(suffix));
		    g_strdown(mapname);
		    if(g_hash_table_lookup(maphash,mapname))
		    {
			g_free(mapname);
		    }
		    else
		    {
			g_hash_table_insert(maphash,mapname,GUINT_TO_POINTER(1));
		    }
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

static gboolean maphashforeachremovefunc(gpointer key, gpointer value, gpointer user_data)
{
    g_free(key);
    return TRUE;
}

/** free all keys and destroy maphash */
void ut_clear_maps(GHashTable* maphash)
{
    if(!maphash)
	return;
    g_hash_table_foreach_remove(maphash, maphashforeachremovefunc, NULL);
    g_hash_table_destroy(maphash);
}

/** create map hash */
GHashTable* ut_init_maphash()
{
    return g_hash_table_new(g_str_hash,g_str_equal);
}

/** return true if mapname is contained in maphash, false otherwise */
gboolean ut_lookup_map(GHashTable* maphash, const char* mapname)
{
    if(g_hash_table_lookup(maphash,mapname))
	return TRUE;
    return FALSE;
}
