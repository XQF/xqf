/* XQF - Quake server browser and launcher
 *
 * GObject wrapper for struct server*, for use with GListStore / GtkColumnView.
 */

#pragma once

#include <glib-object.h>
#include "defs.h"

G_BEGIN_DECLS

#define XQF_TYPE_SERVER_ITEM xqf_server_item_get_type ()
G_DECLARE_FINAL_TYPE (XqfServerItem, xqf_server_item, XQF, SERVER_ITEM, GObject)

XqfServerItem *xqf_server_item_new (struct server *s);
struct server *xqf_server_item_get (XqfServerItem *self);

G_END_DECLS
