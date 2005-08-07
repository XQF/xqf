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

#include <sys/types.h>
#include <stdio.h>	/* FILE, fprintf, fopen, fclose */
#include <string.h>	/* strlen, strcpy, strcmp, strtok */
#include <stdlib.h>	/* strtol */
#include <unistd.h>	/* stat */
#include <sys/stat.h>	/* stat, chmod */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "pref.h"
#include "launch.h"
#include "dialogs.h"
#include "utils.h"
#include "pixmaps.h"
#include "game.h"
#include "stat.h"
#include "server.h"
#include "statistics.h"
#include "server.h"
#include "config.h"
#include "debug.h"
#include "q3maps.h"
#include "utmaps.h"

/* in pref.c */
extern void q1_cmd_or_dir_changed(struct game* g);
extern void qw_cmd_or_dir_changed(struct game* g);
extern void q2_cmd_or_dir_changed(struct game* g);
extern void ut2004_cmd_or_dir_changed(struct game* g);
extern void doom3_cmd_or_dir_changed(struct game* g);

static struct player *poqs_parse_player(char *tokens[], int num, struct server *s);
static struct player *qw_parse_player(char *tokens[], int num, struct server *s);
static struct player *q2_parse_player(char *tokens[], int num, struct server *s);
static struct player *q3_parse_player(char *tokens[], int num, struct server *s);
static struct player *t2_parse_player(char *tokens[], int num, struct server *s);
static struct player *hl_parse_player(char *tokens[], int num, struct server *s);
static struct player *un_parse_player(char *tokens[], int num, struct server *s);
static struct player *descent3_parse_player(char *tokens[], int num, struct server *s);
static struct player *savage_parse_player (char *token[], int n, struct server *s);

static void quake_parse_server (char *tokens[], int num, struct server *s);

static void qw_analyze_serverinfo (struct server *s);
static void q2_analyze_serverinfo (struct server *s);
static void hl_analyze_serverinfo (struct server *s);
static void t2_analyze_serverinfo (struct server *s);
static void q3_analyze_serverinfo (struct server *s);
static void doom3_analyze_serverinfo (struct server *s);
static void un_analyze_serverinfo (struct server *s);
static void bf1942_analyze_serverinfo (struct server *s);
static void descent3_analyze_serverinfo (struct server *s);
static void savage_analyze_serverinfo (struct server *s);

static int quake_config_is_valid (struct server *s);
static int quake3_config_is_valid (struct server *s);
static int config_is_valid_generic (struct server *s);

static int write_q1_vars (const struct condef *con);
static int write_qw_vars (const struct condef *con);
static int write_q2_vars (const struct condef *con);

static int q1_exec_generic (const struct condef *con, int forkit);
static int qw_exec (const struct condef *con, int forkit);
static int q2_exec (const struct condef *con, int forkit);
static int q2_exec_generic (const struct condef *con, int forkit);
static int q3_exec (const struct condef *con, int forkit); // needs quake_private
static int hl_exec(const struct condef *con, int forkit);
static int ut_exec (const struct condef *con, int forkit);
static int t2_exec (const struct condef *con, int forkit);
static int gamespy_exec (const struct condef *con, int forkit);
static int bf1942_exec (const struct condef *con, int forkit);
static int exec_generic (const struct condef *con, int forkit);
static int ssam_exec (const struct condef *con, int forkit);
static int savage_exec (const struct condef *con, int forkit);
static int netpanzer_exec (const struct condef *con, int forkit);
static int descent3_exec (const struct condef *con, int forkit);

/*
static GList *q1_custom_cfgs (struct game* this, char *dir, char *game);
static GList *qw_custom_cfgs (struct game* this, char *dir, char *game);
static GList *q2_custom_cfgs (struct game* this, char *dir, char *game);
static GList *q3_custom_cfgs (struct game* this, char *dir, char *game);
*/

static GList *quake_custom_cfgs (struct game* this, const char *path, const char *mod);

static void quake_save_info (FILE *f, struct server *s);

char **get_custom_arguments(enum server_type type, const char *gamestring);

static void quake_init_maps(enum server_type);
static gboolean quake_has_map(struct server* s);

static void q3_init_maps(enum server_type);
static size_t q3_get_mapshot(struct server* s, guchar** buf);

static void doom3_init_maps(enum server_type);
static size_t doom3_get_mapshot(struct server* s, guchar** buf);
static gboolean doom3_has_map(struct server* s);

static void unreal_init_maps(enum server_type);
static gboolean unreal_has_map(struct server* s);

struct quake_private
{
  GHashTable* maphash;
};

struct unreal_private
{
  GHashTable* maphash;
  const char* suffix;
};

static struct unreal_private ut_private = { NULL, ".unr" };
static struct unreal_private ut2_private = { NULL, ".ut2" };
static struct unreal_private ut2004_private = { NULL, ".ut2" };
static struct unreal_private rune_private = { NULL, ".run" };
static struct unreal_private postal2_private = { NULL, ".fuk" };
static struct unreal_private aao_private = { NULL, ".aao" };

static struct quake_private q1_private, qw_private, q2_private, hl_private;
static struct quake_private q3_private;
static struct quake_private wolf_private;
static struct quake_private wolfet_private;
static struct quake_private mohaa_private;
static struct quake_private cod_private;
static struct quake_private jk3_private;
static struct quake_private doom3_private;

#include "games.c"

struct gsname2type_s
{
	char* name;
	enum server_type type;
};

// gamespy names, used to determine the server type
static struct gsname2type_s gsname2type[] =
{
	{ "ut", UN_SERVER },
	{ "ut2", UT2_SERVER },
	{ "ut2d", UT2_SERVER },
	{ "rune", RUNE_SERVER },
	{ "serioussam", SSAM_SERVER },
	{ "serioussamse", SSAMSE_SERVER },
	{ "postal2", POSTAL2_SERVER },
	{ "postal2d", POSTAL2_SERVER },
	{ "armygame", AAO_SERVER },
	{ "bfield1942", BF1942_SERVER },
	{ NULL, UNKNOWN_SERVER }
};

void init_games()
{
  unsigned i;

  debug(3,"initializing games");

  for (i = 0; i < GAMES_TOTAL; i++)
  {
    g_datalist_init(&games[i].games_data);
  }

  game_set_attribute(SFS_SERVER,"game_notes",strdup(_
   				   ("Note:  Soldier of Fortune will not connect to a server correctly\n"\
    				    "without creating a startup script for the game.  Please see the\n"\
			  	    "XQF documentation for more information."))); 
  game_set_attribute(UN_SERVER,"game_notes",strdup(_
  				   ("Note:  Unreal Tournament will not launch correctly without\n"\
    				    "modifications to the game's startup script.  Please see the\n"\
			  	    "XQF documentation for more information.")));
  game_set_attribute(HL_SERVER,"game_notes",strdup(_
  				   ("Sample Command Line:  wine hl.exe -- hl.exe -console")));

  game_set_attribute(SAS_SERVER,"game_notes",strdup(_
  				   ("Note:  Savage will not launch correctly without\n"\
    				    "modifications to the game's startup script. Please see the\n"\
			  	    "XQF documentation for more information.")));
}

void games_done()
{
  int i;

  for (i = 0; i < GAMES_TOTAL; i++)
  {
    g_free(games[i].real_home);
  }
}

// retreive game specific value that belongs to key, do not free return value!
const char* game_get_attribute(enum server_type type, const char* attr)
{
  return g_datalist_get_data(&games[type].games_data,attr);
}

// set game specific key/value pair, value is _not_ copied and must not be
// freed manually
const char* game_set_attribute(enum server_type type, const char* attr, char* value)
{
  g_datalist_set_data_full(&games[type].games_data,attr,value,g_free);
  return value;
}

enum server_type id2type (const char *id) {
  int i;

  g_return_val_if_fail(id != NULL, UNKNOWN_SERVER);

  for (i = 0; i < GAMES_TOTAL; i++) {
    g_return_val_if_fail(games[i].id != NULL, UNKNOWN_SERVER);
    if (g_strcasecmp (id, games[i].id) == 0)
      return games[i].type;
  }

  for (i = 0; i < GAMES_TOTAL; i++) {
    if (g_strcasecmp (id, games[i].qstat_str) == 0)
      return games[i].type;
  }
  
  // workaround for qstat beta
  if (g_strcasecmp (id, "RWS" ) == 0)
  {
    return WO_SERVER;
  }

  if (g_strcasecmp (id, "QW") == 0)
    return QW_SERVER;
  if (g_strcasecmp (id, "Q2") == 0)
    return Q2_SERVER;

  return UNKNOWN_SERVER;
}


/*
 *  This function should be used only for configuration saving to make
 *  possible qstat2.2<->qstat2.3 migration w/o loss of some game-specific 
 *  preferences.
 */

const char *type2id (enum server_type type) {
  switch (type) {

  default:
    return games[type].id;
  }
}


GtkWidget *game_pixmap_with_label (enum server_type type) {
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *pixmap;

  hbox = gtk_hbox_new (FALSE, 4);

  if (games[type].pix) {
    pixmap = gtk_pixmap_new (games[type].pix->pix, games[type].pix->mask);
    gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
    gtk_widget_show (pixmap);
  }

  label = gtk_label_new (_(games[type].name));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  return hbox;
}


static void q3_unescape (char *dst, char *src) {

  while (*src) {
    if (src[0] != '^' || src[1] == '\0' || src[1] < '0' || src[1] > '9')
      *dst++ = *src++;
    else
      src += 2;
  }
  *dst = '\0';
}


static const char delim[] = " \t\n\r";


static struct player *poqs_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;
  long tmp;

  if (n < 7)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[1]) + 1);
  player->time  = strtol (token[4], NULL, 10);
  player->frags = strtosh (token[3]);
  player->ping = -1;

  tmp = strtol (token[5], NULL, 10);
  player->shirt = fix_qw_player_color (tmp);

  tmp = strtol (token[6], NULL, 10);
  player->pants = fix_qw_player_color (tmp);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[1]);

  return player;
}


