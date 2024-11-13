/* XQF - Quake server browser and launcher
 * Functions for loading image files from disk
 * Copyright (C) 2002 Ludwig Nussel <l-n@users.sourceforge.net>
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

// modified version of what glade generates

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <dlfcn.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "loadpixmap.h"
#include "pixmaps.h"
#include "debug.h"

/* This is an internally used function to check if a pixmap file exists. */
static char* check_file_exists (const char *directory, const char *filename);


/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
	/* columns rows colors chars-per-pixel */
	"1 1 1 1",
	"  c None",
	/* pixels */
	" "
};

/* This is an internally used function to create pixmaps. */
static GdkPixbuf* create_dummy_pixmap (GtkWidget *widget) {
	GdkPixbuf *pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data ( (const char**)dummy_pixmap_xpm);
	if (pixbuf == NULL)
		xqf_error ("Couldn't create replacement pixmap.");

	return pixbuf;
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
	void add_pixmap_directory (const gchar *directory) {
	pixmaps_directories = g_list_prepend (pixmaps_directories, g_strdup (directory));
}

gchar* find_pixmap_directory(const gchar* filename) {
	gchar* found_filename = NULL;
	GList *elem;

	elem = pixmaps_directories;
	while (elem) {
		found_filename = check_file_exists ((gchar*)elem->data, filename);
		if (found_filename)
			break;
		elem = elem->next;
	}
	return found_filename;
}

GtkWidget* load_pixmap (GtkWidget* widget, const gchar* filename) {
	GtkWidget *image;
	struct pixmap pix = { 0, 0, 0 };
	if (!load_pixmap_as_pixmap(widget, filename, &pix)) {
		pix.pixbuf = create_dummy_pixmap(widget);
	}

	image = gtk_image_new_from_pixbuf (pix.pixbuf);
	free_pixmap (&pix);
	return image;
}

/** find a pixmap file either absolute or in the pixmap search path.
 * @returns filename if file exists, NULL otherwise. must be freed
 */
static char* find_pixmap_file(const char* filename) {
	char *found_filename = NULL;

	g_return_val_if_fail(filename!=NULL, NULL);

	if (!filename[0])
		return NULL;

	// load absolute paths directly
	if (filename[0] == '/')
		found_filename = check_file_exists(NULL, filename);
	else
		found_filename = find_pixmap_directory(filename);

	return found_filename;
}

static int is_suffix(const char* filename, const char* suffix) {
	return(strlen(filename)>strlen(suffix) && !strcmp(filename+strlen(filename)-strlen(suffix), suffix));
}

struct pixmap* load_pixmap_as_pixmap (GtkWidget* widget, const gchar* filename, struct pixmap* pix) {
	gchar *found_filename = NULL;

	g_return_val_if_fail(widget!=NULL, NULL);
	g_return_val_if_fail(pix!=NULL, NULL);

	found_filename = find_pixmap_file(filename);
	if (is_suffix(filename, ".xpm")) // try png instead
	{
		char* tmp = g_strdup(filename);
		strcpy(tmp+strlen(tmp)-3, "png");
		found_filename = find_pixmap_file(tmp);
		g_free(tmp);
	}

	if (!found_filename) {
		// not file on disk maybe xpm compiled into binary
		if (is_suffix(filename, ".xpm")) {
			void* xpm;
			char* p;
			p = found_filename = g_strdup(filename);

			if ((*p >= 'a' && *p <= 'z')
					|| (*p >= 'A' && *p <= 'Z'))
				++p;
			else
				*p++ = '_';

			while (*p) {
				if ((*p >= 'a' && *p <= 'z')
						|| (*p >= 'A' && *p <= 'Z')
						|| (*p >= '0' && *p <= '9')) {
					++p;
				}
				else
					*p++ = '_';
			}

			xpm = dlsym(NULL, found_filename);
			if (xpm) {
				pix->pixbuf = gdk_pixbuf_new_from_xpm_data (xpm);
			}
		}
	}
	else {
		pix->pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);
		debug(4, "loading gdk_pixbuf from file: %s", found_filename);
	}

#ifdef GUI_GTK2
	if (pix->pixbuf) {
		gdk_pixbuf_render_pixmap_and_mask (pix->pixbuf, &pix->pix, &pix->mask, 255);
	}

	if (pix->pixbuf == NULL || pix->pix == NULL) {
#else
	if (pix->pixbuf == NULL) {
#endif
		// translator: %s = file name
		xqf_warning (_("Error loading pixmap file: %s"), found_filename?found_filename:filename);
		g_free (found_filename);
		free_pixmap (pix);
		return NULL;
	}
	g_free (found_filename);

	return pix;
}

/** directory may be null */
static char* check_file_exists (const char* directory, const char* filename) {
	char *full_filename;

	if (directory)
		full_filename = g_strconcat(directory, G_DIR_SEPARATOR_S, filename, NULL);
	else
		full_filename = g_strdup(filename);

	if (access(full_filename, R_OK) == 0)
		return full_filename;
	g_free (full_filename);
	return NULL;
}
