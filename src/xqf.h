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

#include "gnuconfig.h"      /* GNU autoconf */

#include <sys/types.h>
#include <netinet/in.h>     /* struct in_addr */
#include <arpa/inet.h>      /* struct in_addr */
#include <time.h>           /* time_t */

#include <gtk/gtk.h>
#include <glib.h>

#define RC_DIR          ".qf"               /* legacy config dir, before 1.0.6 */
#define XDG_RC_DIR      "xqf"               /* new config dir, since 1.0.6 */
#define RC_FILE         "qfrc"
#define CONFIG_FILE     "config"
#define SERVERS_FILE    "servers"
#define PLAYERS_FILE    "players"
#define HOSTCACHE_FILE  "hostcache"
#define EXEC_CFG        "frontend.cfg"
#define PASSWORD_CFG    "__passwd.cfg"
#define LAUNCHINFO_FILE "LaunchInfo.txt"

#define MAX_PING        9999
#define MAX_RETRIES     10


/* max 0x8000, server->flags is unsigned */
enum server_flags {
	PLAYER_GROUP_RED =      0x001,
	PLAYER_GROUP_GREEN =    0x002,
	PLAYER_GROUP_BLUE =     0x004,
	PLAYER_GROUP_MASK =     0x007,

	SERVER_CHEATS =         0x008,
	SERVER_PASSWORD =       0x010,
	SERVER_SP_PASSWORD =    0x020,
	SERVER_SPECTATE =       0x040,
	SERVER_PUNKBUSTER =     0x080,
	SERVER_INCOMPATIBLE =   0x100
};

enum launch_mode {
	LAUNCH_NORMAL,
	LAUNCH_SPECTATE,
	LAUNCH_RECORD
};


// note: there is a limit of 256 servers, see get_server_pixmap in srv-list.c
enum server_type {
	Q1_SERVER = 0,
	QW_SERVER,
	Q2_SERVER,
	Q3_SERVER,
	Q4_SERVER,
	WO_SERVER,
	WOET_SERVER,
	ETL_SERVER,
	DOOM3_SERVER,
	ETQW_SERVER,
	EF_SERVER,
	H2_SERVER,
	HW_SERVER,
	SN_SERVER,
	HL_SERVER_OLD,
	HL_SERVER,
	HL2_SERVER,
	KP_SERVER,
	SFS_SERVER,
	SOF2S_SERVER,
	T2_SERVER,
	HR_SERVER,
	UN_SERVER,
	UT2_SERVER,
	UT2004_SERVER,
	RUNE_SERVER,
	POSTAL2_SERVER,
	AAO_SERVER,
	DESCENT3_SERVER,
	SSAM_SERVER,
	SSAMSE_SERVER,
	MOHAA_SERVER,
	COD_SERVER,
	SAS_SERVER,
	BF1942_SERVER,
	JK2_SERVER,
	JK3_SERVER,
	NETP_SERVER,
	NEXUIZ_SERVER,
	XONOTIC_SERVER,
	WARSOW_SERVER,
	TREMULOUS_SERVER,
	TREMULOUSGPP_SERVER,
	TREMFUSION_SERVER,
	UNVANQUISHED_SERVER,
	OPENARENA_SERVER,
	OTTD_SERVER,
	Q3RALLY_SERVER,
	WOP_SERVER,
	IOURT_SERVER,
	REACTION_SERVER,
	SMOKINGUNS_SERVER,
	ZEQ2LITE_SERVER,
	TURTLEARENA_SERVER,
	ALIENARENA_SERVER,
	GPS_SERVER,
	UNKNOWN_SERVER
};

#define  GAMES_TOTAL UNKNOWN_SERVER

enum master_state {
	SOURCE_NA = 0,
	SOURCE_UP,
	SOURCE_DOWN,
	SOURCE_TIMEOUT,
	SOURCE_ERROR
};

enum master_query_type {
	MASTER_NATIVE=0,
	MASTER_GAMESPY,
	MASTER_HTTP,
	MASTER_LAN,
	MASTER_FILE,
	MASTER_GSLIST,
	MASTER_NUM_QUERY_TYPES,
	MASTER_INVALID_TYPE
};


/**
 * A player. There is no destructor for this struct. Alloc as many data as you
 * need at once and have the pointers point inside.
 */
struct player {
	char *name;
	char *skin;
	char *model;
	int time;
	short frags;
	short ping;
	unsigned char shirt;
	unsigned char pants;
	unsigned char flags;
};

struct host {
	char *name;
	struct in_addr ip;
	time_t refreshed;
	int ref_count;
};


/** server_new, server_free_info, server_move_info must be adapted for
 * changes to this structure
 */
struct server {
	struct host *host;

	gchar *name;
	char *map;
	char **info;
	char *game;         /** pointer into info, do not free */
	char *gametype;     /** pointer into info, do not free */
	GSList *players;    /** GSList<struct player *>  */

#ifdef USE_GEOIP
	int country_id;
#endif

	unsigned flags;

	unsigned flt_last;  /** time of the last filtering */

	time_t refreshed;
	time_t last_answer; /** time of last reply from server */

	int ref_count;

	enum server_type type;
	unsigned short port;

	unsigned short maxplayers;
	unsigned short curplayers;
	unsigned short curbots;
	unsigned short private_client;  /** number of private clients */
	short ping;
	short retries;

	char sv_os;             /** L = Linux, W = windows, M = Mac */

	unsigned char filters;
	unsigned char flt_mask;
};

struct userver {
	char *hostname;
	struct server *s;
	int ref_count;
	unsigned short port;
	unsigned char type;     /* enum server_type type; */
};

typedef struct {
	int portadjust;         /* value added to port */
	char* gsmtype;          /* type for gslist master */
} QFMasterOptions;

typedef struct master {
	char *name;
	enum server_type type;
	int isgroup;            /* is it a real master or master group? */
	int user;               /* is it added or edited by user? */
	QFMasterOptions options;

	struct host *host;
	unsigned short port;
	char *hostname;         /* unresolved hostname */

	char *url;

	GSList *servers;
	GSList *uservers;
	enum master_state state;

	GSList *masters;

	enum master_query_type master_type;

	/** private */
	char* _qstat_master_option; // optional override from games[type].qstat_master_option
} QFMaster;

char* master_qstat_option(struct master* m);
void master_set_qstat_option(struct master* m, const char* opt);

char* master_to_url(QFMaster* m);

extern time_t xqf_start_time;
extern char* xqf_PACKAGE_DATA_DIR;
extern char* xqf_LOCALEDIR;
extern char* xqf_PIXMAPSDIR;

extern char* qstat_configfile;

extern GSList *cur_source;          /*  GSList <struct master *>  */
extern GSList *cur_server_list;     /*  GSList <struct server *>  */
extern GSList *cur_userver_list;    /*  GSList <struct server *>  */

extern struct server *cur_server;

extern struct stat_job *stat_process;


int compare_qstat_version (const char* have, const char* expected);
int start_prog_and_return_fd(char *const argv[], pid_t *pid);
int check_qstat_version(void);

void play_sound (const char *sound, gboolean override);
void play_sound_with (const char* player, const char *sound, gboolean override);

extern void reset_main_status_bar(void);

//extern int filter_start_index;

extern int event_type;

extern int dontlaunch;

extern void refresh_source_list (void);
extern void update_source_callback (GtkWidget *widget, gpointer data);
extern void refresh_n_server(GtkWidget * button, gpointer *data);
extern void stop_callback (GtkWidget *widget, gpointer data);

#endif /* __XQF_H__ */