static struct player *qw_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;
  char *ptr;
  long tmp;
  int name_strlen;
  int skin_strlen;

  if (n < 8)
    return NULL;

  name_strlen = strlen (token[1]) + 1;
  skin_strlen = strlen (token[7]) + 1;

  player = g_malloc0 (sizeof (struct player) + name_strlen + skin_strlen);

  player->time  = strtol (token[3], NULL, 10);
  player->frags = strtosh (token[2]);
  player->ping  = strtosh (token[6]);

  tmp = strtol (token[4], NULL, 10);
  player->shirt = fix_qw_player_color (tmp);

  tmp = strtol (token[5], NULL, 10);
  player->pants = fix_qw_player_color (tmp);

  ptr = (char *) player + sizeof (struct player);
  player->name = strcpy (ptr, token[1]);

  player->skin = strcpy (ptr + name_strlen, token[7]);

  return player;
}

// Parse Tribes2 player info, abuse player->model to show when a Player is only
// a Teamname or a Bot
// player name, frags, team number, team name, player type, tribe tag
static struct player *t2_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;
  char* name=token[0];
  char* frags=token[1];
//  char* team_number=token[2];
  char* team_name=token[3];
  char* player_type=token[4];
//  char* tribe_tag=token[5];

  if (n < 6)
    return NULL;

  // show TEAM in model column
  if (!strcmp(team_name,"TEAM"))
    player_type=team_name;

  player = g_malloc0 (sizeof (struct player) + strlen(name)+1 + strlen(player_type)+1 );
  player->time  = -1;
  player->frags = strtosh (frags);
  player->ping  = -1;

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, name);

  player->model = (char *) player + sizeof (struct player) + strlen(name)+1;
  strcpy (player->model, player_type);
  if( player_type[0] == 'B' ) ++s->curbots;

  return player;
}

static struct player *q2_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[2]);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);

  return player;
}

// Descent 3: player name, frags, deaths, ping time, team
static struct player *descent3_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 5)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[3]);

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);
  if(token[0][0] == '-' && token[0][strlen(token[0])-1] == '-')
    ++s->curbots;

  return player;
}

static struct player *q3_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->time  = -1;
  player->frags = strtosh (token[1]);
  player->ping  = strtosh (token[2]);
   /* FIXME if ( dedicated == 0 ) s->curbots-- */
  if ( player->ping == 0 ) ++s->curbots;

  player->name = (char *) player + sizeof (struct player);
  q3_unescape (player->name, token[0]);

  return player;
}


static struct player *hl_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;

  if (n < 3)
    return NULL;

  player = g_malloc0 (sizeof (struct player) + strlen (token[0]) + 1);
  player->frags = strtosh (token[1]);
  player->time  = strtosh (token[2]);
  player->ping  = -1;

  player->name = (char *) player + sizeof (struct player);
  strcpy (player->name, token[0]);

  return player;
}


static struct player *un_parse_player (char *token[], int n, struct server *s) {
  struct player *player = NULL;
  char *ptr;
  int name_strlen;
  int skin_strlen;
  int mesh_strlen;
  long tmp;

  /* player name, frags, ping time, team number, skin, mesh, face */

  if (n < 6)
    return NULL;

  name_strlen = strlen (token[0]) + 1;
  skin_strlen = strlen (token[4]) + 1;
  mesh_strlen = strlen (token[5]) + 1;

  player = g_malloc0 (sizeof (struct player) + name_strlen + 
                                                   skin_strlen + mesh_strlen);

  ptr = (char *) player + sizeof (struct player);
  player->name = strcpy (ptr, token[0]);
  ptr += name_strlen; 

  player->frags = strtosh (token[1]);
  player->ping = strtosh (token[2]);
  player->time  = -1;

  /* store team number to 'pants' */

  tmp = strtol (token[3], NULL, 10);
  player->pants = (tmp >= 0 && tmp <= 255)? tmp : 0;

  player->skin = strcpy (ptr, token[4]);
  ptr += skin_strlen;

  player->model = strcpy (ptr, token[5]);

  return player;
}

static struct player *savage_parse_player (char *token[], int n, struct server *s)
{
    return q2_parse_player(token, n, s);
}

static void quake_parse_server (char *token[], int n, struct server *server) {
  /*
    This does both Quake (?) and Unreal servers
  */
  int poqs;
  int offs;

  /* debug (6, "quake_parse_server: Parse %s", server->name); */
  poqs = (server->type == Q1_SERVER || server->type == H2_SERVER);

  if (poqs && n < 10)
      return;
  else if(n < 8)
      return;

  if (*(token[2])) {		/* if name is not empty */
    if (server->type != Q3_SERVER) {
      server->name = g_strdup (token[2]);
    }
    else {
      server->name = g_malloc (strlen (token[2]) + 1);
      q3_unescape (server->name, token[2]);
    }
  }

  offs = (poqs)? 5 : 3;

  if (*(token[offs]))            /* if map is not empty */
    server->map  = g_strdup (token[offs]);
    if(server->map) g_strdown(server->map);

    server->maxplayers = strtoush (token[offs + 1]);
    server->curplayers = strtoush (token[offs + 2]);

  server->ping = strtosh (token[offs + 3]);
  server->retries = strtosh (token[offs + 4]);
}


static void qw_analyze_serverinfo (struct server *server) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  server->flags = 0;
  
  /* debug( 6, "qw_analyze_serverinfo: Analyze %s", server->name ); */
  if ((games[server->type].flags & GAME_SPECTATE) != 0)
    server->flags |= SERVER_SPECTATE;

  for (info_ptr = server->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "*gamedir") == 0) {
      server->game = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "*cheats") == 0) {
      server->flags |= SERVER_CHEATS;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        server->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	server->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	server->flags |= SERVER_SP_PASSWORD;
    }
  }
}

static void un_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  unsigned short hostport=0;

  enum server_type oldtype = s->type;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "gamestyle") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "hostport") == 0) {
      hostport = atoi(info_ptr[1]);
    }
    else if (strcmp (*info_ptr, "Mutator") == 0 && strstr(info_ptr[1], "AntiTCC")) {
      s->flags |= SERVER_PUNKBUSTER;
    }
    else if ((s->type == UT2004_SERVER || s->type == UT2_SERVER)
	&& strcmp (*info_ptr, "ServerVersion") == 0
	&& strlen(info_ptr[1]) == 4)
    {
	if(info_ptr[1][0] == '3')
	  s->type = UT2004_SERVER;
	else if(info_ptr[1][0] == '2')
	  s->type = UT2_SERVER;
    }
    else if (strcmp (*info_ptr, "gamename") == 0) {
      unsigned i;
      for( i = 0 ; gsname2type[i].name ; ++i )
      {
	if(!strcmp(info_ptr[1],gsname2type[i].name))
	{
	  s->type = gsname2type[i].type;
	  break;
	}
      }
    }

    //password required?
    // If not password=False or password=0, set SERVER_PASSWORD
    else if ((g_strcasecmp (*info_ptr, "password") == 0 || g_strcasecmp (*info_ptr, "gamepassword") == 0 ) && 
	( g_strcasecmp(info_ptr[1],"false") && strcmp(info_ptr[1],"0") ) )
    {
      s->flags |= SERVER_PASSWORD;
      if (games[s->type].flags & GAME_SPECTATE)
	s->flags |= SERVER_SP_PASSWORD;
    }
  }

  // adjust port for unreal and rune
  if(s->type != oldtype)
  {
    switch(s->type)
    {
      case UN_SERVER:
      case UT2_SERVER:
      case RUNE_SERVER:
      case POSTAL2_SERVER:
//      case AAO_SERVER: // doesn't work on lan
	server_change_port(s,hostport);
	break;
      default:
	break;
    }
  }
}

static void bf1942_analyze_serverinfo (struct server *s)
{
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2)
  {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "Game Id") == 0) {
      s->game = info_ptr[1];
    }
    //password required?
    // If not password=False or password=0, set SERVER_PASSWORD
    else if (strcmp (*info_ptr, "_password") == 0
	&& strcmp(info_ptr[1], "0"))
    {
      s->flags |= SERVER_PASSWORD;
      if (games[s->type].flags & GAME_SPECTATE)
	s->flags |= SERVER_SP_PASSWORD;
    }
  }
}


static void savage_analyze_serverinfo (struct server *s)
{
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
  }
}

static void descent3_analyze_serverinfo (struct server *s) {
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gametype") == 0) {
      s->game = info_ptr[1];
    }
/*
    //password required?
    else if (strcmp (*info_ptr, "password") == 0 && strcmp(info_ptr[1],"False")) {
      s->flags |= SERVER_PASSWORD;
    }
*/
  }
}

static void q2_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  s->flags = 0;
  
  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gamedir") == 0) {
      s->game = info_ptr[1];
    }
    //determine mod
    else if (strcmp (*info_ptr, "gamename") == 0) {
	    switch (s->type)
	    {
		    case Q2_SERVER:
			/* We only set the mod if name is not baseq2 */
		    	if (strcmp(info_ptr[1],"baseq2"))
				s->gametype = info_ptr[1];
			break;
		    default:
				s->gametype = info_ptr[1];
			break;
	    }
    }

    else if (strcmp (*info_ptr, "cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }
    else if (s->type == Q2_SERVER && strcmp (*info_ptr, "protocol") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n < 34)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }

  }
  /* We only set the mod if gamedir is set */
  if (!s->game)
  {
	  s->gametype=NULL;
  }
}

static void t2_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "game") == 0) {
      if (strcmp(info_ptr[1],"base")) // If it's not 'base'
        s->game = info_ptr[1];
    }

    //determine GameType column
    else if (strcmp (*info_ptr, "mission") == 0) {
      s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }
    else if (strcmp (*info_ptr, "maxspectators") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if (n <= 0)
        s->flags &= ~SERVER_SPECTATE;
    }
    else if (strcmp (*info_ptr, "password") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }

  }
  // unset game if game is base
