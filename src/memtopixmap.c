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

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "debug.h"
#include "memtopixmap.h"

GdkPixbuf* renderMemToPixbuf(const guchar* mem, size_t len) {
	GdkPixbufLoader* loader = NULL;
	gboolean ok = FALSE;
	GdkPixbuf* pixbuf = NULL;

	GError *err=NULL;

	debug(4, "loading gdk_pixbuf from memory");

	if (!mem) {
		debug(4, "memory is empty");
		return NULL;
	}

	if (!len) {
		debug(4, "length is null");
		return NULL;
	}

	loader = gdk_pixbuf_loader_new();
	g_return_val_if_fail(loader!=NULL, NULL);

	ok = gdk_pixbuf_loader_write(loader, mem, len, &err);
	if (err != NULL) {
		xqf_warning("%s", err->message);
		g_error_free(err);
	}
	err = NULL;
	gdk_pixbuf_loader_close(loader, &err);
	if (err != NULL) {
		xqf_warning("%s", err->message);
		g_error_free(err);
	}

	if (!ok) {
		return NULL;
	}
	else {
		pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
		g_object_ref(pixbuf);
		g_object_unref(G_OBJECT(loader));
	}

	return pixbuf;
}

void renderMemToGtkPixmap(const guchar* mem, size_t len, GdkPixmap **pix, GdkBitmap **mask, guint* width, guint* height, gushort overBrightBits) {
	GdkPixbuf* pixbuf = renderMemToPixbuf(mem, len);

	if (pixbuf) {
		GdkPixbuf* pixbuf_tmp = NULL;
		pixbuf_tmp = pixbuf;
		pixbuf = gdk_pixbuf_scale_simple(pixbuf, 320, 240, GDK_INTERP_TILES);
		*height = gdk_pixbuf_get_height(pixbuf);
		*width = gdk_pixbuf_get_width(pixbuf);

		if (overBrightBits > 0 && gdk_pixbuf_get_n_channels (pixbuf) >= 3) // gamma correction
		{
			unsigned x, y;
			unsigned w = gdk_pixbuf_get_width (pixbuf);
			unsigned h = gdk_pixbuf_get_height (pixbuf);
			unsigned rs = gdk_pixbuf_get_rowstride (pixbuf);
			unsigned c = gdk_pixbuf_get_n_channels (pixbuf);
			unsigned char* p = gdk_pixbuf_get_pixels (pixbuf);
			register unsigned tmp;
			for (y=0; y < h; ++y) {
				for (x=0; x < w; ++x, p+=c) {
					tmp = ((int)p[0]) << overBrightBits;
					p[0] = (tmp>0xFF)?0xFF:tmp;
					tmp = ((int)p[1]) << overBrightBits;
					p[1] = (tmp>0xFF)?0xFF:tmp;
					tmp = ((int)p[2]) << overBrightBits;
					p[2] = (tmp>0xFF)?0xFF:tmp;
				}
				if (x*c<rs) {
					p += (rs - x*c);
				}
			}
		}

		gdk_pixbuf_render_pixmap_and_mask(pixbuf, pix, mask, 0);

		g_object_unref(pixbuf);
		g_object_unref(pixbuf_tmp);
	}
	else {
		*width=0;
		*height=0;
	}
}
