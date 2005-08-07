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

#ifndef __DIALOGS_H__
#define __DIALOGS_H__

#include <gtk/gtk.h>

static inline GtkWidget* topmost_parent(GtkWidget* widget)
{
  for(; widget && widget->parent; widget = widget->parent);
  return widget;
}

extern	GtkWidget *dialog_create_modal_transient_window (const char *title, 
                int close_on_esc, int allow_resize, GtkSignalFunc on_destroy);

extern	void dialog_ok (const char *title, const char *fmt, ...) G_GNUC_PRINTF(2, 3);
extern	int dialog_yesno (const char *title, int defbutton, char *yes, char *no, 
                                                              char *fmt, ...) G_GNUC_PRINTF(5, 6);
extern	int dialog_yesnoredial (const char *title, int defbutton, char *yes, char *no, char *redial,
                                                              char *fmt, ...) G_GNUC_PRINTF(6, 7);
extern	char *enter_string_dialog (int visible, char *fmt, ...) G_GNUC_PRINTF(2, 3);
extern	char *enter_string_with_option_dialog (
                      int visible, char *optstr, int *optval, char *fmt, ...) G_GNUC_PRINTF(4, 5);

void about_dialog (GtkWidget *widget, gpointer data);

/** Create a new file selection widget
 */
GtkWidget* file_dialog(const char *title, GtkSignalFunc ok_callback, gpointer data);

/** create new file_dialog and connect the ok button to the textentry */
GtkWidget* file_dialog_textentry(const char *title, GtkWidget* entry);

#endif /* __DIALOGS_H__ */
