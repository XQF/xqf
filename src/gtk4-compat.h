/* XQF - Quake server browser and launcher
 *
 * GTK 4 compatibility shim.
 * Maps removed GTK 2/3 API to GTK 4 equivalents, or stubs it out where
 * no equivalent exists.  All symbols are either macros or static-inline so
 * there is no link-time dependency on this file.
 *
 * Include order: this header must be included AFTER <gtk/gtk.h>.
 * xqf.h does that automatically; do not include it directly.
 */

#ifndef GTK4_COMPAT_H
#define GTK4_COMPAT_H

#include <gtk/gtk.h>

/* ------------------------------------------------------------------ */
/* GtkBox / packing                                                     */
/* ------------------------------------------------------------------ */

static inline GtkWidget *gtk_vbox_new (gboolean homogeneous, gint spacing)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_VERTICAL, spacing);
  if (homogeneous)
    gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
  return box;
}

static inline GtkWidget *gtk_hbox_new (gboolean homogeneous, gint spacing)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, spacing);
  if (homogeneous)
    gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
  return box;
}

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

static inline void
gtk_container_set_border_width (GtkWidget *container, guint width)
{
  gtk_widget_set_margin_start  (container, (int)width);
  gtk_widget_set_margin_end    (container, (int)width);
  gtk_widget_set_margin_top    (container, (int)width);
  gtk_widget_set_margin_bottom (container, (int)width);
}

/* ------------------------------------------------------------------ */
/* GtkWindow                                                            */
/* ------------------------------------------------------------------ */

/* GTK4 gtk_window_new() takes no args */
static inline GtkWidget *_xqf_window_new (void) { return gtk_window_new (); }
#define gtk_window_new(type) _xqf_window_new ()

/* gtk_window_set_position was removed in GTK4 */
#define GTK_WIN_POS_NONE           0
#define GTK_WIN_POS_CENTER         1
#define GTK_WIN_POS_MOUSE          2
#define GTK_WIN_POS_CENTER_ALWAYS  3
#define GTK_WIN_POS_CENTER_ON_PARENT 4
static inline void
gtk_window_set_position (GtkWindow *window, int pos)
{ (void)window; (void)pos; }

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
/* Widget state (removed in GTK4)                                       */
/* ------------------------------------------------------------------ */

#define GTK_STATE_NORMAL      0
#define GTK_STATE_ACTIVE      1
#define GTK_STATE_PRELIGHT    2
#define GTK_STATE_SELECTED    3
#define GTK_STATE_INSENSITIVE 4

static inline void
gtk_widget_set_state (GtkWidget *w, int state)
{ (void)w; (void)state; }

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

static inline GtkWidget *gtk_vseparator_new (void)
{ return gtk_separator_new (GTK_ORIENTATION_VERTICAL); }

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
/* GtkTable → GtkGrid                                                   */
/* ------------------------------------------------------------------ */

#define GtkTable GtkGrid
#define GTK_TABLE(x) ((GtkWidget *)GTK_GRID (x))

#define GTK_FILL   0
#define GTK_EXPAND 0
#define GTK_SHRINK 0

static inline GtkWidget *
gtk_table_new (int rows, int cols, gboolean homogeneous)
{
  (void)rows; (void)cols; (void)homogeneous;
  return gtk_grid_new ();
}

static inline void
gtk_table_set_row_spacings (GtkWidget *table, guint spacing)
{ gtk_grid_set_row_spacing (GTK_GRID (table), spacing); }

static inline void
gtk_table_set_col_spacings (GtkWidget *table, guint spacing)
{ gtk_grid_set_column_spacing (GTK_GRID (table), spacing); }

/* Per-cell spacing setters (no GtkGrid equivalent; ignore) */
static inline void
gtk_table_set_col_spacing (GtkWidget *table, guint col, guint spacing)
{ (void)table; (void)col; (void)spacing; }

static inline void
gtk_table_set_row_spacing (GtkWidget *table, guint row, guint spacing)
{ (void)table; (void)row; (void)spacing; }

static inline void
gtk_table_attach (GtkWidget *table, GtkWidget *child,
                  guint left, guint right, guint top, guint bottom,
                  guint xopts, guint yopts, guint xpad, guint ypad)
{
  (void)xopts; (void)yopts; (void)xpad; (void)ypad;
  gtk_grid_attach (GTK_GRID (table), child,
                   (int)left, (int)top,
                   (int)(right - left), (int)(bottom - top));
}

