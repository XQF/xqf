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
#include "loadpixmap.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <ctype.h>
#include <unistd.h> // access()

#include <GeoIP.h>

const unsigned MaxCountries = sizeof(GeoIP_country_code)/3;
static const unsigned LAN_GeoIPid = sizeof(GeoIP_country_code)/3; // MaxCountries doesn't work in C

static GeoIP* gi;

/* array of (struct pixmap*), indexed by country id.
   flags[id] has three states:
     -1     flag unavailable
      0     flag not yet loaded
     <else> pointer to struct pixmap
*/
static struct pixmap* flags = NULL;

void geoip_init(void)
{
  gi = GeoIP_new(GEOIP_STANDARD);
  if(gi)
    flags = g_malloc0((MaxCountries+1) *sizeof(struct pixmap)); /*+1-> flag for LAN server*/
  else
    xqf_error("GeoIP initialization failed");
}

void geoip_done(void)
{
  if(gi)
    GeoIP_delete(gi); 
  g_free(flags);
}

gboolean geoip_is_working (void)
{

  if (gi)
    return TRUE;
  else
    return FALSE;
}

const char* geoip_code_by_id(int id)
{
  if(id < 0 || id > MaxCountries ) return NULL;

  /* LAN server have code ="00" */  
  if (id == LAN_GeoIPid)
    return "00";
  else
    return GeoIP_country_code[id];
}


const char* geoip_name_by_id(int id)
{
  if(id < 0 || id > MaxCountries) return NULL;
  
  if (id == LAN_GeoIPid)
    return "LAN";
  else
    return GeoIP_country_name[id];
}

int geoip_id_by_code(const char *country)
{
  int i;

  if(!gi) return -1;

  if (strcmp(country,"00")==0)
    return LAN_GeoIPid;

  for (i=1;i<MaxCountries;++i)
  {
    if (strcmp(country,GeoIP_country_code[i])==0) 
      return i;
  }

  return 0;
}

/*Checks for RFC1918 private addresses; returns TRUE if is a private address. */
/*from the napshare source*/
static gboolean is_private_ip(guint32 ip)
{
  /* 10.0.0.0 -- (10/8 prefix) */
  if ((ip & 0xff000000) == 0xa000000)
  {
      return TRUE;
  }

  /* 172.16.0.0 -- (172.16/12 prefix) */
  if ((ip & 0xfff00000) == 0xac100000)
  {
      return TRUE;
  }

  /* 192.168.0.0 -- (192.168/16 prefix) */
  if ((ip & 0xffff0000) == 0xc0a80000)
  {
      return TRUE;
  }

  return FALSE;
}


int geoip_id_by_ip(struct in_addr in)
{

  if(!gi) return -1;
    
  /*check if the server is inside a LAN*/
  if (is_private_ip(htonl(in.s_addr)))
    return LAN_GeoIPid;
  else
    return GeoIP_country_id_by_addr(gi, inet_ntoa (in));
}

static char* find_flag_file(int id)
{
  char file[] = "flags/lan.png";
  char* filename;

  if (id != LAN_GeoIPid)
  {
    const char* code = geoip_code_by_id(id);
    if(!code) return NULL;
    strncpy(file+strlen("flags/"),code,2);
    strcpy(file+strlen("flags/")+2,".png");
    g_strdown(file);
  }

  filename = find_pixmap_directory(file);

  return filename;
}

struct pixmap* get_pixmap_for_country(int id)
{
  GdkPixbuf* pixbuf = NULL;
  struct pixmap* pix = NULL;

  char* filename = NULL;

  if(!flags) return NULL;
  if(id < 1) return NULL; // no flag for N/A
  
  pix = &flags[id];

  if(pix->pix == GINT_TO_POINTER(-1)) return NULL;
  if(pix->pix) return pix;

  filename = find_flag_file(id);

  if(!filename || access(filename,R_OK))
  {
    g_free(filename);
    return NULL;
  }

  debug(4,"loading %s",filename);

/*FIXME_GTK2: need GError*/
#ifdef USE_GTK2
  pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
#else
  pixbuf = gdk_pixbuf_new_from_file(filename);
#endif

  if (pixbuf == NULL)
  {
    pix->pix=GINT_TO_POINTER(-1);
    g_warning (_("Error loading pixmap file: %s"), filename);
    g_free (filename);
    return NULL;
  }

  gdk_pixbuf_render_pixmap_and_mask(pixbuf,&pix->pix,&pix->mask,255);

  gdk_pixbuf_unref(pixbuf);
  g_free (filename);

  return pix;
}

#endif
