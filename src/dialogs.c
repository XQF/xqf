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
#include <stdio.h>
#include <stdarg.h>     /* va_start, va_end */

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "utils.h"
#include "dialogs.h"
#include "loadpixmap.h"

static int destroy_on_escape (GtkWidget *widget, GdkEventKey *event) {

	if (event->keyval == GDK_Escape) {
		gtk_widget_destroy (widget);
		return TRUE;
	}

	return FALSE;
}

GtkWidget *dialog_create_modal_transient_window (const char *title,
		int close_on_esc,
		int allow_resize,
		GtkSignalFunc on_destroy) {
	GtkWidget *window;
	GtkWidget *parent;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
	g_signal_connect (GTK_OBJECT (window), "delete_event",
			G_CALLBACK (window_delete_event_callback), NULL);
	if (on_destroy) {
		g_signal_connect (GTK_OBJECT (window), "destroy",
				G_CALLBACK (on_destroy), NULL);
	}
	g_signal_connect (GTK_OBJECT (window), "destroy",
			G_CALLBACK (gtk_main_quit), NULL);

	if (close_on_esc) {
		g_signal_connect (GTK_OBJECT (window), "key_press_event",
				G_CALLBACK (destroy_on_escape), NULL);
	}

	if (title)
		gtk_window_set_title (GTK_WINDOW (window), title);

	gtk_widget_realize (window);

	gdk_window_set_decorations (window->window, (allow_resize)?
			GDK_DECOR_BORDER | GDK_DECOR_TITLE | GDK_DECOR_RESIZEH :
			GDK_DECOR_BORDER | GDK_DECOR_TITLE);

	gdk_window_set_functions (window->window, (allow_resize)?
			GDK_FUNC_MOVE | GDK_FUNC_CLOSE | GDK_FUNC_RESIZE :
			GDK_FUNC_MOVE | GDK_FUNC_CLOSE);

	gtk_window_set_modal (GTK_WINDOW (window), TRUE);

	parent = top_window ();
	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));

	register_window (window);

	return window;
}


void dialog_ok (const char *title, const char *fmt, ...) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	char buf[2048];
	va_list ap;

	if (!fmt)
		return;

	va_start (ap, fmt);
	g_vsnprintf (buf, 2048, fmt, ap);
	va_end (ap);

	window = dialog_create_modal_transient_window (
			(title)? title : _("XQF: Warning!"),
			TRUE, FALSE, NULL);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	/* Message */

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 32);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

	label = gtk_label_new (buf);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	gtk_widget_show (vbox);

	/* Buttons */

	hbox = gtk_hbox_new (TRUE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

	button = gtk_button_new_with_label (_("OK"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);

	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);
}


static void yes_button_clicked_callback (GtkWidget *widget, int *data) {
	*data = 1;
}

static void redial_button_clicked_callback (GtkWidget *widget, int *data) {
	*data = 2;
}


int dialog_yesno (const char *title, int defbutton, char *yes, char *no,
		char *fmt, ...) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	int res = 0;
	char buf[2048];
	va_list ap;

	if (!fmt)
		return res;

	va_start (ap, fmt);
	g_vsnprintf (buf, 2048, fmt, ap);
	va_end (ap);

	window = dialog_create_modal_transient_window (
			(title)? title : _("XQF: Warning!"),
			TRUE, TRUE, NULL);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	/* Message */

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 32);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

	label = gtk_label_new (buf);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	gtk_widget_show (vbox);

	/* Buttons */

	hbox = gtk_hbox_new (TRUE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

	button = gtk_button_new_with_label ((yes)? yes : _("Yes"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			G_CALLBACK (yes_button_clicked_callback), &res);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	if (defbutton == 0)
		gtk_widget_grab_default (button);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ((no)? no : _("No"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	if (defbutton == 1)
		gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);

	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);

	return res;
}

int dialog_yesnoredial (const char *title, int defbutton, char *yes, char *no, char *redial,
		char *fmt, ...) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	int res = 0;
	char buf[2048];
	va_list ap;

	if (!fmt)
		return res;

	va_start (ap, fmt);
	g_vsnprintf (buf, 2048, fmt, ap);
	va_end (ap);

	window = dialog_create_modal_transient_window (
			(title)? title : _("XQF: Warning!"),
			TRUE, TRUE, NULL);

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	/* Message */

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 32);
	gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

	label = gtk_label_new (buf);
	gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);

	gtk_widget_show (vbox);

	/* Buttons */

	hbox = gtk_hbox_new (TRUE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 4);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, TRUE, 0);

	button = gtk_button_new_with_label ((yes)? yes : _("Yes"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			G_CALLBACK (yes_button_clicked_callback), &res);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	if (defbutton == 0)
		gtk_widget_grab_default (button);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ((no)? no : _("No"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	if (defbutton == 1)
		gtk_widget_grab_default (button);
	gtk_widget_show (button);

	button = gtk_button_new_with_label ((redial)? redial : _("Redial"));
	gtk_widget_set_usize (button, 96, -1);
	gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			G_CALLBACK (redial_button_clicked_callback), &res);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	if (defbutton == 2)
		gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);

	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);

	return res;
}


