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
/* GtkBox / packing                                                     */
/* ------------------------------------------------------------------ */

/* expand/fill/padding semantics are dropped; child is appended */
static inline void
gtk_box_pack_start (GtkBox *box, GtkWidget *child,
                    gboolean expand, gboolean fill, guint padding)
{
  (void)expand; (void)fill; (void)padding;
  gtk_box_append (box, child);
}

static inline void
gtk_box_pack_end (GtkBox *box, GtkWidget *child,
                  gboolean expand, gboolean fill, guint padding)
{
  (void)expand; (void)fill; (void)padding;
  gtk_box_append (box, child);
}

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
/* GtkWindow                                                            */
/* ------------------------------------------------------------------ */

/* GTK4 gtk_window_new() takes no args */
static inline GtkWidget *_xqf_window_new (void) { return gtk_window_new (); }
#define gtk_window_new(type) _xqf_window_new ()

/* ------------------------------------------------------------------ */
/* Widget destroy                                                        */
/* ------------------------------------------------------------------ */

static inline void
gtk_widget_destroy (GtkWidget *widget)
{
  if (GTK_IS_WINDOW (widget))
    gtk_window_destroy (GTK_WINDOW (widget));
}

/* ------------------------------------------------------------------ */
/* gtk_widget_show_all (removed in GTK4; children visible by default)   */
/* ------------------------------------------------------------------ */

#define gtk_widget_show_all(w) gtk_widget_show (w)

/* ------------------------------------------------------------------ */
/* Widget default (API removed in GTK4; use gtk_window_set_default_widget) */
/* ------------------------------------------------------------------ */

static inline void
gtk_widget_set_can_default (GtkWidget *w, gboolean can)
{ (void)w; (void)can; }

static inline void
gtk_widget_grab_default (GtkWidget *w)
{ (void)w; }

/* ------------------------------------------------------------------ */
/* GtkEntry text helpers                                                 */
/* ------------------------------------------------------------------ */

#define gtk_entry_get_text(e)    ((e) && GTK_IS_EDITABLE (e) ? gtk_editable_get_text (GTK_EDITABLE (e)) : "")
#define gtk_entry_set_text(e, t) gtk_editable_set_text (GTK_EDITABLE (e), (t))

/* ------------------------------------------------------------------ */
/* Separators                                                            */
/* ------------------------------------------------------------------ */

static inline GtkWidget *gtk_hseparator_new (void)
{ return gtk_separator_new (GTK_ORIENTATION_HORIZONTAL); }

/* ------------------------------------------------------------------ */
/* GtkMisc / label alignment (GtkMisc removed in GTK4)                  */
/* ------------------------------------------------------------------ */

#define GTK_MISC(x) ((GtkWidget *)(x))

static inline void
gtk_misc_set_alignment (GtkWidget *misc, gfloat xalign, gfloat yalign)
{
  if (GTK_IS_LABEL (misc))
    {
      gtk_label_set_xalign (GTK_LABEL (misc), (float)xalign);
      gtk_label_set_yalign (GTK_LABEL (misc), (float)yalign);
    }
}

/* ------------------------------------------------------------------ */
/* GtkScrolledWindow                                                     */
/* ------------------------------------------------------------------ */

/* GTK4 gtk_scrolled_window_new() takes no args */
static inline GtkWidget *_xqf_scrolled_window_new (void)
{ return gtk_scrolled_window_new (); }
#define gtk_scrolled_window_new(h, v) _xqf_scrolled_window_new ()

/* Shadow types removed from most GTK4 widgets */
#define GTK_SHADOW_NONE       0
#define GTK_SHADOW_IN         1
#define GTK_SHADOW_OUT        2
#define GTK_SHADOW_ETCHED_IN  3
#define GTK_SHADOW_ETCHED_OUT 4

/* ------------------------------------------------------------------ */
/* GtkFrame shadow (CSS-only in GTK4)                                   */
/* ------------------------------------------------------------------ */

static inline void gtk_frame_set_shadow_type (GtkFrame *frame, int type)
{ (void)frame; (void)type; }

/* ------------------------------------------------------------------ */
/* gtk_scrolled_window_add_with_viewport (removed in GTK4)              */
/* ------------------------------------------------------------------ */

static inline void
gtk_scrolled_window_add_with_viewport (GtkScrolledWindow *sw, GtkWidget *child)
{ gtk_scrolled_window_set_child (sw, child); }

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
/* GtkButtonBox (removed in GTK4)                                       */
/* ------------------------------------------------------------------ */

typedef GtkBox GtkButtonBox;
#define GTK_BUTTON_BOX(x)      ((GtkWidget *)GTK_BOX (x))
#define gtk_button_box_new(o)  gtk_box_new ((o), 0)
static inline void gtk_button_box_set_layout (GtkWidget *bbox, int layout)
{ (void)bbox; (void)layout; }
#define GTK_BUTTONBOX_SPREAD  1
#define GTK_BUTTONBOX_EDGE    2
#define GTK_BUTTONBOX_START   3
#define GTK_BUTTONBOX_END     4
#define GTK_BUTTONBOX_CENTER  5

static inline GtkWidget *gtk_vbutton_box_new (void)
{ return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }
static inline GtkWidget *gtk_hbutton_box_new (void)
{ return gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0); }

/* ------------------------------------------------------------------ */
/* GtkTextView adjustment (removed; use GtkScrollable interface)        */
/* ------------------------------------------------------------------ */

static inline GtkAdjustment *gtk_text_view_get_vadjustment (GtkTextView *view)
{ return gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (view)); }

static inline GtkAdjustment *gtk_text_view_get_hadjustment (GtkTextView *view)
{ return gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (view)); }

/* GtkFileChooserButton was removed in GTK4; pref.c uses its own helpers. */

/* ------------------------------------------------------------------ */
/* GtkPaned child accessors (renamed in GTK4)                           */
/* ------------------------------------------------------------------ */

#define gtk_paned_get_child1(p) gtk_paned_get_start_child (p)
#define gtk_paned_get_child2(p) gtk_paned_get_end_child (p)

/* GTK2 selection mode alias */
#define GTK_SELECTION_EXTENDED GTK_SELECTION_MULTIPLE

/* ------------------------------------------------------------------ */
/* GdkEventButton stub (removed as a struct in GTK4; event API changed) */
/* ------------------------------------------------------------------ */

typedef GdkEvent GdkEventButton;

/* ------------------------------------------------------------------ */
/* gtk_init: GTK4 takes no arguments                                    */
/* ------------------------------------------------------------------ */

static inline void _xqf_gtk_init (int *argc, char ***argv)
{ (void)argc; (void)argv; gtk_init (); }
#define gtk_init(argc, argv) _xqf_gtk_init ((argc), (argv))

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
