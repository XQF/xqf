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
#include <stdio.h>	/* FILE */
#include <stdlib.h>	/* malloc */
#include <string.h>	/* memset */

#include "skin_pcx.h"


struct pcx_header {
  unsigned char manufacturer;
  unsigned char version;
  unsigned char encoding;
  unsigned char bitsperpixel;
  unsigned char nplanes;
  unsigned bytesperline;
  unsigned width; 
  unsigned height;
};


static int check_pcx_header (FILE *f, struct pcx_header *h, int quake2) {
  unsigned char raw[128];
  
  if (fread (raw, 128, 1, f) != 1) {
#ifdef DEBUG
    fprintf (stderr, "Cannot read PCX header\n");
#endif
    return 0;
  }

  h->manufacturer = raw[0];
  h->version	  = raw[1];
  h->encoding     = raw[2];
  h->bitsperpixel = raw[3];
  h->nplanes      = raw[65];
  h->bytesperline = raw[66] + (raw[67] << 8);
  h->width        = (raw[9] << 8) + raw[8] - ((raw[5] << 8) + raw[4]) + 1;
  h->height       = (raw[11] << 8) + raw[10] - ((raw[7] << 8) + raw[6]) + 1;

  if (h->manufacturer != 10 || h->encoding != 1 || h->bitsperpixel != 8 || 
                                                            h->nplanes != 1) {
#ifdef DEBUG
    fprintf (stderr, "Invalid PCX header: manufacturer %d(10), "
	             "encoding %d(1), bitsperpixel %d(8), nplanes %d(1)\n",
                     h->manufacturer, h->encoding, 
                     h->bitsperpixel, h->nplanes);
#endif
    return 0;
  }

  if (quake2) {
    if (h->width != 32 || h->width != 32) {
#ifdef DEBUG
      fprintf (stderr, "Wrong Quake2 skin size: [%d,%d]\n", 
                                                         h->width, h->height);
#endif
      return 0;
    }
  }
  else {
    if (h->width > 320 || h->width < 296 || 
                                         h->height > 200 || h->height < 194) {
#ifdef DEBUG
      fprintf (stderr, "Wrong QuakeWorld skin size: [%d,%d]\n", 
                                                         h->width, h->height);
#endif
      return 0;
    }
  }

  return 1;
}


static int read_pcx_line (FILE *f, char *buf, int len) {
  char *ptr;
  int cnt;
  int c;

  for (ptr = buf; ptr < buf + len; ) {
    if ((c = getc (f)) == EOF) {
#ifdef DEBUG
      fprintf (stderr, "Unexpected EOF\n");
#endif
      return 0;
    }
    if ((c & 0xC0) != 0xC0)
      *ptr++ = c;
    else {
      cnt = c & 0x3F;
      if ((c = getc (f)) == EOF) {
#ifdef DEBUG
	fprintf (stderr, "Unexpected EOF\n");
#endif
	return 0;
      }
      memset (ptr, c, cnt);
      ptr += cnt;
    }
  }
  return 1;
}


char *read_skin_pcx (char *filename, int quake2) {
  FILE *f = NULL;
  char *buf = NULL;
  char *ptr;
  int lines = 0;
  struct pcx_header h;

  if ((f = fopen (filename, "r")) == NULL)
    return NULL;

  if (!check_pcx_header (f, &h, quake2) || 
      (buf = malloc ((quake2)? 32*32 : 320*200)) == NULL) {
    fclose (f);
    return NULL;
  }

  ptr = buf;

  for (lines = 0; lines < h.height; lines++) {
    if (read_pcx_line (f, ptr, h.width) == 0) {
      free (buf);
      buf = NULL;
    }

    if (quake2) {
      ptr += 32;
    }
    else {
      if (320 - h.width > 0)
	memset (ptr + h.width, 0, 320 - h.width);
      ptr += 320;
    }
  }

  if (!quake2 && lines < 200)
    memset (ptr, 0, (200 - lines) * 320);

  fclose (f);
  return buf;
}

