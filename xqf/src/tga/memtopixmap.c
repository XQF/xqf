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

#include "gnuconfig.h"

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-loader.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "tga.h"
#include "memtopixmap.h"

#ifdef STANDALONE
#define xqf_warning(fmt, rem...) fprintf(stderr, fmt, ##rem)
#else
#include "debug.h"
#endif

static void data_free(guchar* pixels, gpointer data)
{
    g_free(data);
}

GdkPixbuf* renderMemToPixbuf(const guchar* mem, size_t len)
{
  GdkPixbufLoader* loader = NULL;
  gboolean ok = FALSE;
  GdkPixbuf* pixbuf = NULL;

#ifdef USE_GTK2
  GError *err=NULL;
#endif

  if(!mem) return NULL;
  if(!len) return NULL;

#if 0
    {
      int fd = open("mapshot.tga", O_WRONLY|O_CREAT|O_TRUNC, 0644 );
      write(fd, mem, len);
      close(fd);
    }
#endif

  loader = gdk_pixbuf_loader_new();
  g_return_val_if_fail(loader!=NULL, NULL);
  
#ifdef USE_GTK2
  ok = gdk_pixbuf_loader_write(loader, mem, len,&err);
  if(err != NULL)
  {
    xqf_warning("%s", err->message);
    g_error_free(err);
  }
  err = NULL;
  gdk_pixbuf_loader_close(loader,&err);
  if(err != NULL)
  {
    xqf_warning("%s", err->message);
    g_error_free(err);
  }
#else
  ok = gdk_pixbuf_loader_write(loader, mem, len);
  gdk_pixbuf_loader_close(loader);
#endif

  if(!ok)
  {
    unsigned h = 0, w = 0;
    unsigned char* data;
    g_free(loader);
    loader = NULL;

    data = LoadTGA(mem, len, &w, &h);
    if(!data)
	return NULL;

//    printf("%dx%d\n", w, h);

    pixbuf = gdk_pixbuf_new_from_data (data,
                                             GDK_COLORSPACE_RGB,
                                             TRUE,
                                             8,
                                             w,
                                             h,
                                             w*4,
                                             data_free,
                                             data);
  }
  else
  {
    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    g_free(loader);
  }

  return pixbuf;
}

void renderMemToGtkPixmap(const guchar* mem, size_t len,
	GdkPixmap **pix, GdkBitmap **mask, guint* width, guint* height, unsigned char brightness)
{
    GdkPixbuf* pixbuf = renderMemToPixbuf(mem, len);

    if(pixbuf)
    {
	GdkPixbuf* pixbuf_tmp = NULL;
	pixbuf_tmp = pixbuf;
	pixbuf = gdk_pixbuf_scale_simple(pixbuf,320,240,GDK_INTERP_TILES);
	*height = gdk_pixbuf_get_height(pixbuf);
	*width = gdk_pixbuf_get_width(pixbuf);

	if(brightness && gdk_pixbuf_get_n_channels (pixbuf) >= 3) // brightness correction
	{
	    unsigned x, y;
	    unsigned w = gdk_pixbuf_get_width (pixbuf);
	    unsigned h = gdk_pixbuf_get_height (pixbuf);
	    unsigned rs = gdk_pixbuf_get_rowstride (pixbuf);
	    unsigned c = gdk_pixbuf_get_n_channels (pixbuf);
	    unsigned char* p = gdk_pixbuf_get_pixels (pixbuf);
	    register unsigned tmp;
	    for(y=0; y < h; ++y)
	    {
		for(x=0; x < w; ++x, p+=c)
		{
		    tmp = p[0] + brightness;
		    p[0] = (tmp>0xFF)?0xFF:tmp;
		    tmp = p[1] + brightness;
		    p[1] = (tmp>0xFF)?0xFF:tmp;
		    tmp = p[2] + brightness;
		    p[2] = (tmp>0xFF)?0xFF:tmp;
		}
		if(x*c<rs)
		{
		    p += (rs - x*c);
		}
	    }
	}

	gdk_pixbuf_render_pixmap_and_mask(pixbuf,pix,mask,0);

	gdk_pixbuf_unref(pixbuf);
	gdk_pixbuf_unref(pixbuf_tmp);
    }
    else
    {
	*width=0;
	*height=0;
    }
}