//  if (!strcmp(s->game,"base"))
//  {
//  strcpy(s->gametype,"Alex");
//  }
}

static void hl_analyze_serverinfo (struct server *s) {
  char **info_ptr;

  /* Clear out the flags */
  s->flags = 0;
  
  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "gamedir") == 0) {
      s->game = info_ptr[1];
    }
    //determine mod
    else if (strcmp (*info_ptr, "gamename") == 0) {
        s->gametype = info_ptr[1];
    }
    
    //cheats enabled?
    else if (strcmp (*info_ptr, "sv_cheats") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_CHEATS;
    }

    //password required?
    else if (strcmp (*info_ptr, "sv_password") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_PASSWORD;
    }

    //cheating death
    else if (strcmp (*info_ptr, "cdrequired") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_PUNKBUSTER;
    }

    //VAC
    else if (strcmp (*info_ptr, "secure") == 0 && info_ptr[1][0] != '0') {
      s->flags |= SERVER_PUNKBUSTER;
    }

    // reserved slots
    else if (strcmp (*info_ptr, "reserve_slots") == 0) {
      s->private_client = strtol (info_ptr[1], NULL, 10);
    }
  }

  // unset Mod if gamedir is valve
  if (s->game && !strcmp(s->game,"valve"))
  {
	  s->gametype=NULL;
  }
}


// TODO: read this stuff from a config file
#define MAX_Q3A_TYPES 9
static char *q3a_gametypes[MAX_Q3A_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  NULL,  		/* 2 = Single Player */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "1FCTF",		/* 5 = One Flag Capture the Flag */
  "OVR",		/* 6 = Overload */
  "HRV",		/* 7 = Harvester */
  " mod ",              /* 8+ is usually a client-side mod */
};

#define MAX_Q3A_OSP_TYPES 7
static char *q3a_osp_gametypes[MAX_Q3A_OSP_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  "FFA Comp",  		/* 2 = FFA, Competition */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "Clan Arena",		/* 5 = Clan Arena */
  "Custom OSP",         /* 6+ is usually a custom OSP setting */
};


#define MAX_Q3A_UT2_TYPES 9
static char *q3a_ut2_gametypes[MAX_Q3A_UT2_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "FFA",		/* 1 = Free for All */
  "FFA",		/* 2 = Free for All */
  "TDM",		/* 3 = Team Deathmatch */
  "Team Survivor",	/* 4 = Team Survivor */
  "Follow the Leader",	/* 5 = Follow the Leader */
  "Capture & Hold",     /* 6 = Capture & Hold */
  "CTF",		/* 7 = Capture the Flag */
  NULL,			/* 8+ ?? */
};

#define MAX_Q3A_UT3_TYPES 9
static char *q3a_ut3_gametypes[MAX_Q3A_UT3_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "FFA",		/* 1 = Free for All */
  "FFA",		/* 2 = Free for All */
  "TDM",		/* 3 = Team Deathmatch */
  "Team Survivor",	/* 4 = Team Survivor */
  "Follow the Leader",	/* 5 = Follow the Leader */
  "Capture & Hold",     /* 6 = Capture & Hold */
  "CTF",		/* 7 = Capture the Flag */
  "Bomb Mode",		/* 8 = Bomb Mode */
};


#define MAX_Q3A_Q3TC045_TYPES 11
static char *q3a_q3tc045_gametypes[MAX_Q3A_Q3TC045_TYPES] = {
  "FFA",		// 0 = Free for All
  NULL,			// 1 = ?
  NULL,			// 2 = ?
  "Survivor",		// 3 = Survivor
  "TDM",		// 4 = Team Deathmatch
  "CTF",		// 5 = Capture the Flag
  "Team Survivor",	// 6 = Team Survivor
  NULL,			// 7 = ?
  "Capture & Hold",     // 8 = Capture & Hold
  "King Of The Hill",	// 9 = King of the hill
  NULL,			// 10+ ??
};

#define MAX_Q3A_TRUECOMBAT_TYPES 8
static char *q3a_truecombat_gametypes[MAX_Q3A_TRUECOMBAT_TYPES] = {
  "FFA",		// 0 = Free for All
  "Survivor",		// 1 = Survivor/Last Man Standing
  NULL,			// 2 = ?
  "TDM",		// 3 = Team Deathmatch
  "Reverse CTF",	// 4 = Reverse CTF
  "CTF",		// 5 = Capture the Flag
  "Team Survivor",	// 6 = Team Survivor
  "Mission",		// 7 = Mission
};

#define MAX_Q3A_THREEWAVE_TYPES 12
static char *q3a_threewave_gametypes[MAX_Q3A_THREEWAVE_TYPES] = {
  "FFA",		// 0  - Free For All
  "1v1",		// 1  - Tournament
  NULL,			// 2  - Single Player (invalid, don't use this)
  "TDM",		// 3  - Team Deathmatch
  "ThreeWave CTF",	// 4  - ThreeWave CTF
  "One flag CTF",	// 5  - One flag CTF  (invalid, don't use this)
  "Obelisk",		// 6  - Obelisk       (invalid, don't use this)
  "Harvester",		// 7  - Harvester     (invalid, don't use this)
  "Portal", 	        // 8  - Portal        (invalid, don't use this)
  "CaptureStrike",	// 9  - CaptureStrike 
  "Classic CTF", 	// 10 - Classic CTF   
  NULL			// 11+ ???
};

#define MAX_Q3A_TRIBALCTF_TYPES 10
static char *q3a_tribalctf_gametypes[MAX_Q3A_TRIBALCTF_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  NULL,			// 2 - Unknown
  NULL,			// 3 - Unknown
  NULL,			// 4 - Unknown
  NULL,			// 5 - Unknown
  "Freestyle",		// 6 - Freestyle
  "Fixed",		// 7 - Fixed
  "Roulette",		// 8 - Roulette
  NULL			// 9+ ???
};

#define MAX_Q3A_SEALS_TYPES 5
static char *q3a_seals_gametypes[MAX_Q3A_SEALS_TYPES] = {
  NULL,			/* 0 = devmode */
  NULL,			/* 1 = invalid */
  NULL,			/* 2 = invalid */
  "Operations",		/* 3 = Team Deathmatch */
  NULL			/* 4+ invalid */
};

#define MAX_Q3A_AFTERWARDS_TYPES 3
static char *q3a_afterwards_gametypes[MAX_Q3A_AFTERWARDS_TYPES] = {
  "Tactical",		// 0 = Tactical
  "FFA",		// 1 = Deatchmatch
  NULL,			// 2+ ??
};

#define MAX_Q3A_ARENA_TYPES 9
// Not sure what the proper types are, but 99% of them are a game
// type of 8.  Just call them all "arena"
static char *q3a_arena_gametypes[MAX_Q3A_ARENA_TYPES] = {
  "arena",		/* 0 = Arena */
  "arena",		/* 1 = Arena */
  "arena",		/* 2 = Arena */
  "arena",		/* 3 = Arena */
  "arena",		/* 4 = Arena */
  "arena",		/* 5 = Arena */
  "arena",		/* 6 = Arena */
  "arena",		/* 7 = Arena */
  "arena",		/* 8 = Arena */
};

#define MAX_Q3A_CPMA_TYPES 6
static char *q3a_cpma_gametypes[MAX_Q3A_CPMA_TYPES] = {
  "FFA",		/* 0 = Free for All */
  "1v1",	 	/* 1 = Tournament */
  NULL,  		/* 2 = Single Player */
  "TDM",		/* 3 = Team Deathmatch */
  "CTF",		/* 4 = Capture the Flag */
  "Clan Arena",		/* 5 = Clan Arena */
};

#define MAX_Q3A_Q3F_TYPES 6
static char *q3a_q3f_gametypes[MAX_Q3A_Q3F_TYPES] = {
  "q3f",		/* 0 = Arena */
  "q3f",		/* 1 = Arena */
  "q3f",		/* 2 = Arena */
  "q3f",		/* 3 = Arena */
  "q3f",		/* 4 = Arena */
  "q3f",		/* 5 = Arena */
};

#define MAX_Q3A_WQ3_TYPES 6
static char *q3a_wq3_gametypes[MAX_Q3A_WQ3_TYPES] = {
  "FFA",
  "Duel",
  NULL,
  "TDM",
  "Round Teamplay",
  "Bank Robbery"
};

#define MAX_Q3A_WOP_TYPES 9
static char *q3a_wop_gametypes[MAX_Q3A_WOP_TYPES] = {
  NULL,
  NULL,
  NULL,
  "Spray your Color FFA (SyC)", // 3
  "LastPadStanding (LPS)", // 4
  NULL,
  NULL,
  "Spray your Color Teamgame (SyC)", // 7
  "BigBalloon (BB)" // 8
};

#define MAX_Q3A_EXCESSIVEPLUS_TYPES 10
static char *q3a_excessiveplus_gametypes[MAX_Q3A_EXCESSIVEPLUS_TYPES] = {
  "FFA",               /* 0 = Free for All */
  "1v1",               /* 1 = Tournament */
  NULL,                /* 2 = Single Player */
  "TDM",               /* 3 = Team Deathmatch */
  "CTF",               /* 4 = Capture the Flag */
  "RTF",               /* 5 = Return the Flag */
  "One Flag CTF",      /* 6 = One Flag Capture the Flag */
  "Clan Arena",        /* 7 = Clan Arena */
  "Freeze Tag",        /* 8 = Freeze Tag */
  "Protect the Leader" /* 9 = Protect the Leader */
};

#define MAX_WOLF_TYPES 9
static char *wolf_gametypes[MAX_WOLF_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  NULL,			// 2 - Unknown
  NULL,			// 3 - Unknown
  NULL,			// 4 - Unknown
  "WolfMP",		// 5 - standard objective mode
  "WolfSW",		// 6 - Stopwatch mode
  "WolfCP",		// 7 - Checkpoint mode
  NULL			// 8+ ???
};

