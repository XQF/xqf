/* XQF - Quake server browser and launcher
 *
 * GtkColumnView + GListStore implementation for server and player lists.
 * Each cell uses a GtkLabel (text-only; images can be added later).
 * Column-header clicks trigger sort via GtkCustomSorter per column.
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include <glib/gi18n.h>
#include "xqf-lists.h"
#include "xqf-server-item.h"
#include "xqf-player-item.h"
#include "xqf-ui.h"     /* server_list_def, player_list_def */
#include "srv-list.h"   /* assemble_server_address, server_list_sync_selection */
#include "sort.h"       /* compare_servers, compare_players */
#include "game.h"       /* games[], GAME_QUAKE1_PLAYER_COLORS */
#include "pref.h"       /* serverlist_countbots */

GListStore        *server_store     = NULL;
GListStore        *player_store     = NULL;
GtkSelectionModel *server_selection = NULL;
GtkSelectionModel *player_selection = NULL;

static GtkSortListModel *server_sort_model = NULL;
static GtkSortListModel *player_sort_model = NULL;

/* ------------------------------------------------------------------ */
/* Selection-changed callback                                           */
/* ------------------------------------------------------------------ */

static void
on_server_selection_changed (GtkSelectionModel *model G_GNUC_UNUSED,
                              guint pos G_GNUC_UNUSED,
                              guint n G_GNUC_UNUSED,
                              gpointer data G_GNUC_UNUSED)
{
    server_list_sync_selection ();
}

/* ------------------------------------------------------------------ */
/* Server column factory callbacks                                      */
/* ------------------------------------------------------------------ */

static void
server_col_setup (GtkSignalListItemFactory *f G_GNUC_UNUSED,
                  GtkListItem *item,
                  gpointer col G_GNUC_UNUSED)
{
    GtkWidget *label = gtk_label_new (NULL);
    gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
    gtk_widget_set_margin_start (label, 2);
    gtk_list_item_set_child (item, label);
}

static void
server_col_bind (GtkSignalListItemFactory *f G_GNUC_UNUSED,
                 GtkListItem *item,
                 gpointer user_data)
{
    int col = GPOINTER_TO_INT (user_data);
    GObject *obj = gtk_list_item_get_item (item);
    if (!obj) return;

    struct server *s = xqf_server_item_get (XQF_SERVER_ITEM (obj));
    GtkWidget *label = gtk_list_item_get_child (item);
    char buf[256];

    switch (col) {

    case 0: /* Name */
        gtk_label_set_text (GTK_LABEL (label), s->name ? s->name : "");
        break;

    case 1: /* Address */
        assemble_server_address (buf, sizeof (buf), s);
        gtk_label_set_text (GTK_LABEL (label), buf);
        break;

    case 2: /* Ping */
        if (s->ping >= 0)
            g_snprintf (buf, sizeof (buf), "%d",
                        s->ping > MAX_PING ? MAX_PING : s->ping);
        else
            g_strlcpy (buf, "n/a", sizeof (buf));
        gtk_label_set_text (GTK_LABEL (label), buf);
        break;

    case 3: { /* TO (timeout retries) */
        const char *retries;
        if (s->retries >= 0) {
            if (s->ping == MAX_PING + 1)
                retries = "D";
            else if (s->ping == MAX_PING)
                retries = "T";
            else {
                g_snprintf (buf, sizeof (buf), "%d", s->retries);
                retries = buf;
            }
        } else {
            retries = "?";
        }
        gtk_label_set_text (GTK_LABEL (label), retries);
        break;
    }

    case 4: { /* Priv (password / PunkBuster) */
        gboolean pw = (s->flags & SERVER_PASSWORD)   != 0;
        gboolean pb = (s->flags & SERVER_PUNKBUSTER) != 0;
        const char *priv = pw && pb ? "*PB" : pw ? "*" : pb ? "PB" : "";
        gtk_label_set_text (GTK_LABEL (label), priv);
        break;
    }

    case 5: { /* Players */
        unsigned short players = s->curplayers;
        if (serverlist_countbots && s->curbots <= players)
            players -= s->curbots;
        if (s->private_client)
            g_snprintf (buf, sizeof (buf), "%d/%d(-%d)",
                        players, s->maxplayers, s->private_client);
        else
            g_snprintf (buf, sizeof (buf), "%d/%d", players, s->maxplayers);
        gtk_label_set_text (GTK_LABEL (label), buf);
        break;
    }

    case 6: /* Map */
        gtk_label_set_text (GTK_LABEL (label), s->map ? s->map : "");
        break;

    case 7: /* Game */
        gtk_label_set_text (GTK_LABEL (label), s->game ? s->game : "");
        break;

    case 8: /* GameType */
        gtk_label_set_text (GTK_LABEL (label), s->gametype ? s->gametype : "");
        break;

    default:
        gtk_label_set_text (GTK_LABEL (label), "");
    }
}