static inline void
gtk_table_attach_defaults (GtkWidget *table, GtkWidget *child,
                            guint left, guint right, guint top, guint bottom)
{
  gtk_grid_attach (GTK_GRID (table), child,
                   (int)left, (int)top,
                   (int)(right - left), (int)(bottom - top));
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

static inline void
gtk_scrolled_window_set_shadow_type (GtkScrolledWindow *sw, int type)
{ (void)sw; (void)type; }

/* ------------------------------------------------------------------ */
/* GtkRadioButton → GtkCheckButton with group                           */
/* ------------------------------------------------------------------ */

#define GtkRadioButton        GtkCheckButton
#define GTK_RADIO_BUTTON(x)   GTK_CHECK_BUTTON (x)
#define GTK_IS_RADIO_BUTTON(x) GTK_IS_CHECK_BUTTON (x)

/* first-in-group: plain check button (group set later by from_widget) */
#define gtk_radio_button_new_with_label(grp, lbl) \
  gtk_check_button_new_with_label (lbl)

static inline GtkWidget *
gtk_radio_button_new_with_label_from_widget (GtkCheckButton *group,
                                              const char     *label)
{
  GtkWidget *btn = gtk_check_button_new_with_label (label);
  gtk_check_button_set_group (GTK_CHECK_BUTTON (btn), group);
  return btn;
}

#define gtk_radio_button_get_group(btn) (NULL)

/* ------------------------------------------------------------------ */
/* GdkEvent double/triple button press constants                         */
/* GTK4 removed the GDK_DOUBLE/TRIPLE_BUTTON_PRESS names; use values.  */
/* ------------------------------------------------------------------ */

#ifndef GDK_2BUTTON_PRESS
#  define GDK_2BUTTON_PRESS 4
#endif
#ifndef GDK_3BUTTON_PRESS
#  define GDK_3BUTTON_PRESS 5
#endif

/* ------------------------------------------------------------------ */
/* GtkAlignment (removed in GTK4; use margins / halign / valign)        */
/* ------------------------------------------------------------------ */

static inline GtkWidget *
gtk_alignment_new (gfloat xalign, gfloat yalign, gfloat xscale, gfloat yscale)
{
  GtkWidget *box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  (void)xscale; (void)yscale;
  gtk_widget_set_halign (box, (xalign < 0.5f) ? GTK_ALIGN_START :
                               (xalign > 0.5f) ? GTK_ALIGN_END : GTK_ALIGN_CENTER);
  gtk_widget_set_valign (box, (yalign < 0.5f) ? GTK_ALIGN_START :
                               (yalign > 0.5f) ? GTK_ALIGN_END : GTK_ALIGN_CENTER);
  return box;
}
#define GTK_ALIGNMENT(x) ((GtkWidget *)(x))

/* ------------------------------------------------------------------ */
/* GtkBin (removed in GTK4; single-child containers use set_child)      */
/* ------------------------------------------------------------------ */

typedef GtkWidget GtkBin;
#define GTK_BIN(x) ((GtkWidget *)(x))

static inline GtkWidget *gtk_bin_get_child (GtkWidget *bin)
{ return gtk_widget_get_first_child (bin); }

/* ------------------------------------------------------------------ */
/* GtkFrame shadow (CSS-only in GTK4)                                   */
/* ------------------------------------------------------------------ */

static inline void gtk_frame_set_shadow_type (GtkFrame *frame, int type)
{ (void)frame; (void)type; }

/* ------------------------------------------------------------------ */
/* gtk_misc_set_padding (removed; use margins)                           */
/* ------------------------------------------------------------------ */

static inline void gtk_misc_set_padding (GtkWidget *misc, gint xpad, gint ypad)
{
  gtk_widget_set_margin_start  (misc, xpad);
  gtk_widget_set_margin_end    (misc, xpad);
  gtk_widget_set_margin_top    (misc, ypad);
  gtk_widget_set_margin_bottom (misc, ypad);
}

/* ------------------------------------------------------------------ */
/* gtk_widget_ensure_style (removed in GTK4)                            */
/* ------------------------------------------------------------------ */

static inline void gtk_widget_ensure_style (GtkWidget *w) { (void)w; }

/* ------------------------------------------------------------------ */
/* gtk_scrolled_window_add_with_viewport (removed in GTK4)              */
/* ------------------------------------------------------------------ */

static inline void
gtk_scrolled_window_add_with_viewport (GtkScrolledWindow *sw, GtkWidget *child)
{ gtk_scrolled_window_set_child (sw, child); }


/* ------------------------------------------------------------------ */
/* GtkScrollbar                                                          */
/* ------------------------------------------------------------------ */

static inline GtkWidget *gtk_vscrollbar_new (GtkAdjustment *adj)
{ return gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adj); }

