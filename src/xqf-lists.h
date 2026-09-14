/* XQF - Quake server browser and launcher
 *
 * GtkColumnView + GListStore infrastructure for server and player lists.
 * Replaces the GtkCList/GtkCTree stubs for the two main data views.
 */

#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

struct server;  /* forward declaration — full type in defs.h */

extern GListStore        *server_store;
extern GListStore        *player_store;
extern GtkSelectionModel *server_selection;
extern GtkSelectionModel *player_selection;

GtkWidget *create_server_column_view (GtkWidget *scrollwin);
GtkWidget *create_player_column_view (GtkWidget *scrollwin);

/* Search server_selection (sorted view) for a matching server pointer.
 * Returns position in the selection model, or G_MAXUINT if not found. */
guint server_store_find (struct server *s);

G_END_DECLS