/* ------------------------------------------------------------------ */
/* Server column sort compare                                           */
/* ------------------------------------------------------------------ */

static int
server_col_cmp (gconstpointer a, gconstpointer b, gpointer user_data)
{
    int col = GPOINTER_TO_INT (user_data);
    struct server *s1 = xqf_server_item_get (XQF_SERVER_ITEM ((gpointer) a));
    struct server *s2 = xqf_server_item_get (XQF_SERVER_ITEM ((gpointer) b));
    int mode_idx = server_list_def.cols[col].current_sort_mode;
    int mode = server_list_def.cols[col].sort_mode[mode_idx];
    if (mode < 0)
        mode = server_list_def.cols[col].sort_mode[0];
    int res = compare_servers (s1, s2, (enum ssort_mode) mode);
    if (res == 0 && mode != SORT_SERVER_PING)
        res = compare_servers (s1, s2, SORT_SERVER_PING);
    return res;
}

/* ------------------------------------------------------------------ */
/* Player column factory callbacks                                      */
/* ------------------------------------------------------------------ */

static void
player_col_setup (GtkSignalListItemFactory *f G_GNUC_UNUSED,
                  GtkListItem *item,
                  gpointer col G_GNUC_UNUSED)
{
    GtkWidget *label = gtk_label_new (NULL);
    gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
    gtk_widget_set_margin_start (label, 2);
    gtk_list_item_set_child (item, label);
}

static void
player_col_bind (GtkSignalListItemFactory *f G_GNUC_UNUSED,
                 GtkListItem *item,
                 gpointer user_data)
{
    int col = GPOINTER_TO_INT (user_data);
    GObject *obj = gtk_list_item_get_item (item);
    if (!obj) return;

    XqfPlayerItem *pi = XQF_PLAYER_ITEM (obj);
    struct player  *p  = xqf_player_item_get (pi);
    struct server  *owner = xqf_player_item_get_owner (pi);
    GtkWidget *label = gtk_list_item_get_child (item);
    char buf[256];

    switch (col) {

    case 0: /* Name */
        gtk_label_set_text (GTK_LABEL (label),
            (p->name && (p->flags & PLAYER_GROUP_MASK) == 0) ? p->name : "");
        break;

    case 1: /* Frags */
        g_snprintf (buf, sizeof (buf), "%d", (int) p->frags);
        gtk_label_set_text (GTK_LABEL (label), buf);
        break;

    case 2: /* Colors (QW shirt:pants) */
        if (owner && (games[owner->type].flags & GAME_QUAKE1_PLAYER_COLORS) != 0)
            g_snprintf (buf, sizeof (buf), "%d:%d", (int) p->shirt, (int) p->pants);
        else
            buf[0] = '\0';
        gtk_label_set_text (GTK_LABEL (label), buf);
        break;

    case 3: { /* Skin */
        char skin_buf[128];
        const char *skin = "";
        if (p->model && p->skin && *p->model && *p->skin) {
            g_snprintf (skin_buf, sizeof (skin_buf), "%s/%s", p->model, p->skin);
            skin = skin_buf;
        } else if (p->model && *p->model) {
            skin = p->model;
        } else if (p->skin && *p->skin) {
            skin = p->skin;
        }
        gtk_label_set_text (GTK_LABEL (label), skin);
        break;
    }

    case 4: /* Ping */
        if (p->ping >= 0) {
            g_snprintf (buf, sizeof (buf), "%d", (int) p->ping);
            gtk_label_set_text (GTK_LABEL (label), buf);
        } else {
            gtk_label_set_text (GTK_LABEL (label), "");
        }
        break;

    case 5: /* Time */
        if (p->time >= 0) {
            g_snprintf (buf, sizeof (buf), "%02d:%02d",
                        p->time / 60 / 60, p->time / 60 % 60);
            gtk_label_set_text (GTK_LABEL (label), buf);
        } else {
            gtk_label_set_text (GTK_LABEL (label), "");
        }
        break;

    default:
        gtk_label_set_text (GTK_LABEL (label), "");
    }
}

