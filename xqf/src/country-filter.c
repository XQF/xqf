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

#ifdef USE_GEOIP

#include "country-filter.h"
#include "i18n.h"
#include "debug.h"
#include "pixmaps.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctype.h>

#include <GeoIP.h>

const int MaxCountries = sizeof(GeoIP_country_code)/3;

static GeoIP* gi;

static struct pixmap* flags = NULL;

void geoip_init(void)
{
  gi = GeoIP_new(GEOIP_STANDARD);
  if(gi)
    flags = g_malloc0(MaxCountries*sizeof(struct pixmap));
  else
    xqf_error("GeoIP initialization failed");
}

void geoip_done(void)
{
  if(gi)
    GeoIP_delete(gi); 
  g_free(flags);
}

const char* geoip_code_by_id(int id)
{
  if(id < 0 || id >= MaxCountries ) return NULL;
  return GeoIP_country_code[id];
}

const char* geoip_name_by_id(int id)
{
  if(id < 0 || id >= MaxCountries ) return NULL;
  return GeoIP_country_name[id];
}

int geoip_id_by_ip(struct in_addr in)
{
	if(!gi) return -1;
	return GeoIP_country_id_by_addr(gi, inet_ntoa (in));
}

#warning enter KDE path
static char kdeflagpath[]="/opt/kde3/share/locale/l10n/%2s/flag.png";;

struct pixmap* get_pixmap_for_country(int id)
{
  GdkPixbuf* pixbuf = NULL;
  struct pixmap* pix = NULL;

  char* filename = NULL;

  char* code = NULL;

  if(!flags) return NULL;
  if(id < 1) return NULL; // no flag for N/A
  
  code = (char*)geoip_code_by_id(id);
  if(!code) return NULL;

  pix = &flags[id];

  if(pix->pix == GINT_TO_POINTER(-1)) return NULL;
  if(pix->pix) return pix;

  code = g_strdup(code);
  g_strdown(code);

  filename = g_strdup_printf(kdeflagpath,code);
  if(!filename)
  {
    g_free(code);
    return NULL;
  }

  debug(0,"loading %s",filename);
  
  pixbuf = gdk_pixbuf_new_from_file(filename);
  if (pixbuf == NULL)
  {
    pix->pix=GINT_TO_POINTER(-1);
    g_warning (_("Error loading pixmap file: %s"), filename);
    g_free (filename);
    g_free(code);
    return NULL;
  }

  gdk_pixbuf_render_pixmap_and_mask(pixbuf,&pix->pix,&pix->mask,0);

  gdk_pixbuf_unref(pixbuf);
  g_free(code);
  g_free (filename);

  return pix;
}

#endif
