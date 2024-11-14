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

#ifdef USE_GEOIP

#include "country-filter.h"
#include "debug.h"
#include "pixmaps.h"
#include "loadpixmap.h"
#include "xpm/noflag.xpm"

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>

#include <ctype.h>
#include <unistd.h> // access()

#include <GeoIP.h>
#include <dlfcn.h>

static const char*  xqf_geoip_country_code;
static const char** xqf_geoip_country_name;

unsigned MaxCountries;
static unsigned LAN_GeoIPid;

static GeoIP* gi;

/* array of (struct pixmap*), indexed by country id.
flags[id] has three states:
	-1     flag unavailable
	0     flag not yet loaded
	<else> pointer to struct pixmap
*/

static struct pixmap* flags = NULL;

void geoip_init(void) {
	const char* geoipdat = getenv("xqf_GEOIPDAT");

	if (gi) { // already initialized
		return;
	}

	if (geoipdat) {
		gi = GeoIP_open(geoipdat, GEOIP_STANDARD);
	}

	if (!gi) {
		gi = GeoIP_new(GEOIP_STANDARD);
	}

	if (gi && geoip_num_countries()) {
		flags = g_malloc0((MaxCountries+1) *sizeof(struct pixmap)); /*+1-> flag for LAN server*/
	}
	else {
		geoip_done();
		xqf_error("GeoIP initialization failed");
	}
}

void geoip_done(void) {
	if (gi) {
		GeoIP_delete(gi);
	}

	g_free(flags);
}

gboolean geoip_is_working (void) {
	/* gi is a (static GeoIP*), thus do not return a pointer but a boolean */
	return !!gi;
}

const char* geoip_code_by_id(int id) {
	if (id < 0 || id > MaxCountries) {
		return NULL;
	}

	/* LAN server have code ="00" */
	if (id == LAN_GeoIPid) {
		return "00";
	}
	
	return &xqf_geoip_country_code[id*3];
}


const char* geoip_name_by_id(int id) {
	if (!gi || id < 0 || id > MaxCountries) {
		return NULL;
	}

	if (id == LAN_GeoIPid) {
		return "LAN";
	}
	
	return xqf_geoip_country_name[id];
}

int geoip_id_by_code(const char *country) {
	int i;

	if (!gi) {
		return -1;
	}

	if (strcmp(country,"00") == 0) {
		return LAN_GeoIPid;
	}

	for (i=1;i<MaxCountries;++i) {
		if (strcmp(country,xqf_geoip_country_name[i]) == 0) {
			return i;
		}
	}

	return 0;
}

/*Checks for RFC1918 private addresses; returns TRUE if is a private address. */
/*from the napshare source*/
static gboolean is_private_ip(guint32 ip) {
	return (ip & 0xff000000) == 0x7f000000 /* 127.0.0.0 -- (localhost) */
		|| (ip & 0xff000000) == 0xa000000 /* 10.0.0.0 -- (10/8 prefix) */
		|| (ip & 0xfff00000) == 0xac100000 /* 172.16.0.0 -- (172.16/12 prefix) */
		|| (ip & 0xffff0000) == 0xc0a80000; /* 192.168.0.0 -- (192.168/16 prefix) */
}


int geoip_id_by_ip(struct in_addr in) {
	if (!gi) {
		return -1;
	}

	/*check if the server is inside a LAN*/
	if (is_private_ip(htonl(in.s_addr))) {
		return LAN_GeoIPid;
	}
	
	return GeoIP_country_id_by_addr(gi, inet_ntoa (in));
}

static char* find_flag_file(int id) {
	gchar file[] = "flags/lan.png";
	gchar* file_lwr;
	gchar* filename;

	if (id != LAN_GeoIPid) {
		const gchar* code = geoip_code_by_id(id);
		if (!code) return NULL;
		strncpy(file+strlen("flags/"), code, 2);
		strcpy(file+strlen("flags/")+2, ".png");
	}

	file_lwr = g_ascii_strdown(file, -1);
	filename = find_pixmap_directory(file_lwr);
	g_free(file_lwr);

	return filename;
}

struct pixmap* get_pixmap_for_country(int id) {
	struct pixmap* pix = NULL;

	char* filename = NULL;

	if (!flags) {
		return NULL;
	}
	if (id < 1) {   // no flag for N/A
		return NULL;
	}

	pix = &flags[id];

	if (pix->pixbuf == GINT_TO_POINTER(-1)) {
		return NULL;
	}
	if (pix->pixbuf) {
		return pix;
	}

	filename = find_flag_file(id);

	if (!filename || access(filename,R_OK)) {
		g_free(filename);
		return NULL;
	}

	debug(4, "loading gdk_pixbuf from file: %s", filename);

	pix->pixbuf = gdk_pixbuf_new_from_file(filename, NULL);

	if (pix->pixbuf == NULL) {
		pix->pixbuf=GINT_TO_POINTER(-1);
		g_warning (_("Error loading pixmap file: %s"), filename);
		g_free (filename);
		return NULL;
	}

#ifdef GUI_GTK2
	gdk_pixbuf_render_pixmap_and_mask(pix->pixbuf,&pix->pix,&pix->mask,255);
#endif

	g_free (filename);

	return pix;
}

struct pixmap* get_pixmap_for_country_with_fallback(int id) {
	struct pixmap* pix = get_pixmap_for_country(id);
	if (pix) {
		return pix;
	}

	if (!flags || flags[0].pixbuf == GINT_TO_POINTER(-1)) {
		return NULL;
	}

	if (!flags[0].pixbuf) {
		flags[0].pixbuf = gdk_pixbuf_new_from_xpm_data( (const char **)noflag_xpm);
		if (!flags[0].pixbuf)
			flags[0].pixbuf = GINT_TO_POINTER(-1);
#ifdef GUI_GTK2
		else
			gdk_pixbuf_render_pixmap_and_mask(flags[0].pixbuf,&flags[0].pix,&flags[0].mask,255);
#endif
	}
	return &flags[0];
}

static void* lookup_symbol(const char* name) {
	void* p;
	char* error;
	dlerror();
	p = dlsym(NULL, name);
	if ((error = dlerror()) != NULL) {
		xqf_error("failed to lookup %s: %s", name, error);
		return NULL;
	}
	return p;
}

unsigned geoip_num_countries() {
	if ((void*)xqf_geoip_country_code == (void*)-1) {
		return 0;
	}

	if (!xqf_geoip_country_code) {
		unsigned i;

		xqf_geoip_country_name = lookup_symbol("GeoIP_country_name");
		xqf_geoip_country_code = lookup_symbol("GeoIP_country_code");

		if (!xqf_geoip_country_name || !xqf_geoip_country_code) {
			/* nasty hack to determine the number of countries at run time */
			MaxCountries = LAN_GeoIPid = 0;
			xqf_geoip_country_name = (void*)-1;
			xqf_geoip_country_code = (void*)-1;
			return 0;
		}

		for (i = 0; xqf_geoip_country_code[i*3] && i < 333 /* arbitrary limit */; ++i) {
			/* nothing */
		}

		if (i >= 333) {
			xqf_geoip_country_name = (void*)-1;
			xqf_geoip_country_code = (void*)-1;
			xqf_error("failed to determine number of supported countries");
			return 0;
		}

		MaxCountries = LAN_GeoIPid = i;
	}

	debug(1, "MaxCountries %u", MaxCountries);

	return MaxCountries;
}

#endif
