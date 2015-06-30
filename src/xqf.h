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
#include <netinet/in.h>     /* struct in_addr */
#include <arpa/inet.h>      /* struct in_addr */
#include <time.h>           /* time_t */

#include <gtk/gtk.h>
#include <glib.h>

#include "defs.h"
#include "game.h"

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

extern GtkBuilder *builder;

int compare_qstat_version (const char* have, const char* expected);
int start_prog_and_return_fd(char *const argv[], pid_t *pid);
int check_qstat_version(void);

void play_sound (const char *sound, gboolean override);
void play_sound_with (const char* player, const char *sound, gboolean override);

//extern int filter_start_index;

extern int event_type;

extern int dontlaunch;

extern void refresh_source_list (void);
extern void update_source_callback (GtkWidget *widget, gpointer data);
extern void refresh_n_server(GtkWidget * button, gpointer *data);
extern void stop_callback (GtkWidget *widget, gpointer data);
void add_to_player_filter (unsigned mask);

#endif /* __XQF_H__ */