#define MAX_WOLFET_TYPES 7
static char *wolfet_gametypes[MAX_WOLFET_TYPES] = {
  NULL,			// 0 - Unknown
  NULL,			// 1 - Unknown
  "Obj",		// 2 - Single-Map Objective
  "SW",			// 3 - Stopwatch
  "Cmpgn",		// 4 - Campaign
  "LMS",		// 5 - Last Man Standing
  NULL			// 6+ ???
};

#define MAX_JK3_TYPES 9
static char *jk3_gametypes[MAX_JK3_TYPES] = {
  "FFA",		// 0 - Free For All
  NULL,			// 1 - Unknown
  NULL,			// 2 - Unknown
  "Duell",		// 3 - Duell
  "PDuell",		// 4 - Power Duell
  NULL,			// 5 - Unknown
  "TFFA",		// 6 - Team Free For All
  "Siege",		// 7 - Siege
  "CTF"			// 8 - Capture the Flag
};

struct q3a_gametype_s {
  char* mod;
  char** gametypes;
  int number;
};

struct q3a_gametype_s q3a_gametype_map[] =
{
  {
    "baseq3",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "osp",
    q3a_osp_gametypes,
    MAX_Q3A_OSP_TYPES
  },
  {
    "q3ut2",
    q3a_ut2_gametypes,
    MAX_Q3A_UT2_TYPES
  },
  {
    "q3ut3",
    q3a_ut3_gametypes,
    MAX_Q3A_UT3_TYPES
  },
  {
    "threewave",
    q3a_threewave_gametypes,
    MAX_Q3A_THREEWAVE_TYPES
  },
  {
    "seals",
    q3a_seals_gametypes,
    MAX_Q3A_SEALS_TYPES
  },
  {
    "TribalCTF",
    q3a_tribalctf_gametypes,
    MAX_Q3A_TRIBALCTF_TYPES
  },
  {
    "missionpack",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "generations",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "truecombat",
    q3a_truecombat_gametypes,
    MAX_Q3A_TRUECOMBAT_TYPES
  },
  {
    "q3tc045",
    q3a_q3tc045_gametypes,
    MAX_Q3A_Q3TC045_TYPES
  },
  {
    "freeze",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "afterwards",
    q3a_afterwards_gametypes,
    MAX_Q3A_AFTERWARDS_TYPES
  },
  {
    "cpma",
    q3a_cpma_gametypes,
    MAX_Q3A_CPMA_TYPES
  },
  {
    "arena",
    q3a_arena_gametypes,
    MAX_Q3A_ARENA_TYPES
  },
  {
    "instaunlagged",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "instagibplus",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "beryllium",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "excessive",
    q3a_gametypes,
    MAX_Q3A_TYPES
  },
  {
    "q3f",
    q3a_q3f_gametypes,
    MAX_Q3A_Q3F_TYPES
  },
  {
    "q3f2",
    q3a_q3f_gametypes,
    MAX_Q3A_Q3F_TYPES
  },
  {
    "westernq3",
    q3a_wq3_gametypes,
    MAX_Q3A_WQ3_TYPES
  },
  {
    "wop",
    q3a_wop_gametypes,
    MAX_Q3A_WOP_TYPES
  },
  {
    "excessiveplus",
    q3a_excessiveplus_gametypes,
    MAX_Q3A_EXCESSIVEPLUS_TYPES
  },
  {
    NULL,
    NULL,
    0
  }
};

struct q3a_gametype_s wolf_gametype_map[] =
{
  {
    "main",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "wolfmp",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "bani",
    wolf_gametypes, // exports additional gametypes via g_gametype2
    MAX_WOLF_TYPES
  },
  {
    "headshot",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "osp",
    wolf_gametypes,
    MAX_WOLF_TYPES
  },
  {
    "shrubmod",
    wolf_gametypes,
    MAX_WOLF_TYPES
  }
};

// didn't find docu about this, so use q3a types
struct q3a_gametype_s ef_gametype_map[] =
{
  {
    "baseEF",
    q3a_gametypes,
    MAX_Q3A_TYPES
  }
};

struct q3a_gametype_s wolfet_gametype_map[] =
{
  {
    "et",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "etmain",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "ettest",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "etpro",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  },
  {
    "shrubet",
    wolfet_gametypes,
    MAX_WOLFET_TYPES
  }
};

struct q3a_gametype_s jk3_gametype_map[] =
{
  {
    "basejka",
    jk3_gametypes,
    MAX_JK3_TYPES
  },
  {
    "base",
    jk3_gametypes,
    MAX_JK3_TYPES
  },
  {
    "japlus",
    jk3_gametypes,
    MAX_JK3_TYPES
  }
};

void q3_decode_gametype (struct server *s, struct q3a_gametype_s map[])
{
  char *endptr;
  int n;
  int found=0;
  struct q3a_gametype_s* ptr;

  if(!s->game) return;

  n = strtol (s->gametype, &endptr, 10);

  // strtol returns a pointer to the first invalid digit, if both pointers
  // are equal there was no number at all
  if (s->gametype == endptr)
    return;

  for( ptr=map; !found && ptr && ptr->mod != NULL; ptr++ )
  {
    if( !strcasecmp (s->game, ptr->mod)
	&& n >=0
	&& n < ptr->number
	&& ptr->gametypes[n] )
    {
      	s->gametype = ptr->gametypes[n];
	found=1;
    }
//    else
      // Exact match not found - use the first one in the list
      // which should be the game's original game types
//      s->gametype = map->gametypes[n];
  }
}

static void q3_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  long n;
  char *fs_game=NULL;
  char *game=NULL;
  char *gamename=NULL;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  // check if it's really a q3 server. We need to do that first to determine
  // whether the server protcol for that game is compatible
  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (strcmp (*info_ptr, "version" ) == 0) {
      if(!strncmp(info_ptr[1],"Q3",2))
      {
	s->type=Q3_SERVER;
      }
      else if(!strncmp(info_ptr[1],"Wolf",4))
      {
	s->type=WO_SERVER;
      }
      else if(!strncmp(info_ptr[1],"ET 2",4) || !strncmp(info_ptr[1],"ETTV ",5))
      {
	s->type=WOET_SERVER;
      }
      // voyager elite force
      else if(!strncmp(info_ptr[1],"ST:V HM",7))
      {
	s->type=EF_SERVER;
      }
      else if(!strncmp(info_ptr[1],"Medal",5))
      {
	s->type=MOHAA_SERVER;
      }
      else if(!strncmp(info_ptr[1],"SOF2MP",6))
      {
	s->type=SOF2S_SERVER;
      }

      break;
    }
  }

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
 
    /*
      fs_game sets the active directory and is how one chooses
      a mod on the command line.  This should not show up in
      the server string but some times it does.  We will
      take either fs_game, game or gamename as the "game" string.
      --baa
    */
    if (strcmp (*info_ptr, "fs_game") == 0) {
	fs_game  = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "gamename") == 0) {
      gamename  = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "game") == 0) {
      game  = info_ptr[1];
    }

    else if (strcmp (*info_ptr, "version" ) == 0) {
      if (strstr (info_ptr[1], "linux")) {
	s->sv_os = 'L';
      } else if (strstr (info_ptr[1], "win" )) {
	s->sv_os = 'W';
      } else if (strstr (info_ptr[1], "Mac" )) {
	s->sv_os = 'M';
      } else {
	s->sv_os = '?';
      }
    }
    
    else if (!s->gametype && strcmp (*info_ptr, "g_gametype") == 0) {
	s->gametype = info_ptr[1];
    }
    else if (s->type == MOHAA_SERVER &&
	strcmp (*info_ptr, "g_gametypestring") == 0) {
	s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "g_needpass") == 0) {
      n = strtol (info_ptr[1], NULL, 10);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }
    else if (strcmp (*info_ptr, "cheats") == 0) {
      s->flags |= SERVER_CHEATS;
    }
    else if (g_strcasecmp (*info_ptr, "sv_privateClients") == 0) {
      s->private_client = strtol (info_ptr[1], NULL, 10);
    }
    else if (!strcmp(*info_ptr, "sv_punkbuster") && info_ptr[1] && info_ptr[1][0] == '1') {
      s->flags |= SERVER_PUNKBUSTER;
    }
    else if (!strcmp(*info_ptr, "protocol")) {
      const char* masterprotocol = game_get_attribute(s->type,"masterprotocol");
      
      if(masterprotocol && !strcmp(masterprotocol, "auto"))
	masterprotocol = game_get_attribute(s->type, "_masterprotocol");

      if(masterprotocol && strcmp(masterprotocol, info_ptr[1]))
	s->flags |= SERVER_INCOMPATIBLE;
    }
  }

  if(fs_game)
  {
    s->game=fs_game;
  }
  else if(game)
  {
    s->game=game;
  }
  else if(gamename)
  {
    s->game=gamename;
  }

  if(s->gametype) {
    if ( s->type == Q3_SERVER)
    {
      q3_decode_gametype( s, q3a_gametype_map );
    }
    else if ( s->type == WO_SERVER)
    {
      q3_decode_gametype( s, wolf_gametype_map );
    }
    else if ( s->type == EF_SERVER)
    {
      q3_decode_gametype( s, ef_gametype_map );
    }
    else if ( s->type == WOET_SERVER)
    {
      q3_decode_gametype( s, wolfet_gametype_map );
    }
    else if ( s->type == JK3_SERVER)
    {
      q3_decode_gametype( s, jk3_gametype_map );
    }

  }
}