static inline GtkWidget *gtk_hscrollbar_new (GtkAdjustment *adj)
{ return gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, adj); }

/* ------------------------------------------------------------------ */
/* GtkTextView adjustment (removed; use GtkScrollable interface)        */
/* ------------------------------------------------------------------ */

static inline GtkAdjustment *gtk_text_view_get_vadjustment (GtkTextView *view)
{ return gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (view)); }

static inline GtkAdjustment *gtk_text_view_get_hadjustment (GtkTextView *view)
{ return gtk_scrollable_get_hadjustment (GTK_SCROLLABLE (view)); }

/* GtkFileChooserButton was removed in GTK4; pref.c uses its own helpers. */

/* ------------------------------------------------------------------ */
/* GtkButtonBox (removed in GTK4)                                       */
/* ------------------------------------------------------------------ */

typedef GtkBox GtkButtonBox;
#define GTK_BUTTON_BOX(x)      ((GtkWidget *)GTK_BOX (x))
#define gtk_button_box_new(o)  gtk_box_new ((o), 0)
static inline void gtk_button_box_set_layout (GtkWidget *bbox, int layout)
{ (void)bbox; (void)layout; }
static inline void gtk_button_box_set_child_secondary (GtkWidget *bbox,
                                                        GtkWidget *child,
                                                        gboolean   secondary)
{ (void)bbox; (void)child; (void)secondary; }
#define GTK_BUTTONBOX_SPREAD  1
#define GTK_BUTTONBOX_EDGE    2
#define GTK_BUTTONBOX_START   3
#define GTK_BUTTONBOX_END     4
#define GTK_BUTTONBOX_CENTER  5

/* ------------------------------------------------------------------ */
/* GtkAccelGroup (removed in GTK4)                                      */
/* ------------------------------------------------------------------ */

typedef struct { int _dummy; } GtkAccelGroup;
static inline GtkAccelGroup *gtk_accel_group_new (void)
{ return NULL; }
static inline void gtk_window_add_accel_group (GtkWindow *win, GtkAccelGroup *ag)
{ (void)win; (void)ag; }
static inline void gtk_widget_add_accelerator (GtkWidget *w, const char *sig,
                                                GtkAccelGroup *ag,
                                                guint key, GdkModifierType mods,
                                                int flags)
{ (void)w; (void)sig; (void)ag; (void)key; (void)mods; (void)flags; }
static inline void gtk_accel_group_connect (GtkAccelGroup *ag, guint key,
                                             GdkModifierType mods, int flags,
                                             GClosure *closure)
{ (void)ag; (void)key; (void)mods; (void)flags; (void)closure; }


/* GtkButton relief (GTK4 uses has-frame; stub the GTK2/3 API) */
#define GTK_RELIEF_NORMAL 0
#define GTK_RELIEF_HALF   1
#define GTK_RELIEF_NONE   2
static inline void gtk_button_set_relief (GtkButton *btn, int relief)
{ gtk_button_set_has_frame (btn, relief != GTK_RELIEF_NONE); }

/* ------------------------------------------------------------------ */
/* GtkToolbar / GtkToolButton (removed in GTK4)                         */
/* ------------------------------------------------------------------ */

typedef GtkWidget GtkToolbar;
typedef GtkWidget GtkToolItem;
typedef GtkWidget GtkToolButton;
typedef GtkWidget GtkSeparatorToolItem;

#define GTK_TOOLBAR(x)           ((GtkToolbar *)(x))
#define GTK_TOOL_ITEM(x)         ((GtkToolItem *)(x))
#define GTK_TOOL_BUTTON(x)       ((GtkToolButton *)(x))

typedef int GtkToolbarStyle;
#define GTK_TOOLBAR_ICONS  0
#define GTK_TOOLBAR_TEXT   1
#define GTK_TOOLBAR_BOTH   2
#define GTK_TOOLBAR_BOTH_HORIZ 3

static inline void gtk_toolbar_set_style (GtkToolbar *tb, GtkToolbarStyle s)
{ (void)tb; (void)s; }

