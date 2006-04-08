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

#include "gnuconfig.h"

#include <gtk/gtk.h>

#include "utils.h"
#include "pixmaps.h"
#include "game.h"
#include "loadpixmap.h"


// hack to make dlsym work
#define static
#include "xpm/update.xpm"
#include "xpm/refresh.xpm"
#include "xpm/refrsel.xpm"
#include "xpm/stop.xpm"

#include "xpm/connect.xpm"
#include "xpm/observe.xpm"
#include "xpm/record.xpm"

#include "xpm/sfilter.xpm"
#include "xpm/sfilter-cfg.xpm"
#include "xpm/pfilter.xpm"
#include "xpm/pfilter-cfg.xpm"

#include "xpm/q.xpm"
#include "xpm/q1.xpm"
#include "xpm/q2.xpm"
#include "xpm/q3.xpm"
#include "xpm/q4.xpm"
#include "xpm/wo.xpm"
#include "xpm/et.xpm"
#include "xpm/stvef.xpm"
#include "xpm/hex.xpm"
#include "xpm/hw.xpm"
#include "xpm/sn.xpm"
#include "xpm/hl.xpm"
#include "xpm/hl2.xpm"
#include "xpm/kp.xpm"
#include "xpm/sfs.xpm"
#include "xpm/sof2s.xpm"
#include "xpm/t2.xpm"
#include "xpm/hr2.xpm"
#include "xpm/un.xpm"
#include "xpm/postal2.xpm"
#include "xpm/aao.xpm"
#include "xpm/rune.xpm"
#include "xpm/descent3.xpm"
#include "xpm/ut2.xpm"
#include "xpm/ut2004.xpm"
#include "xpm/netp.xpm"
#include "xpm/nexuiz.xpm"
#include "xpm/warsow.xpm"
#include "xpm/tremulous.xpm"

#include "xpm/gamespy3d.xpm"
#include "xpm/ssam.xpm"
#include "xpm/mohaa.xpm"
#include "xpm/savage.xpm"
#include "xpm/cod.xpm"
#include "xpm/bf1942.xpm"
#include "xpm/jk3.xpm"
#include "xpm/doom3.xpm"

#include "xpm/green-plus.xpm"
#include "xpm/red-minus.xpm"

#include "xpm/man-black.xpm"
#include "xpm/man-red.xpm"
#include "xpm/man-yellow.xpm"

#include "xpm/group-red.xpm"
#include "xpm/group-green.xpm"
#include "xpm/group-blue.xpm"

#include "xpm/buddy-red.xpm"
#include "xpm/buddy-green.xpm"
#include "xpm/buddy-blue.xpm"

#include "xpm/error.xpm"
#include "xpm/delete.xpm"

#include "xpm/server-na.xpm"
#include "xpm/server-up.xpm"
#include "xpm/server-down.xpm"
#include "xpm/server-to.xpm"
#include "xpm/server-error.xpm"
#include "xpm/locked.xpm"
#include "xpm/punkbuster.xpm"
#include "xpm/locked_punkbuster.xpm"
#undef static

struct pixmap update_pix;
struct pixmap refresh_pix;
struct pixmap refrsel_pix;
struct pixmap stop_pix;

struct pixmap connect_pix;
struct pixmap observe_pix;
struct pixmap record_pix;

struct pixmap sfilter_pix;
struct pixmap sfilter_cfg_pix;
struct pixmap pfilter_pix;
struct pixmap pfilter_cfg_pix;

struct pixmap gplus_pix;
struct pixmap rminus_pix;

struct pixmap man_black_pix;
struct pixmap man_red_pix;
struct pixmap man_yellow_pix;

struct pixmap group_pix[3];
struct pixmap buddy_pix[9];

struct pixmap error_pix;
struct pixmap delete_pix;

struct pixmap server_status[5];
struct pixmap locked_pix;
struct pixmap punkbuster_pix;
struct pixmap locked_punkbuster_pix;