static void doom3_analyze_serverinfo (struct server *s) {
  char **info_ptr;
  char *fs_game=NULL;

  /* Clear out the flags */
  s->flags = 0;

  if ((games[s->type].flags & GAME_SPECTATE) != 0)
    s->flags |= SERVER_SPECTATE;

  for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2)
  {
 
    if (strcmp (*info_ptr, "fs_game") == 0) {
	fs_game  = info_ptr[1];
    }

    else if (strcmp (*info_ptr, "si_version" ) == 0) {
      if (strstr (info_ptr[1], "linux")) {
	s->sv_os = 'L';
      } else if (strstr (info_ptr[1], "win" )) {
	s->sv_os = 'W';
      } else if (strstr (info_ptr[1], "Mac" )) {
	s->sv_os = 'M';
      } else {
	s->sv_os = '?';
      }
    }
    
    else if (!s->gametype && strcmp (*info_ptr, "si_gameType") == 0) {
	s->gametype = info_ptr[1];
    }
    else if (strcmp (*info_ptr, "si_usepass") == 0) {
      int n = atoi (info_ptr[1]);
      if ((n & 1) != 0)
	s->flags |= SERVER_PASSWORD;
      if ((n & 2) != 0)
	s->flags |= SERVER_SP_PASSWORD;
    }
    else if (strcmp (*info_ptr, "osmask") == 0) {
      char* p;
      unsigned long n = strtol(info_ptr[1], &p, 0);
      if(info_ptr[1] != p && (n & 4) != 4)
      {
	s->flags |= SERVER_INCOMPATIBLE;
      }
    }
  }

  if(fs_game)
  {
    s->game=fs_game;
  }
}

static int quake_config_is_valid (struct server *s) {
  struct stat stat_buf;
  char *cfgdir;
  char *path;
  struct game *g = &games[s->type];

  if(g->main_mod && g->main_mod[0])
    cfgdir = g->main_mod[0];
  else
    return FALSE;

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    // %s = game name e.g. QuakeWorld
    dialog_ok (NULL, _("%s command line is empty."), g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      // directory name, game name
      dialog_ok (NULL, _("\"%s\" is not a directory\n"
	              "Please specify correct %s working directory."), 
                       g->real_dir, g->name);
      return FALSE;
    }
  }

  path = file_in_dir (g->real_dir, cfgdir);

  if (stat (path, &stat_buf) || !S_ISDIR (stat_buf.st_mode)) {
    if (!g->real_dir || g->real_dir[0] == '\0') {
      // game name
      dialog_ok (NULL, _("Please specify correct %s working directory."), 
                                                                     g->name);
    }
    else {
      dialog_ok (NULL,  
		// directory, subdirectory, game name
		 _("Directory \"%s\" doesn\'t contain \"%s\" subdirectory.\n"
		 "Please specify correct %s working directory."), 
		 g->real_dir, cfgdir, g->name);
    }
    g_free (path);
    return FALSE;
  }

  g_free (path);
  return TRUE;
}

/** find the quake3 directory */
static char *quake3_data_dir (struct game* this) {
  struct stat stat_buf;
  char *path = NULL;
  char *dir = NULL;
  unsigned i = 0;

  if(!this)
    return NULL;

  dir = this->real_home?this->real_home:this->real_dir;

  if (!dir)
    return NULL;

  for(i = 0; this->main_mod[i]; ++i)
  {
    path = file_in_dir (dir, this->main_mod[i]);
    if (stat (path, &stat_buf) == 0 && S_ISDIR (stat_buf.st_mode))
    {
      break;
    }
    else
    {
      g_free (path);
      path = NULL;
    }
  }
  return path;
}


static int quake3_config_is_valid (struct server *s) {
  struct game *g = &games[s->type];
  char *path;

  if(!config_is_valid_generic(s))
    return FALSE;
  
  path = quake3_data_dir (g);

  if (path == NULL) {
    if (!g->real_dir || g->real_dir[0] == '\0') {
      dialog_ok (NULL, 
		 // %s Quake3
		 _("~/.q3a directory doesn\'t exist or doesn\'t contain\n"
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please run %s client at least once before running XQF\n"
		 "or specify correct %s working directory."),
		 g->name, g->name);
    }
    else {
      dialog_ok (NULL, 
		 // %s directory, Quake3
		 _("\"%s\" directory doesn\'t exist or doesn\'t contain "
		 "\"baseq3\" (\"demoq3\") subdirectory.\n"
		 "Please specify correct %s working directory\n"
  	         "or leave it empty (~/.q3a is used by default)"), 
		 g->real_dir, g->name);
    }
    return FALSE;
  }
  
  g_free (path);
  return TRUE;
}

static int config_is_valid_generic (struct server *s) {
  struct stat stat_buf;
  struct game *g = &games[s->type];

  if (g->cmd == NULL || g->cmd[0] == '\0') {
    dialog_ok (NULL, "%s command line is empty.", g->name);
    return FALSE;
  }

  if (g->real_dir != NULL && g->real_dir[0] != '\0') {
    if (stat (g->real_dir, &stat_buf) != 0 || !S_ISDIR (stat_buf.st_mode)) {
      // %s directory, game name
      dialog_ok (NULL, _("\"%s\" is not a directory\n"
	              "Please specify correct %s working directory."), 
                       g->real_dir, g->name);
      return FALSE;
    }
  }

  return TRUE;
}

static int complain_launch_cant_write_file(const char* file)
{
  return dialog_yesno (NULL, 1, _("Launch"), _("Cancel"), 
	//%s frontend.cfg
	_("Cannot write to file \"%s\".\n\nLaunch client anyway?"), file);
}

static FILE *open_cfg (const char *filename, int private) {
  FILE *f;
  mode_t saved_umask;

  if(private)
    saved_umask = umask(0077);

  f = fopen (filename, "w");
  if (f)
    fprintf (f, "//\n// generated by XQF, do not modify\n//\n");

  if(private)
    umask(saved_umask);

  return f;
}

static int real_password (const char *password) {
  if (!password || (password[0] == '1' && password[1] == '\0'))
    return FALSE;
  return TRUE;
}

static int write_passwords (const char *filename, const struct condef *con) {
  FILE *f;

  f = open_cfg (filename, TRUE);
  if (!f) 
    return FALSE;

  if (con->password)
    fprintf (f, "password \"%s\"\n", con->password);

  if (con->spectator_password)
    fprintf (f, "spectator \"%s\"\n", con->spectator_password);

  if (con->rcon_password)
    fprintf (f, "rcon_password \"%s\"\n", con->rcon_password);

  fclose (f);
  return TRUE;
}


