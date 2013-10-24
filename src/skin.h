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

#ifndef __SKIN_H__
#define __SKIN_H__

#include <gtk/gtk.h>


#define Q2_SKIN_SCALE	3


extern	GList *get_qw_skin_list (const char *dir);
extern	GList *get_q2_skin_list (const char *dir);

extern	void draw_qw_skin (GtkWidget *preview, guchar *data, 
                                                         int top, int bottom);

extern	void draw_q2_skin (GtkWidget *preview, guchar *data, int scale); 

extern	guchar *get_qw_skin (char *filename, char *path);
extern	guchar *get_q2_skin (char *skin, char *path);

extern	void allocate_quake_player_colors (GdkWindow *window);

extern	void set_bg_color (GtkWidget *widget, int color);

extern	GtkWidget *create_color_menu (void (*callback) (GtkWidget*, int));

extern	GdkPixmap *qw_colors_pixmap_create (GtkWidget *window, 
                     unsigned char top, unsigned char bottom, GSList **cache);


#endif /* __SKIN_H__ */