static GdkGC *pixmaps_gc;
static GdkGC *masks_gc;
static GdkColor mask_pattern;


int pixmap_height (GdkPixmap *pixmap) {
  int height, width;

  if (!pixmap)
    return 0;

  gdk_window_get_size (pixmap, &width, &height);
  return height;
}


int pixmap_width (GdkPixmap *pixmap) {
  int height, width;

  if (!pixmap)
    return 0;

  gdk_window_get_size (pixmap, &width, &height);
  return width;
}


void free_pixmap (struct pixmap *pixmap) {
  if (!pixmap)
    return;

  if (pixmap->pix) {
    gdk_pixmap_unref (pixmap->pix);
    pixmap->pix = NULL;
  }
  if (pixmap->mask) {
    gdk_bitmap_unref (pixmap->mask);
    pixmap->mask = NULL;
  }
}


static void create_pixmap (GtkWidget *widget, const char* file, struct pixmap *pix)
{
  load_pixmap_as_pixmap(widget, file, pix);

  if(!pix->pix)
  {
    pix->pix = error_pix.pix;
    pix->mask = error_pix.mask;
    gdk_pixmap_ref(pix->pix);
    gdk_bitmap_ref(pix->mask);
  }
}

void free_pixmaps (void)
{
  unsigned i;

  free_pixmap (&update_pix);
  free_pixmap (&refresh_pix);
  free_pixmap (&refrsel_pix);
  free_pixmap (&stop_pix);

  free_pixmap (&connect_pix);
  free_pixmap (&observe_pix);
  free_pixmap (&record_pix);

  free_pixmap (&sfilter_pix);
  free_pixmap (&sfilter_cfg_pix);
  free_pixmap (&pfilter_pix);
  free_pixmap (&pfilter_cfg_pix);

  free_pixmap (&gplus_pix);
  free_pixmap (&rminus_pix);

  free_pixmap (&man_black_pix);
  free_pixmap (&man_red_pix);
  free_pixmap (&man_yellow_pix); // He's actually green

  for (i = 0; i < 3; i++)
    free_pixmap (&group_pix[i]);

  for (i = 0; i < 9; i++)
    free_pixmap (&buddy_pix[i]);

  free_pixmap (&error_pix);
  free_pixmap (&delete_pix);

  for (i = 0; i < 5; i++)
    free_pixmap (&server_status[i]);

  free_pixmap (&locked_pix);

  free_pixmap (&punkbuster_pix);
  free_pixmap (&locked_punkbuster_pix);

  for (i = 0; i < GAMES_TOTAL; i++)
  {
    free_pixmap(games[i].pix);
    g_free(games[i].pix);
    games[i].pix = NULL;
  }

  if (pixmaps_gc) {
    gdk_gc_destroy (pixmaps_gc);
    pixmaps_gc = NULL;
  }

  if (masks_gc) {
    gdk_gc_destroy (masks_gc);
    masks_gc = NULL;
  }
}

/** \brief concatenate two pixmaps
 *
 * horizontal concatenation
 * @param window
 * @param dest destination pixmap
 * @param s1 first pixmap
 * @param s2 second pixmap
 * @returns dest for convenience
 */
struct pixmap* cat_pixmaps (GtkWidget *window, struct pixmap *dest, struct pixmap* s1, struct pixmap* s2)
{
  GdkGC *white_gc;
  int h1, w1, h2, w2;

  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  gdk_window_get_size (s1->pix, &w1, &h1);
  gdk_window_get_size (s2->pix, &w2, &h2);

  dest->pix  = gdk_pixmap_new (window->window, w1 + w2, MAX (h1, h2), -1);
  dest->mask = gdk_pixmap_new (window->window, w1 + w2, MAX (h1, h2), 1);

  white_gc = window->style->base_gc[GTK_STATE_NORMAL];

