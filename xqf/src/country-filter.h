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
#ifndef __COUNTRY_FILTER_H__
#define __COUNTRY_FILTER_H__

#ifdef USE_GEOIP

#include <glib.h>
#include "pixmaps.h"
#include <arpa/inet.h>

extern const unsigned MaxCountries;

extern void geoip_init(void);
extern void geoip_done(void);

/** return two letter country code */
const char* geoip_code_by_id(int id);

/** return full name of country */
const char* geoip_name_by_id(int id);

/** return id for an ip address */
int geoip_id_by_ip(struct in_addr in);

/** return id by country code **/
int geoip_id_by_code(const char *country);

/** return TRUE if geoip init was successful */
gboolean geoip_is_working (void);

struct pixmap* get_pixmap_for_country(int id);


#endif

#endif /*__COUNTRY_FILTER_H__*/
