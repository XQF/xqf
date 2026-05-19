/* XQF - Quake server browser and launcher
 *
 * GTK 4 compatibility shim.
 * Maps removed GTK 2/3 API to GTK 4 equivalents, or stubs it out where
 * no equivalent exists.  All symbols are either macros or static-inline so
 * there is no link-time dependency on this file.
 *
 * Include order: this header must be included AFTER <gtk/gtk.h>.
 * xqf.h does that automatically; do not include it directly.
 *
 * Stubs are removed as their call sites are migrated to native GTK4 API.
 * The remaining stubs represent actively-called code in un-migrated files.
 */

#ifndef GTK4_COMPAT_H
#define GTK4_COMPAT_H

#include <gtk/gtk.h>

/* ------------------------------------------------------------------ */
/* GtkFileChooser helpers (gtk_file_chooser_get/set_filename removed)    */
/* ------------------------------------------------------------------ */

static inline char *
gtk_file_chooser_get_filename (GtkFileChooser *chooser)
{
  GFile *file = gtk_file_chooser_get_file (chooser);
  if (!file) return NULL;
  char *path = g_file_get_path (file);
  g_object_unref (file);
  return path;
}

static inline void
gtk_file_chooser_set_filename (GtkFileChooser *chooser, const char *filename)
{
  if (!filename) return;
  GFile *file = g_file_new_for_path (filename);
  gtk_file_chooser_set_file (chooser, file, NULL);
  g_object_unref (file);
}


#endif /* GTK4_COMPAT_H */
