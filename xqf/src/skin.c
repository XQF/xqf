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
#include <string.h>	/* strlen, strcpy, strncpy, strcmp, strncmp, memset */
#include <sys/stat.h>	/* stat */
#include <unistd.h>	/* stat */

#include <gtk/gtk.h>

#include "xqf-ui.h"
#include "pref.h"
#include "utils.h"
#include "pixmaps.h"
#include "dialogs.h"
#include "skin.h"
#include "skin_pcx.h"

#define NOSKIN_BCOLOR	0x80
#define NOSKIN_WCOLOR	0xC0


static guchar quake_pallete[768] = {
#include "quake_pal.h"
};


static guchar quake2_pallete[768] = {
#include "quake2_pal.h"
};


static GdkColor pcolors[14];
static int pcolors_allocated = FALSE;


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


void draw_qw_skin (GtkWidget *preview, guchar *data, int top, int bottom) {
  guchar buf[320*3];
  guchar colormap[256];
  guchar *ptr;
  int i, j, k;
  int fixed_top_color = fix_qw_player_color (top);
  int fixed_bottom_color = fix_qw_player_color (bottom);

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
      gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i, 320);
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
	gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, j+i, 320);
      }
      k = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - k;
    }
  }

  gtk_widget_draw (preview, NULL);
}


void draw_q2_skin (GtkWidget *preview, guchar *data, int scale) {
  guchar *buf;
  guchar *ptr;
  int i, j, k;

  buf = g_malloc (32*scale*3);

  if (data) {
    for (i = 0; i < 32*scale; ) {
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
	gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, i++, 32*scale);
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
	gtk_preview_draw_row (GTK_PREVIEW (preview), buf, 0, j+i, 32*scale);
      }
      k = (NOSKIN_WCOLOR + NOSKIN_BCOLOR) - k;
    }
  }

  g_free (buf);

  gtk_widget_draw (preview, NULL);
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

  if (skinlen > 4 && g_strcasecmp (skin + skinlen - 4, ".pcx") == 0)
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
  int skinlen;

  if (!skin || skin[0] == '\0')
    return NULL;

  skinsdir = file_in_dir (path, "baseq2/players/");
  skinlen = strlen (skin);
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
  c *= 257;			/* scale from char to short */
  c = c + c/3 + 0x2800;		/* add brightness */
  return (c > 0xffff)? 0xffff : c;
}


void allocate_quake_player_colors (GdkWindow *window) {
  GdkColormap *colormap;
  int i, j;

  if (!pcolors_allocated) {
    colormap = gdk_window_get_colormap (window);

    for (i = 0; i < 14; i++) {
      j = (i<8)? 11 : 15 - 11;
      pcolors[i].pixel = 0;
      pcolors[i].red   = convert_color (quake_pallete [(i*16 + j)*3 + 0]);
      pcolors[i].green = convert_color (quake_pallete [(i*16 + j)*3 + 1]);
      pcolors[i].blue  = convert_color (quake_pallete [(i*16 + j)*3 + 2]);
      if (!gdk_color_alloc (colormap, &pcolors[i])) {
	g_warning ("unable to allocate color: ( %d %d %d )", 
		   pcolors[i].red, pcolors[i].green, pcolors[i].blue);
      }
    }
    pcolors_allocated = TRUE;
  }
}


void set_bg_color (GtkWidget *widget, int color) {
  GtkStyle *style;

  style = gtk_style_copy (widget->style);
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
    gtk_widget_set_usize (button, 40, -1);
    gtk_widget_show (button);

    menu_item = gtk_menu_item_new ();
    gtk_container_add (GTK_CONTAINER (menu_item), button);
    gtk_menu_append (GTK_MENU (menu), menu_item);
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
                          GTK_SIGNAL_FUNC (callback), (gpointer) i);
    gtk_widget_show (menu_item);

    set_bg_color (menu_item, i);
    set_bg_color (button, i);
  }

  return menu;
}


GdkPixmap *qw_colors_pixmap_create (GtkWidget *window, unsigned char top, 
                                       unsigned char bottom, GSList **cache) {
  GdkPixmap *pixmap;
  unsigned key;
  int w, h;

  key = (top << 8) + bottom;

  if (cache) {
    pixmap_cache_lookup (*cache, &pixmap, NULL, key);
    if (pixmap)
      return pixmap;
  }

  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  h = player_clist->row_height - 2;
  w = h * 3 / 2;

  pixmap = two_colors_pixmap (window->window, w, h, &pcolors[top], 
                                                            &pcolors[bottom]);
  if (cache)
    pixmap_cache_add (cache, pixmap, NULL, key);

  return pixmap;
}

