/* XQF - Quake server browser and launcher
 * render a chunk of memory as GdkPixmap and GdkBitmap
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

#ifndef _MEMTOPIXMAP_H_
#define _MEMTOPIXMAP_H_

void renderMemToGtkPixmap(const guchar* mem, size_t len,
    GdkPixmap **pix, GdkBitmap **mask, guint* width, guint* height, unsigned char brightness);

#endif