/* ------------------------------------------------------------------ */
/* Player column sort compare                                           */
/* ------------------------------------------------------------------ */

static int
player_col_cmp (gconstpointer a, gconstpointer b, gpointer user_data)
{
    int col = GPOINTER_TO_INT (user_data);
    struct player *p1 = xqf_player_item_get (XQF_PLAYER_ITEM ((gpointer) a));
    struct player *p2 = xqf_player_item_get (XQF_PLAYER_ITEM ((gpointer) b));
    int mode = player_list_def.cols[col].sort_mode[0];
    if (mode < 0) mode = SORT_PLAYER_NAME;
    return compare_players (p1, p2, (enum psort_mode) mode);
}

/* ------------------------------------------------------------------ */
/* Public: create_server_column_view                                   */
/* ------------------------------------------------------------------ */

GtkWidget *
create_server_column_view (GtkWidget *scrollwin)
{
    server_store = g_list_store_new (XQF_TYPE_SERVER_ITEM);

    GtkWidget *cv = gtk_column_view_new (NULL);
    gtk_column_view_set_show_row_separators (GTK_COLUMN_VIEW (cv), TRUE);

    for (int i = 0; i < server_list_def.columns; i++) {
        GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
        g_signal_connect (factory, "setup", G_CALLBACK (server_col_setup),
                          GINT_TO_POINTER (i));
        g_signal_connect (factory, "bind",  G_CALLBACK (server_col_bind),
                          GINT_TO_POINTER (i));

        GtkColumnViewColumn *col =
            gtk_column_view_column_new (_(server_list_def.cols[i].name), factory);
        gtk_column_view_column_set_resizable (col, TRUE);
        if (i == 0)
            gtk_column_view_column_set_expand (col, TRUE);
        else
            gtk_column_view_column_set_fixed_width (col, server_list_def.cols[i].width);

        GtkSorter *sorter = GTK_SORTER (
            gtk_custom_sorter_new (server_col_cmp, GINT_TO_POINTER (i), NULL));
        gtk_column_view_column_set_sorter (col, sorter);
        g_object_unref (sorter);

        gtk_column_view_append_column (GTK_COLUMN_VIEW (cv), col);
        g_object_unref (col);
        /* factory ownership transferred to col via gtk_column_view_column_new */
    }

    /* Connect GtkColumnView's combined sorter to the sort model. */
    server_sort_model = GTK_SORT_LIST_MODEL (
        gtk_sort_list_model_new (G_LIST_MODEL (server_store), NULL));
    gtk_sort_list_model_set_sorter (server_sort_model,
                                    gtk_column_view_get_sorter (GTK_COLUMN_VIEW (cv)));

    server_selection = GTK_SELECTION_MODEL (
        gtk_multi_selection_new (G_LIST_MODEL (server_sort_model)));

    gtk_column_view_set_model (GTK_COLUMN_VIEW (cv), server_selection);

    g_signal_connect (server_selection, "selection-changed",
                      G_CALLBACK (on_server_selection_changed), NULL);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), cv);

    /* Initial sort: Ping column (index 2), ascending — mirrors server_list_def. */
    GListModel *cols = gtk_column_view_get_columns (GTK_COLUMN_VIEW (cv));
    GtkColumnViewColumn *ping_col =
        GTK_COLUMN_VIEW_COLUMN (g_list_model_get_item (cols, 2));
    if (ping_col) {
        gtk_column_view_sort_by_column (GTK_COLUMN_VIEW (cv),
                                        ping_col, GTK_SORT_ASCENDING);
        g_object_unref (ping_col);
    }

    return cv;
}