typedef GtkWidget GtkToggleToolButton;
#define GTK_TOGGLE_TOOL_BUTTON(x) ((GtkToggleToolButton *)(x))
static inline GtkWidget *gtk_toggle_tool_button_new (void)
{ return gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0); }
static inline void gtk_tool_button_set_label (GtkToolButton *btn, const char *lbl)
{ (void)btn; (void)lbl; }
static inline void gtk_tool_button_set_icon_widget (GtkToolButton *btn, GtkWidget *icon)
{ (void)btn; (void)icon; }
static inline void gtk_toolbar_insert (GtkToolbar *tb, GtkToolItem *item, int pos)
{ (void)tb; (void)item; (void)pos; }
static inline void gtk_toggle_tool_button_set_active (GtkToggleToolButton *btn, gboolean active)
{ (void)btn; (void)active; }

typedef GtkWidget GtkRadioMenuItem;
#define GTK_RADIO_MENU_ITEM(x) ((GtkRadioMenuItem *)(x))
static inline GtkWidget *gtk_radio_menu_item_new_with_label (GSList *grp, const char *lbl)
{ (void)grp; (void)lbl; return NULL; }
static inline GSList *gtk_radio_menu_item_get_group (GtkRadioMenuItem *mi)
{ (void)mi; return NULL; }

/* ------------------------------------------------------------------ */
/* GtkCList / GtkCTree stubs (to be replaced by GtkColumnView)          */
/*                                                                        */
/* Direct struct-field accesses (->rows, ->selection) are replaced by    */
/* the accessor functions below; call sites must be updated.             */
/* ------------------------------------------------------------------ */

typedef GtkWidget GtkCList;
typedef GtkWidget GtkCTree;
typedef void *GtkCTreeNode;   /* forward typedef; full stub API follows below */

#define GTK_CLIST(x)      ((GtkCList *)(x))
#define GTK_CTREE(x)      ((GtkCTree *)(x))
#define GTK_CTREE_NODE(p) ((GtkCTreeNode *)(p))

static inline int    gtk_clist_get_rows        (GtkCList *cl) { (void)cl; return 0; }
static inline GList *gtk_clist_get_selection   (GtkCList *cl) { (void)cl; return NULL; }
static inline int    gtk_clist_get_sort_column (GtkCList *cl) { (void)cl; return -1; }
static inline int    gtk_clist_get_column_width(GtkCList *cl, int c) { (void)cl; (void)c; return 0; }
static inline int    gtk_clist_get_row_height  (GtkCList *cl) { (void)cl; return 18; }

/* GtkCListRow stub: used in sort-compare callbacks (ptr1/ptr2 args) */
typedef struct { gpointer data; } GtkCListRow;

/* GtkCTreeNode helpers */
static inline GtkCTreeNode *gtk_ctree_node_nth (GtkCTree *t, int n)
{ (void)t; (void)n; return NULL; }
static inline void gtk_ctree_node_get_text (GtkCTree *t, GtkCTreeNode *n,
                                             int col, char **text)
{ (void)t; (void)n; (void)col; if (text) *text = (char *)""; }

/* GTK2 selection mode alias */
#define GTK_SELECTION_EXTENDED GTK_SELECTION_MULTIPLE

/* GtkPaned child accessors renamed in GTK4 */
#define gtk_paned_get_child1(p) gtk_paned_get_start_child (p)
#define gtk_paned_get_child2(p) gtk_paned_get_end_child (p)

/* GdkEventKey (removed as separate type in GTK4) */
typedef GdkEvent GdkEventKey;

/* GdkWindow pointer / screen helpers (removed in GTK4) */
#define GDK_POINTER_MOTION_HINT_MASK   0
#define GDK_BUTTON1_MOTION_MASK        0
#define GDK_BUTTON_RELEASE_MASK        0
static inline int  gdk_pointer_grab   (gpointer w, gboolean o, guint m,
                                        gpointer cw, gpointer c, guint32 t)
{ (void)w;(void)o;(void)m;(void)cw;(void)c;(void)t; return 0; }
static inline void gdk_pointer_ungrab (guint32 t) { (void)t; }
static inline int  gdk_screen_width   (void) { return 1920; }
static inline int  gdk_screen_height  (void) { return 1080; }
static inline void gdk_window_get_origin (gpointer w, int *x, int *y)
{ (void)w; if (x) *x = 0; if (y) *y = 0; }

