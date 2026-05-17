/* XQF - Quake server browser and launcher
 *
 * Small GTK4 helpers that have no equivalent in the GTK4 library itself.
 * These are XQF-specific utilities, not compatibility shims.
 *
 * Include order: must be included after <gtk/gtk.h>.
 * xqf.h does that automatically.
 */

#ifndef XQF_UTILS_H
#define XQF_UTILS_H

#include <gtk/gtk.h>

/* Set all four margins to the same value (GTK4 has no single-call equivalent) */
static inline void
xqf_widget_set_margin_all (GtkWidget *widget, int margin)
{
  gtk_widget_set_margin_start  (widget, margin);
  gtk_widget_set_margin_end    (widget, margin);
  gtk_widget_set_margin_top    (widget, margin);
  gtk_widget_set_margin_bottom (widget, margin);
}

#endif /* XQF_UTILS_H */