static int write_q1_vars (const struct condef *con) {
  FILE *f;
  int ret = TRUE;
  struct game *g = &games[con->s->type];
  char* file = NULL;

  if(g->main_mod && g->main_mod[0])
    file = g->main_mod[0];
  else
    return FALSE;

  file = g_strjoin("/", g->real_dir, file, EXEC_CFG, NULL);

  f = open_cfg (file, FALSE);
  if (!f)
    { ret = FALSE; goto out; }

  if (default_q1_name)
    fprintf (f, "name \"%s\"\n", default_q1_name);

  fprintf (f, "color %d %d\n", default_q1_top_color, default_q1_bottom_color);

  if (games[Q1_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q1_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);

out:

  if(!ret)
    ret = complain_launch_cant_write_file(file);

  g_free(file);
  return ret;
}

static int write_qw_vars (const struct condef *con) {
  FILE *f;
  int auto_pl;
  int ret = TRUE;
  struct game *g = &games[con->s->type];
  char* file = NULL;

  if(g->main_mod && g->main_mod[0])
    file = g->main_mod[0];
  else
    return FALSE;

  file = g_strjoin("/", g->real_dir, file, EXEC_CFG, NULL);

  f = open_cfg (file, FALSE);
  if (!f)
    { ret = FALSE; goto out; }

  if (default_qw_name)
    fprintf (f, "name \"%s\"\n", default_qw_name);

  if (default_qw_skin)
    fprintf (f, "skin \"%s\"\n", default_qw_skin);

  fprintf (f, "team \"%s\"\n", (default_qw_team)? default_qw_team : "");
  fprintf (f, "topcolor    \"%d\"\n", default_qw_top_color);
  fprintf (f, "bottomcolor \"%d\"\n", default_qw_bottom_color);

  switch (pushlatency_mode) {

  case 1:	/* automatic pushlatency */
    if (con->s->ping <= 0)
      auto_pl = 10;				/* "min" value */
    else if (con->s->ping >= 2000)
      auto_pl = 1000;				/* "max" value */
    else {
      auto_pl = con->s->ping / 2;
      auto_pl = ((auto_pl + 9) / 10) * 10;	/* beautify it */
    }
    fprintf (f, "pushlatency %d\n", -auto_pl);
    break;

  case 2:	/* fixed value */
    fprintf (f, "pushlatency %d\n", pushlatency_value);
    break;

  default:	/* do not touch */
    break;

  }

  fprintf (f, "rate        \"%d\"\n", default_qw_rate);
  fprintf (f, "cl_nodelta  \"%d\"\n", default_qw_cl_nodelta);
  fprintf (f, "cl_predict_players \"%d\"\n", default_qw_cl_predict);
  fprintf (f, "noaim       \"%d\"\n", default_noaim);
  fprintf (f, "noskins     \"%d\"\n", default_qw_noskins);
  if (default_w_switch >= 0)
    fprintf (f, "setinfo w_switch \"%d\"\n", default_w_switch);
  if (default_b_switch >= 0)
    fprintf (f, "setinfo b_switch \"%d\"\n", default_b_switch);

  if (games[QW_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[QW_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);

out:

  if(!ret)
    ret = complain_launch_cant_write_file(file);

  g_free(file);
  return ret;
}

static int write_q2_vars (const struct condef *con) {
  FILE *f;
  int ret = TRUE;
  struct game *g = &games[con->s->type];
  char* file = NULL;

  if(g->main_mod && g->main_mod[0])
    file = g->main_mod[0];
  else
    return FALSE;

  file = g_strjoin("/", g->real_dir, file, EXEC_CFG, NULL);

  f = open_cfg (file, FALSE);
  if (!f)
    { ret = FALSE; goto out; }

  if (default_q2_name)
    fprintf (f, "set name \"%s\"\n", default_q2_name);

  if (default_q2_skin)
    fprintf (f, "set skin \"%s\"\n", default_q2_skin);

  fprintf (f, "set rate        \"%d\"\n", default_q2_rate);
  fprintf (f, "set cl_nodelta  \"%d\"\n", default_q2_cl_nodelta);
  fprintf (f, "set cl_predict  \"%d\"\n", default_q2_cl_predict);
  fprintf (f, "set cl_noskins  \"%d\"\n", default_q2_noskins);

  if (games[Q2_SERVER].game_cfg)
    fprintf (f, "exec \"%s\"\n", games[Q2_SERVER].game_cfg);

  if (con->custom_cfg)
    fprintf (f, "exec \"%s\"\n", con->custom_cfg);

  fclose (f);

out:

  if(!ret)
    ret = complain_launch_cant_write_file(file);

  g_free(file);
  return ret;
}

#if 0
static int is_dir (const char *dir, const char *gamedir) {
  struct stat stat_buf;
  char *path;
  int res = FALSE;

  if (dir && gamedir) {
    path = file_in_dir (dir, gamedir);
    res = (stat (path, &stat_buf) == 0 && S_ISDIR (stat_buf.st_mode));
    g_free (path);
  }
  return res;
}
#endif

static int q1_exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char buf[16];
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound)
    argv[argi++] = "-nosound";

  if (default_nocdaudio)
    argv[argi++] = "-nocdaudio";

  if (con->gamedir) {
    argv[argi++] = "-game";
    argv[argi++] = con->gamedir;
  }

  argv[argi++] = "+exec";
  argv[argi++] = EXEC_CFG;

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }
  
  if (con->server) {
    if (con->s->port != g->default_port) {
      g_snprintf (buf, 16, "%d", con->s->port);
      argv[argi++] = "+port";
      argv[argi++] = buf;
    }
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}


static int qw_exec (const struct condef *con, int forkit)
{
  char *argv[32];
  int argi = 0;
  char *cmd;
  char *file = NULL;
  struct game *g = &games[con->s->type];
  int retval=-1;


  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound)
    argv[argi++] = "-nosound";

  if (default_nocdaudio)
    argv[argi++] = "-nocdaudio";

  if (con->gamedir) {
    argv[argi++] = "-gamedir";
    argv[argi++] = con->gamedir;
  }

  if (con->password || con->rcon_password
      || real_password (con->spectator_password)) {

    if(g->main_mod && g->main_mod[0])
      file = g_strjoin("/", g->real_dir, g->main_mod[0], PASSWORD_CFG, NULL);

    if (file && !write_passwords (file, con)
	&& !complain_launch_cant_write_file(file))
      goto out;

    argv[argi++] = "+exec";
    argv[argi++] = PASSWORD_CFG;
  }
  else {
    if (con->spectator_password) {
      argv[argi++] = "+spectator";
      argv[argi++] = con->spectator_password;
    }
  }

  argv[argi++] = "+exec";
  argv[argi++] = EXEC_CFG;

  if (con->server) {
    if (con->demo) {
      argv[argi++] = "+record";
      argv[argi++] = con->demo;
      if (con->s->type == QW_SERVER)
	argv[argi++] = con->server;
    }
    else {
      argv[argi++] = "+connect";
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

out:
  g_free(file);
  g_free(cmd);

  return retval;
}

static int q2_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  char *file = NULL;
  struct game *g = &games[con->s->type];
  int retval=-1;
  
  if(g->main_mod && g->main_mod[0])
    file = g_strjoin("/", g->real_dir, g->main_mod[0], PASSWORD_CFG, NULL);

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  argv[argi++] = "+set";
  argv[argi++] = "cd_nocd";
  argv[argi++] = (default_nocdaudio)? "1" : "0";

  if (con->gamedir) {
    argv[argi++] = "+set";
    argv[argi++] = "game";
    argv[argi++] = con->gamedir;
  }

  if (con->password || con->rcon_password
      || real_password (con->spectator_password)) {

    if (file && !write_passwords (file, con)
	&& !complain_launch_cant_write_file(file))
      goto out;

    argv[argi++] = "+exec";
    argv[argi++] = PASSWORD_CFG;
  }
  else {
    if (con->spectator_password) {
	argv[argi++] = "+spectator";
	argv[argi++] = con->spectator_password;
    }
  }

  argv[argi++] = "+exec";
  argv[argi++] = EXEC_CFG;

  if (con->server) {
      argv[argi++] = "+connect";
      argv[argi++] = con->server;
  }
  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

out:
  g_free (file);
  g_free (cmd);
  return retval;
}

static int q2_exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  argv[argi++] = "+set";
  argv[argi++] = "cd_nocd";
  argv[argi++] = (default_nocdaudio)? "1" : "0";

  if (con->gamedir) {
    argv[argi++] = "+set";
    argv[argi++] = "game";
    argv[argi++] = con->gamedir;
    
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);

  return retval;
}

static int q3_exec (const struct condef *con, int forkit) {
  char *argv[64];
  int argi = 0;
  int cmdi = 0;

  char* cmd = NULL;
  char *protocol = NULL;
  char** cmdtokens = NULL;
  char *punkbuster;
  char* protocmdtofree = NULL;
  char** additional_args = NULL;

  char** memoryoptions = NULL;
  int i;

  struct game *g = NULL;
  int retval;
  
  int game_match_result = 0;
  
  char *real_game_dir = NULL;

  int vmfix = 0;
  int setfs_game = 0;
  int set_punkbuster = 0;
  int pass_memory_options = 0;
  
  int vm_game_set = 0;
  int vm_cgame_set = 0;
  int vm_ui_set = 0;

  struct quake_private* pd = NULL;
  
  if(!con) return -1;
  if(!con->s) return -1;

  g = &games[con->s->type];
  if(!g) return -1;

  pd = (struct quake_private*)games[con->s->type].pd;

  vmfix               = str2bool(game_get_attribute(g->type,"vmfix"));
  setfs_game          = str2bool(game_get_attribute(g->type,"setfs_game"));
  set_punkbuster      = str2bool(game_get_attribute(g->type,"set_punkbuster"));
  pass_memory_options = str2bool(game_get_attribute(g->type,"pass_memory_options"));

  cmdtokens = g_strsplit(g->cmd," ",0);

  if(cmdtokens && *cmdtokens[cmdi])
    cmd=cmdtokens[cmdi++];

  /*
    Figure out what protocal the server is running so we can try to connect
    with a specialized quake3 script. You need to name the scripts like
    quake3proto48.
  */

  protocol = strdup_strip(find_server_setting_for_key ("protocol", con->s->info));
  if (cmd && protocol)
  {
    char* tmp = g_strdup_printf("%sproto%s",cmd,protocol);
    char* tmp_cmd = expand_tilde(tmp);
    g_free(tmp);
    tmp = NULL;

    g_free(protocol);
    protocol = NULL;

    debug (1, "Check for '%s' as a command", tmp_cmd);
  
    // Check to see if we can find that generated filename
    // absolute path?
    if(tmp_cmd[0]=='/')
    {
      if(!access(tmp_cmd,X_OK))
      {
	cmd = protocmdtofree = strdup(tmp_cmd);
	debug(1,"absolute path %s",cmd);
      }
    }
    // no, check $PATH
    else
    {
      char* file = find_file_in_path(tmp_cmd);
      if(file)
      {
	cmd = protocmdtofree = file;
	debug(1,"use file %s in path",cmd);
      }
    }
    g_free(tmp_cmd);
  }

  while(cmd)
  {
    if(*cmd) // skip empty
    {
      argv[argi++] = cmd;
    }
    cmd = cmdtokens[cmdi++];
  }

  if (default_nosound) {
    argv[argi++] = "+set";
    argv[argi++] = "s_initsound";
    argv[argi++] = "0";
  }

  if (con->rcon_password)
  {
    argv[argi++] = "+rconpassword";
    argv[argi++] = con->rcon_password;
  }

  if (con->password)
  {
    argv[argi++] = "+password";
    argv[argi++] = con->password;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->demo) {
    argv[argi++] = "+record";
    argv[argi++] = con->demo;
  }

  if(g->game_cfg) {
    argv[argi++] = "+exec";
    argv[argi++] = g->game_cfg;
  }

  if (con->custom_cfg) {
    argv[argi++] = "+exec";
    argv[argi++] = con->custom_cfg;
  }

// useful for wolf too, ef doesn't have it
//  if(g->type == Q3_SERVER)
  {
    /* The 1.32 release of Q3A needs +set cl_punkbuster 1 on the command line. */
    punkbuster = find_server_setting_for_key ("sv_punkbuster", con->s->info);
    if( punkbuster != NULL && strcmp( punkbuster, "1" ) == 0 )
    {
      if( set_punkbuster )
      {
	argv[argi++] = "+set";
	argv[argi++] = "cl_punkbuster";
	argv[argi++] = "1";
      }
      else
      {
	char* option = g_strdup_printf("/" CONFIG_FILE "/Game: %s/punkbuster dialog shown",type2id(g->type));
	debug( 1, "Got %s for punkbuster\n", punkbuster );
	if(!config_get_bool (option))
	{
	  dialog_ok (NULL, _("The server has Punkbuster enabled but it is not going\nto be set on the command line.\nYou may have problems connecting.\nYou can fix this in the game preferences."));
	  config_set_bool (option,TRUE);
	}
	g_free(option);
      }
    }
  }
  
  /*
    If the s->game is set, we want to put fs_game on the command
    line so that the mod is loaded when we connect.
  */
  if (setfs_game && con->s->game)
  {
    gboolean is_mainmod = FALSE;
    for(i = 0; g->main_mod && g->main_mod[i]; ++i)
    {
      if(!g_strcasecmp(con->s->game, g->main_mod[i]))
      {
	is_mainmod = TRUE;
	break;
      }
    }

    if(!is_mainmod)
    {
      // Look in (e.g.) ~/.q3a first
      if(g->real_home)
      {
	real_game_dir = find_game_dir(g->real_home, con->s->game, &game_match_result);
      }
      
      if (!real_game_dir)
      {
	// Didn't find in home directory so look in real directory if defined
	real_game_dir = find_game_dir(g->real_dir, con->s->game, &game_match_result);
      }
      
      argv[argi++] = "+set";
      argv[argi++] = "fs_game";
      argv[argi++] = real_game_dir?real_game_dir:con->s->game;
    }
  }

  if(pass_memory_options == TRUE)
  {
    memoryoptions = g_new0(char*,5); // must be null terminated => max four entries
    argv[argi++] = "+set";
    argv[argi++] = "com_hunkmegs";
    argv[argi++] = memoryoptions[0] = strdup(game_get_attribute(g->type,"com_hunkmegs"));
    argv[argi++] = "+set";
    argv[argi++] = "com_zonemegs";
    argv[argi++] = memoryoptions[1] = strdup(game_get_attribute(g->type,"com_zonemegs"));
    argv[argi++] = "+set";
    argv[argi++] = "com_soundmegs";
    argv[argi++] = memoryoptions[2] = strdup(game_get_attribute(g->type,"com_soundmegs"));
    argv[argi++] = "+set";
    argv[argi++] = "cg_precachedmodels";
    argv[argi++] = memoryoptions[3] = strdup(game_get_attribute(g->type,"cg_precachedmodels"));
  }

  // Append additional args if needed
  i = 0;
  additional_args = get_custom_arguments(g->type, con->s->game);

  while(additional_args && additional_args[i] )
  {
    argv[argi++] = additional_args[i];

    // Check to see if vm_game, vm_cgame or vm_ui was set in custom args
    // Used to prevent them from being re-set if vmfix is enabled below.
    if (strcmp(additional_args[i], "vm_game") == 0)
      vm_game_set = 1;
    else if (strcmp(additional_args[i], "vm_cgame") == 0)
      vm_cgame_set = 1;
    else if (strcmp(additional_args[i], "vm_ui") == 0)
      vm_ui_set = 1;
    i++;
  }

  if(vmfix) {
    if (!vm_game_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_game";
      argv[argi++] = "2";
    }
    if (!vm_cgame_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_cgame";
      argv[argi++] = "2";
    }
    if (!vm_ui_set)
    {
      argv[argi++] = "+set";
      argv[argi++] = "vm_ui";
      argv[argi++] = "2";
    }
  }    
   
  argv[argi] = NULL;

  debug(1,"argument count %d", argi);

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (real_game_dir);
  g_free (protocmdtofree);
  g_strfreev(additional_args);
  g_strfreev(cmdtokens);
  g_strfreev(memoryoptions);
  return retval;
}