#ifdef STANDALONE
guchar* rgbbuf;

gboolean
on_darea_expose (GtkWidget *widget,
                 GdkEventExpose *event,
                 GdkPixbuf* pixbuf)
{
  gdk_draw_rgb_image (widget->window, widget->style->fg_gc[GTK_STATE_NORMAL],
                      0, 0, gdk_pixbuf_get_width(pixbuf), gdk_pixbuf_get_height(pixbuf),
                      GDK_RGB_DITHER_MAX, rgbbuf, gdk_pixbuf_get_width(pixbuf) * 3);

  return TRUE;
}

void pixbuf2rgbbuf(GdkPixbuf* pixbuf)
{
	guchar* buf;
	guchar* pos;
	unsigned w;
	unsigned h;
	unsigned rs;
	unsigned x;
	unsigned y;
	unsigned c;

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);
	rs = gdk_pixbuf_get_rowstride (pixbuf);
	c = gdk_pixbuf_get_n_channels (pixbuf);

	printf("%d(%d)x%d %d\n", w, rs, h, c);

	pos = rgbbuf = g_new0(guchar, h*w*3);
	buf = gdk_pixbuf_get_pixels (pixbuf);
	for(y=0; y < h; ++y)
	{
	    for(x=0; x < w; ++x)
	    {
		*pos++ = *buf++;
		*pos++ = *buf++;
		*pos++ = *buf++;
		buf++;
	    }
	    if(x*c<rs)
	    {
		buf += (rs - x*c);
	    }
	}
}

int main (int argc, char* argv[])
{
    int fd;
    struct stat statbuf;
    guchar* mem = NULL;
    GtkWidget* main_window;

    gtk_init (&argc, &argv);

    if(argc < 2)
    {
	puts("need file");
	return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if(fd < 0) return -1;
    if(fstat(fd, &statbuf) == -1) return -1;

    mem = g_new0(guchar, statbuf.st_size);
    if(read(fd, mem, statbuf.st_size) != statbuf.st_size) return -1;
    close(fd);

    main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		  GTK_SIGNAL_FUNC (gtk_main_quit), NULL);


    if(0)
    {
	GtkWidget* widget = NULL;
	GdkPixmap* pix = NULL;
	GdkBitmap* mask = NULL;
	guint width = 0, height = 0;
	
	renderMemToGtkPixmap(mem, statbuf.st_size, &pix, &mask, &width, &height, 64);

	widget = gtk_pixmap_new(pix, mask);
	gtk_container_add (GTK_CONTAINER (main_window), widget);
	gtk_widget_show(widget);
    }
    else if(1)
    {
	GdkPixbuf* pixbuf = renderMemToPixbuf(mem, statbuf.st_size);
	GtkWidget* darea;

	darea = gtk_drawing_area_new ();
	//gtk_widget_set_size_request (darea, IMAGE_WIDTH, IMAGE_HEIGHT);
	gtk_widget_set_usize (darea, gdk_pixbuf_get_width (pixbuf), gdk_pixbuf_get_height(pixbuf));
	gtk_container_add (GTK_CONTAINER (main_window), darea);
	gtk_signal_connect (GTK_OBJECT (darea), "expose-event",
		GTK_SIGNAL_FUNC (on_darea_expose), pixbuf);
	gtk_widget_show_all (main_window);

	pixbuf2rgbbuf(pixbuf);
    }
    else
    {
	unsigned w;
	unsigned h;
	unsigned y;
	GtkWidget* preview;
	GdkPixbuf* pixbuf = renderMemToPixbuf(mem, statbuf.st_size);

	w = gdk_pixbuf_get_width (pixbuf);
	h = gdk_pixbuf_get_height (pixbuf);

	preview = gtk_preview_new(GTK_PREVIEW_COLOR);
	gtk_preview_size(GTK_PREVIEW(preview), w, h);

	pixbuf2rgbbuf(pixbuf);
	for(y=0; y < h; ++y)
	{
	    gtk_preview_draw_row(GTK_PREVIEW(preview), rgbbuf+y*w, 0, y, w);
	}
	gtk_container_add (GTK_CONTAINER (main_window), preview);
	gtk_widget_show(preview);
    }

    gtk_widget_show(main_window);

    gtk_main ();

    g_free(mem);

    return 0;
}
#endif

// vim: sw=4