/* GTK_WINDOW_POPUP removed in GTK4 */
#define GTK_WINDOW_POPUP GTK_WINDOW_TOPLEVEL

/* gtk_window_move removed in GTK4 (windows can't be positioned by app) */
static inline void gtk_window_move (GtkWindow *w, int x, int y)
{ (void)w; (void)x; (void)y; }

/* gtk_editable_copy_clipboard: removed in GTK4; use GdkClipboard API */
static inline void gtk_editable_copy_clipboard (GtkEditable *e) { (void)e; }

/* gtk_builder_connect_signals: removed in GTK4; use individual g_signal_connect */
static inline void gtk_builder_connect_signals (GtkBuilder *b, gpointer data)
{ (void)b; (void)data; }

/* gtk_init: GTK4 takes no arguments */
static inline void _xqf_gtk_init (int *argc, char ***argv)
{ (void)argc; (void)argv; gtk_init (); }
#define gtk_init(argc, argv) _xqf_gtk_init ((argc), (argv))

/* ------------------------------------------------------------------ */
/* GdkPixmap / GdkBitmap (removed in GTK4; kept as opaque stub types)  */
/* ------------------------------------------------------------------ */

typedef struct _GdkPixmapStub GdkPixmap;
typedef struct _GdkPixmapStub GdkBitmap;

/* ------------------------------------------------------------------ */
/* GdkColor and GtkStyle were removed in GTK4; call sites now use CSS. */

/* ------------------------------------------------------------------ */
/* GdkEventButton stub (removed as a struct in GTK4; event API changed) */
/* ------------------------------------------------------------------ */

typedef GdkEvent GdkEventButton;
typedef GdkEvent GdkEventScroll;
typedef GdkEvent GdkEventMotion;

/* ------------------------------------------------------------------ */
/* GtkVisibility (was a GtkCList concept; removed with GtkCList)        */
/* ------------------------------------------------------------------ */

typedef enum {
  GTK_VISIBILITY_NONE    = 0,
  GTK_VISIBILITY_PARTIAL = 1,
  GTK_VISIBILITY_FULL    = 2
} GtkVisibility;

/* ------------------------------------------------------------------ */
/* GtkCList / GtkCTree full stub API                                     */
/*                                                                        */
/* All functions are no-ops or return safe defaults.  The real           */
/* implementation will use GtkColumnView in Phase 1.                     */
/* ------------------------------------------------------------------ */

static inline GtkWidget *gtk_clist_new (int cols)
{ (void)cols; return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }

static inline GtkWidget *
gtk_clist_new_with_titles (int cols, char **titles)
{ (void)cols; (void)titles; return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }

#define GTK_IS_CLIST(x) (FALSE)

static inline void gtk_clist_set_shadow_type (GtkCList *cl, int type) { (void)cl; (void)type; }

static inline int  gtk_clist_append  (GtkCList *cl, char **t) { (void)cl; (void)t; return -1; }
static inline int  gtk_clist_insert  (GtkCList *cl, int r, char **t) { (void)cl; (void)r; (void)t; return r; }
static inline void gtk_clist_remove  (GtkCList *cl, int r) { (void)cl; (void)r; }
static inline void gtk_clist_clear   (GtkCList *cl) { (void)cl; }
static inline void gtk_clist_freeze  (GtkCList *cl) { (void)cl; }
static inline void gtk_clist_thaw    (GtkCList *cl) { (void)cl; }
static inline void gtk_clist_sort    (GtkCList *cl) { (void)cl; }

static inline void gtk_clist_set_text (GtkCList *cl, int r, int c, const char *t)
{ (void)cl; (void)r; (void)c; (void)t; }

static inline void gtk_clist_set_pixtext (GtkCList *cl, int r, int c,
                                           const char *t, guint8 sp,
                                           GdkPixmap *px, GdkBitmap *mask)
{ (void)cl; (void)r; (void)c; (void)t; (void)sp; (void)px; (void)mask; }

static inline void gtk_clist_set_pixmap (GtkCList *cl, int r, int c,
                                          GdkPixmap *px, GdkBitmap *mask)
{ (void)cl; (void)r; (void)c; (void)px; (void)mask; }

static inline void gtk_clist_set_shift (GtkCList *cl, int r, int c, int v, int h)
{ (void)cl; (void)r; (void)c; (void)v; (void)h; }