  if (!masks_gc) {
    masks_gc = gdk_gc_new (dest->mask);
    gdk_gc_set_exposures (masks_gc, FALSE);
  }

  mask_pattern.pixel = 0;
  gdk_gc_set_foreground (masks_gc, &mask_pattern);
  gdk_draw_rectangle (dest->mask, masks_gc, TRUE, 0, 0, -1, -1);

  mask_pattern.pixel = 1;
  gdk_gc_set_foreground (masks_gc, &mask_pattern);

  gdk_gc_set_clip_origin (white_gc, 0, 0);
  gdk_gc_set_clip_mask (white_gc, s1->mask);
  gdk_draw_pixmap (dest->pix, white_gc, s1->pix, 0, 0, 0, 0, w1, h1);
  gdk_draw_pixmap (dest->mask, masks_gc, s1->mask, 0, 0, 0, 0, w1, h1);

  gdk_gc_set_clip_origin (white_gc, w1, 0);
  gdk_gc_set_clip_mask (white_gc, s2->mask);
  gdk_draw_pixmap (dest->pix, white_gc, s2->pix, 0, 0, w1, 0, w2, h2);
  gdk_draw_pixmap (dest->mask, masks_gc, s2->mask, 0, 0, w1, 0, w2, h2);

  gdk_gc_set_clip_origin (white_gc, 0, 0);
  gdk_gc_set_clip_mask (white_gc, NULL);

  return dest;
}


void init_pixmaps (GtkWidget *window)
{
  unsigned i = 0;

  free_pixmaps ();

  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  create_pixmap (window, "update.xpm", &update_pix);
  create_pixmap (window, "refresh.xpm", &refresh_pix);
  create_pixmap (window, "refrsel.xpm", &refrsel_pix);
  create_pixmap (window, "stop.xpm", &stop_pix);

  create_pixmap (window, "connect.xpm", &connect_pix);
  create_pixmap (window, "observe.xpm", &observe_pix);
  create_pixmap (window, "record.xpm", &record_pix);

  create_pixmap (window, "sfilter.xpm", &sfilter_pix);
  create_pixmap (window, "sfilter_cfg.xpm", &sfilter_cfg_pix);

  create_pixmap (window, "pfilter.xpm", &pfilter_pix);
  create_pixmap (window, "pfilter_cfg.xpm", &pfilter_cfg_pix);

  create_pixmap (window, "green_plus.xpm", &gplus_pix);
  create_pixmap (window, "red_minus.xpm", &rminus_pix);

  create_pixmap (window, "man_black.xpm", &man_black_pix);
  create_pixmap (window, "man_red.xpm", &man_red_pix);
  create_pixmap (window, "man_yellow.xpm", &man_yellow_pix);

  create_pixmap (window, "group_red.xpm", &group_pix[0]);
  create_pixmap (window, "group_green.xpm", &group_pix[1]);
  create_pixmap (window, "group_blue.xpm", &group_pix[2]);

  create_pixmap (window, "buddy_red.xpm", &buddy_pix[1]);
  create_pixmap (window, "buddy_green.xpm", &buddy_pix[2]);
  create_pixmap (window, "buddy_blue.xpm", &buddy_pix[4]);

  create_pixmap (window, "server_na.xpm", &server_status[0]);
  create_pixmap (window, "server_up.xpm", &server_status[1]);
  create_pixmap (window, "server_down.xpm", &server_status[2]);
  create_pixmap (window, "server_to.xpm", &server_status[3]);
  create_pixmap (window, "server_error.xpm", &server_status[4]);

  create_pixmap (window, "error.xpm", &error_pix);
  create_pixmap (window, "delete.xpm", &delete_pix);
  create_pixmap (window, "locked.xpm", &locked_pix);
  create_pixmap (window, "punkbuster.xpm", &punkbuster_pix);
  cat_pixmaps(window, &locked_punkbuster_pix, &punkbuster_pix, &locked_pix);

  for (i = 0; i < GAMES_TOTAL; i++)
  {
    struct pixmap* pix = NULL;

    pix = g_malloc0(sizeof(struct pixmap));

    if(games[i].icon)
    {
      create_pixmap(window, games[i].icon, pix);
    }

    games[i].pix = pix;
  }
}


