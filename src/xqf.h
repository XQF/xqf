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

#ifndef __XQF_H__
#define __XQF_H__

#include <sys/types.h>
#include <netinet/in.h> /* struct in_addr */
#include <arpa/inet.h>	/* struct in_addr */
#include <time.h>	/* time_t */

#include <gtk/gtk.h>

#include "gnuconfig.h"	/* GNU autoconf */

#define RC_DIR		".qf"
#define RC_FILE		"qfrc"
#define CONFIG_FILE	"config"
#define SERVERS_FILE	"servers"
#define PLAYERS_FILE	"players"
#define HOSTCACHE_FILE	"hostcache"
#define EXEC_CFG	"frontend.cfg"
#define PASSWORD_CFG	"__passwd.cfg"
#define LAUNCHINFO_FILE	"LaunchInfo.txt"

#define MAX_SERVER_FILTERS 10

#define MAX_PING	9999
#define MAX_RETRIES	10

#define	Q1_DEFAULT_PORT		26000		/* Quake */
#define	QW_DEFAULT_PORT		27500		/* QuakeWorld */
#define	Q2_DEFAULT_PORT		27910		/* Quake2 */
#define	Q3_DEFAULT_PORT		27960		/* Quake3: Arena */
#define	WO_DEFAULT_PORT		27960		/* Wolfenstein */
#define	H2_DEFAULT_PORT		26900		/* Hexen2 */
#define	HW_DEFAULT_PORT		26950		/* HexenWorld */
#define	SN_DEFAULT_PORT		22450		/* Sin */
#define	HL_DEFAULT_PORT		27015		/* Half-Life */
#define	KP_DEFAULT_PORT		31510		/* Kingpin */
#define	SFS_DEFAULT_PORT	28910		/* Soldier of Fortune */
#define	T2_DEFAULT_PORT		28000		/* Tribes 2 */
#define	HR_DEFAULT_PORT		28910		/* Heretic2 */
#define	UN_DEFAULT_PORT		7777		/* Unreal */
#define	GPS_DEFAULT_PORT	27888		/* Gamespy Generic */
#define	DESCENT3_DEFAULT_PORT	2092		/* Descent 3 */

#define	QWM_DEFAULT_PORT	27000		/* QuakeWorld */
#define	Q2M_DEFAULT_PORT	27900		/* Quake2 master */
#define	Q3M_DEFAULT_PORT	27950		/* Quake3 master */
#define	WOM_DEFAULT_PORT	27950		/* Wolfenstein master */
#define	HLM_DEFAULT_PORT	27010		/* Half-Life master */
#define	D3M_DEFAULT_PORT	3445		/* Descent 3 master */

#define PLAYER_GROUP_MASK	0x07
#define PLAYER_GROUP_RED	0x01
#define PLAYER_GROUP_GREEN	0x02
#define PLAYER_GROUP_BLUE	0x04

#define SERVER_CHEATS		0x08
#define SERVER_PASSWORD		0x10
#define SERVER_SP_PASSWORD	0x20
#define SERVER_SPECTATE		0x40

enum launch_mode { 
  LAUNCH_NORMAL,
  LAUNCH_SPECTATE,
  LAUNCH_RECORD
};

enum server_type { 
  Q1_SERVER = 0,
  QW_SERVER,
  Q2_SERVER,
#ifdef QSTAT23
  Q3_SERVER,
#endif
  WO_SERVER,
  H2_SERVER,
  HW_SERVER,
  SN_SERVER,
  HL_SERVER,
  KP_SERVER,
  SFS_SERVER,
  T2_SERVER,
  HR_SERVER,
#ifdef QSTAT_HAS_UNREAL_SUPPORT
  UN_SERVER,
  RUNE_SERVER,
#endif
  DESCENT3_SERVER,
  GPS_SERVER,
  UNKNOWN_SERVER
};

#define  GAMES_TOTAL	UNKNOWN_SERVER

enum master_state { 
  SOURCE_NA = 0,
  SOURCE_UP,
  SOURCE_DOWN,
  SOURCE_TIMEOUT,
  SOURCE_ERROR
};

struct player {
  char	*name;
  int	time;
  short frags;
  short ping;
  unsigned char shirt;
  unsigned char pants;
  unsigned char flags;
  char	*skin;
  char	*model;
};

struct host {
  char *name;
  struct in_addr ip;
  time_t refreshed;
  int ref_count;
};

struct server {
  struct host *host;
  unsigned short port;

  char	*name;
  char	*map;
  char  *gametype;
  unsigned short maxplayers;
  unsigned short curplayers;
  short	ping;
  short retries;
  char 	**info;
  char 	*game;		/* a reference to `info' */
  GSList *players;	/* GSList<struct player *>  */

  char sv_os;         /* L = Linux, W = windows, M = Mac */

  unsigned char type;	/* enum server_type type; */
  unsigned char flags;

  unsigned char filters;
  unsigned char flt_mask;
  unsigned flt_last;	/* time of the last filtering */
  unsigned private_client; /* number of private clients */

  time_t refreshed;

  int 	ref_count;
};

struct userver {
  char *hostname;
  unsigned short port;
  unsigned char type;	/* enum server_type type; */
  struct server *s;
  int 	ref_count;
};

struct master {
  char *name;
  enum server_type type;
  int isgroup;		/* is it a real master or master group? */
  int user;		/* is it added or edited by user? */

  struct host *host;
  unsigned short port;
  char *hostname;		/* unresolved hostname */

  char *url;

  GSList *servers;
  GSList *uservers;
  enum master_state state;

  GSList *masters;
  
  int master_type;
};

extern	time_t xqf_start_time;

extern 	GtkWidget *main_window;
extern	GtkWidget *source_ctree;
extern  GtkCList  *server_clist;
extern  GtkCList  *player_clist;
extern  GtkCTree  *srvinf_ctree;

extern	GSList *cur_source;		/*  GSList <struct master *>  */
extern	GSList *cur_server_list;	/*  GSList <struct server *>  */
extern	GSList *cur_userver_list;	/*  GSList <struct server *>  */

extern	struct server *cur_server;

extern	struct stat_job *stat_process;

extern	GtkWidget *view_hostnames_menu_item;
extern	GtkWidget *view_defport_menu_item;

extern	int window_delete_event_callback (GtkWidget *widget, gpointer data);
extern	void register_window (GtkWidget *window);
extern	void unregister_window (GtkWidget *window);
extern	GtkWidget *top_window (void);

extern	void set_widgets_sensitivity (void);

/* This is defined in here so that filter.c can see it */
GtkWidget *server_filter_widget[MAX_SERVER_FILTERS + 3];

int filter_start_index;

#endif /* __XQF_H__ */