static inline void gtk_clist_set_foreground (GtkCList *cl, int r, GdkRGBA *clr)
{ (void)cl; (void)r; (void)clr; }

static inline void gtk_clist_set_row_data (GtkCList *cl, int r, gpointer d)
{ (void)cl; (void)r; (void)d; }

static inline void gtk_clist_set_row_data_full (GtkCList *cl, int r, gpointer d,
                                                 GDestroyNotify fn)
{ (void)cl; (void)r; (void)d; (void)fn; }

static inline gpointer gtk_clist_get_row_data (GtkCList *cl, int r)
{ (void)cl; (void)r; return NULL; }

static inline int gtk_clist_find_row_from_data (GtkCList *cl, gpointer d)
{ (void)cl; (void)d; return -1; }

static inline void gtk_clist_select_row   (GtkCList *cl, int r, int c) { (void)cl; (void)r; (void)c; }
static inline void gtk_clist_unselect_all (GtkCList *cl) { (void)cl; }

static inline void gtk_clist_moveto (GtkCList *cl, int r, int c, gfloat ra, gfloat ca)
{ (void)cl; (void)r; (void)c; (void)ra; (void)ca; }

static inline GtkVisibility gtk_clist_row_is_visible (GtkCList *cl, int r)
{ (void)cl; (void)r; return GTK_VISIBILITY_FULL; }

static inline gboolean gtk_clist_get_selection_info (GtkCList *cl, int x, int y,
                                                      int *row, int *col)
{ (void)cl; (void)x; (void)y; if (row) *row = -1; if (col) *col = -1; return FALSE; }

static inline void gtk_clist_swap_rows (GtkCList *cl, int r1, int r2)
{ (void)cl; (void)r1; (void)r2; }

static inline void gtk_clist_set_selection_mode (GtkCList *cl, GtkSelectionMode m)
{ (void)cl; (void)m; }

static inline void gtk_clist_set_reorderable (GtkCList *cl, gboolean f)
{ (void)cl; (void)f; }

static inline void gtk_clist_set_column_width (GtkCList *cl, int c, int w)
{ (void)cl; (void)c; (void)w; }

static inline void gtk_clist_set_column_widget (GtkCList *cl, int c, GtkWidget *w)
{ (void)cl; (void)c; (void)w; }

static inline void gtk_clist_set_column_resizeable (GtkCList *cl, int c, gboolean f)
{ (void)cl; (void)c; (void)f; }

static inline void gtk_clist_column_titles_passive (GtkCList *cl) { (void)cl; }

static inline int gtk_clist_optimal_column_width (GtkCList *cl, int c) { (void)cl; (void)c; return 0; }

/* GtkButtonBox old helpers */
static inline GtkWidget *gtk_vbutton_box_new (void)
{ return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }
static inline GtkWidget *gtk_hbutton_box_new (void)
{ return gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0); }

/* These stub functions are needed by xqf-ui.c / srv-list.c */
static inline void gtk_clist_set_row_height (GtkCList *cl, int h) { (void)cl; (void)h; }
static inline void gtk_clist_set_column_justification (GtkCList *cl, int c, GtkJustification j)
{ (void)cl; (void)c; (void)j; }
static inline void gtk_clist_set_column_title (GtkCList *cl, int c, const char *t)
{ (void)cl; (void)c; (void)t; }
static inline void gtk_clist_column_titles_show (GtkCList *cl) { (void)cl; }
static inline void gtk_clist_set_sort_column (GtkCList *cl, int c) { (void)cl; (void)c; }
static inline void gtk_clist_set_sort_type (GtkCList *cl, GtkSortType t) { (void)cl; (void)t; }
static inline GtkSortType gtk_clist_get_sort_type (GtkCList *cl) { (void)cl; return GTK_SORT_ASCENDING; }

/* GtkCList internal flags (removed with GtkCList) */
#define GTK_CLIST_SET_FLAG(cl, flag) ((void)(cl))
#define GTK_CLIST_UNSET_FLAG(cl, flag) ((void)(cl))
#define CLIST_SHOW_TITLES 0

/* GtkCTree node visibility / scrolling */
static inline GtkVisibility gtk_ctree_node_is_visible (GtkCTree *t, GtkCTreeNode *n)
{ (void)t; (void)n; return GTK_VISIBILITY_FULL; }
static inline void gtk_ctree_node_moveto (GtkCTree *t, GtkCTreeNode *n,
                                           int col, gfloat row_align, gfloat col_align)
{ (void)t; (void)n; (void)col; (void)row_align; (void)col_align; }

