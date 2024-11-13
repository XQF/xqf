/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>
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

#include <sys/types.h>
#include <string.h>     /* strlen, strcpy, strncpy, strcmp, strncmp, memset */
#include <sys/stat.h>   /* stat */
#include <unistd.h>     /* stat */
#include <math.h>       /* pow */

#include <gtk/gtk.h>

#include "xqf-ui.h"
#include "pref.h"
#include "utils.h"
#include "pixmaps.h"
#include "dialogs.h"
#include "skin.h"
#include "skin_pcx.h"

#define NOSKIN_BCOLOR 0x80
#define NOSKIN_WCOLOR 0xC0


static guchar quake_pallete[768] = {
#include "quake_pal.h"
};


static guchar quake2_pallete[768] = {
#include "quake2_pal.h"
};


static GdkColor pcolors[14];
static int pcolors_allocated = FALSE;

static guchar gamma_lookup[256];
static double gamma_lookup_value = 1.0;


static char *dir_qw_skin_filter (const char *dir, const char *str) {
	int len;
	char *res;

	if (!str)
		return NULL;

	len = strlen (str);

	if (len <= 4 || strcmp (str + len - 4, ".pcx"))
		return NULL;

	len -= 4;

	res = g_malloc (len + 1);
	strncpy (res, str, len);
	res[len] = '\0';

	return res;
}


static char *dir_q2_skin_filter (const char *dir, const char *str) {
	int len;
	char *res;

	if (!str)
		return NULL;

	len = strlen (str);

	if (len <= 6 || strcmp (str + len - 6, "_i.pcx"))
		return NULL;

	len -= 6;

	res = g_malloc (len + 1);
	strncpy (res, str, len);
	res[len] = '\0';

	return res;
}


GList *get_qw_skin_list (const char *dir) {
	GList *list;
	char *path;

	path = file_in_dir (dir, "qw/skins");
	list = dir_to_list (path, dir_qw_skin_filter);
	g_free (path);

	return list;
}


static char *dir_alldirs_filter (const char *dir, const char *str) {
	struct stat st_buf;
	int res;
	char *file;

	if (!str || str[0] == '.')
		return NULL;

	file = file_in_dir (dir, str);
	res = stat (file, &st_buf);
	g_free (file);

	if (res == 0 && S_ISDIR (st_buf.st_mode))
		return g_strdup (str);

	return NULL;
}


GList *get_q2_skin_list (const char *dir) {
	GList *list = NULL;
	GList *dirs;
	GList *tmp;
	GList *skins;
	GList *skin;
	char *path;
	char *skin_dir;
	char *skin_str;

	path = file_in_dir (dir, "baseq2/players");
	dirs = dir_to_list (path, dir_alldirs_filter);

	for (tmp = dirs; tmp; tmp = tmp->next) {
		skin_dir = file_in_dir (path, (char *) tmp->data);
		skins = dir_to_list (skin_dir, dir_q2_skin_filter);
		g_free (skin_dir);

		for (skin = skins; skin; skin = skin->next) {
			skin_str = file_in_dir ((char *) tmp->data, (char *) skin->data);
			list = g_list_prepend (list, skin_str);
		}
	}

	g_free (path);
	list = g_list_reverse (list);
	return list;
}


// Copied from GTK2 gtk_fill_lookup_array()
static void pixbuf_draw_set_gamma (double gamma) {
	double one_over_gamma;
	double ind;
	int val;
	int i;

	if (gamma_lookup_value == gamma) {
		return;
	}

	one_over_gamma = 1.0 / gamma;

	for (i = 0; i < 256; i++) {
		ind = (double) i / 255.0;
		val = (int) (255 * pow (ind, one_over_gamma));
		gamma_lookup[i] = val;
	}

	gamma_lookup_value = gamma;
}

static void pixbuf_draw_row (GdkPixbuf *pixbuf, guchar *data, int x, int y, int width) {
	guchar *pixels = gdk_pixbuf_get_pixels (pixbuf);
	int n_channels = gdk_pixbuf_get_n_channels (pixbuf);
	int rowstride  = gdk_pixbuf_get_rowstride (pixbuf);
	int pb_width  = gdk_pixbuf_get_width (pixbuf);
	int pb_height  = gdk_pixbuf_get_height (pixbuf);

	if (x < 0 || y < 0 || width < 0 || x+width > pb_width || y+1 > pb_height) {
		g_warning ("pixbuf_draw_row: invalid draw bounds (x: %d, y: %d, w: %d) for pixbuf (w: %d, h: %d)", x, y, width, pb_width, pb_height);
		return;
	}

	 guchar *dst = pixels + y * rowstride + x * n_channels;

	if (gamma_lookup_value == 1.0) {
		memcpy (dst, data, width * n_channels);
	} else {
		guchar *src = data;
		int i;

		for (i = 0; i < width; i++) {
			dst[0] = gamma_lookup[ src[0] ];
			dst[1] = gamma_lookup[ src[1] ];
			dst[2] = gamma_lookup[ src[2] ];

			if (n_channels == 4) {
				dst[3] = src[3];
			}

			src += n_channels;
			dst += n_channels;
		}
	}
}


