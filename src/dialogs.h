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


extern	GtkWidget *dialog_create_modal_transient_window (char *title, 
                int close_on_esc, int allow_resize, GtkSignalFunc on_destroy);

extern	void dialog_ok (char *title, char *fmt, ...);
extern	int dialog_yesno (char *title, int defbutton, char *yes, char *no, 
                                                              char *fmt, ...);
extern	int dialog_yesnoredial (char *title, int defbutton, char *yes, char *no, char *redial,
                                                              char *fmt, ...);
extern	char *enter_string_dialog (int visible, char *fmt, ...);
extern	char *enter_string_with_option_dialog (
                      int visible, char *optstr, int *optval, char *fmt, ...);

#endif /* __DIALOGS_H__ */