static GtkWidget *enter_string_entry;
static GtkWidget *enter_string_opt_button;
static char *enter_string_res;
static int *enter_string_optval;


static void enter_string_activate_callback (GtkWidget *widget, gpointer data) {
	enter_string_res = strdup_strip (gtk_entry_get_text (GTK_ENTRY (widget)));
	if (enter_string_optval) {
		*enter_string_optval = GTK_TOGGLE_BUTTON (enter_string_opt_button)->active;
	}
}


static char *va_enter_string_dialog (int visible, char *optstr, int *optval, char *fmt, va_list ap) {
	GtkWidget *window;
	GtkWidget *main_vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *label;
	char buf[2048];

	g_vsnprintf (buf, 2048, fmt, ap);

	enter_string_res = NULL;
	enter_string_optval = optval;

	window = dialog_create_modal_transient_window (buf, TRUE, FALSE, NULL);

	main_vbox = gtk_vbox_new (FALSE, 8);
	gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 16);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	/* Message */

	label = gtk_label_new (buf);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	/* Entry */

	enter_string_entry = gtk_entry_new_with_max_length (128);
	gtk_entry_set_visibility (GTK_ENTRY (enter_string_entry), visible);
	gtk_widget_set_usize (enter_string_entry, 128, -1);
	gtk_box_pack_start (GTK_BOX (hbox), enter_string_entry, TRUE, TRUE, 0);
	g_signal_connect (GTK_OBJECT (enter_string_entry), "activate",
			G_CALLBACK (enter_string_activate_callback), NULL);
	g_signal_connect_swapped (GTK_OBJECT (enter_string_entry), "activate",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	gtk_widget_grab_focus (enter_string_entry);
	gtk_widget_show (enter_string_entry);

	/* OK Button */

	button = gtk_button_new_with_label (_("OK"));
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (enter_string_activate_callback),
			GTK_OBJECT (enter_string_entry));
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_show (button);

	/* Cancel Button */

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	/* Option */

	if (optstr) {
		hbox = gtk_hbox_new (FALSE, 8);
		gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

		enter_string_opt_button = gtk_check_button_new_with_label (optstr);

		if (optval) {
			gtk_toggle_button_set_active (
					GTK_TOGGLE_BUTTON (enter_string_opt_button), *optval);
		}
		else {
			gtk_widget_set_sensitive (enter_string_opt_button, FALSE);
		}

		gtk_box_pack_start (GTK_BOX (hbox), enter_string_opt_button,
				FALSE, FALSE, 8);
		gtk_widget_show (enter_string_opt_button);

		gtk_widget_show (hbox);
	}

	gtk_widget_show (main_vbox);
	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);

	return enter_string_res;
}


char *enter_string_dialog (int visible, char *fmt, ...) {
	char *res;
	va_list ap;

	va_start (ap, fmt);
	res = va_enter_string_dialog (visible, NULL, NULL, fmt, ap);
	va_end (ap);

	return res;
}