void draw_qw_skin (GtkWidget *image, guchar *data, int top, int bottom) {
	GdkPixbuf *pixbuf;
	guchar buf[320*3];
	guchar colormap[256];
	guchar *ptr;
	int i, j, k;
	int fixed_top_color = fix_qw_player_color (top);
	int fixed_bottom_color = fix_qw_player_color (bottom);

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 320, 200);
	pixbuf_draw_set_gamma (1.5);

	if (data) {
		for (i = 0; i < 256; i++)
			colormap[i] = i;

		if (fixed_top_color != 1) {
			for (i = 0; i<16; i++)
				colormap[16*1 + i] = fixed_top_color*16 +
					((fixed_top_color <= 7)? i : 15 - i);
		}

		if (fixed_bottom_color != 6) {
			for (i = 0; i<16; i++)
				colormap[16*6 + i] = fixed_bottom_color*16 +
					((fixed_bottom_color <= 7)? i : 15 - i);
		}

		for (i = 0; i < 200; i++) {
			ptr = buf;
			for (j = 0; j < 320; j++) {
				*ptr++ = quake_pallete[colormap[*data] * 3 + 0];
				*ptr++ = quake_pallete[colormap[*data] * 3 + 1];
				*ptr++ = quake_pallete[colormap[*data] * 3 + 2];
				data++;
			}
			pixbuf_draw_row (pixbuf, buf, 0, i, 320);
		}
	}
	else {
		for (j = 0, k = NOSKIN_WCOLOR; j < 200; j += 40) {
			i = k;
			for (ptr = buf; ptr < buf+320*3; ptr += 40*3) {
				memset (ptr, i, 40*3);
				i = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - i;
			}
			for (i = 0; i < 40; i++) {
				pixbuf_draw_row (pixbuf, buf, 0, j+i, 320);
			}
			k = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - k;
		}
	}

	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);

	g_object_unref (G_OBJECT (pixbuf));
}


void draw_q2_skin (GtkWidget *image, guchar *data, int scale) {
	GdkPixbuf *pixbuf;
	guchar *buf;
	guchar *ptr;
	int i, j, k;

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, 32*scale, 32*scale);
	pixbuf_draw_set_gamma (1.5);

	buf = g_malloc (32*scale*3);

	if (data) {
		for (i = 0; i < 32*scale;) {
			ptr = buf;
			for (j = 0; j < 32; j++) {
				for (k = 0; k < scale; k++) {
					*ptr++ = quake2_pallete[*data * 3 + 0];
					*ptr++ = quake2_pallete[*data * 3 + 1];
					*ptr++ = quake2_pallete[*data * 3 + 2];
				}
				data++;
			}
			for (k = 0; k < scale; k++) {
				pixbuf_draw_row (pixbuf, buf, 0, i++, 32*scale);
			}
		}
	}
	else {
		for (j = 0, k = NOSKIN_WCOLOR; j < 32*scale; j += 32) {
			i = k;
			for (ptr = buf; ptr < buf+32*scale*3; ptr += 32*3) {
				memset (ptr, i, 32*3);
				i = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - i;
			}
			for (i = 0; i < 32; i++) {
				pixbuf_draw_row (pixbuf, buf, 0, j+i, 32*scale);
			}
			k = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - k;
		}
	}

	g_free (buf);

	gtk_image_set_from_pixbuf (GTK_IMAGE (image), pixbuf);

	g_object_unref (G_OBJECT (pixbuf));
}


static char *skin_filename (char *skin, char *subdir, char *ext) {
	char *filename;
	char *ptr;

	filename = g_malloc (strlen (subdir) + strlen (skin) + strlen (ext) + 1);

	ptr = filename;
	strcpy (ptr, subdir); ptr += strlen (subdir);
	strcpy (ptr, skin);   ptr += strlen (skin);
	strcpy (ptr, ext);

	return filename;
}


