// XQF Game Server Browser
// Copyright (C) 1998-2015 XQF Team - https://xqf.github.io
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

#ifndef __DEFS_H__
#define __DEFS_H__

#include <sys/types.h>
#include <netinet/in.h>     /* struct in_addr */
#include <arpa/inet.h>      /* struct in_addr */
#include <time.h>           /* time_t */

#include <gtk/gtk.h>
#include <glib.h>

#include GAMES_H_INCLUDE

// max 0x8000, server->flags is unsigned
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

// A player. There is no destructor for this struct. Alloc as many data as you need at once and have the pointers point inside.

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

// server_new, server_free_info, server_move_info must be adapted for changes to this structure

struct server {
	struct host *host;

	gchar *name;
	char *map;
	char **info;
	char *game;         // pointer into info, do not free
	char *gametype;     // pointer into info, do not free
	GSList *players;    // GSList<struct player *>

#ifdef USE_GEOIP
	int country_id;
#endif

	unsigned long flags;

	unsigned flt_last;  // time of the last filtering

	time_t refreshed;
	time_t last_answer; // time of last reply from server

	int ref_count;

	enum server_type type;
	enum server_type server_query_type;
	unsigned short port;

	unsigned short maxplayers;
	unsigned short curplayers;
	unsigned short curbots;
	unsigned short private_client;  // number of private clients
	short ping;
	short retries;

	char sv_os;             // L = Linux, W = windows, M = Mac

	unsigned char filters;
	unsigned char flt_mask;
};

struct userver {
	char *hostname;
	struct server *s;
	int ref_count;
	unsigned short port;
	enum server_type type;                  // enum server_type type;
	enum server_type server_query_type;     // enum server_type type;
};

typedef struct {
	int portadjust;         // value added to port
	char* gsmtype;          // type for gslist master
} QFMasterOptions;

typedef struct master {
	char *name;
	enum server_type type;
	enum server_type server_query_type;
	int isgroup;            // is it a real master or master group?
	int user;               // is it added or edited by user?
	QFMasterOptions options;

	struct host *host;
	unsigned short port;
	char *hostname;         // unresolved hostname

	char *url;

	GSList *servers;
	GSList *uservers;
	enum master_state state;

	GSList *masters;

	enum master_query_type master_type;

	// private
	char* _qstat_master_option; // optional override from games[type].qstat_master_option
} QFMaster;

#endif //__DEFS_H__