char *enter_string_with_option_dialog (int visible, char *optstr, int *optval, char *fmt, ...) {
	char *res;
	va_list ap;

	va_start (ap, fmt);
	res = va_enter_string_dialog (visible, optstr, optval, fmt, ap);
	va_end (ap);

	return res;
}

void about_dialog (GtkWidget *widget, gpointer data) {
	const gchar *authors[] = {
		_("Creator:"),
		"   Roman Pozlevich <roma@botik.ru>",
		_("Maintainers:"),
		"   Thomas Debesse <xqf@illwieckz.net>",
		"   Zack Middleton <zturtleman@gmail.com>",
		"   Jordi Mallach <jordi@sindominio.net>",
		"   Artem Vorotnikov <artem@vorotnikov.me>",
		"   Ludwig Nussel <ludwig.nussel@suse.de>",
		"   Alex Burger <alex_b@users.sourceforge.net>",
		"   Bill Adams <bill@evilbill.org>",
		_("Contributors:"),
		"   http://github.com/XQF/xqf/wiki/Contributors", /* https do not create link */
		/* FIXME hacky, limit to the "website" entry & infos in configure.in */
		_("Bug reports and feature requests:"),
		"   http://github.com/XQF/xqf",		/* https do not create link */
	NULL};

	const gchar *artists[] = {
		_("XQF Logo:"),
		"   Luca Camillo <kamy@tutorials.it>",
	NULL};

/*	const gchar *documenters[] = {
		"",
	NULL};
*/
	gchar *copyright;
	copyright = g_strconcat(_("Copyright \xc2\xa9"), " 1998-2018 ", _("XQF Team"), NULL);

	gtk_show_about_dialog (
		NULL, //GTK_WINDOW (window),
		"program-name", _("XQF"),
		"version", XQF_VERSION,
		"copyright", copyright,
//		"license-type", GTK_LICENSE_GPL_2_0,			// TODO
		"comments", _("Game Server Browser"),
		"authors", authors,
//		"documenters", documenters,
		"artists", artists,
		"translator-credits", _("translator-credits"),
		"logo-icon-name", "xqf",
		"website", "https://xqf.github.io",
	NULL);

	g_free(copyright);
}

/** ok callback for file_dialog that sets the selected filename in the
 * textentry that was passed as user data to file_dialog()
 */
static void file_dialog_ok_set_textentry (GtkWidget *widget, gpointer textentry) {
	const char *filename = NULL;
	GtkWidget* filesel = topmost_parent(widget);

	filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (filesel));

	if (!filename)
		return;

	gtk_entry_set_text (GTK_ENTRY (textentry), filename);
}

GtkWidget* file_dialog(const char *title, GtkSignalFunc ok_callback, gpointer data) {
	GtkFileSelection* file_selector;

	file_selector = GTK_FILE_SELECTION(gtk_file_selection_new (title));

	if (!file_selector)
		return NULL;

	gtk_window_set_modal (GTK_WINDOW(file_selector),TRUE);

	// g_signal_connect (GTK_OBJECT (file_selector), "destroy", (GtkSignalFunc) file_dialog_destroy_callback, &file_selector);

	g_signal_connect (GTK_OBJECT (file_selector->ok_button),
			"clicked", ok_callback, data);

	g_signal_connect_swapped (GTK_OBJECT (file_selector->ok_button),
			"clicked", (GtkSignalFunc) gtk_widget_destroy,
			GTK_OBJECT (file_selector));

	/* Connect the cancel_button to destroy the widget */
	g_signal_connect_swapped (GTK_OBJECT (file_selector->cancel_button),
			"clicked", (GtkSignalFunc) gtk_widget_destroy,
			GTK_OBJECT (file_selector));

	gtk_widget_show(GTK_WIDGET(file_selector));

	return GTK_WIDGET(file_selector);
}

GtkWidget* file_dialog_textentry(const char *title, GtkWidget* entry) {
	GtkWidget* filesel = file_dialog(title, G_CALLBACK(file_dialog_ok_set_textentry), entry);
	const char* text = gtk_entry_get_text(GTK_ENTRY (entry));
	if (text && *text) {
		gtk_file_selection_set_filename(GTK_FILE_SELECTION(filesel), text);
	}
	return filesel;
}
