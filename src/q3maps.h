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

/** free all keys and destroy maphash */
void q3_clear_maps(GHashTable* maphash);

/** create map hash */
GHashTable* q3_init_maphash();

/** return true if mapname is contained in maphash, false otherwise */
gboolean q3_lookup_map(GHashTable* maphash, const char* mapname);

/**
 * find all maps in .pk3 files one level under startdir
 */
void findq3maps_dir(GHashTable* maphash, const char* startdir);

#endif