static int hl_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if(con->s->type == HL2_SERVER) // XXX
  {
    argv[argi++] = "-steam";
  }

  if (con->gamedir) {
    argv[argi++] = "-game";
    argv[argi++] = con->gamedir;
  }

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  if (con->password) {
    argv[argi++] = "+password";
    argv[argi++] = con->password;
  }

  if (con->rcon_password)
  {
    argv[argi++] = "+rcon_password";
    argv[argi++] = con->rcon_password;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}


static int ut_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;
  char* hostport=NULL;
  char* real_server=NULL;
  char** additional_args = NULL;
  char **info_ptr;
  int i;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;


// Pass server IP address first otherwise it won't work.
// Make sure ut/ut script (from installed game) contains
// exec "./ut-bin" $* -log and not -log $* at the end
// otherwise XQF you can not connect via the command line!

  if(g->flags & GAME_LAUNCH_HOSTPORT) {
    // go through all server rules
    for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
      if (!strcmp (*info_ptr, "hostport")) {
        hostport=info_ptr[1];
      }
    }
  }
  
  if (con->server)
  {
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
    }
    else
      real_server = strdup(con->server);

    if (con->spectate) {
      char* tmp = NULL;
      if(real_password(con->spectator_password))
	tmp = g_strdup_printf("%s?spectatoronly=true?password=%s",real_server,con->spectator_password);
      else
	tmp = g_strdup_printf("%s?spectatoronly=true",real_server);
      g_free(real_server);
      real_server=tmp;
    }

    // Add password if exists
    if (con->password) {
      char* tmp = g_strdup_printf("%s?password=%s",real_server,con->password);
      g_free(real_server);
      real_server=tmp;
    }
  }

  if(con->s)
  {
    // Append additional args if needed
    i = 0;

    additional_args = get_custom_arguments(con->s->type, con->s->game);

//    if (!(additional_args && additional_args[i]))
      argv[argi++] = real_server;
    
    while(additional_args && additional_args[i] )
    {
      argv[argi++] = additional_args[i];
      /*
      // append first argument to server address
      if(i == 0)
      {
	tmp = g_strconcat(real_server,additional_args[i],NULL);
	g_free(real_server);
	real_server=tmp;
	argv[argi++] = real_server;
      }
      else
      {
	argv[argi++] = additional_args[i];
      }
      */
      i++;
    }
  }

  if (default_nosound) {
    argv[argi++] = "-nosound";
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  if(additional_args) g_strfreev(additional_args);
  return retval;
}

static int savage_exec(const struct condef *con, int forkit)
{
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;
  char* connect_arg = NULL;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = "set";
    argv[argi++] = "autoexec";

    connect_arg = g_strdup_printf("connect %s",con->server);
    
    if(con->password)
    {
      char* tmp = connect_arg;
      connect_arg = g_strdup_printf("%s; set cl_password %s", connect_arg, con->password);
      g_free(tmp);
    }
    if(con->rcon_password)
    {
      char* tmp = connect_arg;
      connect_arg = g_strdup_printf("%s; set cl_adminpassword %s", connect_arg, con->rcon_password);
      g_free(tmp);
    }
    
    argv[argi++] = connect_arg;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (connect_arg);
  return retval;

  return 0;
}


