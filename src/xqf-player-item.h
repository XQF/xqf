/* XQF - Quake server browser and launcher
 *
 * GObject wrapper for struct player*, for use with GListStore / GtkColumnView.
 *
 * Players are owned by their parent server (s->players GSList); this wrapper
 * holds a server_ref on the owning server to keep the player list alive for
 * as long as any player item exists.
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