/* ------------------------------------------------------------------ */
/* Public: create_player_column_view                                   */
/* ------------------------------------------------------------------ */

GtkWidget *
create_player_column_view (GtkWidget *scrollwin)
{
    player_store = g_list_store_new (XQF_TYPE_PLAYER_ITEM);

    GtkWidget *cv = gtk_column_view_new (NULL);
    gtk_column_view_set_show_row_separators (GTK_COLUMN_VIEW (cv), TRUE);

    for (int i = 0; i < player_list_def.columns; i++) {
        GtkListItemFactory *factory = gtk_signal_list_item_factory_new ();
        g_signal_connect (factory, "setup", G_CALLBACK (player_col_setup),
                          GINT_TO_POINTER (i));
        g_signal_connect (factory, "bind",  G_CALLBACK (player_col_bind),
                          GINT_TO_POINTER (i));

        GtkColumnViewColumn *col =
            gtk_column_view_column_new (_(player_list_def.cols[i].name), factory);
        gtk_column_view_column_set_resizable (col, TRUE);
        if (i == 0)
            gtk_column_view_column_set_expand (col, TRUE);
        else
            gtk_column_view_column_set_fixed_width (col, player_list_def.cols[i].width);

        GtkSorter *sorter = GTK_SORTER (
            gtk_custom_sorter_new (player_col_cmp, GINT_TO_POINTER (i), NULL));
        gtk_column_view_column_set_sorter (col, sorter);
        g_object_unref (sorter);

        gtk_column_view_append_column (GTK_COLUMN_VIEW (cv), col);
        g_object_unref (col);
        /* factory ownership transferred to col via gtk_column_view_column_new */
    }

    player_sort_model = GTK_SORT_LIST_MODEL (
        gtk_sort_list_model_new (G_LIST_MODEL (player_store), NULL));
    gtk_sort_list_model_set_sorter (player_sort_model,
                                    gtk_column_view_get_sorter (GTK_COLUMN_VIEW (cv)));

    player_selection = GTK_SELECTION_MODEL (
        gtk_single_selection_new (G_LIST_MODEL (player_sort_model)));

    gtk_column_view_set_model (GTK_COLUMN_VIEW (cv), player_selection);

    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
                                    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), cv);

    /* Initial sort: Frags column (index 1), descending. */
    GListModel *cols = gtk_column_view_get_columns (GTK_COLUMN_VIEW (cv));
    GtkColumnViewColumn *frags_col =
        GTK_COLUMN_VIEW_COLUMN (g_list_model_get_item (cols, 1));
    if (frags_col) {
        gtk_column_view_sort_by_column (GTK_COLUMN_VIEW (cv),
                                        frags_col, GTK_SORT_DESCENDING);
        g_object_unref (frags_col);
    }

    return cv;
}

/* ------------------------------------------------------------------ */
/* Public: server_store_find                                            */
/* ------------------------------------------------------------------ */

guint
server_store_find (struct server *s)
{
    if (!server_selection || !s) return G_MAXUINT;
    guint n = g_list_model_get_n_items (G_LIST_MODEL (server_selection));
    for (guint i = 0; i < n; i++) {
        XqfServerItem *item = XQF_SERVER_ITEM (
            g_list_model_get_item (G_LIST_MODEL (server_selection), i));
        struct server *ss = xqf_server_item_get (item);
        g_object_unref (item);
        if (ss == s) return i;
    }
    return G_MAXUINT;
}
