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

#include "gnuconfig.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "loadpixmap.h"
#include "pixmaps.h"
#include "i18n.h"
#include "debug.h"

/* This is an internally used function to check if a pixmap file exists. */
gchar* check_file_exists        (const gchar     *directory,
                                        const gchar     *filename);

/* This is an internally used function to create pixmaps. */
static GtkWidget* create_dummy_pixmap  (GtkWidget       *widget);


/* This is a dummy pixmap we use when a pixmap can't be found. */
static char *dummy_pixmap_xpm[] = {
  /* columns rows colors chars-per-pixel */
  "1 1 1 1",
  "  c None",
  /* pixels */
  " "
};

/* This is an internally used function to create pixmaps. */
static GtkWidget*
create_dummy_pixmap                    (GtkWidget       *widget)
{
  GdkColormap *colormap;
  GdkPixmap *gdkpixmap;
  GdkBitmap *mask;
  GtkWidget *pixmap;

  colormap = gtk_widget_get_colormap (widget);
  gdkpixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, colormap, &mask,
                                                     NULL, dummy_pixmap_xpm);
  if (gdkpixmap == NULL)
    xqf_error ("Couldn't create replacement pixmap.");
  pixmap = gtk_pixmap_new (gdkpixmap, mask);
  gdk_pixmap_unref (gdkpixmap);
  gdk_bitmap_unref (mask);
  return pixmap;
}

static GList *pixmaps_directories = NULL;

/* Use this function to set the directory containing installed pixmaps. */
void
add_pixmap_directory                   (const gchar     *directory)
{
  pixmaps_directories = g_list_prepend (pixmaps_directories,
                                        g_strdup (directory));
}

gchar* find_pixmap_directory(const gchar* filename)
{
  gchar* found_filename = NULL;
  GList *elem;

  elem = pixmaps_directories;
  while (elem)
  {
    found_filename = check_file_exists ((gchar*)elem->data, filename);
    if (found_filename)
      break;
    elem = elem->next;
  }
  return found_filename;
}

GtkWidget* load_pixmap (GtkWidget* widget, const gchar* filename)
{
  GtkWidget *pixmap;
  struct pixmap pix;
  if(!load_pixmap_as_pixmap(widget, filename, &pix))
  {
    return create_dummy_pixmap(widget);
  }

  pixmap = gtk_pixmap_new (pix.pix, pix.mask);
  gdk_pixmap_unref (pix.pix);
  if(pix.mask) gdk_bitmap_unref (pix.mask);
  return pixmap;
}

/** find a pixmap file either absolute or in the pixmap search path.
 * @returns filename if file exists, NULL otherwise. must be freed
 */
static char* find_pixmap_file(const char* filename)
{
  char *found_filename = NULL;

  g_return_val_if_fail(filename!=NULL,NULL);

  if (!filename[0])
    return NULL;

  // load absolute paths directly
  if(filename[0]=='/')
    found_filename = check_file_exists(NULL, filename);
  else
    found_filename = find_pixmap_directory(filename);

  return found_filename;
}

struct pixmap* load_pixmap_as_pixmap (GtkWidget* widget, const gchar* filename, struct pixmap* pix)
{
  gchar *found_filename = NULL;
  GdkColormap *colormap = NULL;

  g_return_val_if_fail(widget!=NULL,NULL);
  g_return_val_if_fail(pix!=NULL,NULL);

  found_filename = find_pixmap_file(filename);

  if(!found_filename)
  {
    xqf_warning (_("Error loading pixmap file: %s"), filename);
    return NULL;
  }

  if(strlen(found_filename)>4 && !strcmp(found_filename+strlen(found_filename)-4,".xpm"))
  {
    colormap = gtk_widget_get_colormap (widget);
    pix->pix = gdk_pixmap_colormap_create_from_xpm (NULL, colormap, &pix->mask,
						   NULL, found_filename);
  }
  else
  {

/*FIXME_GTK2: need GError*/
#ifdef USE_GTK2
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);
#else
    GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(found_filename);
#endif
    if (pixbuf == NULL)
    {
      // translator: %s = file name
      xqf_warning (_("Error loading pixmap file: %s"), found_filename);
      g_free (found_filename);
      return NULL;
    }

    gdk_pixbuf_render_pixmap_and_mask(pixbuf,&pix->pix,&pix->mask,255);

    gdk_pixbuf_unref(pixbuf);
  }

  if (pix->pix == NULL)
  {
    // translator: %s = file name
    xqf_warning (_("Error loading pixmap file: %s"), found_filename);
    g_free (found_filename);
    return NULL;
  }
  g_free (found_filename);

  return pix;
}

void* load_pixmap_as_pixbuf (const gchar* filename)
{
  gchar *found_filename = NULL;
  GdkPixbuf* pixbuf = NULL;
  
  found_filename = find_pixmap_file(filename);

  if(!found_filename)
  {
    // translator: %s = file name
    xqf_warning (_("Error loading pixmap file: %s"), filename);
    return NULL;
  }

/*FIXME_GTK2: need GError*/
#ifdef USE_GTK2
  pixbuf = gdk_pixbuf_new_from_file(found_filename, NULL);
#else
  pixbuf = gdk_pixbuf_new_from_file(found_filename);
#endif
  if (pixbuf == NULL)
    // translator: %s = file name
    xqf_warning (_("Error loading pixmap file: %s"), found_filename);

  g_free (found_filename);

  return pixbuf;
}

/** directory may be null */
gchar* check_file_exists (const gchar* directory, const gchar* filename)
{
  gchar *full_filename;
  struct stat s;
  gint status;

  if(directory)
    full_filename = g_strconcat(directory, G_DIR_SEPARATOR_S, filename, NULL);
  else
    full_filename = g_strdup(filename);

  status = stat (full_filename, &s);
  if (status == 0 && S_ISREG (s.st_mode))
    return full_filename;
  g_free (full_filename);
  return NULL;
}
