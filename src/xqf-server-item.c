/* XQF - Quake server browser and launcher
 *
 * GObject wrapper for struct server*, for use with GListStore / GtkColumnView.
 */

#include "xqf-server-item.h"
#include "server.h"   /* server_ref / server_unref */

struct _XqfServerItem {
  GObject parent_instance;
  struct server *server;
};

G_DEFINE_TYPE (XqfServerItem, xqf_server_item, G_TYPE_OBJECT)

static void
xqf_server_item_finalize (GObject *obj)
{
  server_unref (XQF_SERVER_ITEM (obj)->server);
  G_OBJECT_CLASS (xqf_server_item_parent_class)->finalize (obj);
}

static void
xqf_server_item_class_init (XqfServerItemClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = xqf_server_item_finalize;
}

static void
xqf_server_item_init (XqfServerItem *self)
{
  (void)self;
}

XqfServerItem *
xqf_server_item_new (struct server *s)
{
  XqfServerItem *self = g_object_new (XQF_TYPE_SERVER_ITEM, NULL);
  self->server = s;
  server_ref (s);
  return self;
}

struct server *
xqf_server_item_get (XqfServerItem *self)
{
  return self->server;
}
