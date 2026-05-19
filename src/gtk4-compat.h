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
/* GtkContainer (removed in GTK4)                                       */
/* ------------------------------------------------------------------ */

#define GTK_CONTAINER(x) ((GtkWidget *)(x))

static inline void
gtk_container_add (GtkWidget *container, GtkWidget *child)
{
  if (GTK_IS_WINDOW (container))
    gtk_window_set_child (GTK_WINDOW (container), child);
  else if (GTK_IS_SCROLLED_WINDOW (container))
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (container), child);
  else if (GTK_IS_FRAME (container))
    gtk_frame_set_child (GTK_FRAME (container), child);
  else if (GTK_IS_EXPANDER (container))
    gtk_expander_set_child (GTK_EXPANDER (container), child);
  else if (GTK_IS_BOX (container))
    gtk_box_append (GTK_BOX (container), child);
  else
    g_warning ("gtk_container_add: unhandled container %s",
               G_OBJECT_TYPE_NAME (container));
}

/* ------------------------------------------------------------------ */
/* GtkMisc (removed in GTK4)                                            */
/* ------------------------------------------------------------------ */

#define GTK_MISC(x) ((GtkWidget *)(x))

/* ------------------------------------------------------------------ */
/* GtkMisc padding (removed; use margins)                               */
/* ------------------------------------------------------------------ */

static inline void gtk_misc_set_padding (GtkWidget *misc, gint xpad, gint ypad)
{
  gtk_widget_set_margin_start  (misc, xpad);
  gtk_widget_set_margin_end    (misc, xpad);
  gtk_widget_set_margin_top    (misc, ypad);
  gtk_widget_set_margin_bottom (misc, ypad);
}

/* ------------------------------------------------------------------ */
/* GtkBin (removed in GTK4; single-child containers use set_child)      */
/* ------------------------------------------------------------------ */

typedef GtkWidget GtkBin;
#define GTK_BIN(x) ((GtkWidget *)(x))

static inline GtkWidget *gtk_bin_get_child (GtkWidget *bin)
{ return gtk_widget_get_first_child (bin); }

/* ------------------------------------------------------------------ */
/* GtkTextView adjustment (removed; use GtkScrollable interface)        */
/* ------------------------------------------------------------------ */

static inline GtkAdjustment *gtk_text_view_get_vadjustment (GtkTextView *view)
{ return gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (view)); }

static inline GtkAdjustment *gtk_text_view_get_hadjustment (GtkTextView *view)
{ return gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (view)); }

/* GtkFileChooserButton was removed in GTK4; pref.c uses its own helpers. */

/* ------------------------------------------------------------------ */
/* GdkEventButton stub (removed as a struct in GTK4; event API changed) */
/* ------------------------------------------------------------------ */

typedef GdkEvent GdkEventButton;

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
