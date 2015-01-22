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

#ifndef __PREF_H__
#define __PREF_H__


#include <gtk/gtk.h>
#include "game.h"


#ifndef PATH_MAX
# ifdef MAXPATHLEN
#  define PATH_MAX MAXPATHLEN
# else
#  define PATH_MAX 8192
# endif
#endif

enum {
	PREF_PAGE_GENERAL = 0,
	PREF_PAGE_GAMES,
	PREF_PAGE_APPEARANCE,
	PREF_PAGE_QSTAT,
	PREF_PAGE_SOUNDS,
	PREF_PAGE_SCRIPTS
};

extern char *user_rcdir;

extern char *default_q1_name;
extern char *default_qw_name;
extern char *default_q2_name;
extern char *default_t2_name;
extern char *default_qw_team;
extern char *default_qw_skin;
extern char *default_q2_skin;
extern int default_q1_top_color;
extern int default_q1_bottom_color;
extern int default_qw_top_color;
extern int default_qw_bottom_color;

extern int default_qw_rate;
extern int default_q2_rate;
extern int default_qw_cl_nodelta;
extern int default_q2_cl_nodelta;
extern int default_qw_cl_predict;
extern int default_q2_cl_predict;
extern int default_noaim;
extern int default_qw_noskins;
extern int default_q2_noskins;
extern int default_b_switch;
extern int default_w_switch;

extern int pushlatency_mode;
extern int pushlatency_value;

extern int default_qw_is_quakeforge;

extern int default_nosound;
extern int default_nocdaudio;

extern int show_hostnames;
extern int show_default_port;

extern int serverlist_countbots;

extern int default_terminate;
extern int default_stopxmms;
extern int default_iconify;
extern int default_launchinfo;
extern int default_prelaunchexec;
extern int default_save_lists;
extern int default_save_srvinfo;
extern int default_save_plrinfo;
extern int default_auto_favorites;
extern int default_show_tray_icon;
extern int default_always_resolve;
extern int default_toolbar_style;
extern int default_toolbar_tips;
extern int default_refresh_sorts;
extern int default_refresh_on_update;
extern int default_resolve_on_update;
extern int default_show_only_configured_games;
extern char* default_icontheme;

extern int maxretries;
extern int maxsimultaneous;
extern char* qstat_srcip;
extern unsigned short qstat_srcport_low;
extern unsigned short qstat_srcport_high;

extern int sound_enable;
extern char *sound_player;
extern char *sound_xqf_start;
extern char *sound_xqf_quit;
extern char *sound_update_done;
extern char *sound_refresh_done;
extern char *sound_stop;
extern char *sound_server_connect;
extern char *sound_redial_success;

extern int skip_startup_mapscan;

void q1_prefs_load(struct game* g);
void qw_prefs_load(struct game* g);
void q2_prefs_load(struct game* g);
void q3_prefs_load(struct game* g);
void q3_prefs_load_common(struct game* g);
void tribes2_prefs_load(struct game* g);

void q1_update_prefs (struct game* g);
void qw_update_prefs (struct game* g);
void q2_update_prefs (struct game* g);
void q3_update_prefs (struct game* g);
void q3_update_prefs_common (struct game* g);
void doom3_update_prefs (struct game* g);
void quake4_update_prefs (struct game* g);
void tribes2_update_prefs (struct game* g);
void ut2004_update_prefs (struct game* g);

extern int set_working_dir (char *dir);
extern void preferences_dialog (int page_num);
extern int fix_qw_player_color (int color);

extern int init_user_info (void);
extern void free_user_info (void);

extern int prefs_load (void);

#endif /* __PREF_H__ */
