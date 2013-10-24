/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include <sys/types.h>
#include <string.h>		/* strcmp */

#include <glib.h>

#include "xqf.h"
#include "config.h"
#include "history.h"


struct history *history_new (char *id) {
  struct history *h;
  char path[128];
  config_key_iterator* iterator;
  char *keyval;

  h = g_malloc0 (sizeof (struct history));
  h->id = g_strdup (id);
  h->size = HISTORY_DEFAULT_MAX_ITEMS;

  g_snprintf (path, 128, "/" CONFIG_FILE "/History: %s", h->id);

  iterator = config_init_iterator (path);
  while (iterator) {
    iterator = config_iterator_next (iterator, NULL, &keyval);
    h->items = g_list_append (h->items, keyval);
  }

  return h;
}


char *history_add (struct history *h, char *str) {
  GList *list;

  if (!h || !str)
    return str;

  for (list = h->items; list; list = list->next) {
    if (!strcmp (str, (char *) list->data)) {
      g_free (list->data);
      h->items = g_list_remove_link (h->items, list);
      break;
    }
  }

  while (h->items && g_list_length (h->items) >= h->size) {
    list = g_list_last (h->items);
    g_free (list->data);
    h->items = g_list_remove_link (h->items, list); 
  }
  
  h->items = g_list_prepend (h->items, g_strdup (str));

  return str;
}


void history_free (struct history *h) {
  GList *list;
  char fmt[128];
  char key[128];
  int i;

  if(!h) return;

  g_snprintf (fmt, 128, "/" CONFIG_FILE "/History: %s/%%i", h->id);

  for (i = 0, list = h->items; list; i++, list = list->next) {
    g_snprintf (key, 128, fmt, i);
    config_set_string (key, (char *) list->data);
  }

  if (h) {
    if (h->items) {
      g_list_foreach (h->items, (GFunc) g_free, NULL);
      g_list_free (h->items);
      h->items = NULL;
    }
    if (h->id) {
      g_free (h->id);
      h->id = NULL;
    }
    g_free (h);
  }
}

