/* XQF - Quake server browser and launcher
 *
 * GObject wrapper for struct player*, for use with GListStore / GtkColumnView.
 *
 * Players are owned by their parent server (s->players GSList). This wrapper
 * holds a server_ref on the owning server, which only keeps the struct server
 * itself alive -- NOT the individual struct player* this item wraps.
 * server_free_info() (server.c) frees and rebuilds s->players on every stat
 * refresh regardless of ref_count, so a stale XqfPlayerItem can outlive its
 * player. xqf_player_item_get() defends against this by verifying the
 * pointer is still in owner->players before returning it, and returns NULL
 * otherwise -- callers must handle that.
 */

#pragma once

#include <glib-object.h>
#include "defs.h"

G_BEGIN_DECLS

#define XQF_TYPE_PLAYER_ITEM xqf_player_item_get_type ()
G_DECLARE_FINAL_TYPE (XqfPlayerItem, xqf_player_item, XQF, PLAYER_ITEM, GObject)

XqfPlayerItem *xqf_player_item_new  (struct player *p, struct server *owner);
struct player *xqf_player_item_get  (XqfPlayerItem *self);
struct server *xqf_player_item_get_owner (XqfPlayerItem *self);

G_END_DECLS