guchar *get_qw_skin (char *skin, char *path) {
	char *filename;
	char *skinsdir;
	guchar *pcx;
	int skinlen;

	if (!skin || skin[0] == '\0')
		skin = "base.pcx";

	skinsdir = file_in_dir (path, "qw/skins/");
	skinlen = strlen (skin);

	if (skinlen > 4 && g_ascii_strcasecmp (skin + skinlen - 4, ".pcx") == 0)
		filename = file_in_dir (skinsdir, skin);
	else
		filename = skin_filename (skin, skinsdir, ".pcx");

	pcx = read_skin_pcx (filename, FALSE);

#ifdef DEBUG
	if (!pcx)
		fprintf (stderr, "cannot load QW skin %s\n", filename);
#endif

	g_free (filename);
	g_free (skinsdir);
	return pcx;
}

guchar *get_q2_skin (char *skin, char *path) {
	char *skinsdir;
	char *filename;
	guchar *pcx;
	// int skinlen;

	if (!skin || skin[0] == '\0')
		return NULL;

	skinsdir = file_in_dir (path, "baseq2/players/");
	// skinlen = strlen (skin);
	filename = skin_filename (skin, skinsdir, "_i.pcx");

	pcx = read_skin_pcx (filename, TRUE);

#ifdef DEBUG
	if (!pcx)
		fprintf (stderr, "cannot load Q2 skin %s\n", filename);
#endif

	g_free (filename);
	g_free (skinsdir);
	return pcx;
}


static gushort convert_color (unsigned c) {
	c *= 257;               /* scale from char to short */
	c = c + c/3 + 0x2800;   /* add brightness */
	return (c > 0xffff)? 0xffff : c;
}


void allocate_quake_player_colors (GdkWindow *window) {
#ifdef GUI_GTK2
	GdkColormap *colormap;
#endif
	int i, j;

	if (!pcolors_allocated) {
#ifdef GUI_GTK2
		colormap = gdk_drawable_get_colormap (GDK_DRAWABLE (window));
#endif

		for (i = 0; i < 14; i++) {
			j = (i<8)? 11 : 15 - 11;
			pcolors[i].pixel = 0;
			pcolors[i].red   = convert_color (quake_pallete [(i*16 + j)*3 + 0]);
			pcolors[i].green = convert_color (quake_pallete [(i*16 + j)*3 + 1]);
			pcolors[i].blue  = convert_color (quake_pallete [(i*16 + j)*3 + 2]);
#ifdef GUI_GTK2
			if (!gdk_colormap_alloc_color (colormap, &pcolors[i], FALSE, TRUE)) {
				g_warning ("unable to allocate color: ( %d %d %d )",
						pcolors[i].red, pcolors[i].green, pcolors[i].blue);
			}
#endif
		}
		pcolors_allocated = TRUE;
	}
}


void set_bg_color (GtkWidget *widget, int color) {
	GtkStyle *style;

	style = gtk_style_copy (gtk_widget_get_style (widget));
	style->bg [GTK_STATE_NORMAL]   = pcolors [color];
	style->bg [GTK_STATE_ACTIVE]   = pcolors [color];
	style->bg [GTK_STATE_PRELIGHT] = pcolors [color];
	style->bg [GTK_STATE_SELECTED] = pcolors [color];
	style->bg [GTK_STATE_INSENSITIVE] = pcolors [color];

	gtk_widget_set_style (widget, style);
}


GtkWidget *create_color_menu (void (*callback) (GtkWidget*, int)) {
	GtkWidget *menu;
	GtkWidget *menu_item;
	GtkWidget *button;
	int i;

	menu = gtk_menu_new ();

	for (i = 0; i < 14; i++) {

		/* ugly, ugly, ugly... */

		button = gtk_button_new_with_label (" ");
		gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_HALF);
		gtk_widget_set_sensitive (button, FALSE);
		gtk_widget_set_size_request (button, 40, -1);
		gtk_widget_show (button);

		menu_item = gtk_menu_item_new ();
		gtk_container_add (GTK_CONTAINER (menu_item), button);
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		g_signal_connect (menu_item, "activate", G_CALLBACK (callback), GINT_TO_POINTER(i));
		gtk_widget_show (menu_item);

		set_bg_color (menu_item, i);
		set_bg_color (button, i);
	}

	return menu;
}


void qw_colors_pixmap_create (GtkWidget *window, unsigned char top, unsigned char bottom, GSList **cache, struct pixmap *pix) {
	unsigned key;
	int w, h;

	key = (top << 8) + bottom;

	if (cache) {
		if (pixmap_cache_lookup (*cache, pix, key)) {
			return;
		}
	}

	if (!gtk_widget_get_realized (window))
		gtk_widget_realize (window);

	h = player_clist->row_height - 2;
	w = h * 3 / 2;

	two_colors_pixmap (gtk_widget_get_window (window), w, h, &pcolors[top], &pcolors[bottom], pix);
	if (cache)
		pixmap_cache_add (cache, pix, key);
}
