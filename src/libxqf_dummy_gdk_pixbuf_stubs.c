/* XQF - Quake server browser and launcher
 * Dummy functions for gdk pixbuf
 * Copyright (C) 2005 Ludwig Nussel <l-n@users.sourceforge.net>
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

#include <stddef.h>

void *gdk_pixbuf_new_from_file (const char *filename)
{
	return NULL;
}

void gdk_pixbuf_render_pixmap_and_mask(void *pixbuf,
		void **pixmap_return, void **mask_return,
		int alpha_threshold)
{
	*pixmap_return = NULL;
	*mask_return = NULL;
}

void gdk_pixbuf_unref(void *p) {};
