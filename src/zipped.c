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
#include <stdio.h>	/* FILE, popen, pclose, ... */
#include <unistd.h>	/* unlink, stat */
#include <sys/stat.h>	/* stat */

#include <gtk/gtk.h>

#include "zipped.h"


struct compr {
  char *suffix;
  char *compress;
  char *expand;
};


#ifdef COMPRESSOR_BZIP2
# define COMPRESSOR_DEFAULT 1
#else
# ifdef COMPRESSOR_GZIP
#  define COMPRESSOR_DEFAULT 2
# else
#  define COMPRESSOR_DEFAULT 0	/* None */
# endif
#endif


static struct compr compressor[] = {
  { "",      NULL,        NULL          },
  { ".bz2",  "bzip2 -1",  "bzip2 -d -c" },
  { ".gz",   "gzip -1",   "gzip -d -c"  },
  { NULL,    NULL,        NULL          }
};


void zstream_open_r (struct zstream *z, const char *name) {
  struct stat stbuf;
  char *fn;
  char *cmd;
  int i;

  if (!z) return;

  for (i = 0; compressor[i].suffix; i++) {
    fn = g_strconcat (name, compressor[i].suffix, NULL);
  
    if (stat (fn, &stbuf) == 0) {
      if (compressor[i].expand) {
	cmd = g_strconcat (compressor[i].expand, " ", fn, NULL);

#ifdef DEBUG
	fprintf (stderr, "popen(\"%s\", \"r\")\n", cmd);
#endif

	z->f = popen (cmd, "r");
	z->is_pipe = TRUE;
	g_free (cmd);
      }
      else {
	z->f = fopen (name, "r");
	z->is_pipe = FALSE;
      }
      return;
    }

    g_free (fn);
  }

  z->f = NULL;
}


void zstream_open_w (struct zstream *z, const char *name) {
  char *cmd;

  if (!z) return;

  zstream_unlink (name);

  z->f = NULL;

  if (compressor[COMPRESSOR_DEFAULT].compress) {
    cmd = g_strconcat (compressor[COMPRESSOR_DEFAULT].compress, " >", 
		       name, compressor[COMPRESSOR_DEFAULT].suffix, 
		       NULL);
#ifdef DEBUG
    fprintf (stderr, "popen(\"%s\", \"w\")\n", cmd);
#endif

    z->f = popen (cmd, "w");
    z->is_pipe = TRUE;
    g_free (cmd);
  }
  else {
    z->f = fopen (name, "w");
    z->is_pipe = FALSE;
  }
}


void zstream_close (struct zstream *z) {
  if (z && z->f) {

    if (z->is_pipe)
      pclose (z->f);
    else
      fclose (z->f);

    z->f = NULL;
  }
}


void zstream_unlink (const char *name) {
  char *fn;
  int i;

  for (i = 0; compressor[i].suffix; i++) {
    fn = g_strconcat (name, compressor[i].suffix, NULL);
    unlink (fn);
    g_free (fn);
  }
}