void ensure_buddy_pix (GtkWidget *window, int n) {
  int width, height;
  GdkGC *white_gc;
  int pri;
  int sec;

  if (!buddy_pix[1].pix)	/* not initialized */
    return;

  if (n < 0 || n > 9 || buddy_pix[n].pix)
    return;

  sec = ((n & 0x04) != 0)? 0x04 : 0x02;
  pri = n & ~sec;

  ensure_buddy_pix (window, pri);

  if (!pri || !sec)
    return;

  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  gdk_window_get_size (buddy_pix[1].pix, &width, &height);

  buddy_pix[n].pix = gdk_pixmap_new (window->window, width, height, -1);
  buddy_pix[n].mask = gdk_pixmap_new (window->window, width, height, 1);

  white_gc = window->style->white_gc;

  if (!masks_gc) {
    masks_gc = gdk_gc_new (buddy_pix[n].mask);
    gdk_gc_set_exposures (masks_gc, FALSE);
  }

  gdk_gc_set_foreground (masks_gc, &window->style->white);

  gdk_draw_pixmap (buddy_pix[n].pix, white_gc, buddy_pix[pri].pix,
                                                   0, 0, 0, 0, width, height);
  gdk_draw_pixmap (buddy_pix[n].mask, masks_gc, buddy_pix[pri].mask,
                                                   0, 0, 0, 0, width, height);

  gdk_gc_set_clip_mask (white_gc, buddy_pix[sec].mask);
  gdk_draw_pixmap (buddy_pix[n].pix, white_gc, buddy_pix[sec].pix,
                                                   0, 0, 0, 0, width, height);
  gdk_gc_set_clip_mask (white_gc, NULL);

  gdk_gc_set_clip_mask (masks_gc, buddy_pix[sec].mask);
  gdk_draw_rectangle (buddy_pix[n].mask, masks_gc, TRUE, 0, 0, width, height);
  gdk_gc_set_clip_mask (masks_gc, NULL);
}


GdkPixmap *two_colors_pixmap (GdkWindow *window, int width, int height,
                                            GdkColor *top, GdkColor *bottom) {
  GdkPixmap *pixmap;

  pixmap = gdk_pixmap_new (window, width, height, -1);

  if (!pixmaps_gc)
    pixmaps_gc = gdk_gc_new (window);

  gdk_gc_set_foreground (pixmaps_gc, top);
  gdk_draw_rectangle (pixmap, pixmaps_gc, TRUE, 0, 0, width, height/2);

  gdk_gc_set_foreground (pixmaps_gc, bottom);
  gdk_draw_rectangle (pixmap, pixmaps_gc, TRUE, 0, height/2, width, 
                                                           height - height/2);
  return pixmap;
}


