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

#ifndef Q3MAPS_H
#define Q3MAPS_H

#include <glib.h>

/**
  * name is the full path of a file at found at level l
  */
typedef void (*FoundFileFunction)(const char* name, int l, gpointer data);

/**
 * the current directory which is at level l contains directory name.
 * return TRUE if this directory should be visited, false otherwise
 */
typedef gboolean (*FoundDirFunction)(const char* name, int l, gpointer data);

/** free all keys and destroy maphash */
void q3_clear_maps(GHashTable* maphash);

/** create map hash */
GHashTable* q3_init_maphash();

/** return true if mapname is contained in maphash, false otherwise */
gboolean q3_lookup_map(GHashTable* maphash, const char* mapname);

/** return true if mapname is contained in maphash, false otherwise
 * same as q3_lookup_mapshot except operates on basename of mapname
 */
gboolean doom3_lookup_map(GHashTable* maphash, const char* mapname);

/** acquire image data, function allocates space in buf, returns size. buf must
 * be freed by caller */
size_t q3_lookup_mapshot(GHashTable* maphash, const char* mapname, guchar** buf);

/** acquire image data, function allocates space in buf, returns size. buf must
 * be freed by caller
 *
 * same as q3_lookup_mapshot except operates on basename of mapname
 */
size_t doom3_lookup_mapshot(GHashTable* maphash, const char* mapname, guchar** buf);

/**
 * find all maps in .pk3 files one level under startdir
 */
void findq3maps(GHashTable* maphash, const char* startdir);

/**
 * find all maps in .pk4 files one level under startdir
 */
void finddoom3maps(GHashTable* maphash, const char* startdir);
void findquake4maps(GHashTable* maphash, const char* startdir);

void findquakemaps(GHashTable* maphash, const char* startdir);

#endif