/* GtkCList sort-compare callback type */
typedef int (*GtkCListCompareFunc)(GtkCList *, gconstpointer, gconstpointer);

static inline void gtk_clist_set_compare_func (GtkCList *cl, GtkCListCompareFunc fn)
{ (void)cl; (void)fn; }

/* GtkCTree stubs */
static inline GtkWidget *gtk_ctree_new (int cols, int tree_col)
{ (void)cols; (void)tree_col; return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }

static inline GtkWidget *gtk_ctree_new_with_titles (int cols, int tree_col, char **titles)
{ (void)cols; (void)tree_col; (void)titles; return gtk_box_new (GTK_ORIENTATION_VERTICAL, 0); }

static inline void gtk_ctree_set_line_style (GtkCTree *t, int s) { (void)t; (void)s; }
static inline void gtk_ctree_set_expander_style (GtkCTree *t, int s) { (void)t; (void)s; }
static inline void gtk_ctree_set_indent (GtkCTree *t, int i) { (void)t; (void)i; }

static inline GtkCTreeNode *
gtk_ctree_insert_node (GtkCTree *t, GtkCTreeNode *parent, GtkCTreeNode *sibling,
                       char **text, guint8 sp, GdkPixmap *px, GdkBitmap *mask,
                       GdkPixmap *xpx, GdkBitmap *xmask, gboolean is_leaf, gboolean expanded)
{ (void)t; (void)parent; (void)sibling; (void)text; (void)sp;
  (void)px; (void)mask; (void)xpx; (void)xmask; (void)is_leaf; (void)expanded;
  return NULL; }

static inline void gtk_ctree_remove_node (GtkCTree *t, GtkCTreeNode *n)
{ (void)t; (void)n; }

static inline GtkCTreeNode *
gtk_ctree_find_by_row_data (GtkCTree *t, GtkCTreeNode *n, gpointer d)
{ (void)t; (void)n; (void)d; return NULL; }

static inline void gtk_ctree_node_set_row_data (GtkCTree *t, GtkCTreeNode *n, gpointer d)
{ (void)t; (void)n; (void)d; }

static inline gpointer gtk_ctree_node_get_row_data (GtkCTree *t, GtkCTreeNode *n)
{ (void)t; (void)n; return NULL; }

static inline void gtk_ctree_expand (GtkCTree *t, GtkCTreeNode *n) { (void)t; (void)n; }
static inline void gtk_ctree_select (GtkCTree *t, GtkCTreeNode *n) { (void)t; (void)n; }
static inline void gtk_ctree_unselect_recursive (GtkCTree *t, GtkCTreeNode *n) { (void)t; (void)n; }
static inline void gtk_ctree_sort_node (GtkCTree *t, GtkCTreeNode *n) { (void)t; (void)n; }

static inline gboolean
gtk_ctree_get_node_info (GtkCTree *t, GtkCTreeNode *n,
                         char **text, guint8 *sp,
                         GdkPixmap **px, GdkBitmap **mask,
                         GdkPixmap **xpx, GdkBitmap **xmask,
                         gboolean *is_leaf, gboolean *expanded)
{ (void)t; (void)n; (void)text; (void)sp; (void)px; (void)mask;
  (void)xpx; (void)xmask;
  if (is_leaf) *is_leaf = TRUE;
  if (expanded) *expanded = FALSE;
  return FALSE; }

static inline gboolean
gtk_ctree_set_node_info (GtkCTree *t, GtkCTreeNode *n,
                         const char *text, guint8 sp,
                         GdkPixmap *px, GdkBitmap *mask,
                         GdkPixmap *xpx, GdkBitmap *xmask,
                         gboolean is_leaf, gboolean expanded)
{ (void)t; (void)n; (void)text; (void)sp; (void)px; (void)mask;
  (void)xpx; (void)xmask; (void)is_leaf; (void)expanded; return TRUE; }

static inline void gtk_ctree_set_row_data (GtkCTree *t, GtkCTreeNode *n, gpointer d)
{ (void)t; (void)n; (void)d; }

#define GTK_CTREE_LINES_NONE       0
#define GTK_CTREE_LINES_SOLID      1
#define GTK_CTREE_LINES_DOTTED     2
#define GTK_CTREE_EXPANDER_TRIANGLE 3

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
