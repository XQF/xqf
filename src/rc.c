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

#include <stdio.h>
#include <ctype.h>	/* is...() */
#include <string.h>	/* strcmp */
#include <sys/stat.h>	/* stat. mkdir */
#include <unistd.h>	/* stat, mkdir, unlink */
#include <sys/types.h>	/* mkdir */
#include <fcntl.h>	/* mkdir */

#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "filter.h"
#include "utils.h"
#include "config.h"
#include "rc.h"


static FILE *rc;
static int token = -1;
static char *token_str = NULL;
static int token_int = 0;
static char *tptr = NULL;
static int state = STATE_SPACE;

static char *rcfilename = NULL;
static int line;
static int pos;


static struct keyword  keywords[] = {

  { "q1_top",	 KEYWORD_INT,   "/" CONFIG_FILE "/Game: QS/top" },
  { "q1_bottom", KEYWORD_INT,	"/" CONFIG_FILE "/Game: QS/bottom" },
  { "team",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/team" },
  { "qw_skin",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/skin" },
  { "qw_top",	 KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/top" },
  { "qw_bottom", KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/bottom" },
  { "q2_skin",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q2S/skin" },
  { "rate", 	 KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/rate" },
  { "cl_nodelta",KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/cl_nodelta" },
  { "cl_predict",KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/cl_predict" },
  { "noaim",	 KEYWORD_INT,   "/" CONFIG_FILE "/Game: QWS/noaim" },
  { "pushlatency",KEYWORD_INT,  "/" CONFIG_FILE "/Game: QWS/pushlatency mode" },
  { "noskins",	 KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/noskins" },
  { "w_switch",	 KEYWORD_INT,   "/" CONFIG_FILE "/Game: QWS/w_switch" },
  { "b_switch",	 KEYWORD_INT,   "/" CONFIG_FILE "/Game: QWS/b_switch" },

  { "name",	 KEYWORD_STRING,"/" CONFIG_FILE "/Games Config/player name" },
  { "nosound",   KEYWORD_BOOL,  "/" CONFIG_FILE "/Games Config/nosound" },
  { "nocdaudio", KEYWORD_BOOL,  "/" CONFIG_FILE "/Games Config/nocdaudio" },

  { "retries",   KEYWORD_INT, 	"/" CONFIG_FILE "/Server Filter/retries" },
  { "ping",  	 KEYWORD_INT,	"/" CONFIG_FILE "/Server Filter/ping" },
  { "notfull",   KEYWORD_BOOL,	"/" CONFIG_FILE "/Server Filter/not full" },
  { "notempty",  KEYWORD_BOOL,	"/" CONFIG_FILE "/Server Filter/not empty" },
  { "nocheats",  KEYWORD_BOOL,	"/" CONFIG_FILE "/Server Filter/no cheats" },
  { "nopasswd",  KEYWORD_BOOL,	"/" CONFIG_FILE "/Server Filter/no password" },

  { "terminate",  KEYWORD_BOOL, "/" CONFIG_FILE "/General/terminate" },
  { "iconify",    KEYWORD_BOOL, "/" CONFIG_FILE "/General/iconify" },
  { "savelists",  KEYWORD_BOOL, "/" CONFIG_FILE "/General/save lists" },
  { "saveservers",KEYWORD_BOOL, "/" CONFIG_FILE "/General/save srvinfo" },
  { "saveplayers",KEYWORD_BOOL, "/" CONFIG_FILE "/General/save players" },

  { "autofavorites", KEYWORD_BOOL,
    "/" CONFIG_FILE "/General/refresh favorites" },

  { "tb_style",	 KEYWORD_INT,	"/" CONFIG_FILE "/Appearance/toolbar style" },
  { "tb_tips",	 KEYWORD_BOOL,	"/" CONFIG_FILE "/Appearance/toolbar tips" },

  { "sort_on_refresh",	KEYWORD_BOOL,
    "/" CONFIG_FILE "/Appearance/sort on refresh" },
  { "ref_on_update",	KEYWORD_BOOL,
    "/" CONFIG_FILE "/Appearance/refresh on update" },
  { "alwaysresolve",	KEYWORD_BOOL,
    "/" CONFIG_FILE "/Appearance/show hostnames" },
  { "maxsimultaneous",	KEYWORD_INT, 
    "/" CONFIG_FILE "/QStat/maxsimultaneous" },
  { "maxretries", 	KEYWORD_INT,
    "/" CONFIG_FILE "/QStat/maxretires" },

  { "q1_custom_cfg", KEYWORD_STRING,"/" CONFIG_FILE "/Game: QS/custom cfg" },
  { "qw_custom_cfg", KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/custom cfg" },
  { "q2_custom_cfg", KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q2S/custom cfg" },
  { "q3_custom_cfg", KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q3S/custom cfg" },

  { "q1_dir",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QS/dir" },
  { "q1_cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QS/cmd" },
  { "qw_dir",    KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/dir" },
  { "qw_cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/cmd" },
  { "q2_dir",    KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q2S/dir" },
  { "q2_cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q2S/cmd" },
  { "q3_dir",    KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q3S/dir" },
  { "q3_cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: Q3S/cmd" },
  { "hl_dir",    KEYWORD_STRING,"/" CONFIG_FILE "/Game: HLS/dir" },
  { "hl_cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: HLS/cmd" },

  /* compatibility with ancient versions */

  { "top",	 KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/top" },
  { "bottom",	 KEYWORD_INT,	"/" CONFIG_FILE "/Game: QWS/bottom" },

  { "cl_predict_players", KEYWORD_INT, 
    "/" CONFIG_FILE "/Game: QWS/cl_predict" },

  { "skin",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/skin" },
  { "dir",       KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/dir" },
  { "cmd",	 KEYWORD_STRING,"/" CONFIG_FILE "/Game: QWS/dir" },

  { NULL, 	0, 		NULL }
};


static void unexpected_char_error (char c) {
  fprintf (stderr, "Unexpected character: ");
  fprintf (stderr, (isprint (c))? "\'%c\'" : "\\%03o", c);
  fprintf (stderr, " in file %s[line:%d,pos:%d]\n", rcfilename, line, pos);
}


static void unexpected_eof_error (void) {
  fprintf (stderr, "Unexpected end of line in file %s[line:%d,pos:%d]\n", 
           rcfilename, line, pos);
}


static void syntax_error (void) {
  fprintf (stderr, "Syntax error in file %s[line:%d,pos:%d]\n", 
           rcfilename, line, pos);
}


static void unexpected_token_error (int need_token) {
  fprintf (stderr, "Syntax error in file %s[line:%d,pos:%d]\n",
                                                   rcfilename, line, pos);
  fprintf (stderr, "Skipping to the end of the line...\n");
}


static inline int rc_getc (FILE *rc) {
  pos++;
  return getc (rc);
}


static inline int rc_ungetc (char c, FILE *rc) {
  pos--;
  return ungetc (c, rc);
}


static int rc_open (char *filename) {
  rc = fopen (filename, "r");
  if (rc == NULL)
    return -1;

  token_str = g_malloc (BUFFER_SIZE);
  tptr = token_str;
  state = STATE_SPACE;
  token_int = 0;
  line = pos = 1;
  rcfilename = g_strdup (filename);
  return 0;
}


static void rc_close (void) {
  if (rc) {
    fclose (rc);
    rc = NULL;
  }

  if (token_str) {
    g_free (token_str);
    token_str = NULL;
  }

  if (rcfilename) {
    g_free (rcfilename);
    rcfilename = NULL;
  }
}


static int rc_next_token (void) {
  int c;
  int num, i;
  int sign = 1;

  if (!rc)
    return TOKEN_EOF;

  if ((c = rc_getc (rc)) == EOF) {
    rc_close ();
    return TOKEN_EOF;
  }

  tptr = token_str;

  while (1) {

    switch (state) {

    case STATE_SPACE:
      if (c == ' ' || c == '\r' || c == '\t')
	break;

      if (isdigit (c)) {
	sign = 1;
	token_int = c - '0';
	state = STATE_INT;
	break;
      }

      if (isalnum (c) || c == '_') {
	*tptr++ = c;
	state = STATE_TOKEN;
	break;
      }

      switch (c) {

      case '\n':
	line++;
	pos = 1;
	return TOKEN_EOL;
	break;

      case ',':
	return TOKEN_COMMA;
	break;

      case '\"':
	state = STATE_STRING;
	break;

      case '-':
	sign = -1;
	token_int = 0;
	state = STATE_INT;
	break;
	
      case '#':
	state = STATE_COMMENT;
	break;

      default:
	unexpected_char_error (c);
	break;

      }	/* switch */
      
      break;

    case STATE_COMMENT:
      if (c == '\n') {
	line++;
	pos = 1;
	state = STATE_SPACE;
	return TOKEN_EOL;
      }
      break;

    case STATE_INT:
      if (!isdigit (c)) {
	rc_ungetc (c, rc);
	token_int = token_int * sign;
	state = STATE_SPACE;
	return TOKEN_INT;
      }

      token_int = token_int*10 + c - '0';
      break;

    case STATE_TOKEN:
      if (!isalnum (c) && c != '_') {
	rc_ungetc (c, rc);
	*tptr = '\0';
	state = STATE_SPACE;
	return TOKEN_KEYWORD;
      }

      *tptr++ = c;
      break;
      
    case STATE_STRING:
      if (c == '\"') {
	*tptr = '\0';
	state = STATE_SPACE;
	return TOKEN_STRING;
      }

      if (c == '\n') {
	unexpected_eof_error ();
	rc_ungetc (c, rc);
	*tptr = '\0';
	state = STATE_SPACE;
	return TOKEN_STRING;
      }

      if (c == '\\') {
	if ((c = rc_getc (rc)) == EOF) {
	  *tptr = '\0';
	  rc_close ();
	  return TOKEN_STRING;
	}

	if (c >= '0' && c <='7') {
	  for (i=0, num=0; i < 3; i++) {
	    num <<= 3;
	    num |= c - '0';
	    c = rc_getc (rc);
	    if (c < '0' || c > '7')
	      break;
	  }

	  *tptr++ = num;

	  if (c == EOF) {
	    *tptr = '\0';
	    rc_close ();
	    return TOKEN_STRING;
	  }
	  
	  continue;
	} /* if (c >= '0' && c <='7') */

	switch (c) {

	case 'n':
	  *tptr++ = '\n';
	  break;

	case 'r':
	  *tptr++ = '\r';
	  break;

	case 't':
	  *tptr++ = '\t';
	  break;

	default:
	  *tptr++ = c;
	  break;

	} /* switch (c) */
	break;
	
      } /* if (c == '\\') */

      *tptr++ = c;
      break;

    } /* switch (state) */

    c = rc_getc (rc);

  } /* while (1) */

}


static void rc_skip_to_eol (void) {
  while (token != TOKEN_EOL && token != TOKEN_EOF) {
    token = rc_next_token ();
  }
}


static int rc_expect_token (int need_token) {
  if ((token = rc_next_token ()) == need_token)
    return TRUE;

  unexpected_token_error (need_token);
  rc_skip_to_eol ();
  return FALSE;
}


int rc_parse (void) {
  char *fn;
  struct keyword *kw;

  fn = file_in_dir (user_rcdir, RC_FILE);
  rc_open (fn);
  g_free (fn);

  if(!rc)
    return -1;

  while ((token = rc_next_token ()) != TOKEN_EOF) {

    switch (token) {

    case TOKEN_KEYWORD:
      for (kw = keywords; kw->name; kw++) {
	if (strcmp (token_str, kw->name) == 0) {

	  switch (kw->required) {
	  case KEYWORD_INT:
	    if (rc_expect_token (TOKEN_INT))
	      config_set_int (kw->config, token_int);
	    break;

	  case KEYWORD_BOOL:
	    if (rc_expect_token (TOKEN_INT))
	      config_set_bool (kw->config, token_int);
	    break;

	  case KEYWORD_STRING:
	    if (rc_expect_token (TOKEN_STRING))
	      config_set_string (kw->config, token_str);
	    break;
	  }

	  break;
	}
      }
      break;

    case TOKEN_EOL:
    case TOKEN_EOF:
      break;

    default:
      syntax_error ();
      rc_skip_to_eol ();
      break;

    } /* switch */
  } /* while */

  rc_close ();

  /* Compatibility with old versions */

  if (!games[Q1_SERVER].dir && games[QW_SERVER].dir) {
    games[Q1_SERVER].dir = g_strdup (games[QW_SERVER].dir);
    config_set_string ("/" CONFIG_FILE "/Game: QS/dir", games[Q1_SERVER].dir);
  }

  if (default_w_switch < 0) default_w_switch = 0;
  if (default_b_switch < 0) default_b_switch = 0;

  return 0;
}


int rc_save (void) {
  char *fn;

  fn = file_in_dir (user_rcdir, RC_FILE);
  unlink (fn);
  g_free (fn);
  return 0;
}


int rc_check_dir (void) {
  struct stat st_buf;

  if (stat (user_rcdir, &st_buf) == -1) {
    return mkdir (user_rcdir, 0755);
  }
  else {
    if (!S_ISDIR (st_buf.st_mode)) {
      fprintf (stderr, "%s is not a directory\n", user_rcdir);
      return -1;
    }
  }

  return 0;
}
