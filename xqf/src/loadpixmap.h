/* XQF - Quake server browser and launcher
 * Functions for loading image files from disk
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

#ifndef LOADPIXMAP_H
#define LOADPIXMAP_H

/* Use this function to set the directory containing installed pixmaps. */
void add_pixmap_directory (const gchar* directory);

/* search a pixmap in the paths added by add_pixmap_directory()
 * must free returned path manually
 * returns NULL if file was not found
 */
gchar* find_pixmap_directory(const gchar* filename);

/** This is used to create the pixmaps in the interface. */
GtkWidget* load_pixmap (GtkWidget* widget, const gchar* filename);

/** fill in passed pixmap struct, return pointer to this struct on success,
 * NULL otherwise
 */
struct pixmap* load_pixmap_as_pixmap (GtkWidget* widget, const gchar* filename, struct pixmap* pix);

/** load a pixmap in search path as GdkPixbuf, void* just to avoid a special header */
void* load_pixmap_as_pixbuf (const gchar* filename);
#endif