// this one just passes the ip address as first parameter
static int exec_generic (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

// Serious Sam: only supports +connect
static int ssam_exec(const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = "+connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

// Netpanzer: only supports -c
static int netpanzer_exec(const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {
    argv[argi++] = "-c";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

// launch any game that uses the gamespy protocol
// the first argument is the content of gamename field (may be empty),
// the second one the ip of the server
static int gamespy_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = NULL;
  int retval;
  char **info_ptr;
  char* gamename=NULL;

  char* hostport=NULL;
  char* real_server=NULL;
  
  if(!con || !con->s)
    return 1;

  g = &games[con->s->type];

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  // go through all server rules
  for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (!strcmp (*info_ptr, "gamename")) {
      gamename = strdup_strip(info_ptr[1]);
    }
    else if (!strcmp (*info_ptr, "hostport")) {
      hostport=info_ptr[1];
    }
  }

  if(gamename)
    argv[argi++] = gamename;
  else
    argv[argi++] = "";
  
  if (con->server) {
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
      argv[argi++] = real_server;
    }
    else
    {
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  g_free (gamename);
  return retval;
}

static int t2_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = &games[con->s->type];
  int retval;

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  if (con->server) {

    if(default_t2_name && *default_t2_name) {   
      argv[argi++] = "-login";
      argv[argi++] = default_t2_name;
    }

    argv[argi++] = "-online";
    argv[argi++] = "-connect";
    argv[argi++] = con->server;
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  return retval;
}

static int bf1942_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = NULL;
  int retval;
  char **info_ptr;
  char* gameid=NULL;

  char* hostport=NULL;
  char* real_server=NULL;
  
  if(!con || !con->s)
    return 1;

  g = &games[con->s->type];

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  argv[argi++] = "+restart";
  argv[argi++] = "1";

  // go through all server rules
  for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (!strcmp (*info_ptr, "gameid")) {
      gameid=info_ptr[1];
    }
    else if (!strcmp (*info_ptr, "hostport")) {
      hostport=info_ptr[1];
    }
  }

  if(gameid)
  {
    argv[argi++] = "+game";
    argv[argi++] = gameid;
  }

//  argv[argi++] = strdup_strip (gameid);
  
  if (con->server) {
    argv[argi++] = "+joinServer";
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
      argv[argi++] = real_server;
    }
    else
    {
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  return retval;
}

static int descent3_exec (const struct condef *con, int forkit) {
  char *argv[32];
  int argi = 0;
  char *cmd;
  struct game *g = NULL;
  int retval;
  char **info_ptr;

  char* hostport=NULL;
  char* real_server=NULL;
  
  if(!con || !con->s)
    return 1;

  g = &games[con->s->type];

  cmd = strdup_strip (g->cmd);

  argv[argi++] = strtok (cmd, delim);
  while ((argv[argi] = strtok (NULL, delim)) != NULL)
    argi++;

  // go through all server rules
  for (info_ptr = con->s->info; info_ptr && *info_ptr; info_ptr += 2) {
    if (!strcmp (*info_ptr, "hostport")) {
      hostport=info_ptr[1];
    }
  }

  if (con->server) {
    argv[argi++] = "--directip";
    // gamespy port can be different from game port
    if(hostport)
    {
      real_server = g_strdup_printf ("%s:%s", inet_ntoa (con->s->host->ip), hostport);
      argv[argi++] = real_server;
    }
    else
    {
      argv[argi++] = con->server;
    }
  }

  argv[argi] = NULL;

  retval = client_launch_exec (forkit, g->real_dir, argv, con->s);

  g_free (cmd);
  g_free (real_server);
  return retval;
}


static char *dir_custom_cfg_filter (const char *dir, const char *str) {
  static const char *cfgext[] = { ".cfg", ".scr", ".rc", NULL };
  const char **ext;
  size_t len;

  if (!str)
    return NULL;

  len = strlen (str);

  for (ext = cfgext; *ext; ext++) {
    if (len > strlen (*ext) && 
                          strcasecmp (str + len - strlen (*ext), *ext) == 0) {
      return g_strdup (str);
    }
  }

  return NULL;
}


static GList *custom_cfg_filter (GList *list) {
  GList *tmp;
  GList *res = NULL;
  char *str;

  for (tmp = list; tmp; tmp = tmp->next) {
    str = (char *) tmp->data;
    if (strcmp (str, EXEC_CFG) && strcmp (str, PASSWORD_CFG) && 
        strcmp (str, "__qf__.cfg") && strcasecmp (str, "config.cfg") && 
        strcasecmp (str, "server.cfg") && strcasecmp (str, "autoexec.cfg") &&
        strcasecmp (str, "quake.rc") && strcasecmp (str, "q3config.cfg")) {
      res = g_list_prepend (res, str);
    }
    else {
      g_free (str);
    }
  }

  g_list_free (list);
  res = g_list_reverse (res);
  return res;
}


static GList *quake_custom_cfgs (struct game* this, const char *path, const char *mod)
{
  GList *cfgs = NULL;
  const char* dirs[2] = {0};
  unsigned d, i;

  debug(4, "%s, %s", path, mod);

  dirs[0] = path?path:this->real_dir;
  dirs[1] = this->real_home;

  for(d = 0; d < sizeof(dirs)/sizeof(dirs[0]); ++d)
  {
    if(!(dirs[d] && *dirs[d]))
      continue;

    for(i = 0; this->main_mod && this->main_mod[i]; ++i)
    {
      char* dir = file_in_dir (dirs[d], this->main_mod[i]);
      GList* tmp = dir_to_list (dir, dir_custom_cfg_filter);
      cfgs = merge_sorted_string_lists (cfgs, tmp);
      g_free (dir);
    }

    if(mod)
    {
      char* dir = file_in_dir (dirs[d], mod);
      GList* tmp = dir_to_list (dir, dir_custom_cfg_filter);
      cfgs = merge_sorted_string_lists (cfgs, tmp);
      g_free (dir);
    }
  }

  cfgs = custom_cfg_filter (cfgs);

  return cfgs;
}

static void quake_save_server_rules (FILE *f, struct server *s) {
  char **info;

  if (!s || !s->info)
    return;

  info = s->info;

  while (info[0]) {

    if (info[1])
      fprintf (f, "%s=%s", info[0], info[1]);
    else
      fprintf (f, "%s", info[0]);

    info += 2;

    if (info[0])
      fputc (QSTAT_DELIM, f);
  }

  fputc ('\n', f);
}


static void quake_save_info (FILE *f, struct server *s) {
  struct player *p;
  GSList *list;

  if (!s->name && !s->map && !s->players && !s->info && s->ping < 0)
    return;

  fprintf (f, "%ld %ld\n", s->refreshed, s->last_answer);

  switch (s->type) {

    /* POQS */

  case Q1_SERVER:
  case H2_SERVER: 
    fprintf (f, 
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "0"  QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d\n", 
	     games[s->type].id,
	     inet_ntoa (s->host->ip), s->port,
	     (s->name)? s->name : "",
	     inet_ntoa (s->host->ip), s->port,
	     (s->map)? s->map : "",
	     s->maxplayers,
	     s->curplayers,
	     s->ping,
	     s->retries);
    break;

    /* QW, Q2 */

  default:
    fprintf (f, 
	     "%s" QSTAT_DELIM_STR
	     "%s:%d" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%s" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d" QSTAT_DELIM_STR
	     "%d\n", 
	     games[s->type].id,
	     inet_ntoa (s->host->ip), s->port,
	     (s->name)? s->name : "",
	     (s->map)? s->map : "",
	     s->maxplayers, 
	     s->curplayers,
	     s->ping,
	     s->retries);
    break;

  } /* switch (s->type) */

  quake_save_server_rules (f, s);

  if (default_save_plrinfo) {

    for (list = s->players; list; list = list->next) {
      p = (struct player *) list->data;

      switch (s->type) {

	/* POQS */

      case Q1_SERVER:
      case H2_SERVER: 
	fprintf (f, 
		 "0"  QSTAT_DELIM_STR
		 "%s" QSTAT_DELIM_STR
	              QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->time,
		 p->shirt,
		 p->pants);
	break;

	/* QW */

      case QW_SERVER:
      case HW_SERVER:
	fprintf (f, 
		 "0"  QSTAT_DELIM_STR
		 "%s" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%d" QSTAT_DELIM_STR
		 "%s\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->time,
		 p->shirt,
		 p->pants,
		 p->ping,
		 (p->skin)? p->skin : "");
	break;

      case UN_SERVER:
      case UT2_SERVER:
      case UT2004_SERVER:
      case RUNE_SERVER:
      case POSTAL2_SERVER:
      case AAO_SERVER:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR
		 "\n",
		 (p->name)? p->name : "",
		 p->frags,
		 p->ping, 
		 p->pants, 
		 (p->skin)? p->skin : "",
		 (p->model)? p->model : "");
	break;

	/* Q2, etc... */

      case T2_SERVER:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "%s" QSTAT_DELIM_STR 
		 "\n", //tribe tag not supported yet
		 (p->name)? p->name : "",
		 p->frags,
		 0,  // team number not supported yet
		 "x",  // team name not supported yet
		 (p->model)? p->model : ""); // player_type
	break;

      case DESCENT3_SERVER:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR
		 "\n",
		 p->name?p->name:"",
		 p->frags,
		 0, // deaths
		 p->ping);
	break;

      default:
	fprintf (f, 
		 "%s" QSTAT_DELIM_STR 
		 "%d" QSTAT_DELIM_STR 
		 "%d\n",
		 (p->name)? p->name : "",
		 p->frags,
		 (s->type == HL_SERVER)? p->time : p->ping);
	break;

      } /* switch (s->type) */

    } /* for (list = s->players ... */

  }  /* if (default_save_plrinfo) */

  fputc ('\n', f);
}

// fetch additional arguments when launching server of type 'type' and it's
// game matches 'gamestring'
// returns a newly-allocated array of strings. Use g_strfreev() to free it. 
// TODO use struct server instead of char* to be able to match any variable

char **get_custom_arguments(enum server_type type, const char *gamestring)
{
  struct game *g = &games[type];
  char *arg = NULL;
  int j;
  char conf[15];
  char *token[2];
  int n;
  char ** ret = NULL;
  GSList *temp;

  if(!gamestring) return NULL;
  
  temp = g_slist_nth(g->custom_args, 0);

  j = 0;
  while (temp) {
    g_snprintf (conf, 15, "custom_arg%d", j);
    arg = g_strdup((char *) temp->data);

    n = tokenize (arg, token, 2, ",");
    
    if(!(strcasecmp(token[0],gamestring))) {
      ret = g_strsplit(token[1]," ",0);
      debug(1, "found entry for:%s.  Returning argument:%s\n",gamestring,token[1]);
      g_free(arg);
      return ret;
    }
    temp = g_slist_next(temp);
  }
  debug(1, "get_custom_arguments: Didn't find an entry for %s",gamestring);
  g_free(arg);
  return NULL;
}


/** map functions */

static void quake_init_maps(enum server_type type)
{
  struct quake_private* pd = NULL;

  pd = (struct quake_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  q3_clear_maps(pd->maphash); // important to avoid memleak when called second time
  pd->maphash = q3_init_maphash();

  findquakemaps(pd->maphash,games[type].real_dir);

  if(games[type].real_home && access(games[type].real_home, R_OK) == 0)
  {
    findquakemaps(pd->maphash,games[type].real_home);
  }
}

static void q3_init_maps(enum server_type type)
{
  struct quake_private* pd = NULL;

  pd = (struct quake_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  q3_clear_maps(pd->maphash); // important to avoid memleak when called second time
  pd->maphash = q3_init_maphash();
  findq3maps(pd->maphash,games[type].real_dir);

  if(games[type].real_home && access(games[type].real_home, R_OK) == 0)
  {
    findq3maps(pd->maphash,games[type].real_home);
  }
}

static void doom3_init_maps(enum server_type type)
{
  struct quake_private* pd = NULL;

  pd = (struct quake_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  q3_clear_maps(pd->maphash); // important to avoid memleak when called second time
  pd->maphash = q3_init_maphash();
  finddoom3maps(pd->maphash,games[type].real_dir);

  if(games[type].real_home && access(games[type].real_home, R_OK) == 0)
  {
    finddoom3maps(pd->maphash, games[type].real_home);
  }
}

static void unreal_init_maps(enum server_type type)
{
  struct unreal_private* pd = NULL;

  pd = (struct unreal_private*)games[type].pd;
  g_return_if_fail(pd!=NULL);
  
  ut_clear_maps(pd->maphash);
  pd->maphash = ut_init_maphash();
  findutmaps_dir(pd->maphash,games[type].real_dir,pd->suffix);

  if(games[type].real_home && access(games[type].real_home, R_OK) == 0)
  {
    findutmaps_dir(pd->maphash,games[type].real_home,pd->suffix);
  }
}

static gboolean quake_has_map(struct server* s)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return q3_lookup_map(hash,s->map);
}

static gboolean doom3_has_map(struct server* s)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return doom3_lookup_map(hash,s->map);
}


static size_t q3_get_mapshot(struct server* s, guchar** buf)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return q3_lookup_mapshot(hash,s->map, buf);
}

static size_t doom3_get_mapshot(struct server* s, guchar** buf)
{
  struct quake_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct quake_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return doom3_lookup_mapshot(hash,s->map, buf);
}

static gboolean unreal_has_map(struct server* s)
{
  struct unreal_private* pd = NULL;
  GHashTable* hash = NULL;

  g_return_val_if_fail(s!=NULL,TRUE);
  
  pd = (struct unreal_private*)games[s->type].pd;
  g_return_val_if_fail(pd!=NULL,TRUE);
  
  hash = (GHashTable*)pd->maphash;
  if(!hash) return TRUE;

  if(!s->map)
    return FALSE;

  return ut_lookup_map(hash,s->map);
}

// vim: sw=2