void create_server_pixmap (GtkWidget *window, struct pixmap *stype, 
                                   int n, GdkPixmap **pix, GdkBitmap **mask) {
  GdkGC *white_gc;
  int hb, wb, hs, ws;

  if (!GTK_WIDGET_REALIZED (window))
    gtk_widget_realize (window);

  gdk_window_get_size (buddy_pix[1].pix, &wb, &hb);
  gdk_window_get_size (stype->pix, &ws, &hs);

  *pix  = gdk_pixmap_new (window->window, wb + ws, MAX (hs, hb), -1);
  *mask = gdk_pixmap_new (window->window, wb + ws, MAX (hs, hb), 1);

  white_gc = window->style->base_gc[GTK_STATE_NORMAL];

  if (!masks_gc) {
    masks_gc = gdk_gc_new (*mask);
    gdk_gc_set_exposures (masks_gc, FALSE);
  }

  mask_pattern.pixel = 0;
  gdk_gc_set_foreground (masks_gc, &mask_pattern);
  gdk_draw_rectangle (*mask, masks_gc, TRUE, 0, 0, -1, -1);

  mask_pattern.pixel = 1;
  gdk_gc_set_foreground (masks_gc, &mask_pattern);

  if (n) {
    ensure_buddy_pix (window, n);

    gdk_gc_set_clip_origin (white_gc, 0, 0);
    gdk_gc_set_clip_mask (white_gc, buddy_pix[n].mask);
    gdk_draw_pixmap (*pix, white_gc, buddy_pix[n].pix, 0, 0, 0, 0, wb, hb);

    gdk_draw_pixmap (*mask, masks_gc, buddy_pix[n].mask, 0, 0, 0, 0, wb, hb);
  }

  gdk_gc_set_clip_origin (white_gc, wb, 0);
  gdk_gc_set_clip_mask (white_gc, stype->mask);
  gdk_draw_pixmap (*pix, white_gc, stype->pix, 0, 0, wb, 0, ws, hs);

  gdk_draw_pixmap (*mask, masks_gc, stype->mask, 0, 0, wb, 0, ws, hs);

  gdk_gc_set_clip_origin (white_gc, 0, 0);
  gdk_gc_set_clip_mask (white_gc, NULL);
}


void pixmap_cache_lookup (GSList *cache, GdkPixmap **pix, GdkBitmap **mask, 
                                                               unsigned key) {
  struct cached_pixmap *cp;
  GdkPixmap *res_pix = NULL;
  GdkBitmap *res_mask = NULL;

  if (!pix)
    return;

  while (cache) {
    cp = (struct cached_pixmap *) cache->data;
    if (cp->key == key) {
      cp->weight += 2;
      res_pix = cp->pix;
      res_mask = cp->mask;
      break;
    }
    cache = cache->next;
  }

  *pix = res_pix;
  if (res_pix) 
    gdk_pixmap_ref (res_pix);

  if (mask) {
    *mask = res_mask;
    if (res_mask)
      gdk_bitmap_ref (res_mask);
  }
}


void pixmap_cache_add (GSList **cache, GdkPixmap *pix, GdkBitmap *mask, 
                                                               unsigned key) {
  struct cached_pixmap *cp;

  if (pix && cache) {
    cp = g_malloc0 (sizeof (struct cached_pixmap));

    cp->pix = pix;
    gdk_pixmap_ref (pix);

    if (mask) {
      cp->mask = mask;
      gdk_bitmap_ref (mask);
    }

    cp->key = key;
    cp->weight = 10;

    *cache = g_slist_prepend (*cache, cp);
  }
}


static int cached_pixmap_cmp (const struct cached_pixmap *a, 
                                              const struct cached_pixmap *b) {
  return b->weight - a->weight;	/* descending order */
}


static void free_cached_pixmap (struct cached_pixmap *cp) {

  gdk_pixmap_unref (cp->pix);

  if (cp->mask) 
    gdk_bitmap_unref (cp->mask);

  g_free (cp);
}


void pixmap_cache_clear (GSList **cache, int maxitems) {
  GSList *tmp = NULL;
  GSList *last = NULL;
  struct cached_pixmap *cp;

  if (cache && *cache) {
    if (maxitems > 0) {
      *cache = g_slist_sort (*cache, (GCompareFunc) cached_pixmap_cmp);

      for (tmp = *cache; (maxitems-- > 0) && tmp; tmp = tmp->next) {
	cp = (struct cached_pixmap *) tmp->data;
	cp->weight /= 2;
	last = tmp;
      }

      if (last)
	last->next = NULL;
    }
    else {
      tmp = *cache;
      *cache = NULL;
    }

    g_slist_foreach (tmp, (GFunc) free_cached_pixmap, NULL);
    g_slist_free (tmp);
  }
}

