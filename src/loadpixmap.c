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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#include "loadpixmap.h"
#include "pixmaps.h"
#include "debug.h"
#include "utils.h"

static char* check_file_exists (const char *directory, const char *filename);

static GList *pixmaps_directories = NULL;

void add_pixmap_directory (const gchar *directory) {
	pixmaps_directories = g_list_prepend (pixmaps_directories, g_strdup (directory));
}

gchar* find_pixmap_directory(const gchar* filename) {
	GList *elem = pixmaps_directories;
	while (elem) {
		gchar *found = check_file_exists ((gchar*)elem->data, filename);
		if (found)
			return found;
		elem = elem->next;
	}
	return NULL;
}

/** Find a pixmap file by absolute path or in the registered search directories.
 *  If the filename has a .xpm extension, looks for the .png equivalent instead.
 *  Returns an allocated path string on success, NULL if not found. Caller must g_free().
 */
static char* find_pixmap_file(const char* filename) {
	char *candidate;

	g_return_val_if_fail(filename != NULL, NULL);
	if (!filename[0])
		return NULL;

	/* Remap .xpm → .png: all game/UI icons are installed as PNGs. */
	if (stri_has_ext(filename, ".xpm")) {
		size_t len = strlen(filename);
		char *png_name = g_strdup(filename);
		strcpy(png_name + len - 3, "png");   /* ".xpm"[-3:] = "xpm" → "png" */
		if (filename[0] == '/')
			candidate = check_file_exists(NULL, png_name);
		else
			candidate = find_pixmap_directory(png_name);
		g_free(png_name);
		return candidate;
	}

	if (filename[0] == '/')
		return check_file_exists(NULL, filename);
	return find_pixmap_directory(filename);
}

GtkWidget* load_pixmap (GtkWidget* widget, const gchar* filename) {
	struct pixmap pix = { 0, 0, 0 };
	if (!load_pixmap_as_pixmap(widget, filename, &pix)) {
		/* 1×1 transparent placeholder */
		pix.pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 1, 1);
		if (pix.pixbuf) {
			gdk_pixbuf_fill (pix.pixbuf, 0x00000000);
			pix.texture = gdk_texture_new_for_pixbuf (pix.pixbuf);
		}
	}
	GtkWidget *image = gtk_image_new_from_paintable (pix.texture ? GDK_PAINTABLE (pix.texture) : NULL);
	free_pixmap (&pix);
	return image;
}

struct pixmap* load_pixmap_as_pixmap (GtkWidget* widget, const gchar* filename, struct pixmap* pix) {
	g_return_val_if_fail(widget != NULL, NULL);
	g_return_val_if_fail(pix != NULL, NULL);

	char *found = find_pixmap_file(filename);
	if (!found) {
		xqf_warning (_("Error loading pixmap file: %s"), filename);
		return NULL;
	}

	debug(4, "loading gdk_pixbuf from file: %s", found);
	pix->pixbuf = gdk_pixbuf_new_from_file(found, NULL);
	g_free(found);

	if (!pix->pixbuf) {
		xqf_warning (_("Error loading pixmap file: %s"), filename);
		return NULL;
	}

	pix->texture = gdk_texture_new_for_pixbuf (pix->pixbuf);
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
