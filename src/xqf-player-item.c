/* XQF - Quake server browser and launcher
 *
 * GObject wrapper for struct player*, for use with GListStore / GtkColumnView.
 */

#include "xqf-player-item.h"
#include "server.h"   /* server_ref / server_unref */

struct _XqfPlayerItem {
  GObject parent_instance;
  struct player *player;
  struct server *owner;   /* ref-counted; keeps s->players alive */
};

G_DEFINE_TYPE (XqfPlayerItem, xqf_player_item, G_TYPE_OBJECT)

static void
xqf_player_item_finalize (GObject *obj)
{
  server_unref (XQF_PLAYER_ITEM (obj)->owner);
  G_OBJECT_CLASS (xqf_player_item_parent_class)->finalize (obj);
}

static void
xqf_player_item_class_init (XqfPlayerItemClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = xqf_player_item_finalize;
}

static void
xqf_player_item_init (XqfPlayerItem *self)
{
  (void)self;
}

XqfPlayerItem *
xqf_player_item_new (struct player *p, struct server *owner)
{
  XqfPlayerItem *self = g_object_new (XQF_TYPE_PLAYER_ITEM, NULL);
  self->player = p;
  self->owner  = owner;
  server_ref (owner);
  return self;
}

struct player *
xqf_player_item_get (XqfPlayerItem *self)
{
  return self->player;
}

struct server *
xqf_player_item_get_owner (XqfPlayerItem *self)
{
  return self->owner;
}
