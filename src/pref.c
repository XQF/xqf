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

#include <sys/types.h>      /* FD_SETSIZE */
#include <stdio.h>          /* fprintf */
#include <stdlib.h>         /* strtol */
#include <string.h>         /* strcmp, strlen */
#include <sys/time.h>       /* FD_SETSIZE */
#include <sys/stat.h>       /* stat */
#include <unistd.h>         /* readlink */
#include <sys/resource.h>   /* setrlimit */
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "game.h"
#include "pref.h"
#include "dialogs.h"
#include "skin.h"
#include "utils.h"
#include "srv-prop.h"
#include "pixmaps.h"
#include "config.h"
#include "rc.h"
#include "debug.h"
#include "sort.h"
#include "q3maps.h"
#include "srv-list.h"
#include "scripts.h"

static struct generic_prefs* new_generic_prefs (void);
static GtkWidget *custom_args_options_page (enum server_type type);

static void scan_maps_for(enum server_type type);

char *user_rcdir = NULL;

char *default_q1_name = NULL;
char *default_qw_name = NULL;
char *default_q2_name = NULL;
char *default_t2_name = NULL;
char *default_qw_team = NULL;
char *default_qw_skin = NULL;
char *default_q2_skin = NULL;
int default_q1_top_color = 0;
int default_q1_bottom_color = 0;
int default_qw_top_color = 0;
int default_qw_bottom_color = 0;

int default_qw_rate;
int default_q2_rate;
int default_qw_cl_nodelta;
int default_q2_cl_nodelta;
int default_qw_cl_predict;
int default_q2_cl_predict;
int default_noaim;
int default_qw_noskins;
int default_q2_noskins;
int default_b_switch;
int default_w_switch;

int pushlatency_mode;
int pushlatency_value;

int default_nosound;
int default_nocdaudio;

int show_hostnames;
int show_default_port;

int serverlist_countbots;

int default_terminate;
int default_launchinfo;
int default_prelaunchexec;
int default_save_lists;
int default_save_srvinfo;
int default_save_plrinfo;
int default_auto_favorites;
int default_auto_maps;
int skip_startup_mapscan;
int default_refresh_sorts;
int default_refresh_on_update;
int default_resolve_on_update;
int default_show_only_configured_games;
char* default_icontheme;

int maxretries;
int maxsimultaneous;
char* qstat_srcip;
unsigned short qstat_srcport_low;
unsigned short qstat_srcport_high;
gboolean qstat_srcport_changed;
gboolean qstat_srcip_changed;

int sound_enable;
char *sound_player = NULL;
char *sound_xqf_start = NULL;
char *sound_xqf_quit = NULL;
char *sound_update_done = NULL;
char *sound_refresh_done = NULL;
char *sound_stop = NULL;
char *sound_server_connect = NULL;
char *sound_redial_success = NULL;

static int pref_q1_top_color;
static int pref_q1_bottom_color;
static int pref_qw_top_color;
static int pref_qw_bottom_color;
static int pref_b_switch;
static int pref_w_switch;
static int pref_qw_noskins;
static int pref_q2_noskins;
static char *pref_qw_skin;
static char *pref_q2_skin;

static GtkWidget *games_list;
static GtkWidget *games_notebook;
static GtkWidget *pref_notebook;

static GtkWidget *rate_spinner[2];
static GtkWidget *cl_nodelta_check_button[2];
static GtkWidget *cl_predict_check_button[2];
static GtkWidget *noaim_check_button;
static GtkWidget *nosound_check_button;
static GtkWidget *nocdaudio_check_button;
static GtkWidget *name_q1_entry;
static GtkWidget *name_qw_entry;
static GtkWidget *name_q2_entry;
static GtkWidget *name_t2_entry;
static GtkWidget *team_qw_entry;
static GtkWidget *q1_skin_preview = NULL;
static GtkWidget *qw_skin_preview = NULL;
static GtkWidget *q2_skin_preview = NULL;
static GtkWidget *qw_skin_combo;
static GtkWidget *q2_skin_combo;
static GtkWidget *q1_top_color_button;
static GtkWidget *q1_bottom_color_button;
static GtkWidget *qw_top_color_button;
static GtkWidget *qw_bottom_color_button;

static GtkWidget *terminate_check_button;
static GtkWidget *launchinfo_check_button;
static GtkWidget *prelaunchexec_check_button;
static GtkWidget *save_lists_check_button;
static GtkWidget *save_srvinfo_check_button;
static GtkWidget *save_plrinfo_check_button;
static GtkWidget *auto_favorites_check_button;
static GtkWidget *auto_maps_check_button;

static GtkWidget *show_hostnames_check_button;
static GtkWidget *show_defport_check_button;
static GtkWidget *countbots_check_button;
static GtkWidget *refresh_sorts_check_button;
static GtkWidget *refresh_on_update_check_button;
static GtkWidget *resolve_on_update_check_button;
static GtkWidget *show_only_configured_games_check_button;

static GtkWidget *pushlatency_mode_radio_buttons[3];
static GtkWidget *pushlatency_value_spinner;

static GtkWidget *maxretries_spinner;
static GtkWidget *maxsimultaneous_spinner;
static GtkWidget *qstat_srcip_entry;
static GtkWidget *qstat_srcport_entry_low;
static GtkWidget *qstat_srcport_entry_high;

static GtkWidget *sound_enable_check_button;

static GtkWidget *sound_player_file_dialog_button;

static GtkWidget *sound_xqf_start_file_dialog_button;
static GtkWidget *sound_xqf_quit_file_dialog_button;
static GtkWidget *sound_update_done_file_dialog_button;
static GtkWidget *sound_refresh_done_file_dialog_button;
static GtkWidget *sound_stop_file_dialog_button;
static GtkWidget *sound_server_connect_file_dialog_button;
static GtkWidget *sound_redial_success_file_dialog_button;

static guchar *q1_skin_data = NULL;
static int q1_skin_is_valid = TRUE;

static guchar *qw_skin_data = NULL;
static int qw_skin_is_valid = TRUE;

static guchar *q2_skin_data = NULL;
static int q2_skin_is_valid = TRUE;

static GtkWidget *color_menu = NULL;

static GtkWidget *custom_args_add_button[UNKNOWN_SERVER];
static GtkWidget *custom_args_entry_game[UNKNOWN_SERVER];
static GtkWidget *custom_args_entry_args[UNKNOWN_SERVER];
static int current_row = -1;

struct generic_prefs {
	char *pref_dir;
	char *real_dir;
	GtkWidget *dir_entry;
	GtkWidget *cmd_entry;
	GtkWidget *cfg_combo;
	GtkWidget *game_button;
	GtkTreePath *tree_path;
	// function for adding game specific tabs to notebook
	void (*add_options_to_notebook) (GtkWidget *notebook, enum server_type type);

	// game specific data
	GData* games_data;

	GSList *custom_args;
} *genprefs = NULL;

enum {
	Q3_PREF_SETFS_GAME = 0x01,
	Q3_PREF_PB         = 0x02,
	Q3_PREF_CONSOLE    = 0x04,
};

/* Quake 3 settings */
struct q3_common_prefs_s {
	const char** protocols;
	const char* defproto;
	unsigned flags;
	GtkWidget *setfs_gamebutton;
	GtkWidget *set_punkbusterbutton;
	GtkWidget *proto_entry;
	GtkWidget *console_button;
};

// change config_get_string below too!
static const char* q3_masterprotocols[] = {
	"68 - v1.32",
	"67 - v1.31",
	"66 - v1.30",
	"48 - v1.27",
	"46 - v1.25",
	"45 - v1.17",
	"43 - v1.11",
	NULL
};

static struct q3_common_prefs_s q3_prefs = {
	.protocols = q3_masterprotocols,
	.defproto  = "68",
	.flags     = Q3_PREF_SETFS_GAME | Q3_PREF_PB,
};

static GtkWidget *pass_memory_options_button;
static GtkWidget *com_hunkmegs_spinner;
static GtkWidget *com_soundmegs_spinner;
static GtkWidget *com_zonemegs_spinner;


// change config_get_string below too!
static const char* wo_masterprotocols[] = {
	"60 - v1.4",
	"59 - v1.32",
	"58 - v1.3",
	"57 - retail",
	"56 - test2",
	"55 - test1",
	NULL
};

static struct q3_common_prefs_s wo_prefs = {
	.protocols = wo_masterprotocols,
	.defproto  = "60",
	.flags     = Q3_PREF_SETFS_GAME | Q3_PREF_PB,
};

// change config_get_string below too!
static const char* woet_masterprotocols[] = {
	"84 - v2.60",
	"83 - v2.56",
	"82 - v2.55 (Release)",
	"71 - v2.32 (test)",
	NULL
};

static struct q3_common_prefs_s woet_prefs = {
	.protocols = woet_masterprotocols,
	.defproto  = "84",
	.flags     = Q3_PREF_SETFS_GAME | Q3_PREF_PB,
};

static const char* etl_masterprotocols[] = {
	"84 - v2.71",
	NULL
};

static struct q3_common_prefs_s etl_prefs = {
	.protocols = etl_masterprotocols,
	.defproto  = "84",
	.flags     = Q3_PREF_SETFS_GAME | Q3_PREF_PB,
};

static const char* ef_masterprotocols[] = {
	"24",
	NULL
};

static struct q3_common_prefs_s ef_prefs = {
	.protocols = ef_masterprotocols,
	.defproto  = "24",
};

static const char* cod_masterprotocols[] = {
	"6 - v1.5",
	"5 - v1.4",
	"1 - retail",
	NULL
};

static struct q3_common_prefs_s cod_prefs = {
	.protocols = cod_masterprotocols,
	.defproto  = "6",
	.flags     = Q3_PREF_PB,
};

static const char* coduo_masterprotocols[] = {
	"22 - v1.51",
	"21 - v1.41",
	NULL
};

static struct q3_common_prefs_s coduo_prefs = {
	.protocols = coduo_masterprotocols,
	.defproto  = "22",
	.flags     = Q3_PREF_PB,
};

static const char* cod2_masterprotocols[] = {
	"118 - v1.3",
	"117 - v1.2",
	"116 - v1.1",
	"115 - v1.0",
	NULL
};

static struct q3_common_prefs_s cod2_prefs = {
	.protocols = cod2_masterprotocols,
	.defproto  = "118",
	.flags     = Q3_PREF_PB,
};

static const char* cod4_masterprotocols[] = {
	"6 - v1.7",
	"5 - v1.6",
	"4 - v1.5",
	"3 - v1.4",
	"2 - v1.3",
	"1 - v1.0",
	NULL
};

static struct q3_common_prefs_s cod4_prefs = {
	.protocols = cod4_masterprotocols,
	.defproto  = "6",
	.flags     = Q3_PREF_PB,
};

static const char* codwaw_masterprotocols[] = {
	"101 - current",
	NULL
};

static struct q3_common_prefs_s codwaw_prefs = {
	.protocols = codwaw_masterprotocols,
	.defproto  = "101",
	.flags     = Q3_PREF_PB,
};

static const char* jk2_masterprotocols[] = {
	"16 - v1.04",
	"15 - v1.02",
	NULL
};

static struct q3_common_prefs_s jk2_prefs = {
	.protocols = jk2_masterprotocols,
	.defproto  = "16",
};

static const char* jk3_masterprotocols[] = {
	"26 - v1.01",
	"25 - v1.0",
	NULL
};

static struct q3_common_prefs_s jk3_prefs = {
	.protocols = jk3_masterprotocols,
	.defproto  = "26",
};

static const char* sof2_masterprotocols[] = {
	"2004 - SOF2 1.02",
	"2003 - SOF2 1.01",
	"2002 - SOF2 1.00",
	NULL
};

static struct q3_common_prefs_s sof2_prefs = {
	.protocols = sof2_masterprotocols,
	.defproto  = "2004",
};

static const char* doom3_masterprotocols[] = {
	"auto",
	"1.40 - 1.3.1302",
	"1.35 - 1.1.1282",
	"1.33 - retail",
	NULL
};

static struct q3_common_prefs_s doom3_prefs = {
	.protocols = doom3_masterprotocols,
	.defproto  = "auto",
	.flags     = Q3_PREF_PB,
};

static const char* quake4_masterprotocols[] = {
	"auto",
	"0 - any",
	"2.63 - German retail",
	"2.62 - retail",
	"2.32837 - 1.2 German",
	"2.69 - 1.2",
	"2.32853 - 1.4.2 German",
	"2.85 - 1.4.2",
	NULL
};

static struct q3_common_prefs_s q4_prefs = {
	.protocols = quake4_masterprotocols,
	.defproto  = "auto",
	.flags     = Q3_PREF_PB | Q3_PREF_CONSOLE,
};

static const char* etqw_masterprotocols[] = {
	"auto",
	"0 - any",
	"10.16 - retail",
	"10.17 - 1.2",
	NULL
};

static struct q3_common_prefs_s etqw_prefs = {
	.protocols = etqw_masterprotocols,
	.defproto  = "auto",
	.flags     = Q3_PREF_PB | Q3_PREF_CONSOLE,
};

static const char* nexuiz_masterprotocols[] = {
	"3",
	NULL
};

static struct q3_common_prefs_s nexuiz_prefs = {
	.protocols = nexuiz_masterprotocols,
	.defproto  = "3",
};

static const char* xonotic_masterprotocols[] = {
	"3",
	NULL
};

static struct q3_common_prefs_s xonotic_prefs = {
	.protocols = xonotic_masterprotocols,
	.defproto  = "3",
};

static const char* cdiesel_masterprotocols[] = {
	"auto",
	"11908000 - v0.1.0.82",
	NULL
};

static struct q3_common_prefs_s cdiesel_prefs = {
	.protocols = cdiesel_masterprotocols,
	.defproto  = "11908000",
};

static const char* warfork_masterprotocols[] = {
	"auto",
	"26 - v2.15",
	NULL
};

static struct q3_common_prefs_s warfork_prefs = {
	.protocols = warfork_masterprotocols,
	.defproto  = "26",
};

static const char* warsow_masterprotocols[] = {
	"auto",
	"22 - v2.1.0",
	"21 - v2.0",
	"20 - v1.50",
	"15 - v1.02",
	"10 - v0.40",
	"9 - v0.32",
	"8 - v0.2",
	"7 - v0.1",
	"6 - v0.072",
	NULL
};

static struct q3_common_prefs_s warsow_prefs = {
	.protocols = warsow_masterprotocols,
	.defproto  = "22",
};

static const char* tremulous_masterprotocols[] = {
	"auto",
	"69 - v1.1.0",
	NULL
};

static struct q3_common_prefs_s tremulous_prefs = {
	.protocols = tremulous_masterprotocols,
	.defproto  = "69",
};

static const char* tremulousgpp_masterprotocols[] = {
	"auto",
	"70 - GPP",
	NULL
};

static struct q3_common_prefs_s tremulousgpp_prefs = {
	.protocols = tremulousgpp_masterprotocols,
	.defproto  = "70",
};

static const char* unvanquished_masterprotocols[] = {
	"auto",
	"86",
	NULL
};

static struct q3_common_prefs_s unvanquished_prefs = {
	.protocols = unvanquished_masterprotocols,
	.defproto  = "86",
};

static const char* openarena_masterprotocols[] = {
	"auto",
	"71 - v0.8.8",
	"70 - v0.8.0",
	NULL
};

static struct q3_common_prefs_s openarena_prefs = {
	.protocols = openarena_masterprotocols,
	.defproto  = "71",
};

static const char* q3rally_masterprotocols[] = {
	"auto",
	"71 - v0.0.0.3",
	NULL
};

static struct q3_common_prefs_s q3rally_prefs = {
	.protocols = q3rally_masterprotocols,
	.defproto  = "71",
};

static const char* wop_masterprotocols[] = {
	"auto",
	"71",
	"70",
	"69",
	"68",
	NULL
};

static struct q3_common_prefs_s wop_prefs = {
	.protocols = wop_masterprotocols,
	.defproto  = "71",
};

static const char* iourt_masterprotocols[] = {
	"auto",
	"68 - v1.35",
	NULL
};

static struct q3_common_prefs_s iourt_prefs = {
	.protocols = iourt_masterprotocols,
	.defproto  = "68",
};

static const char* reaction_masterprotocols[] = {
	"auto",
	"68",
	NULL
};

static struct q3_common_prefs_s reaction_prefs = {
	.protocols = reaction_masterprotocols,
	.defproto  = "68",
};

static const char* smokinguns_masterprotocols[] = {
	"auto",
	"68 - v1.1",
	NULL
};

static struct q3_common_prefs_s smokinguns_prefs = {
	.protocols = smokinguns_masterprotocols,
	.defproto  = "68",
};

static const char* zeq2lite_masterprotocols[] = {
	"auto",
	"75",
	NULL
};

static struct q3_common_prefs_s zeq2lite_prefs = {
	.protocols = zeq2lite_masterprotocols,
	.defproto  = "75",
};

static const char* turtlearena_masterprotocols[] = {
	"auto",
	"11 - v0.7",
	NULL
};

static struct q3_common_prefs_s turtlearena_prefs = {
	.protocols = turtlearena_masterprotocols,
	.defproto  = "11",
};

static const char* alienarena_masterprotocols[] = {
	"auto",
	"34 - v7.66",
	NULL
};

static struct q3_common_prefs_s alienarena_prefs = {
	.protocols = alienarena_masterprotocols,
	.defproto  = "34",
};

static struct q3_common_prefs_s* get_pref_widgets_for_game(enum server_type type);

static void game_file_dialog(enum server_type type);
static void game_dir_dialog(enum server_type type);
static void game_file_activate_callback(enum server_type type);

static inline int compare_slist_strings (gconstpointer str1, gconstpointer str2) {
	int res;

	if (str1) {
		if (str2) {
			res = g_ascii_strcasecmp (str1, str2);
			if (!res) {
				res = strcmp (str1, str2);
			}
		}
		else {
			res = 1;
		}
	}
	else {
		res = (str2)? -1 : 0;
	}
	return res;
}


static void get_new_defaults_for_game (enum server_type type) {
	struct game *g = &games[type];
	struct generic_prefs *prefs = &genprefs[type];
	char buf[256];

	debug (5, "get_new_defaults_for_game(%d)",type);

	if (prefs->cmd_entry) {
		if (g->cmd) g_free(g->cmd);
		g->cmd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (prefs->cmd_entry)));
	}

	if (prefs->dir_entry) {
		if (g->dir) g_free(g->dir);
		g->dir = strdup_strip (gtk_entry_get_text (GTK_ENTRY (prefs->dir_entry)));

		if (g->real_dir) g_free(g->real_dir);
		g->real_dir = expand_tilde (g->dir);

		if (prefs->pref_dir) g_free(prefs->pref_dir);
		prefs->pref_dir = NULL;

		if (prefs->real_dir) g_free(prefs->real_dir);
		prefs->real_dir = NULL;
	}

	if (prefs->cfg_combo) {
		if (g->game_cfg) g_free(g->game_cfg);
		g->game_cfg = strdup_strip (gtk_entry_get_text (combo_get_entry (prefs->cfg_combo)));
	}

	g_snprintf (buf, sizeof(buf), "/" CONFIG_FILE "/Game: %s", type2id (type));
	config_push_prefix (buf);

	if (g->cmd)
		config_set_string ("cmd", g->cmd);
	else
		config_clean_key ("cmd");

	if (g->dir)
		config_set_string ("dir", g->dir);
	else
		config_clean_key ("dir");

	if (g->game_cfg)
		config_set_string ("custom cfg", g->game_cfg);
	else
		config_clean_key ("custom cfg");


	// Set custom arguments
	if (g->custom_args) {
		g_slist_free (g->custom_args);
	}

	g->custom_args = g_slist_sort (prefs->custom_args, compare_slist_strings);

	{
		int i = 0;
		char *str = NULL;
		int isdefault = FALSE;
		GSList *list = g->custom_args;

		for (i = 0;list; list = g_slist_next(list), ++i) {
			g_snprintf (buf, sizeof(buf), "custom_arg%d", i);

			config_set_string (buf, (char *) list->data);
		}

		// Clear remaining existing arguments
		g_snprintf (buf, sizeof(buf), "custom_arg%d", i);

		str = config_get_string_with_default (buf,&isdefault);
		while (!isdefault) {
			config_clean_key (buf);

			++i;
			g_snprintf (buf, sizeof(buf), "custom_arg%d", i);
			g_free(str);
			str = config_get_string_with_default (buf,&isdefault);
		}
		g_free(str);
	}

	if (g->update_prefs) {
		g->update_prefs(g);
	}

	config_pop_prefix();
}


static void load_game_defaults (enum server_type type) {
	struct game *g = &games[type];
	char str[256];

	int isdefault = FALSE;
	char *str2;
	char conf[64];
	int j;

	g_snprintf (str, 256, "/" CONFIG_FILE "/Game: %s", type2id (type));
	config_push_prefix (str);

	g_free(g->cmd);
	g->cmd = config_get_string("cmd");

	g_free(g->dir);
	g->dir = config_get_string("dir");

	g_free(g->real_dir);
	g->real_dir = expand_tilde (g->dir);

	g_free(g->game_cfg);
	g->game_cfg = config_get_string("custom cfg");

	g->real_home = expand_tilde (g->default_home);

	// Load custom arguments
	j = 0;
	g_snprintf (conf, 64, "custom_arg%d", j);
	str2 = config_get_string_with_default (conf,&isdefault);
	while (!isdefault) {
		g->custom_args = g_slist_append(g->custom_args, str2);
		debug(2,"game: %s: %s=%s",type2id (type), conf,str2);

		++j;
		g_snprintf (conf, sizeof(conf), "custom_arg%d", j);
		g_free(str2);
		str2 = config_get_string_with_default (conf,&isdefault);
	}
	g_free(str2);

	config_pop_prefix();
}

void q1_update_prefs (struct game* g) {
	char* str;

	if (pref_q1_top_color != default_q1_top_color) {
		config_set_int ("top", default_q1_top_color = pref_q1_top_color);
	}

	if (pref_q1_bottom_color != default_q1_bottom_color) {
		config_set_int ("bottom", default_q1_bottom_color = pref_q1_bottom_color);
	}

	str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (name_q1_entry)));
	if (str == NULL ||  default_q1_name == NULL || (default_q1_name && strcmp (str, default_q1_name))) {
		if (default_q1_name) {
			g_free(default_q1_name);
		}
		default_q1_name = str;
		config_set_string ("player name", (str)? str : "");
	}
	str=NULL;
}

/* QuakeWorld (some network settings are used by Q2) */
void qw_update_prefs (struct game* g) {
	char* str;
	int i;

	str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (name_qw_entry)));
	if (str == NULL ||  default_qw_name == NULL || (default_qw_name && strcmp (str, default_qw_name))) {
		if (default_qw_name) {
			g_free(default_qw_name);
		}
		default_qw_name = str;
		config_set_string ("player name", (str)? str : "");
	}
	str=NULL;

	str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (team_qw_entry)));
	if (str == NULL || default_qw_team == NULL || (default_qw_team && strcmp (str, default_qw_team))) {
		if (default_qw_team) {
			g_free(default_qw_team);
		}
		default_qw_team = str;
		config_set_string ("team", (str)? str : "");
	}
	str=NULL;

	if (pref_qw_top_color != default_qw_top_color) {
		config_set_int ("top", default_qw_top_color = pref_qw_top_color);
	}

	if (pref_qw_bottom_color != default_qw_bottom_color) {
		config_set_int ("bottom", default_qw_bottom_color = pref_qw_bottom_color);
	}

	if (pref_qw_skin == NULL || (default_qw_skin && strcmp (pref_qw_skin, default_qw_skin))) {
		if (default_qw_skin) {
			g_free(default_qw_skin);
		}
		default_qw_skin = pref_qw_skin;
		config_set_string ("skin", (pref_qw_skin)? pref_qw_skin : "");
	}
	pref_qw_skin = NULL;

	i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (rate_spinner[0]));
	if (i != default_qw_rate) {
		config_set_int ("rate", default_qw_rate = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cl_nodelta_check_button[0]));
	if (i != default_qw_cl_nodelta) {
		config_set_int ("cl_nodelta", default_qw_cl_nodelta = i);
	}

	i = 1 - gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cl_predict_check_button[0]));
	if (i != default_qw_cl_predict) {
		config_set_int ("cl_predict", default_qw_cl_predict = i);
	}

	if (pref_qw_noskins != default_qw_noskins) {
		config_set_int ("noskins", default_qw_noskins = pref_qw_noskins);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (noaim_check_button));
	if (i != default_noaim) {
		config_set_int ("noaim", default_noaim = i);
	}

	for (i = 0; i < 3; i++) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pushlatency_mode_radio_buttons[i]))) {
			if (i != pushlatency_mode) {
				config_set_int  ("pushlatency mode", pushlatency_mode = i);
			}
			break;
		}
	}

	i = gtk_spin_button_get_value_as_int (
			GTK_SPIN_BUTTON (pushlatency_value_spinner));
	if (i != pushlatency_value) {
		config_set_int ("pushlatency value", pushlatency_value = i);
	}

	if (pref_b_switch != default_b_switch) {
		config_set_int ("b_switch", default_b_switch = pref_b_switch);
	}

	if (pref_w_switch != default_w_switch) {
		config_set_int ("w_switch", default_w_switch = pref_w_switch);
	}
}

void q2_update_prefs (struct game* g) {
	char* str;
	int i;

	if (pref_q2_skin == NULL ||
			(default_q2_skin && strcmp (pref_q2_skin, default_q2_skin))) {
		if (default_q2_skin) g_free(default_q2_skin);
		default_q2_skin = pref_q2_skin;
		config_set_string ("skin", (pref_q2_skin)? pref_q2_skin : "");
	}
	pref_q2_skin = NULL;

	str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (name_q2_entry)));
	if (str == NULL || default_q2_name == NULL ||
			(default_q2_name && strcmp (str, default_q2_name))) {
		if (default_q2_name) g_free(default_q2_name);
		default_q2_name = str;
		config_set_string ("player name", (str)? str : "");
	}
	str=NULL;

	i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (rate_spinner[1]));
	if (i != default_q2_rate) {
		config_set_int ("rate", default_q2_rate = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cl_nodelta_check_button[1]));
	if (i != default_q2_cl_nodelta) {
		config_set_int ("cl_nodelta", default_q2_cl_nodelta = i);
	}

	i = 1 - gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (cl_predict_check_button[1]));
	if (i != default_q2_cl_predict) {
		config_set_int ("cl_predict", default_q2_cl_predict = i);
	}

	if (pref_q2_noskins != default_q2_noskins) {
		config_set_int ("noskins", default_q2_noskins = pref_q2_noskins);
	}
}

void q3_update_prefs_common (struct game* g) {
	char* str, *str1;
	const char* strold;
	int i;
	enum server_type type = g->type;
	struct q3_common_prefs_s* w;

	w = get_pref_widgets_for_game(type);
	g_return_if_fail(w != NULL);

	str = strdup_strip (gtk_entry_get_text (combo_get_entry (w->proto_entry)));
	if (str)
	{
		// locate first space and mark it as str's end
		str1 = strchr(str,' ');
		if (str1) *str1='\0';

		strold = game_get_attribute(type,"masterprotocol");
		if (strcmp(str, strold?strold:"")) {
			game_set_attribute(type,"masterprotocol",strdup_strip(str));
			config_set_string ("protocol", (str)? str : "");
		}
		g_free(str);
		str=NULL;
	}

	if (w->setfs_gamebutton) {
		int o;
		i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w->setfs_gamebutton));
		o = str2bool(game_get_attribute(type,"setfs_game"));
		if (i != o) {
			config_set_bool ("setfs_game", i);
			game_set_attribute(type,"setfs_game",g_strdup(bool2str(i)));
		}
	}

	if (w->set_punkbusterbutton) {
		int o;
		i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w->set_punkbusterbutton));
		o = str2bool(game_get_attribute(type,"set_punkbuster"));
		if (i != o) {
			config_set_bool ("set_punkbuster", i);
			game_set_attribute(type,"set_punkbuster",g_strdup(bool2str(i)));
		}
	}

	if (w->console_button) {
		int o;
		i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w->console_button));
		o = str2bool(game_get_attribute(type,"enable_console"));
		if (i != o) {
			config_set_bool ("enable_console", i);
			game_set_attribute(type,"enable_console",g_strdup(bool2str(i)));
		}
	}
}

void q3_update_prefs (struct game* g) {
	int i;
	enum server_type type = g->type;
	struct q3_common_prefs_s* w;

	w = get_pref_widgets_for_game(type);
	g_return_if_fail(w != NULL);

	q3_update_prefs_common(g);

	if ( type == Q3_SERVER ) {
		i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (pass_memory_options_button));
		config_set_bool ("pass_memory_options", i);
		game_set_attribute(type,"pass_memory_options",g_strdup(bool2str(i)));

		i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(com_hunkmegs_spinner));
		config_set_int ("com_hunkmegs", i);
		game_set_attribute(type,"com_hunkmegs",g_strdup_printf("%d",i));

		i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(com_soundmegs_spinner));
		config_set_int ("com_soundmegs", i);
		game_set_attribute(type,"com_soundmegs",g_strdup_printf("%d",i));

		i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON(com_zonemegs_spinner));
		config_set_int ("com_zonemegs", i);
		game_set_attribute(type,"com_zonemegs",g_strdup_printf("%d",i));
	}
}

static void doom3_detect_version(struct game* g) {
	FILE* f = NULL;
	char* verinfo = NULL;
	// const char* attrproto;
	char line[64];

	debug(3, "cmd: %s, dir: %s", g->cmd, g->real_dir);

	// attrproto = game_get_attribute(g->type,"masterprotocol");

	if (g->real_home && *g->real_home) {
		verinfo = file_in_dir (g->real_home, "version.info");
		f = fopen(verinfo, "r");
	}

	if (!f) {
		g_free(verinfo);
		verinfo = file_in_dir (g->real_dir, "version.info");
		f = fopen(verinfo, "r");
	}

	if (!f) {
		goto out;
	}

	if (!fgets(line, sizeof(line), f)) {
		goto out;
	}

	if (!fgets(line, sizeof(line), f)) {
		goto out;
	}

	if (strlen(line) && line[strlen(line)-1] == '\n') {
		line[strlen(line)-1] = '\0';
	}

	debug(3, "detected %s protocol version %s", g->name, line);

	game_set_attribute(g->type, "_masterprotocol", g_strdup(line));

out:
	if (f) {
		fclose(f);
	}
	g_free(verinfo);
}

void doom3_update_prefs (struct game* g) {
	q3_update_prefs_common(g);
	doom3_detect_version(g);
}

void quake4_update_prefs (struct game* g) {
	q3_update_prefs_common(g);
	doom3_detect_version(g);
}

void tribes2_update_prefs (struct game* g) {
	char* str;

	str = strdup_strip (gtk_entry_get_text (GTK_ENTRY (name_t2_entry)));
	if (str == NULL || default_t2_name == NULL || (default_t2_name && strcmp (str, default_t2_name))) {
		if (default_t2_name) {
			g_free(default_t2_name);
		}
		default_t2_name = str;
		config_set_string ("player name", (str)? str : "");
	}
	str=NULL;
}

static void ut2004_detect_cdkey(struct game* g) {
	FILE* f = NULL;
	char* keyfile = NULL;

	debug(2, "cmd: %s, dir: %s, home: %s", g->cmd, g->real_dir, g->real_home);

	keyfile = file_in_dir (g->real_home, "System/cdkey");
	if (keyfile) {
		f = fopen(keyfile, "r");
		if (!f) {
			g_free(keyfile);
			keyfile = NULL;
		}
	}

	if (!keyfile) {
		if (f) {
			fclose(f);
		}
		keyfile = file_in_dir (g->real_dir, "System/cdkey");
		if (keyfile) {
			f = fopen(keyfile, "r");
			if (!f) {
				g_free(keyfile);
				keyfile = NULL;
			}
		}
	}

	if (keyfile || (!keyfile && game_get_attribute(g->type,"cdkey"))) {
		game_set_attribute(g->type, "cdkey", keyfile);
	}

	if (f) {
		fclose(f);
	}
}

void ut2004_update_prefs (struct game* g) {
	ut2004_detect_cdkey(g);
}

static void get_new_defaults (void) {
	int i;

	debug (5, "get_new_defaults()");

	/* Common part of games config */

	config_push_prefix ("/" CONFIG_FILE "/Games Config");

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nosound_check_button));
	if (i != default_nosound) {
		config_set_bool ("nosound", default_nosound = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (nocdaudio_check_button));
	if (i != default_nocdaudio) {
		config_set_bool ("nocdaudio", default_nocdaudio = i);
	}

	config_pop_prefix();

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		get_new_defaults_for_game (i);
	}

	/* Appearance */

	config_push_prefix ("/" CONFIG_FILE "/Appearance");

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (countbots_check_button));
	if (i != serverlist_countbots) {
		config_set_bool ("count bots", serverlist_countbots = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (refresh_sorts_check_button));
	if (i != default_refresh_sorts) {
		config_set_bool ("sort on refresh", default_refresh_sorts = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (refresh_on_update_check_button));
	if (i != default_refresh_on_update) {
		config_set_bool ("refresh on update", default_refresh_on_update = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (resolve_on_update_check_button));
	if (i != default_resolve_on_update) {
		config_set_bool ("resolve on update", default_resolve_on_update = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_only_configured_games_check_button));
	if (i != default_show_only_configured_games) {
		config_set_bool ("show only configured games", default_show_only_configured_games = i);
	}

	config_pop_prefix();

	/* General */

	config_push_prefix ("/" CONFIG_FILE "/General");

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (terminate_check_button));
	if (i != default_terminate) {
		config_set_bool ("terminate", default_terminate = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (launchinfo_check_button));
	if (i != default_launchinfo) {
		config_set_bool ("launchinfo", default_launchinfo = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prelaunchexec_check_button));
	if (i != default_prelaunchexec) {
		config_set_bool ("prelaunchexec", default_prelaunchexec = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_lists_check_button));
	if (i != default_save_lists) {
		config_set_bool ("save lists", default_save_lists = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_srvinfo_check_button));
	if (i != default_save_srvinfo) {
		config_set_bool ("save srvinfo", default_save_srvinfo = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_plrinfo_check_button));
	if (i != default_save_plrinfo) {
		config_set_bool ("save players", default_save_plrinfo = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (auto_favorites_check_button));
	if (i != default_auto_favorites) {
		config_set_bool ("refresh favorites", default_auto_favorites = i);
	}

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (auto_maps_check_button));
	if (i != default_auto_maps) {
		config_set_bool ("search maps", default_auto_maps = i);
	}

	config_pop_prefix();

	/* QStat */

	config_push_prefix ("/" CONFIG_FILE "/QStat");

	i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (maxretries_spinner));
	if (i != maxretries) {
		config_set_int ("maxretires", maxretries = i);
	}

	i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (maxsimultaneous_spinner));
	if (i != maxsimultaneous) {
		config_set_int ("maxsimultaneous", maxsimultaneous = i);
	}

	if (qstat_srcip_changed) {
		if (qstat_srcip) {
			config_set_string ("srcip", qstat_srcip);
		}
		else {
			config_clean_key("srcip");
		}
		qstat_srcip_changed = FALSE;
	}

	if (qstat_srcport_changed) {
		if (qstat_srcport_low) {
			config_set_int ("port_low", qstat_srcport_low);
			config_set_int ("port_high", qstat_srcport_high);
		}
		else {
			config_clean_key("port_low");
			config_clean_key("port_high");
		}
		qstat_srcport_changed = FALSE;
	}

	config_pop_prefix();

	/* Sounds */

	config_push_prefix ("/" CONFIG_FILE "/Sounds");

	i = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (sound_enable_check_button));
	if (i != sound_enable) {
		config_set_bool ("sound_enable", sound_enable = i);
	}


	sound_player = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sound_player_file_dialog_button));

	sound_xqf_start = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_xqf_start_file_dialog_button));
	sound_xqf_quit = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_xqf_quit_file_dialog_button));
	sound_update_done = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_update_done_file_dialog_button));
	sound_refresh_done = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_refresh_done_file_dialog_button));
	sound_stop = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_stop_file_dialog_button));
	sound_server_connect = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_server_connect_file_dialog_button));
	sound_redial_success = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(sound_redial_success_file_dialog_button));

	config_set_string ("sound_player", (sound_player)? sound_player : "");
	config_set_string ("sound_xqf_start", (sound_xqf_start)? sound_xqf_start : "");
	config_set_string ("sound_xqf_quit", (sound_xqf_quit)? sound_xqf_quit : "");
	config_set_string ("sound_update_done", (sound_update_done)? sound_update_done : "");
	config_set_string ("sound_refresh_done", (sound_refresh_done)? sound_refresh_done : "");
	config_set_string ("sound_stop", (sound_stop)? sound_stop : "");
	config_set_string ("sound_server_connect", (sound_server_connect)? sound_server_connect : "");
	config_set_string ("sound_redial_success", (sound_redial_success)? sound_redial_success : "");

	config_pop_prefix();

	/* These are set from chained calls to "activate" callbacks */

	gtk_check_menu_item_set_active (
			GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "view_hostnames_menu_item")),
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_hostnames_check_button)));

	gtk_check_menu_item_set_active (
			GTK_CHECK_MENU_ITEM (gtk_builder_get_object (builder, "view_defport_menu_item")),
			gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (show_defport_check_button)));

	//  i = gtk_notebook_get_current_page (GTK_NOTEBOOK (profile_notebook));
	//  config_set_string ("/" CONFIG_FILE "/Player Profile/game", type2id (i));

	i = gtk_notebook_get_current_page (GTK_NOTEBOOK (games_notebook));
	config_set_string ("/" CONFIG_FILE "/Games Config/game", type2id (i));

	config_sync();
	rc_save();
}

static int check_qstat_source_port() {
	unsigned short low, high;
	const char* val;
	struct in_addr in;

	val = gtk_entry_get_text(GTK_ENTRY(qstat_srcport_entry_low));
	low = atoi(val);

	val = gtk_entry_get_text(GTK_ENTRY(qstat_srcport_entry_high));
	high = atoi(val);

	val = gtk_entry_get_text(GTK_ENTRY(qstat_srcip_entry));
	if (val && *val) {
		if (!inet_aton(val, &in)) {
			dialog_ok(NULL, _("Invalid source IP address"));
			gtk_notebook_set_current_page (GTK_NOTEBOOK (pref_notebook), PREF_PAGE_QSTAT);
			return FALSE;
		}
		else {
			val = inet_ntoa(in);
		}
	}

	if ((low != 0 || high != 0) && (low < 1024 || high < 1024 || low > high)) {
		dialog_ok(NULL, _("Invalid source port range"));
		gtk_notebook_set_current_page (GTK_NOTEBOOK (pref_notebook), PREF_PAGE_QSTAT);
		return FALSE;
	}

	if (val && *val) {
		if (!qstat_srcip || strcmp(qstat_srcip,val)) {
			g_free(qstat_srcip);
			qstat_srcip = g_strdup(val);
			qstat_srcip_changed = TRUE;
		}
	}
	else if (qstat_srcip) {
		g_free(qstat_srcip);
		qstat_srcip = NULL;
		qstat_srcip_changed = TRUE;
	}

	if (qstat_srcport_low != low) {
		qstat_srcport_low = low;
		qstat_srcport_changed = TRUE;
	}
	if (qstat_srcport_high != high) {
		qstat_srcport_high = high;
		qstat_srcport_changed = TRUE;
	}

	return TRUE;
}

// call various verification fuctions, store settings and destroy preferences
// window if everything's fine
static void ok_callback (GtkWidget *widget, GtkWidget* window) {
	if (!check_qstat_source_port()) {
		return;
	}

	if (!check_script_prefs()) {
		return;
	}

	save_script_prefs();

	get_new_defaults();
	gtk_widget_destroy(window);

	// Refresh list of sources on screen in case the 'Show only configured games'
	// setting has changes, or a game command line has been added or removed.
	refresh_source_list();
}

static void update_q1_skin (void) {

	if (q1_skin_preview == NULL) {
		return;     /* no configuration window */
	}

	if (q1_skin_data) {
		g_free(q1_skin_data);
		q1_skin_data = NULL;
	}

	/* Use QuakeWorld 'base' skin */

	q1_skin_data = get_qw_skin ("base", genprefs[QW_SERVER].real_dir);

	if (q1_skin_data || q1_skin_is_valid) {
		draw_qw_skin (q1_skin_preview, q1_skin_data, pref_q1_top_color, pref_q1_bottom_color);
		q1_skin_is_valid = (q1_skin_data)? TRUE : FALSE;
	}
}


static void update_qw_skins (char *initstr) {
	GList *list;
	char *str = NULL;

	if (qw_skin_preview == NULL) {
		return;     /* no configuration window */
	}

	if (qw_skin_data) {
		g_free(qw_skin_data);
		qw_skin_data = NULL;
	}

	list = get_qw_skin_list (genprefs[QW_SERVER].real_dir);

	if (initstr) {
		combo_set_vals (qw_skin_combo, list, initstr);
	}
	else {
		str = g_strdup (gtk_entry_get_text (combo_get_entry (qw_skin_combo)));
		combo_set_vals (qw_skin_combo, list, str);
	}

	if (list) {
		qw_skin_data = get_qw_skin (pref_qw_skin, genprefs[QW_SERVER].real_dir);
		g_list_foreach (list, (GFunc) g_free, NULL);
		g_list_free (list);
	}

	if (str) {
		g_free(str);
	}

	if (qw_skin_data || qw_skin_is_valid) {
		draw_qw_skin (qw_skin_preview, qw_skin_data, pref_qw_top_color, pref_qw_bottom_color);
		qw_skin_is_valid = (qw_skin_data)? TRUE : FALSE;
	}
}


static void update_q2_skins (char *initstr) {
	GList *list;
	char *str = NULL;

	if (q2_skin_preview == NULL) {
		return;     /* no configuration window */
	}

	if (q2_skin_data) {
		g_free(q2_skin_data);
		q2_skin_data = NULL;
	}

	list = get_q2_skin_list (genprefs[Q2_SERVER].real_dir);

	if (initstr) {
		combo_set_vals (q2_skin_combo, list, initstr);
	}
	else {
		str = g_strdup (gtk_entry_get_text (combo_get_entry (q2_skin_combo)));
		combo_set_vals (q2_skin_combo, list, str);
	}

	if (list) {
		q2_skin_data = get_q2_skin (pref_q2_skin, genprefs[Q2_SERVER].real_dir);
		g_list_foreach (list, (GFunc) g_free, NULL);
		g_list_free (list);
	}

	if (str)
		g_free(str);

	if (q2_skin_data || q2_skin_is_valid) {
		draw_q2_skin (q2_skin_preview, q2_skin_data, Q2_SKIN_SCALE);
		q2_skin_is_valid = (q2_skin_data)? TRUE : FALSE;
	}
}


static void update_cfgs (enum server_type type, char *dir, char *initstr) {
	struct generic_prefs *prefs = &genprefs[type];

	GList *cfgs;
	char *str = NULL;

	debug (5, "update_cfgs(%d,%s,%s)",type,dir,initstr);

	if (!prefs->cfg_combo) {
		return;
	}

	cfgs = (*games[type].custom_cfgs) (&games[type], dir, NULL);

	if (initstr) {
		combo_set_vals (prefs->cfg_combo, cfgs, initstr);
	}
	else {
		str = g_strdup (gtk_entry_get_text (combo_get_entry (prefs->cfg_combo)));
		combo_set_vals (prefs->cfg_combo, cfgs, str);
	}

	if (cfgs) {
		g_list_foreach (cfgs, (GFunc) g_free, NULL);
		g_list_free (cfgs);
	}

	if (str) {
		g_free(str);
	}
}


static gboolean dir_entry_activate_callback (GtkWidget *widget, gpointer data) {
	struct generic_prefs *prefs;
	enum server_type type;

	type = GPOINTER_TO_INT(g_object_get_data (G_OBJECT (widget), "user_data"));
	prefs = &genprefs[type];

	debug (3, "type=%d",type);

	if (prefs->pref_dir) g_free(prefs->pref_dir);
	prefs->pref_dir = strdup_strip (gtk_entry_get_text (GTK_ENTRY (prefs->dir_entry)));

	g_free(prefs->real_dir);
	prefs->real_dir = expand_tilde (prefs->pref_dir);

	if (games[type].cmd_or_dir_changed) {
		games[type].cmd_or_dir_changed(&games[type]);
	}

	update_cfgs (type, prefs->real_dir, NULL);

	return FALSE;
}


static void qw_skin_combo_changed_callback (GtkWidget *widget, gpointer data) {
	char *new_skin;

	new_skin = strdup_strip (gtk_entry_get_text (combo_get_entry (qw_skin_combo)));

	if (!pref_qw_skin && !new_skin) {
		return;
	}

	if (pref_qw_skin && new_skin) {
		if (strcmp (pref_qw_skin, new_skin) == 0) {
			g_free(new_skin);
			return;
		}
	}

	if (pref_qw_skin) g_free(pref_qw_skin);
	pref_qw_skin = new_skin;

	if (qw_skin_data) g_free(qw_skin_data);
	qw_skin_data = get_qw_skin (pref_qw_skin, genprefs[QW_SERVER].real_dir);

	if (qw_skin_data || qw_skin_is_valid) {
		draw_qw_skin (qw_skin_preview, qw_skin_data,
				pref_qw_top_color, pref_qw_bottom_color);
		qw_skin_is_valid = (qw_skin_data)? TRUE : FALSE;
	}
}


static GtkWidget *color_button_event_widget = NULL;


static void set_player_color (GtkWidget *widget, int i) {

	if (color_button_event_widget == qw_top_color_button) {
		if (pref_qw_top_color != i) {
			pref_qw_top_color = i;
			set_bg_color (qw_top_color_button, pref_qw_top_color);
			if (qw_skin_is_valid) {
				draw_qw_skin (qw_skin_preview, qw_skin_data,
						pref_qw_top_color, pref_qw_bottom_color);
			}
		}
		return;
	}

	if (color_button_event_widget == qw_bottom_color_button) {
		if (pref_qw_bottom_color != i) {
			pref_qw_bottom_color = i;
			set_bg_color (qw_bottom_color_button, pref_qw_bottom_color);
			if (qw_skin_is_valid) {
				draw_qw_skin (qw_skin_preview, qw_skin_data,
						pref_qw_top_color, pref_qw_bottom_color);
			}
		}
		return;
	}

	if (color_button_event_widget == q1_top_color_button) {
		if (pref_q1_top_color != i) {
			pref_q1_top_color = i;
			set_bg_color (q1_top_color_button, pref_q1_top_color);
			if (q1_skin_is_valid) {
				draw_qw_skin (q1_skin_preview, q1_skin_data,
						pref_q1_top_color, pref_q1_bottom_color);
			}
		}
		return;
	}

	if (color_button_event_widget == q1_bottom_color_button) {
		if (pref_q1_bottom_color != i) {
			pref_q1_bottom_color = i;
			set_bg_color (q1_bottom_color_button, pref_q1_bottom_color);
			if (q1_skin_is_valid) {
				draw_qw_skin (q1_skin_preview, q1_skin_data,
						pref_q1_top_color, pref_q1_bottom_color);
			}
		}
		return;
	}
}


static int color_button_event_callback (GtkWidget *widget, GdkEvent *event) {
	GdkEventButton *bevent;

	if (event->type == GDK_BUTTON_PRESS) {
		bevent = (GdkEventButton *) event;
		color_button_event_widget = widget;

		if (color_menu == NULL) {
			color_menu = create_color_menu (set_player_color);
		}

		gtk_menu_popup (GTK_MENU (color_menu), NULL, NULL, NULL, NULL,
				bevent->button, bevent->time);
		return TRUE;
	}
	return FALSE;
}


static GtkWidget *q1_skin_box_create (void) {
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* Top and Bottom Colors */

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_box_pack_end (GTK_BOX (hbox), table, FALSE, FALSE, 2);

	/* Top (Shirt) Color */

	label = gtk_label_new (_("Top"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	gtk_widget_show (label);

	q1_top_color_button = gtk_button_new_with_label (" ");
	gtk_widget_set_size_request (q1_top_color_button, 40, -1);
	g_signal_connect (G_OBJECT (q1_top_color_button), "event",
			G_CALLBACK (color_button_event_callback), NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), q1_top_color_button,
			1, 2, 0, 1);
	set_bg_color (q1_top_color_button, fix_qw_player_color (pref_q1_top_color));
	gtk_widget_show (q1_top_color_button);

	/* Bottom (Pants) Color */

	label = gtk_label_new (_("Bottom"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	gtk_widget_show (label);

	q1_bottom_color_button = gtk_button_new_with_label (" ");
	gtk_widget_set_size_request (q1_bottom_color_button, 40, -1);
	g_signal_connect (G_OBJECT (q1_bottom_color_button), "event",
			G_CALLBACK (color_button_event_callback), NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), q1_bottom_color_button,
			1, 2, 1, 2);
	set_bg_color (q1_bottom_color_button,
			fix_qw_player_color (pref_q1_bottom_color));
	gtk_widget_show (q1_bottom_color_button);

	gtk_widget_show (table);

	gtk_widget_show (hbox);

	/* Skin Preview  */

	alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);

	q1_skin_preview = gtk_image_new ();
	gtk_container_add (GTK_CONTAINER (frame), q1_skin_preview);
	gtk_widget_show (q1_skin_preview);

	gtk_widget_show (frame);

	gtk_widget_show (alignment);

	gtk_widget_show (vbox);

	return vbox;
}


static GtkWidget *qw_skin_box_create (void) {
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	/* QW Skin ComboBox */

	alignment = gtk_alignment_new (0, 0, 0, 0);
	gtk_box_pack_start(GTK_BOX (hbox), alignment, FALSE, FALSE, 0);

	qw_skin_combo = gtk_combo_box_text_new_with_entry();
	gtk_entry_set_max_length(combo_get_entry(qw_skin_combo), 256);
	gtk_widget_set_size_request(GTK_WIDGET(combo_get_entry(qw_skin_combo)), 112, -1);
	g_signal_connect (G_OBJECT(combo_get_entry(qw_skin_combo)),
			"changed", G_CALLBACK(qw_skin_combo_changed_callback), NULL);
	gtk_container_add(GTK_CONTAINER(alignment), qw_skin_combo);
	gtk_widget_show(qw_skin_combo);

	gtk_widget_show(alignment);

	/* Top and Bottom Colors */

	table = gtk_table_new(2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE (table), 2);
	gtk_table_set_col_spacings(GTK_TABLE (table), 4);
	gtk_box_pack_end(GTK_BOX(hbox), table, FALSE, FALSE, 2);

	/* Top (Shirt) Color */

	label = gtk_label_new(_("Top"));
	gtk_misc_set_alignment(GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	gtk_widget_show(label);

	qw_top_color_button = gtk_button_new_with_label (" ");
	gtk_widget_set_size_request (qw_top_color_button, 40, -1);
	g_signal_connect (G_OBJECT (qw_top_color_button), "event",
			G_CALLBACK (color_button_event_callback), NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), qw_top_color_button,
			1, 2, 0, 1);
	set_bg_color (qw_top_color_button, fix_qw_player_color (pref_qw_top_color));
	gtk_widget_show (qw_top_color_button);

	/* Bottom (Pants) Color */

	label = gtk_label_new (_("Bottom"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	gtk_widget_show (label);

	qw_bottom_color_button = gtk_button_new_with_label (" ");
	gtk_widget_set_size_request (qw_bottom_color_button, 40, -1);
	g_signal_connect (G_OBJECT (qw_bottom_color_button), "event", G_CALLBACK (color_button_event_callback), NULL);
	gtk_table_attach_defaults (GTK_TABLE (table), qw_bottom_color_button, 1, 2, 1, 2);
	set_bg_color (qw_bottom_color_button, fix_qw_player_color (pref_qw_bottom_color));
	gtk_widget_show (qw_bottom_color_button);

	gtk_widget_show (table);

	gtk_widget_show (hbox);

	/* Skin Preview  */

	alignment = gtk_alignment_new (0.5, 0.5, 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (alignment), frame);

	qw_skin_preview = gtk_image_new ();
	gtk_container_add (GTK_CONTAINER (frame), qw_skin_preview);
	gtk_widget_show (qw_skin_preview);

	gtk_widget_show (frame);

	gtk_widget_show (alignment);

	gtk_widget_show (vbox);

	return vbox;
}


static void q2_skin_combo_changed_callback (GtkWidget *widget, gpointer data) {
	char *new_skin;

	new_skin = strdup_strip (gtk_entry_get_text (combo_get_entry (q2_skin_combo)));

	if (!pref_q2_skin && !new_skin) {
		return;
	}

	if (pref_q2_skin && new_skin) {
		if (strcmp (pref_q2_skin, new_skin) == 0) {
			g_free(new_skin);
			return;
		}
	}

	if (pref_q2_skin) g_free(pref_q2_skin);
	pref_q2_skin = new_skin;

	if (q2_skin_data) g_free(q2_skin_data);
	q2_skin_data = get_q2_skin (pref_q2_skin, genprefs[Q2_SERVER].real_dir);

	if (q2_skin_data || q2_skin_is_valid) {
		draw_q2_skin (q2_skin_preview, q2_skin_data, Q2_SKIN_SCALE);
		q2_skin_is_valid = (q2_skin_data)? TRUE : FALSE;
	}
}


static GtkWidget *q2_skin_box_create (void) {
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *alignment;
	GtkWidget *frame;

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* Skin Preview  */

	alignment = gtk_alignment_new(0, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(alignment), frame);

	q2_skin_preview = gtk_image_new();
	gtk_container_add(GTK_CONTAINER(frame), q2_skin_preview);
	gtk_widget_show(q2_skin_preview);

	gtk_widget_show(frame);
	gtk_widget_show(alignment);

	/* Q2 Skin ComboBox */

	alignment = gtk_alignment_new(1.0, 0, 0, 0);
	gtk_box_pack_end(GTK_BOX(hbox), alignment, FALSE, FALSE, 0);

	q2_skin_combo = gtk_combo_box_text_new_with_entry();
	gtk_entry_set_max_length(GTK_ENTRY(combo_get_entry(q2_skin_combo)), 256);
	gtk_widget_set_size_request(GTK_WIDGET(combo_get_entry(q2_skin_combo)), 144, -1);
	g_signal_connect(G_OBJECT(combo_get_entry(q2_skin_combo)), "changed", G_CALLBACK(q2_skin_combo_changed_callback), NULL);
	gtk_container_add(GTK_CONTAINER(alignment), q2_skin_combo);
	gtk_widget_show(q2_skin_combo);

	gtk_widget_show(alignment);

	gtk_widget_show(hbox);

	gtk_widget_show(vbox);

	return vbox;
}


static GtkWidget *player_profile_q1_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *q1_skin;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *label;

	page_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 6);

	hbox = gtk_hbox_new(TRUE, 4);
	gtk_box_pack_start(GTK_BOX(page_vbox), hbox, FALSE, FALSE, 0);

	// Player Name

	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);

	label = gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	name_q1_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (name_q1_entry), 32);
	gtk_widget_set_size_request(name_q1_entry, 96, -1);
	if (default_q1_name) {
		gtk_entry_set_text(GTK_ENTRY(name_q1_entry), default_q1_name);
		gtk_editable_set_position(GTK_EDITABLE(name_q1_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2), name_q1_entry, FALSE, FALSE, 0);
	gtk_widget_show(name_q1_entry);
	gtk_widget_show(hbox2);

	// /Player Name

	/* Q1 Colors */

	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(page_vbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Colors"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(alignment), frame);

	q1_skin = q1_skin_box_create();
	gtk_container_add(GTK_CONTAINER(frame), q1_skin);

	gtk_widget_show(frame);
	gtk_widget_show(alignment);

	gtk_widget_show(hbox);
	gtk_widget_show(page_vbox);

	return page_vbox;
}

static GtkWidget *player_profile_t2_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *label;

	page_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 6);

	hbox = gtk_hbox_new(TRUE, 4);
	gtk_box_pack_start(GTK_BOX(page_vbox), hbox, FALSE, FALSE, 0);

	// Player Name

	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);

	label = gtk_label_new(_("Login name"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	name_t2_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (name_t2_entry), 32);
	gtk_widget_set_size_request(name_t2_entry, 96, -1);
	if (default_t2_name) {
		gtk_entry_set_text(GTK_ENTRY(name_t2_entry), default_t2_name);
		gtk_editable_set_position(GTK_EDITABLE(name_t2_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2), name_t2_entry, FALSE, FALSE, 0);
	gtk_widget_show(name_t2_entry);
	gtk_widget_show(hbox2);

	// /Player Name

	gtk_widget_show(hbox);
	gtk_widget_show(page_vbox);

	return page_vbox;
}

static GtkWidget *player_profile_qw_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *qw_skin;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *label;

	page_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 6);

	hbox = gtk_hbox_new(TRUE, 4);
	gtk_box_pack_start(GTK_BOX(page_vbox), hbox, FALSE, FALSE, 0);

	/* QW Skin */

	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(page_vbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Skin/Colors"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(alignment), frame);

	qw_skin = qw_skin_box_create();
	gtk_container_add(GTK_CONTAINER(frame), qw_skin);

	gtk_widget_show(frame);
	gtk_widget_show(alignment);

	// Player Name

	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 0);

	label = gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	name_qw_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (name_qw_entry), 32);
	gtk_widget_set_size_request(name_qw_entry, 96, -1);
	if (default_qw_name) {
		gtk_entry_set_text(GTK_ENTRY(name_qw_entry), default_qw_name);
		gtk_editable_set_position(GTK_EDITABLE(name_qw_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2), name_qw_entry, FALSE, FALSE, 0);
	gtk_widget_show(name_qw_entry);
	gtk_widget_show(hbox2);

	// /Player Name

	/* QW Team */

	hbox2 = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), hbox2, FALSE, FALSE, 4);

	label = gtk_label_new(_("Team"));
	gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	team_qw_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (team_qw_entry), 32);
	gtk_widget_set_size_request(team_qw_entry, 96, -1);
	if (default_qw_team) {
		gtk_entry_set_text(GTK_ENTRY(team_qw_entry), default_qw_team);
		gtk_editable_set_position(GTK_EDITABLE(team_qw_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox2), team_qw_entry, FALSE, FALSE, 0);
	gtk_widget_show(team_qw_entry);

	gtk_widget_show(hbox2);

	gtk_widget_show(hbox);

	gtk_widget_show(page_vbox);

	return page_vbox;
}


static GtkWidget *player_profile_q2_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *alignment;
	GtkWidget *frame;
	GtkWidget *q2_skin;
	GtkWidget *label;
	GtkWidget *hbox;

	page_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 8);

	// Player Name

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(page_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	name_q2_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (name_q2_entry), 32);
	gtk_widget_set_size_request(name_q2_entry, 96, -1);
	if (default_q2_name) {
		gtk_entry_set_text(GTK_ENTRY(name_q2_entry), default_q2_name);
		gtk_editable_set_position(GTK_EDITABLE(name_q2_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox), name_q2_entry, FALSE, FALSE, 0);
	gtk_widget_show(name_q2_entry);
	gtk_widget_show(hbox);

	// /Player Name

	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_box_pack_start(GTK_BOX(page_vbox), alignment, FALSE, FALSE, 0);

	frame = gtk_frame_new(_("Model/Skin"));
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);
	gtk_container_add(GTK_CONTAINER(alignment), frame);

	q2_skin = q2_skin_box_create();
	gtk_container_add(GTK_CONTAINER(frame), q2_skin);

	gtk_widget_show(frame);
	gtk_widget_show(alignment);

	gtk_widget_show(page_vbox);

	return page_vbox;
}

#if 0
static GtkWidget *player_profile_page(void) {
	GValue hborder = G_VALUE_INIT;
	GtkWidget *page_vbox;
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *game_label;
	char *typestr;
	enum server_type type = QW_SERVER;

	page_vbox = gtk_vbox_new(FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 8);

	hbox = gtk_hbox_new(FALSE, 4);
	gtk_box_pack_start(GTK_BOX(page_vbox), hbox, FALSE, FALSE, 8);

	/* Player Name */

	label = gtk_label_new(_("Name"));
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_widget_show(label);

	name_entry = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (name_entry), 32);
	gtk_widget_set_size_request(name_entry, 96, -1);
	if (default_name) {
		gtk_entry_set_text(GTK_ENTRY(name_entry), default_name);
		gtk_editable_set_position(GTK_EDITABLE(name_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox), name_entry, FALSE, FALSE, 0);
	gtk_widget_show(name_entry);

	gtk_widget_show(hbox);

	profile_notebook = gtk_notebook_new();
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(profile_notebook), GTK_POS_TOP);
	g_value_init (&hborder, G_TYPE_INT);
	g_value_set_int (&hborder, 4);
	g_object_set_property (G_OBJECT (profile_notebook), "tab-hborder", &hborder);
	gtk_box_pack_start(GTK_BOX(page_vbox), profile_notebook, FALSE, FALSE, 0);

	game_label = game_pixmap_with_label(Q1_SERVER);

	page = player_profile_q1_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(profile_notebook), page, game_label);

	game_label = game_pixmap_with_label(QW_SERVER);

	page = player_profile_qw_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(profile_notebook), page, game_label);

	game_label = game_pixmap_with_label(Q2_SERVER);

	page = player_profile_q2_page();
	gtk_notebook_append_page(GTK_NOTEBOOK(profile_notebook), page, game_label);

	typestr = config_get_string("/" CONFIG_FILE "/Player Profile/game");
	if (typestr) {
		type = id2type(typestr);
		g_free(typestr);
	}

	gtk_notebook_set_current_page(GTK_NOTEBOOK(profile_notebook),
			(type == Q2_SERVER)? 2 :(type == Q1_SERVER)? 0 : 1);

	gtk_widget_show(profile_notebook);

	gtk_widget_show(page_vbox);

	return page_vbox;
}
#endif


static char *wb_switch_labels[9] = {
	N_("--- Anything ---"),
	N_("Axe"),
	N_("Shotgun"),
	N_("Super Shotgun"),
	N_("Nailgun"),
	N_("Super Nailgun"),
	N_("Grenade Launcher"),
	N_("Rocket Launcher"),
	N_("ThunderBolt")
};

static void set_w_switch_callback(GtkWidget *widget, gpointer userdata) {
	pref_w_switch = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void set_b_switch_callback(GtkWidget *widget, gpointer userdata) {
	pref_b_switch = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void set_wb_switch_menu(GtkWidget *option_menu) {
	int i;

	for (i = 0; i < 9; i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (option_menu), _(wb_switch_labels[i]));
	}
}


static char *noskins_labels[3] = {
	N_("Use skins"),
	N_("Don\'t use skins"),
	N_("Don\'t download new skins")
};

static void qw_noskins_option_menu_callback (GtkWidget *widget, gpointer userdata) {
	pref_qw_noskins = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void q2_noskins_option_menu_callback (GtkWidget *widget, gpointer userdata) {
	pref_q2_noskins = gtk_combo_box_get_active (GTK_COMBO_BOX (widget));
}

static void set_noskins_menu (GtkWidget *option_menu) {
	int i;

	for (i = 0; i < 3; i++) {
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (option_menu), _(noskins_labels[i]));
	}
}

// fill working directory with result of the directory guess function for first
// token of command line if working directory is empty
static void pref_guess_dir(enum server_type type, const char* cmdline, gboolean interactive) {
	const char* dir_entry = NULL;

	dir_entry = gtk_entry_get_text(GTK_ENTRY(genprefs[type].dir_entry));

	if (dir_entry && *dir_entry && !interactive) {
		return;
	}

	if (cmdline && *cmdline) { // if not empty
		char *guessed_dir = NULL;
		char** cmds = NULL;

		cmds = g_strsplit(cmdline," ",2);

		if (cmds && *cmds && **cmds) {
			guessed_dir = resolve_path(*cmds);
			if (guessed_dir) {
				gtk_entry_set_text(GTK_ENTRY(genprefs[type].dir_entry), guessed_dir);
			}
			g_free(guessed_dir);
			g_strfreev(cmds);

			return;
		}

		g_strfreev(cmds);
	}

	if (interactive) {
		dialog_ok(NULL, _("You must configure a command line first"));
	}
}


/**
 * return true if game 'type' has commands to suggest
 */
static gboolean pref_can_suggest(enum server_type type) {
	return (games[type].command != NULL);
}

/**
 * find game binary in path and fill in command entry
 */
static void pref_suggest_command(enum server_type type) {
	char** files = NULL;
	char* suggested_file = NULL;
	const char* prevcmd = NULL;
	unsigned i = 0;

	files = games[type].command;
	if (!files || !files[0]) {
		return;
	}

	// start suggestion based on last found binary
	prevcmd = gtk_entry_get_text(GTK_ENTRY(genprefs[type].cmd_entry));
	for (i = 0; prevcmd && files[i]; ++i) {
		if (files[i+1] && !strcmp(files[i], prevcmd)) {
			files = files+i+1;
			break;
		}
	}

	suggested_file = find_file_in_path_list_relative(files);
	if (!suggested_file) {
		dialog_ok(_("Game not found"),
				// %s name of a game
				_("%s not found"),
				games[type].name);
		return;
	}

	// gtk entry does the freeing? -- no
	gtk_entry_set_text(GTK_ENTRY(genprefs[type].cmd_entry), suggested_file);

	pref_guess_dir(type, suggested_file, TRUE);

	if (games[type].cmd_or_dir_changed) {
		games[type].cmd_or_dir_changed(&games[type]);
	}

	g_free(suggested_file);

	return;
}

static int custom_args_compare_func (gconstpointer ptr1, gconstpointer ptr2) {
	// ptr1 = entire string
	// ptr2 = game
	gchar *token[2];
	gchar *tmpstr;


	tmpstr = g_strdup((gchar *)ptr1);
	tokenize(tmpstr, token, 2, ",");
	g_free(tmpstr);

	if (strcasecmp(token[0], ptr2) == 0) {
		return (0);
	}
	else {
		return (1);
	}
}

static void add_custom_args_defaults2 (char *str1, char *str2, enum server_type type, gpointer data) {
	char *temp;
	char *temp2[2];

	temp = g_strconcat(str1, ",", str2, NULL);

	temp2[0] = strdup_strip(str1);
	temp2[1] = strdup_strip(str2);

	if (str1 && str2) {
		if (g_slist_find_custom(genprefs[type].custom_args, str1, custom_args_compare_func) == NULL) {
			genprefs[type].custom_args = g_slist_append(genprefs[type].custom_args, g_strdup(temp));
			gtk_clist_append(GTK_CLIST((GtkCList *) data), temp2);
		}
		else {
			dialog_ok(NULL, _("An entry already exists for the game %s.\n\n"
						"A default entry for this game will not be added.\n\n"
						"Delete the entry and try again."), str1);
		}
	}
	g_free(temp);
}


// Add default custom arguments
// TODO: implement a proper generic solution
static void add_custom_args_defaults (GtkWidget *widget, gpointer data) {
	enum server_type type;

	type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_data"));

	switch(type) {

		case UN_SERVER:
			add_custom_args_defaults2("s_SWATGame","-ini=TacticalOps.ini",UN_SERVER, data);
			add_custom_args_defaults2("SFTeamDM","-ini=StrikeForce.ini -userini=SFUser.ini",UN_SERVER, data);
			break;

		case UT2_SERVER:
			add_custom_args_defaults2("DB_DeathBall",
					"-USERLOGO=dbsplash.bmp -INI=DeathBall.ini -USERINI=DBUser.ini",UT2_SERVER, data);
			add_custom_args_defaults2("FragOpsMission",
					"-INI=FragOps.ini -USERINI=FOUser.ini",UT2_SERVER, data);
			break;

		case UT2004_SERVER:
			add_custom_args_defaults2("ROTeamGame", "-mod=RedOrchestra -log=RedOrchestra.log",UT2004_SERVER, data);
			add_custom_args_defaults2("TObjectiveGame", "-mod=Troopers -log=Troopers.log",UT2004_SERVER, data);
			add_custom_args_defaults2("TTeamGame", "-mod=Troopers -log=Troopers.log",UT2004_SERVER, data);
			add_custom_args_defaults2("TCTFGame", "-mod=Troopers -log=Troopers.log",UT2004_SERVER, data);
			add_custom_args_defaults2("AoGameInfo", "-mod=AlienSwarm -log=AlienSwarm.log",UT2004_SERVER, data);
			add_custom_args_defaults2("AoCampaignLobbyGame", "-mod=AlienSwarm -log=AlienSwarm.log",UT2004_SERVER, data);
			break;

		case HL2_SERVER:
			add_custom_args_defaults2("cstrike", "-applaunch 240",HL2_SERVER, data);
			add_custom_args_defaults2("dod", "-applaunch 300",HL2_SERVER, data);
			add_custom_args_defaults2("hl2mp", "-applaunch 320",HL2_SERVER, data);
			break;

		case HL_SERVER:
			add_custom_args_defaults2("cstrike", "-applaunch 10",HL_SERVER, data);
			add_custom_args_defaults2("tfc", "-applaunch 20",HL_SERVER, data);
			add_custom_args_defaults2("dod", "-applaunch 30",HL_SERVER, data);
			add_custom_args_defaults2("dmc", "-applaunch 40",HL_SERVER, data);
			add_custom_args_defaults2("op4", "-applaunch 50",HL_SERVER, data);
			add_custom_args_defaults2("ricochet", "-applaunch 60",HL_SERVER, data);
			add_custom_args_defaults2("valve", "-applaunch 70",HL_SERVER, data);
			add_custom_args_defaults2("czero", "-applaunch 80",HL_SERVER, data);
			break;

		default:
			dialog_ok(NULL, _("There are no defaults for this game"));
			break;
	}
}

static void new_custom_args_callback (GtkWidget *widget, gpointer data) {

	enum server_type type;

	type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_data"));

	current_row = -1;

	gtk_widget_set_sensitive(custom_args_entry_game[type], TRUE);
	gtk_widget_set_sensitive(custom_args_entry_args[type], TRUE);
	gtk_widget_set_sensitive(custom_args_add_button[type], TRUE);

	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_game[type]), "");
	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_args[type]), "");

	gtk_widget_grab_focus(GTK_WIDGET(custom_args_entry_game[type]));
}

static void add_custom_args_callback (GtkWidget *widget, gpointer data) {
	GSList *link;
	int row;
	char *temp[2];
	enum server_type type;

	type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_data"));

	temp[0] = strdup_strip(gtk_entry_get_text(GTK_ENTRY(custom_args_entry_game[type])));
	temp[1] = strdup_strip(gtk_entry_get_text(GTK_ENTRY(custom_args_entry_args[type])));

	if (current_row > -1) {
		row = current_row;

		link = g_slist_nth(genprefs[type].custom_args, current_row);

		genprefs[type].custom_args = g_slist_remove_link(genprefs[type].custom_args, link);

		current_row = -1;
		gtk_clist_remove(GTK_CLIST((GtkCList *) data), row);
	}

	if (temp[0] && temp[1]) {
		if (g_slist_find_custom(genprefs[type].custom_args, temp[0], custom_args_compare_func) == NULL) {
			genprefs[type].custom_args = g_slist_append(genprefs[type].custom_args, g_strconcat(temp[0], ",",temp[1], NULL));

			gtk_clist_append(GTK_CLIST((GtkCList *) data), temp);

			gtk_widget_set_sensitive(custom_args_entry_game[type], FALSE);
			gtk_widget_set_sensitive(custom_args_entry_args[type], FALSE);
			gtk_widget_set_sensitive(custom_args_add_button[type], FALSE);

			gtk_entry_set_text(GTK_ENTRY(custom_args_entry_game[type]), "");
			gtk_entry_set_text(GTK_ENTRY(custom_args_entry_args[type]), "");
		}
		else
			dialog_ok(NULL, _("There is already an entry for this game.\n\nTo modify it, select it from the list, "\
						"modify it\nand then click Add/Update."));
	}
	else
		dialog_ok(NULL, _("You must enter both a game and at least one argument."));

	g_free(temp[0]);
	g_free(temp[1]);
}

static void delete_custom_args_callback (GtkWidget *widget, gpointer data) {
	GSList *link;
	int row;
	enum server_type type;

	type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_data"));

	if (current_row < 0) {
		return;
	}

	row = current_row;

	link = g_slist_nth(genprefs[type].custom_args, current_row);

	genprefs[type].custom_args = g_slist_remove_link(genprefs[type].custom_args, link);

	current_row = -1;
	gtk_clist_remove(GTK_CLIST((GtkCList *) data), row);

	current_row = -1;

	gtk_widget_set_sensitive(custom_args_entry_game[type], FALSE);
	gtk_widget_set_sensitive(custom_args_entry_args[type], FALSE);
	gtk_widget_set_sensitive(custom_args_add_button[type], FALSE);

	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_game[type]), "");
	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_args[type]), "");
}

static void custom_args_clist_select_row_callback (GtkWidget *widget, int row, int column, GdkEventButton *event, GtkCList *clist) {
	enum server_type type;
	GSList* item;
	char* argstr;
	char* game_args[2] = {0} ;

	type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "user_data"));

	current_row = row;

	if (row<0) return;

	item = g_slist_nth(genprefs[type].custom_args, row);

	if (!item) return;

	argstr = g_strdup(item->data);
	tokenize(argstr, game_args, 2, ",");

	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_game[type]), game_args[0]);
	gtk_entry_set_text(GTK_ENTRY(custom_args_entry_args[type]), game_args[1]);

	gtk_widget_set_sensitive(custom_args_entry_game[type], TRUE);
	gtk_widget_set_sensitive(custom_args_entry_args[type], TRUE);
	gtk_widget_set_sensitive(custom_args_add_button[type], TRUE);

	g_free(argstr);
}

static GtkWidget *generic_game_frame (enum server_type type) {
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *page_vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *notebook;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *hbox2 = NULL;
	struct generic_prefs *prefs = &genprefs[type];

	page_vbox = gtk_vbox_new(FALSE, 4);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_ETCHED_OUT);
	label = gtk_label_new(_(games[type].name));
	gtk_container_add(GTK_CONTAINER(frame), label);
	gtk_widget_show(label);

	gtk_box_pack_start(GTK_BOX(page_vbox), frame, FALSE, FALSE, 0);
	gtk_widget_show(frame);

	notebook = gtk_notebook_new();

	vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	label = gtk_label_new(_("Invoking"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	gtk_widget_show(label);

	gtk_box_pack_start(GTK_BOX(page_vbox), notebook, TRUE, TRUE, 0);

	if ((games[type].flags & GAME_CONNECT) == 0) {
		char* message = type == LAN_SERVER? "": _("*** Not Implemented ***");
		label = gtk_label_new(message);
		gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
		gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
		gtk_widget_show(label);

		gtk_widget_show(vbox);
		gtk_widget_show(notebook);
		gtk_widget_show(page_vbox);
		return page_vbox;
	}

	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), custom_args_options_page(type), gtk_label_new(_("Custom Args")));

	// call game specific function to add its options
	if (genprefs[type].add_options_to_notebook) {
		genprefs[type].add_options_to_notebook(notebook, type);
	}


	table = gtk_table_new((games[type].custom_cfgs)? 3 : 2, 2, FALSE);
	gtk_table_set_row_spacings(GTK_TABLE(table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	label = gtk_label_new(_("Command Line"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
			GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 0, 1);
	gtk_widget_show(hbox);

	genprefs[type].cmd_entry = gtk_entry_new();
	if (games[type].cmd) {
		gtk_entry_set_text(GTK_ENTRY(genprefs[type].cmd_entry), games[type].cmd);
		gtk_editable_set_position(GTK_EDITABLE(genprefs[type].cmd_entry), 0);
	}
	g_signal_connect_swapped(G_OBJECT(genprefs[type].cmd_entry), "activate", G_CALLBACK(game_file_activate_callback), GINT_TO_POINTER(type));
	gtk_box_pack_start(GTK_BOX(hbox),genprefs[type].cmd_entry , TRUE, TRUE, 0);
	gtk_widget_show(genprefs[type].cmd_entry);

	button = gtk_button_new_with_label("...");
	g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(game_file_dialog), GINT_TO_POINTER(type));
	gtk_box_pack_start(GTK_BOX(hbox),button , FALSE, FALSE, 3);
	gtk_widget_show(button);

	// translator: button for command suggestion
	button = gtk_button_new_with_label(_("Suggest"));
	g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(pref_suggest_command), GINT_TO_POINTER(type));
	gtk_widget_set_sensitive(button, pref_can_suggest(type));

	gtk_box_pack_start(GTK_BOX(hbox),button , FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(button, _("Searches the path for the game executable"));
	gtk_widget_show(button);


	/////


	label = gtk_label_new(_("Working Directory"));
	gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show(label);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 1, 2);
	gtk_widget_show(hbox);

	genprefs[type].dir_entry = gtk_entry_new();
	if (genprefs[type].pref_dir) {
		gtk_entry_set_text(GTK_ENTRY(genprefs[type].dir_entry), genprefs[type].pref_dir);
		gtk_editable_set_position(GTK_EDITABLE(genprefs[type].dir_entry), 0);
	}
	gtk_box_pack_start(GTK_BOX(hbox),genprefs[type].dir_entry , TRUE, TRUE, 0);

	button = gtk_button_new_with_label("...");
	g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(game_dir_dialog), (gpointer)type);
	gtk_box_pack_start(GTK_BOX(hbox),button , FALSE, FALSE, 3);
	gtk_widget_show(button);

	// translator: button for directory guess
	button = gtk_button_new_with_label(_("Suggest"));
	g_signal_connect_swapped(G_OBJECT(button), "clicked", G_CALLBACK(game_file_activate_callback), (gpointer)type);

	gtk_box_pack_start(GTK_BOX(hbox),button , FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text(button, _("Tries to guess the working directory based on the command line"));
	gtk_widget_show(button);

	if (games[type].custom_cfgs) {
		g_object_set_data(G_OBJECT(genprefs[type].dir_entry), "user_data", (gpointer) type);
		g_signal_connect(G_OBJECT(genprefs[type].dir_entry),
				"activate",
				G_CALLBACK(dir_entry_activate_callback),
				NULL);
		g_signal_connect(G_OBJECT(genprefs[type].dir_entry),
				"focus_out_event",
				G_CALLBACK(dir_entry_activate_callback),
				NULL);
	}
	gtk_widget_show(genprefs[type].dir_entry);

	if (games[type].custom_cfgs) {
		label = gtk_label_new(_("Custom CFG"));
		gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
		gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
		gtk_widget_show(label);

		prefs->cfg_combo = gtk_combo_box_text_new_with_entry();
		gtk_entry_set_max_length(combo_get_entry(prefs->cfg_combo), 256);
		gtk_table_attach_defaults(GTK_TABLE(table), prefs->cfg_combo, 1, 2, 2, 3);
		gtk_widget_show(prefs->cfg_combo);
	}

	// Game specific notes
	if (game_get_attribute(type,"game_notes")) {

		hbox2 = gtk_hbox_new(FALSE, 0);
		gtk_box_pack_end(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);
		gtk_widget_show(hbox2);

		label = gtk_label_new(game_get_attribute(type,"game_notes"));

		gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start(GTK_BOX(hbox2), label, FALSE, FALSE, 0);
		gtk_widget_show(label);
	}

	gtk_widget_show(table);

	gtk_widget_show(vbox);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), 0);

	gtk_widget_show(notebook);
	gtk_widget_show(page_vbox);

	return page_vbox;
}

// Used by user_fix_defaults to add a custom arg


static GtkWidget *custom_args_options_page (enum server_type type) {
	GtkWidget *hbox1;
	GtkWidget *vbox1;
	GtkWidget *scrolledwindow1;
	GtkWidget *arguments_clist;
	GtkWidget *game_label;
	GtkWidget *arguments_label;
	GtkWidget *frame1;
	GtkWidget *hbox2;
	GtkWidget *vbuttonbox1;
	GtkWidget *new_button;
	GtkWidget *delete_button;
	GtkWidget *defaults_button;
	GtkWidget *page_vbox;
	struct game *g;
	int width;

	g = &games[type];

	genprefs[type].custom_args = g_slist_copy(g->custom_args);

	page_vbox = gtk_vbox_new(FALSE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(page_vbox), 8);

	hbox1 = gtk_hbox_new(FALSE, 0);
	g_object_ref(G_OBJECT(hbox1));
	g_object_set_data_full(G_OBJECT(page_vbox), "hbox1", hbox1, (GDestroyNotify) g_object_unref);
	gtk_widget_show(hbox1);
	gtk_container_add(GTK_CONTAINER(page_vbox), hbox1);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 3);

	vbox1 = gtk_vbox_new(FALSE, 0);
	g_object_ref(G_OBJECT(vbox1));
	g_object_set_data_full(G_OBJECT(page_vbox), "vbox1", vbox1, (GDestroyNotify) g_object_unref);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox1, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox1), 2);

	scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
	g_object_ref(G_OBJECT(scrolledwindow1));
	g_object_set_data_full(G_OBJECT(page_vbox), "scrolledwindow1", scrolledwindow1, (GDestroyNotify) g_object_unref);
	gtk_widget_show(scrolledwindow1);
	gtk_box_pack_start(GTK_BOX(vbox1), scrolledwindow1, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 2);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	arguments_clist = gtk_clist_new(2);
	g_object_ref(G_OBJECT(arguments_clist));
	g_object_set_data_full(G_OBJECT(page_vbox), "arguments_clist", arguments_clist, (GDestroyNotify) g_object_unref);
	gtk_widget_show(arguments_clist);
	gtk_container_add(GTK_CONTAINER(scrolledwindow1), arguments_clist);
	gtk_clist_column_titles_show(GTK_CLIST(arguments_clist));
	g_signal_connect(G_OBJECT(arguments_clist), "select_row", G_CALLBACK(custom_args_clist_select_row_callback), arguments_clist);
	g_object_set_data(G_OBJECT(arguments_clist), "user_data", (gpointer) type);

	game_label = gtk_label_new(_("Game"));
	g_object_ref(G_OBJECT(game_label));
	g_object_set_data_full(G_OBJECT(page_vbox), "game_label", game_label, (GDestroyNotify) g_object_unref);
	gtk_label_set_justify(GTK_LABEL(game_label), GTK_JUSTIFY_LEFT);
	gtk_widget_show(game_label);
	gtk_clist_set_column_widget(GTK_CLIST(arguments_clist), 0, game_label);

	arguments_label = gtk_label_new(_("Arguments"));
	g_object_ref(G_OBJECT(arguments_label));
	g_object_set_data_full(G_OBJECT(page_vbox), "arguments_label", arguments_label, (GDestroyNotify) g_object_unref);
	gtk_label_set_justify(GTK_LABEL(arguments_label), GTK_JUSTIFY_LEFT);
	gtk_widget_show(arguments_label);
	gtk_clist_set_column_widget(GTK_CLIST(arguments_clist), 1, arguments_label);

	frame1 = gtk_frame_new(_("Game and Arguments"));
	g_object_ref(G_OBJECT(frame1));
	g_object_set_data_full(G_OBJECT(page_vbox), "frame1", frame1, (GDestroyNotify) g_object_unref);
	gtk_widget_show(frame1);
	gtk_box_pack_start(GTK_BOX(vbox1), frame1, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame1), 3);

	hbox2 = gtk_hbox_new(FALSE, 0);
	g_object_ref(G_OBJECT(hbox2));
	g_object_set_data_full(G_OBJECT(page_vbox), "hbox2", hbox2, (GDestroyNotify) g_object_unref);
	gtk_widget_show(hbox2);
	gtk_container_add(GTK_CONTAINER(frame1), hbox2);
	gtk_container_set_border_width(GTK_CONTAINER(hbox2), 4);

	custom_args_entry_game[type] = gtk_entry_new();
	g_object_ref(G_OBJECT(custom_args_entry_game[type]));
	gtk_widget_set_size_request(custom_args_entry_game[type], 90, -2);
	g_object_set_data_full(G_OBJECT(page_vbox), "custom_args_entry_game[type]",
		custom_args_entry_game[type],
		(GDestroyNotify) g_object_unref);
	gtk_widget_show(custom_args_entry_game[type]);
	gtk_box_pack_start(GTK_BOX(hbox2), custom_args_entry_game[type], FALSE, TRUE, 0);
	gtk_widget_set_tooltip_text(custom_args_entry_game[type], _("Enter the game name from the game column"));
	gtk_widget_set_sensitive(custom_args_entry_game[type], FALSE);

	custom_args_entry_args[type] = gtk_entry_new();
	g_object_ref(G_OBJECT(custom_args_entry_args[type]));
	g_object_set_data_full(G_OBJECT(page_vbox), "custom_args_entry_args[type]",
		custom_args_entry_args[type],
		(GDestroyNotify) g_object_unref);
	gtk_widget_show(custom_args_entry_args[type]);
	gtk_box_pack_start(GTK_BOX(hbox2), custom_args_entry_args[type], TRUE, TRUE, 0);
	gtk_widget_set_tooltip_text(custom_args_entry_args[type], _("Enter the arguments separated by spaces"));
	gtk_widget_set_sensitive(custom_args_entry_args[type], FALSE);

	vbuttonbox1 = gtk_vbox_new(FALSE, 0);
	g_object_ref(G_OBJECT(vbuttonbox1));
	g_object_set_data_full(G_OBJECT(page_vbox), "vbuttonbox1", vbuttonbox1, (GDestroyNotify) g_object_unref);
	gtk_widget_show(vbuttonbox1);
	gtk_box_pack_start(GTK_BOX(hbox1), vbuttonbox1, FALSE, TRUE, 0);

	new_button = gtk_button_new_with_label(_("New"));
	g_object_ref(G_OBJECT(new_button));
	g_object_set_data_full(G_OBJECT(page_vbox), "new_button", new_button, (GDestroyNotify) g_object_unref);
	gtk_widget_show(new_button);
	gtk_box_pack_start(GTK_BOX(vbuttonbox1), new_button, FALSE, FALSE, 5);
	gtk_widget_set_can_default(new_button, TRUE);

	delete_button = gtk_button_new_with_label(_("Delete"));
	g_object_ref(G_OBJECT(delete_button));
	g_object_set_data_full(G_OBJECT(page_vbox), "delete_button", delete_button, (GDestroyNotify) g_object_unref);
	gtk_widget_show(delete_button);
	gtk_box_pack_start(GTK_BOX(vbuttonbox1), delete_button, FALSE, FALSE, 5);
	gtk_widget_set_can_default(delete_button, TRUE);

	defaults_button = gtk_button_new_with_label(_("Add Defaults"));
	g_object_ref(G_OBJECT(defaults_button));
	g_object_set_data_full(G_OBJECT(page_vbox), "defaults_button", defaults_button, (GDestroyNotify) g_object_unref);
	gtk_widget_show(defaults_button);
	gtk_box_pack_start(GTK_BOX(vbuttonbox1), defaults_button, FALSE, FALSE, 5);
	gtk_widget_set_can_default(defaults_button, TRUE);

	custom_args_add_button[type] = gtk_button_new_with_label(_("Add/Update"));
	g_object_ref(G_OBJECT(custom_args_add_button[type]));
	g_object_set_data_full(G_OBJECT(page_vbox), "add_button", custom_args_add_button[type], (GDestroyNotify) g_object_unref);
	gtk_widget_show(custom_args_add_button[type]);
	gtk_box_pack_end(GTK_BOX(vbuttonbox1), custom_args_add_button[type], FALSE, FALSE, 7);
	gtk_widget_set_sensitive(custom_args_add_button[type], FALSE);
	gtk_widget_set_can_default(custom_args_add_button[type], TRUE);

	g_object_set_data(G_OBJECT(new_button), "user_data", (gpointer) type);
	g_signal_connect(G_OBJECT(new_button), "clicked",
		G_CALLBACK(new_custom_args_callback),
		(gpointer) arguments_clist);

	g_object_set_data(G_OBJECT(delete_button), "user_data", (gpointer) type);
	g_signal_connect(G_OBJECT(delete_button), "clicked",
		G_CALLBACK(delete_custom_args_callback),
		(gpointer) arguments_clist);

	g_object_set_data(G_OBJECT(defaults_button), "user_data", (gpointer) type);
	g_signal_connect(G_OBJECT(defaults_button), "clicked",
		G_CALLBACK(add_custom_args_defaults),
		(gpointer) arguments_clist);

	g_object_set_data(G_OBJECT(custom_args_add_button[type]), "user_data", (gpointer) type);
	g_signal_connect(G_OBJECT(custom_args_add_button[type]), "clicked",
		G_CALLBACK(add_custom_args_callback),
		(gpointer) arguments_clist);

	// Populate clist with custom_args from g_slist
	{
		GSList *list;
		char *token[2];

		list = genprefs[type].custom_args;
		while (list) {
			char* tmp = g_strdup((char *)list->data);
			tokenize(tmp, token, 2, ",");
			gtk_clist_append(GTK_CLIST(arguments_clist), token);
			g_free(tmp);

			list = g_slist_next(list);
		}
	}

	width = gtk_clist_optimal_column_width(GTK_CLIST(arguments_clist), 0);
	gtk_clist_set_column_width(GTK_CLIST(arguments_clist), 0, width?width:60);
	gtk_clist_set_column_width(GTK_CLIST(arguments_clist), 1, gtk_clist_optimal_column_width(GTK_CLIST(arguments_clist), 1));


	gtk_widget_show(page_vbox);

	return page_vbox;
}


#if 1 // GTK2 and GTK3
enum {
	GAMESLIST_ATTR_TYPE,
	GAMESLIST_ATTR_ICON,
	GAMESLIST_ATTR_NAME,
	GAMESLIST_ATTR_COUNT
};

static void game_selection_changed_callback (GtkTreeSelection *selection, gpointer data) {
	GtkTreeIter iter;
	GtkTreeModel *model;
	gint type;

	if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
		gtk_tree_model_get (model, &iter, GAMESLIST_ATTR_TYPE, &type, -1);

		gtk_notebook_set_current_page (GTK_NOTEBOOK (games_notebook), type);
	}
}

static GtkWidget *create_games_list (void) {
	GtkTreeStore *store;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	int i, row;

	store = gtk_tree_store_new (GAMESLIST_ATTR_COUNT,
	                            G_TYPE_INT,
	                            GDK_TYPE_PIXBUF,
	                            G_TYPE_STRING
	                            );

	for (i = LAN_SERVER, row = 0; i < UNKNOWN_SERVER; i++, row++) {
		GtkTreeIter iter;
		GdkPixbuf *pixbuf = games[i].pix->pixbuf;
		char *name = _(games[i].name);

		gtk_tree_store_append (store, &iter, NULL);

		gtk_tree_store_set (store, &iter,
		                    GAMESLIST_ATTR_TYPE, i,
		                    GAMESLIST_ATTR_ICON, pixbuf,
		                    GAMESLIST_ATTR_NAME, name,
		                    -1);

		genprefs[i].tree_path = gtk_tree_path_new_from_indices(row, -1);
	}

	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));

	g_object_unref (G_OBJECT (store));

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (tree), FALSE);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, "Game");

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column,
	                                     renderer,
	                                     "pixbuf", GAMESLIST_ATTR_ICON,
	                                     NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column,
	                                     renderer,
	                                     "text", GAMESLIST_ATTR_NAME,
	                                     NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);
	g_signal_connect (G_OBJECT (select), "changed",
	                  G_CALLBACK (game_selection_changed_callback),
	                  NULL);

	games_list = tree;

	return tree;
}

static void games_list_select (enum server_type type) {
	GtkTreeSelection *select = gtk_tree_view_get_selection (GTK_TREE_VIEW (games_list));

	gtk_tree_selection_unselect_all (select);

	gtk_tree_selection_select_path (select, genprefs[type].tree_path);
}
#else // GTK1 (deprecated)
// If this code is removed, remove genprefs[].game_button as well.

static void game_listitem_selected_callback (GtkItem *item, enum server_type type) {
	gtk_notebook_set_current_page (GTK_NOTEBOOK (games_notebook), type);
}

static GtkWidget *create_games_list (void) {
	GtkWidget *gtklist = gtk_list_new ();

	for (i = LAN_SERVER; i < UNKNOWN_SERVER; i++) {
		genprefs[i].game_button = gtk_list_item_new();

		gtk_container_add (GTK_CONTAINER (genprefs[i].game_button), game_pixmap_with_label (i));

		g_signal_connect (G_OBJECT (genprefs[i].game_button), "select",
			G_CALLBACK (game_listitem_selected_callback), GINT_TO_POINTER(i));

		gtk_widget_show(genprefs[i].game_button);
		gtk_container_add (GTK_CONTAINER (gtklist), genprefs[i].game_button);
	}

	return gtklist;
}

static void games_list_select (enum server_type type) {
	gtk_list_item_select(GTK_LIST_ITEM(genprefs[type].game_button));
}
#endif

#define GAMES_COLS 3
#define GAMES_ROWS ((UNKNOWN_SERVER - KNOWN_SERVER_START + GAMES_COLS - 1) / GAMES_COLS)

// create dialog where commandline and working dir for all games are configured
static GtkWidget *games_config_page (int defgame) {
	GtkWidget *page_vbox;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *page;
	GtkWidget *label;
	GtkWidget *gtklist=NULL;
	GtkWidget *scrollwin=NULL;
	GtkWidget *games_hbox;
	char *typestr;
	int i;

	page_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	games_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (games_hbox), 0);
	gtk_box_pack_start (GTK_BOX (page_vbox), games_hbox, TRUE, TRUE, 0);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

	scrollwin = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	gtklist = create_games_list ();

	gtk_widget_set_size_request (gtklist, 136, -1);

	//  gtk_container_add (GTK_CONTAINER (scrollwin), gtklist);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin), gtklist);

	gtk_widget_show(gtklist);
	gtk_container_add (GTK_CONTAINER (frame), scrollwin);
	gtk_widget_show(scrollwin);
	gtk_box_pack_start (GTK_BOX (games_hbox), frame, FALSE, FALSE, 0);
	gtk_widget_show (frame);

	games_notebook = gtk_notebook_new ();
	// the tabs are hidden, so nobody will notice its a notebook
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (games_notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (games_notebook), FALSE);
	gtk_box_pack_start (GTK_BOX (games_hbox), games_notebook, FALSE, FALSE, 15);

	for (i = LAN_SERVER; i < UNKNOWN_SERVER; i++) {
		page = generic_game_frame (i);

		label = gtk_label_new (games[i].name);
		gtk_widget_show (label);

		gtk_notebook_append_page (GTK_NOTEBOOK (games_notebook), page, label);
	}

	if (defgame == UNKNOWN_SERVER) {
		defgame = QW_SERVER;
		typestr = config_get_string("/" CONFIG_FILE "/Games Config/game");
		if (typestr) {
			defgame = id2type (typestr);
			g_free(typestr);
		}
	}

	if (defgame >= UNKNOWN_SERVER) {
		defgame = QW_SERVER;
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (games_notebook), defgame);

	games_list_select (defgame);

	gtk_widget_show (games_notebook);

	/* Common Options */

	frame = gtk_frame_new (_("Common Options"));
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 15);

	hbox = gtk_hbox_new (TRUE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), hbox);

	/* Disable CD Audio */

	nocdaudio_check_button = gtk_check_button_new_with_label (_("Disable CD Audio"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nocdaudio_check_button), default_nocdaudio);
	gtk_box_pack_end (GTK_BOX (hbox), nocdaudio_check_button, TRUE, FALSE, 0);
	gtk_widget_show (nocdaudio_check_button);

	/* Disable Sound */

	nosound_check_button = gtk_check_button_new_with_label (_("Disable Sound"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (nosound_check_button), default_nosound);
	gtk_box_pack_end (GTK_BOX (hbox), nosound_check_button, TRUE, FALSE, 0);
	gtk_widget_show (nosound_check_button);

	gtk_widget_show (hbox);
	gtk_widget_show (frame);

	gtk_widget_show (page_vbox);

	gtk_widget_show (games_hbox);

	return page_vbox;
}


static void add_pushlatency_options (GtkWidget *vbox) {
	GtkWidget *hbox;
	GtkAdjustment *adj;
	GSList *group = NULL;
	int i;

	static const char *pushlatency_modes[] = {
		N_("Do not set (use game default)"),
		N_("Automatically calculate from server ping time"),
		N_("Fixed value")
	};

	for (i = 0; i < 3; i++) {
		hbox = gtk_hbox_new (FALSE, 4);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

		pushlatency_mode_radio_buttons[i] = gtk_radio_button_new_with_label (group, _(pushlatency_modes[i]));
		group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (pushlatency_mode_radio_buttons[i]));
		gtk_box_pack_start (GTK_BOX (hbox), pushlatency_mode_radio_buttons[i], FALSE, FALSE, 0);
		gtk_widget_show (pushlatency_mode_radio_buttons[i]);

		gtk_widget_show (hbox);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pushlatency_mode_radio_buttons[pushlatency_mode]), TRUE);

	adj = (GtkAdjustment *) gtk_adjustment_new (pushlatency_value, -1000.0, -10.0, 10.0, 50.0, 0.0);

	pushlatency_value_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (pushlatency_value_spinner), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (pushlatency_value_spinner, 64, -1);
	gtk_box_pack_start (GTK_BOX (hbox), pushlatency_value_spinner, FALSE, FALSE, 0);
	gtk_widget_show (pushlatency_value_spinner);
}

static struct q3_common_prefs_s* get_pref_widgets_for_game(enum server_type type) {
	switch(type) {
		case Q3_SERVER: return &q3_prefs;
		case Q4_SERVER: return &q4_prefs;
		case WO_SERVER: return &wo_prefs;
		case WOET_SERVER: return &woet_prefs;
		case ETL_SERVER: return &etl_prefs;
		case DOOM3_SERVER: return &doom3_prefs;
		case ETQW_SERVER: return &etqw_prefs;
		case EF_SERVER: return &ef_prefs;
		case SOF2S_SERVER: return &sof2_prefs;
		case CDIESEL_SERVER: return &cdiesel_prefs;
		case COD_SERVER: return &cod_prefs;
		case CODUO_SERVER: return &coduo_prefs;
		case COD2_SERVER: return &cod2_prefs;
		case COD4_SERVER: return &cod4_prefs;
		case CODWAW_SERVER: return &codwaw_prefs;
		case JK2_SERVER: return &jk2_prefs;
		case JK3_SERVER: return &jk3_prefs;
		case NEXUIZ_SERVER: return &nexuiz_prefs;
		case REXUIZ_SERVER: return &nexuiz_prefs;
		case XONOTIC_SERVER: return &xonotic_prefs;
		case WARFORK_SERVER: return &warfork_prefs;
		case WARSOW_SERVER: return &warsow_prefs;
		case TREMULOUS_SERVER: return &tremulous_prefs;
		case TREMULOUSGPP_SERVER: return &tremulousgpp_prefs;
		case UNVANQUISHED_SERVER: return &unvanquished_prefs;
		case OPENARENA_SERVER: return &openarena_prefs;
		case Q3RALLY_SERVER: return &q3rally_prefs;
		case WOP_SERVER: return &wop_prefs;
		case IOURT_SERVER: return &iourt_prefs;
		case REACTION_SERVER: return &reaction_prefs;
		case SMOKINGUNS_SERVER: return &smokinguns_prefs;
		case ZEQ2LITE_SERVER: return &zeq2lite_prefs;
		case TURTLEARENA_SERVER: return &turtlearena_prefs;
		case ALIENARENA_SERVER: return &alienarena_prefs;
		default: xqf_error("need to define preferences"); return NULL;
	}
}

static GtkWidget *q3_options_page (enum server_type type) {
	GtkWidget *page_vbox;
	GtkWidget *hbox;
	GtkWidget *label;

	struct q3_common_prefs_s* w = get_pref_widgets_for_game(type);

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	if (w->protocols) {
		GList *list;

		hbox = gtk_hbox_new (FALSE, 8);
		gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

		label = gtk_label_new (_("Masterserver Protocol Version"));
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_widget_show (label);

		w->proto_entry = gtk_combo_box_text_new_with_entry ();

		list = createGListfromchar ((char**)w->protocols);

		combo_set_vals (w->proto_entry, list, game_get_attribute(type,"masterprotocol"));

		g_list_free (list);

		gtk_box_pack_start (GTK_BOX (hbox), w->proto_entry, FALSE, FALSE, 0);
		gtk_widget_show (w->proto_entry);

		gtk_widget_show (hbox);

	}

	if (w->flags & Q3_PREF_SETFS_GAME) {
		w->setfs_gamebutton = gtk_check_button_new_with_label (_("set fs_game on connect"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w->setfs_gamebutton), str2bool(game_get_attribute(type,"setfs_game")));
		gtk_box_pack_start (GTK_BOX (page_vbox), w->setfs_gamebutton, FALSE, FALSE, 0);
		gtk_widget_show (w->setfs_gamebutton);
	}

	if (w->flags & Q3_PREF_PB) {
		w->set_punkbusterbutton = gtk_check_button_new_with_label (_("set cl_punkbuster on connect"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w->set_punkbusterbutton), str2bool(game_get_attribute(type,"set_punkbuster")));
		gtk_box_pack_start (GTK_BOX (page_vbox), w->set_punkbusterbutton, FALSE, FALSE, 0);
		gtk_widget_show (w->set_punkbusterbutton);
	}

	if (w->flags & Q3_PREF_CONSOLE) {
		w->console_button = gtk_check_button_new_with_label (_("enable console"));
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w->console_button), str2bool(game_get_attribute(type,"enable_console")));
		gtk_box_pack_start (GTK_BOX (page_vbox), w->console_button, FALSE, FALSE, 0);
		gtk_widget_show (w->console_button);
	}

	gtk_widget_show (page_vbox);

	return page_vbox;
}

static void q3_set_memory_callback (GtkWidget *widget, int what) {
	int com_hunkmegs        = 56;
	int com_zonemegs        = 16;
	int com_soundmegs       = 8;

	if (what == 1) {
		com_hunkmegs        = 72;
		com_zonemegs        = 24;
		com_soundmegs       = 16;
	}
	else if (what == 2) {
		com_hunkmegs        = 96;
		com_zonemegs        = 24;
		com_soundmegs       = 16;
	}

	gtk_adjustment_set_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(com_hunkmegs_spinner)), com_hunkmegs);
	gtk_adjustment_set_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(com_zonemegs_spinner)), com_zonemegs);
	gtk_adjustment_set_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(com_soundmegs_spinner)), com_soundmegs);
}

static GtkWidget *q3_mem_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *label;
	GtkWidget *frame;
	GtkAdjustment *adj;
	GtkWidget *button;

	int pass_memory_options = str2bool(game_get_attribute(Q3_SERVER,"pass_memory_options"));
	int com_hunkmegs        = atoi(game_get_attribute(Q3_SERVER,"com_hunkmegs"));
	int com_zonemegs        = atoi(game_get_attribute(Q3_SERVER,"com_zonemegs"));
	int com_soundmegs       = atoi(game_get_attribute(Q3_SERVER,"com_soundmegs"));

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	pass_memory_options_button = gtk_check_button_new_with_label (_("Pass memory settings on command line"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pass_memory_options_button), pass_memory_options);
	gtk_box_pack_start (GTK_BOX (page_vbox), pass_memory_options_button, FALSE, FALSE, 0);
	gtk_widget_show (pass_memory_options_button);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (com_hunkmegs, 32, 1024, 8, 32, 0);
	com_hunkmegs_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (com_hunkmegs_spinner), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (com_hunkmegs_spinner, 64, -1);

	gtk_box_pack_start (GTK_BOX (hbox), com_hunkmegs_spinner, FALSE, FALSE, 0);
	gtk_widget_show (com_hunkmegs_spinner);

	// Mega Byte
	label = gtk_label_new (_("MB"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	label = gtk_label_new (_("com_hunkmegs"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (com_zonemegs, 16, 1024, 4, 8, 0);
	com_zonemegs_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (com_zonemegs_spinner), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (com_zonemegs_spinner, 64, -1);

	gtk_box_pack_start (GTK_BOX (hbox), com_zonemegs_spinner, FALSE, FALSE, 0);
	gtk_widget_show (com_zonemegs_spinner);

	// Mega Byte
	label = gtk_label_new (_("MB"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	label = gtk_label_new (_("com_zonemegs"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

	adj = (GtkAdjustment *) gtk_adjustment_new (com_soundmegs, 8, 1024, 4, 8, 0);
	com_soundmegs_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (com_soundmegs_spinner), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (com_soundmegs_spinner, 64, -1);

	gtk_box_pack_start (GTK_BOX (hbox), com_soundmegs_spinner, FALSE, FALSE, 0);
	gtk_widget_show (com_soundmegs_spinner);

	// Mega Byte
	label = gtk_label_new (_("MB"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	label = gtk_label_new (_("com_soundmegs"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 8);

	frame = gtk_frame_new (_("Preset values"));
	gtk_box_pack_start (GTK_BOX (hbox), frame, FALSE, FALSE, 0);

	hbox2 = gtk_hbox_new (FALSE, 8);
	gtk_container_set_border_width(GTK_CONTAINER(hbox2),8);
	gtk_container_add (GTK_CONTAINER (frame), hbox2);

	button = gtk_button_new_with_label(_("Default"));
	gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (q3_set_memory_callback), (gpointer) 0);
	gtk_widget_show (button);

	button = gtk_button_new_with_label(_("128MB"));
	gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (q3_set_memory_callback), (gpointer) 1);
	gtk_widget_show (button);

	button = gtk_button_new_with_label(_(">256MB"));
	gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (q3_set_memory_callback), (gpointer) 2);
	gtk_widget_show (button);

	gtk_widget_show (hbox2);

	gtk_widget_show (frame);

	gtk_widget_show (hbox);

	gtk_widget_show (page_vbox);

	return page_vbox;
}

void add_q3_options_to_notebook(GtkWidget *notebook, enum server_type type) {
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), q3_options_page(type), gtk_label_new (_("Options")));

	if (type == Q3_SERVER) {
		gtk_notebook_append_page (GTK_NOTEBOOK (notebook), q3_mem_options_page(), gtk_label_new (_("Memory")));
	}
}

void add_un_options_to_notebook(GtkWidget *notebook, enum server_type type) {
}

void add_ut2_options_to_notebook(GtkWidget *notebook, enum server_type type) {
}

static GtkWidget *qw_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *label;
	GtkWidget *frame2;
	GtkWidget *hbox;
	GtkWidget *vbox2;
	GtkWidget *option_menu;

	debug (5, "qw_options_page()");

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	/* QW Specific Features */


	/* 'w_switch' & 'b_switch' control */

	frame2 = gtk_frame_new (_("The highest weapon that Quake should switch to..."));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame2, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
	gtk_container_add (GTK_CONTAINER (frame2), vbox2);

	/* 'w_switch' */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("upon a weapon pickup"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	option_menu = gtk_combo_box_text_new ();
	set_wb_switch_menu (option_menu);
	g_signal_connect(option_menu, "changed", G_CALLBACK (set_w_switch_callback), NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (option_menu), pref_w_switch);
	gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
	gtk_widget_show (option_menu);

	gtk_widget_show (hbox);

	/* 'b_switch' */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("upon a backpack pickup"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	option_menu = gtk_combo_box_text_new ();
	set_wb_switch_menu (option_menu);
	g_signal_connect(option_menu, "changed", G_CALLBACK (set_b_switch_callback), NULL);
	gtk_combo_box_set_active (GTK_COMBO_BOX (option_menu), pref_b_switch);
	gtk_box_pack_end (GTK_BOX (hbox), option_menu, FALSE, FALSE, 0);
	gtk_widget_show (option_menu);

	gtk_widget_show (hbox);
	gtk_widget_show (vbox2);
	gtk_widget_show (frame2);

	/* 'noaim' */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 10);

	noaim_check_button = gtk_check_button_new_with_label (_("Disable auto-aiming"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (noaim_check_button), default_noaim);
	gtk_box_pack_start (GTK_BOX (hbox), noaim_check_button, FALSE, FALSE, 0);
	gtk_widget_show (noaim_check_button);

	gtk_widget_show (hbox);

	gtk_widget_show (page_vbox);

	return page_vbox;

}

// qworq2: 0=qw/1=q2
static GtkWidget *qw_q2_options_page (int qworq2) {
	GtkWidget *page_vbox;
	GtkWidget *label;
	GtkWidget *frame2;
	GtkWidget *hbox;
	GtkWidget *hbox2;
	GtkWidget *vbox2;
	GtkWidget *option_menu;
	GtkAdjustment *adj;

	debug (5, "qw_q2_options_page(%d)",qworq2);

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	hbox = gtk_hbox_new (FALSE, 16);
	gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

	/* Skins */

	hbox2 = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

	label = gtk_label_new (_("Skins"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	option_menu = gtk_combo_box_text_new ();
	g_signal_connect(option_menu, "changed", G_CALLBACK
	    (qworq2 ? q2_noskins_option_menu_callback : qw_noskins_option_menu_callback), NULL);
	set_noskins_menu (option_menu);
	gtk_combo_box_set_active (GTK_COMBO_BOX (option_menu), qworq2?pref_q2_noskins:pref_qw_noskins);
	gtk_box_pack_end (GTK_BOX (hbox2), option_menu, FALSE, FALSE, 0);
	gtk_widget_show (option_menu);

	gtk_widget_show (hbox2);

	/* Network Options */

	/* Rate */

	hbox2 = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

	label = gtk_label_new (_("Rate"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
	gtk_widget_show (label);

	adj = (GtkAdjustment *) gtk_adjustment_new (qworq2?default_q2_rate:default_qw_rate, 0.0, 25000.0, 500.0, 1000.0, 0.0);

	rate_spinner[qworq2] = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (rate_spinner[qworq2]), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (rate_spinner[qworq2], 64, -1);
	gtk_box_pack_end (GTK_BOX (hbox2), rate_spinner[qworq2], FALSE, FALSE, 0);
	gtk_widget_show (rate_spinner[qworq2]);

	gtk_widget_show (hbox2);

	gtk_widget_show (hbox);

	/* QW Specific Features */
	if (qworq2 == 0) {
		/* 'pushlatency' */

		frame2 = gtk_frame_new (_("pushlatency"));
		gtk_box_pack_start (GTK_BOX (page_vbox), frame2, FALSE, FALSE, 10);

		vbox2 = gtk_vbox_new (FALSE, 2);
		gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
		gtk_container_add (GTK_CONTAINER (frame2), vbox2);

		add_pushlatency_options (vbox2);

		gtk_widget_show (vbox2);
		gtk_widget_show (frame2);
	}

	/* Troubleshooting */

	frame2 = gtk_frame_new (_("Troubleshooting"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame2, FALSE, FALSE, 10);

	vbox2 = gtk_vbox_new (FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
	gtk_container_add (GTK_CONTAINER (frame2), vbox2);

	/* 'cl_nodelta' */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	cl_nodelta_check_button[qworq2] = gtk_check_button_new_with_label (
		_("Disable delta-compression (cl_nodelta)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cl_nodelta_check_button[qworq2]),
		qworq2?default_q2_cl_nodelta:default_qw_cl_nodelta);
	gtk_box_pack_start (GTK_BOX (hbox), cl_nodelta_check_button[qworq2], FALSE, FALSE, 0);
	gtk_widget_show (cl_nodelta_check_button[qworq2]);

	gtk_widget_show (hbox);

	/* 'cl_predict_players' ('cl_predict' in Q2) */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (vbox2), hbox, FALSE, FALSE, 0);

	cl_predict_check_button[qworq2] = gtk_check_button_new_with_label (
		_("Disable player/entity prediction (cl_predict_players)"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cl_predict_check_button[qworq2]),
		1 - (qworq2?default_q2_cl_predict:default_qw_cl_predict));
	gtk_box_pack_start (GTK_BOX (hbox), cl_predict_check_button[qworq2], FALSE, FALSE, 0);
	gtk_widget_show (cl_predict_check_button[qworq2]);

	gtk_widget_show (hbox);

	gtk_widget_show (vbox2);
	gtk_widget_show (frame2);

	gtk_widget_show (page_vbox);

	return page_vbox;
}

void add_qw_options_to_notebook(GtkWidget *notebook, enum server_type type) {
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), qw_options_page(), gtk_label_new (_("Weapons")));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), qw_q2_options_page(0), gtk_label_new (_("Options")));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), player_profile_qw_page(), gtk_label_new (_("Player Profile")));
}

void add_q2_options_to_notebook(GtkWidget *notebook, enum server_type type) {
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), qw_q2_options_page(1), gtk_label_new (_("Options")));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), player_profile_q2_page(), gtk_label_new (_("Player Profile")));
}

void add_q1_options_to_notebook(GtkWidget *notebook, enum server_type type) {
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), player_profile_q1_page(), gtk_label_new (_("Player Profile")));
}

void add_t2_options_to_notebook(GtkWidget *notebook, enum server_type type) {
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), player_profile_t2_page(), gtk_label_new (_("Player Profile")));
}

static void terminate_toggled_callback (GtkWidget *widget, gpointer data) {
	//gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (terminate_check_button));
}

static void launchinfo_toggled_callback (GtkWidget *widget, gpointer data) {
	//gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (launchinfo_check_button));
}

static void prelaunchexec_toggled_callback (GtkWidget *widget, gpointer data) {
	//gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prelaunchexec_check_button));
}

static void save_srvinfo_toggled_callback (GtkWidget *widget, gpointer data) {
	int val;

	val = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (save_srvinfo_check_button));
	gtk_widget_set_sensitive (save_plrinfo_check_button, val);
}

static void scan_maps_callback (GtkWidget *widget, gpointer data) {
	int i;

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		scan_maps_for(i);
	}

	server_clist_set_list (cur_server_list);
}


static GtkWidget *appearance_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	frame = gtk_frame_new (_("Server List"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	/* Lookup host names */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	show_hostnames_check_button = gtk_check_button_new_with_label (_("Show host names"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_hostnames_check_button), show_hostnames);
	gtk_box_pack_start (GTK_BOX (hbox), show_hostnames_check_button, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text (show_hostnames_check_button, _("Show hostnames instead of IP addresses if possible"));
	gtk_widget_show (show_hostnames_check_button);

	gtk_widget_show (hbox);

	/* Show default port */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	show_defport_check_button = gtk_check_button_new_with_label (_("Show default port"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_defport_check_button), show_default_port);
	gtk_box_pack_start (GTK_BOX (hbox), show_defport_check_button, FALSE, FALSE, 0);
	gtk_widget_show (show_defport_check_button);

	gtk_widget_show (hbox);

	/* show bots */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	countbots_check_button = gtk_check_button_new_with_label (_("Do not count bots as players"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (countbots_check_button), serverlist_countbots);
	gtk_box_pack_start (GTK_BOX (hbox), countbots_check_button, FALSE, FALSE, 0);
	gtk_widget_show (countbots_check_button);

	gtk_widget_show (hbox);


	/* Sort servers real-time during refresh */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	refresh_sorts_check_button = gtk_check_button_new_with_label (_("Sort servers real-time during refresh"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (refresh_sorts_check_button), default_refresh_sorts);
	gtk_box_pack_start (GTK_BOX (hbox), refresh_sorts_check_button, FALSE, FALSE, 0);
	gtk_widget_show (refresh_sorts_check_button);

	gtk_widget_show (hbox);

	/* Refresh on update */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	refresh_on_update_check_button = gtk_check_button_new_with_label (_("Refresh on update"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (refresh_on_update_check_button), default_refresh_on_update);
	gtk_box_pack_start (GTK_BOX (hbox), refresh_on_update_check_button, FALSE, FALSE, 0);
	gtk_widget_show (refresh_on_update_check_button);

	gtk_widget_show (hbox);

	/* Resolve on update */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	resolve_on_update_check_button = gtk_check_button_new_with_label (_("Resolve hostnames on update"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (resolve_on_update_check_button), default_resolve_on_update);
	gtk_widget_set_tooltip_text (resolve_on_update_check_button, _("Enable or disable DNS resolution of IP addresses"));
	gtk_box_pack_start (GTK_BOX (hbox), resolve_on_update_check_button, FALSE, FALSE, 0);
	gtk_widget_show (resolve_on_update_check_button);

	gtk_widget_show (hbox);


	/* Show only configured games */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	show_only_configured_games_check_button = gtk_check_button_new_with_label (_("Show only configured games"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (show_only_configured_games_check_button), default_show_only_configured_games);
	gtk_box_pack_start (GTK_BOX (hbox), show_only_configured_games_check_button, FALSE, FALSE, 0);
	gtk_widget_show (show_only_configured_games_check_button);

	gtk_widget_show (hbox);


	gtk_widget_show (vbox);
	gtk_widget_show (frame);

	gtk_widget_show (page_vbox);

	return page_vbox;
}

#ifdef GUI_GTK2
static GtkWidget *general_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *vbox;

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	/* On Startup */

	frame = gtk_frame_new (_("On Startup"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	/* Refresh Favorites */

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	auto_favorites_check_button = gtk_check_button_new_with_label (_("Refresh Favorites"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_favorites_check_button), default_auto_favorites);
	gtk_box_pack_start (GTK_BOX (hbox), auto_favorites_check_button, FALSE, FALSE, 0);
	gtk_widget_show (auto_favorites_check_button);

	gtk_widget_show (hbox);

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show (hbox);


	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	auto_maps_check_button = gtk_check_button_new_with_label (_("Scan for maps"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (auto_maps_check_button), default_auto_maps);

	gtk_widget_set_tooltip_text (auto_maps_check_button,
		_("Scan game directories for installed maps. xqf will"
		" take longer to start up when enabled."));
	gtk_box_pack_start (GTK_BOX (hbox), auto_maps_check_button, FALSE, FALSE, 0);
	gtk_widget_show (auto_maps_check_button);

	{
		GtkWidget* button = gtk_button_new_with_label(_("scan now"));
		gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
		gtk_misc_set_padding(GTK_MISC(gtk_bin_get_child(GTK_BIN(button))),4,0);
		g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (scan_maps_callback), NULL);
		gtk_widget_show (button);
	}

	gtk_widget_show (hbox);

	gtk_widget_show (vbox);
	gtk_widget_show (frame);

	/* When launching a Game */

	frame = gtk_frame_new (_("When launching a game..."));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	/* Terminate */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	terminate_check_button = gtk_check_button_new_with_label (_("Terminate XQF"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (terminate_check_button), default_terminate);
	g_signal_connect (G_OBJECT (terminate_check_button), "toggled", G_CALLBACK (terminate_toggled_callback), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), terminate_check_button, FALSE, FALSE, 0);
	gtk_widget_show (terminate_check_button);

	gtk_widget_show (hbox);

	/* Launchinfo */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	launchinfo_check_button = gtk_check_button_new_with_label (_("Create LaunchInfo.txt"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (launchinfo_check_button), default_launchinfo);
	g_signal_connect (G_OBJECT (launchinfo_check_button), "toggled", G_CALLBACK (launchinfo_toggled_callback), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), launchinfo_check_button, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text (launchinfo_check_button, _("Creates the file ~/.config/xqf/LaunchInfo.txt with: ping ip:port name map curplayers maxplayers"));
	gtk_widget_show (launchinfo_check_button);

	gtk_widget_show (hbox);

	/* Prelaunchinfo */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	prelaunchexec_check_button = gtk_check_button_new_with_label (_("Execute prelaunch"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prelaunchexec_check_button), default_prelaunchexec);
	g_signal_connect (G_OBJECT (prelaunchexec_check_button), "toggled", G_CALLBACK (prelaunchexec_toggled_callback), NULL);
	gtk_box_pack_start (GTK_BOX (hbox), prelaunchexec_check_button, FALSE, FALSE, 0);
	gtk_widget_set_tooltip_text (prelaunchexec_check_button, _("Executes ~/.config/xqf/PreLaunch (if it exists) before launching the game"));
	gtk_widget_show (prelaunchexec_check_button);

	gtk_widget_show (hbox);

	gtk_widget_show (vbox);
	gtk_widget_show (frame);

	/* On Exit */

	frame = gtk_frame_new (_("On Exit"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	/* Save master lists */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	save_lists_check_button = gtk_check_button_new_with_label (_("Save server lists"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_lists_check_button), default_save_lists);
	gtk_box_pack_start (GTK_BOX (hbox), save_lists_check_button, FALSE, FALSE, 0);
	gtk_widget_show (save_lists_check_button);

	gtk_widget_show (hbox);

	/* Save server information */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	save_srvinfo_check_button = gtk_check_button_new_with_label (_("Save server information"));
	gtk_box_pack_start (GTK_BOX (hbox), save_srvinfo_check_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_srvinfo_check_button), default_save_srvinfo);
	g_signal_connect (G_OBJECT (save_srvinfo_check_button), "toggled", G_CALLBACK (save_srvinfo_toggled_callback), NULL);
	gtk_widget_show (save_srvinfo_check_button);

	gtk_widget_show (hbox);

	/* Save player information */

	hbox = gtk_hbox_new (FALSE, 4);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	save_plrinfo_check_button = gtk_check_button_new_with_label (_("Save player information"));
	gtk_box_pack_start (GTK_BOX (hbox), save_plrinfo_check_button, FALSE, FALSE, 0);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (save_plrinfo_check_button), default_save_plrinfo);

	if (!default_save_srvinfo) {
		gtk_widget_set_sensitive (save_plrinfo_check_button, FALSE);
	}

	gtk_widget_show (save_plrinfo_check_button);

	gtk_widget_show (hbox);

	gtk_widget_show (vbox);
	gtk_widget_show (frame);

	gtk_widget_show (hbox);
	gtk_widget_show (vbox);
	gtk_widget_show (frame);

	gtk_widget_show (page_vbox);

	return page_vbox;
}
#endif

static GtkWidget *qstat_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;
	GtkAdjustment *adj;
	GtkWidget* alignment;
	GtkWidget* hbox;
	unsigned row = 0;

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	/* QStat preferences -- maxsimultaneous & maxretries */

	frame = gtk_frame_new (_("QStat Options"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	table = gtk_table_new (2, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_container_add (GTK_CONTAINER (frame), table);

	/* maxsimultaneous */

	label = gtk_label_new (_("Number of simultaneous servers to query"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);


	adj = (GtkAdjustment *) gtk_adjustment_new (maxsimultaneous, 1.0, FD_SETSIZE, 1.0, 5.0, 0.0);

	maxsimultaneous_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (maxsimultaneous_spinner), GTK_UPDATE_ALWAYS);
	gtk_widget_set_size_request (maxsimultaneous_spinner, 48, -1);

	alignment = gtk_alignment_new (1, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment), maxsimultaneous_spinner);

	gtk_table_attach_defaults (GTK_TABLE (table), alignment, 1, 2, row, row+1);
	gtk_widget_show (maxsimultaneous_spinner);
	gtk_widget_show(alignment);

	++row;

	/* maxretries */

	label = gtk_label_new (_("Number of retries"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	adj = (GtkAdjustment *) gtk_adjustment_new (maxretries, 1.0, MAX_RETRIES, 1.0, 1.0, 0.0);
	maxretries_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
	gtk_widget_set_size_request (maxretries_spinner, 48, -1);

	alignment = gtk_alignment_new (1, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment), maxretries_spinner);
	gtk_table_attach_defaults (GTK_TABLE (table), alignment, 1, 2, row, row+1);
	gtk_widget_show (maxretries_spinner);
	gtk_widget_show(alignment);

	++row;

	/* srcip */

	label = gtk_label_new (_("Source IP Address"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	qstat_srcip_entry = gtk_entry_new ();
	gtk_entry_set_max_length(GTK_ENTRY(qstat_srcip_entry), 15);
	gtk_entry_set_text (GTK_ENTRY (qstat_srcip_entry), qstat_srcip?qstat_srcip:"");

	alignment = gtk_alignment_new (1, 0.5, 0, 0);
	gtk_container_add (GTK_CONTAINER (alignment), qstat_srcip_entry);
	gtk_table_attach_defaults (GTK_TABLE (table), alignment, 1, 2, row, row+1);
	gtk_widget_show (qstat_srcip_entry);
	gtk_widget_show (alignment);

	++row;

	/* srcport */

	{
		char buf[6];
		label = gtk_label_new (_("Source Port Range"));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
		gtk_widget_show (label);

		hbox = gtk_hbox_new (FALSE, 4);

		if (qstat_srcport_low) {
			snprintf(buf, sizeof(buf), "%hu", qstat_srcport_low);
		}
		else {
			*buf = '\0';
		}

		qstat_srcport_entry_low = gtk_entry_new ();
		gtk_entry_set_max_length(GTK_ENTRY(qstat_srcport_entry_low), 5);
		gtk_entry_set_text (GTK_ENTRY (qstat_srcport_entry_low), buf);
		gtk_box_pack_start(GTK_BOX(hbox), qstat_srcport_entry_low, FALSE, FALSE, 0);
		gtk_widget_set_size_request (qstat_srcport_entry_low, 70, -1);

		label = gtk_label_new ("-");
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

		if (qstat_srcport_high) {
			snprintf(buf, sizeof(buf), "%hu", qstat_srcport_high);
		}
		else {
			*buf = '\0';
		}

		qstat_srcport_entry_high = gtk_entry_new ();
		gtk_entry_set_max_length(GTK_ENTRY(qstat_srcport_entry_high), 5);
		gtk_entry_set_text (GTK_ENTRY (qstat_srcport_entry_high), buf);
		gtk_box_pack_start(GTK_BOX(hbox), qstat_srcport_entry_high, FALSE, FALSE, 0);
		gtk_widget_set_size_request (qstat_srcport_entry_high, 70, -1);

		alignment = gtk_alignment_new (1, 0.5, 0, 0);
		gtk_container_add (GTK_CONTAINER (alignment), hbox);
		gtk_table_attach_defaults (GTK_TABLE (table), alignment, 1, 2, row, row+1);
		gtk_widget_show (qstat_srcport_entry_low);
		gtk_widget_show (label);
		gtk_widget_show (qstat_srcport_entry_high);
		gtk_widget_show (alignment);
		gtk_widget_show (hbox);

		++row;
	}

	gtk_widget_show (table);
	gtk_widget_show (frame);

	gtk_widget_show (page_vbox);

	return page_vbox;
}

void pref_sound_play (GtkWidget *dialog_button) {
	char *file   = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog_button));
	char *player = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (sound_player_file_dialog_button));

	play_sound_with (player, file, 1);

	g_free (file);
	g_free (player);
}

void pref_sound_conf_clear(GtkWidget *dialog_button) {
	gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (dialog_button));
}

GtkWidget *sound_clear_button_new() {
	GtkWidget* button = gtk_button_new_with_label("⌫");
	return button;
}

GtkWidget *sound_test_button_new() {
	GtkWidget* button = gtk_button_new_with_label("♪");
	return button;
}

GtkWidget *pref_sound_conf_append (char *file, char *name, GtkWidget *table, int i) {
	GtkWidget *label;
	GtkWidget *clear_button;
	GtkWidget *dialog_button;
	GtkWidget *test_button;

	// Label
	label = gtk_label_new (name);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, i, i+1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);

	// File selection dialog
	dialog_button = gtk_file_chooser_button_new (_("Select a File"), GTK_FILE_CHOOSER_ACTION_OPEN);
	if (file != NULL && *file != '\0') {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (dialog_button), file);
	}
	gtk_table_attach_defaults (GTK_TABLE (table), dialog_button, 1, 2, i, i+1);
	gtk_widget_show (dialog_button);

	// Clear button
	clear_button = sound_clear_button_new ();
	g_signal_connect_swapped (clear_button, "clicked", G_CALLBACK (pref_sound_conf_clear), dialog_button);
	gtk_table_attach (GTK_TABLE (table), clear_button, 3, 4, i, i+1, 0, 0, 0, 0);
	gtk_widget_show (clear_button);

	// Test button
	test_button = sound_test_button_new ();
	g_signal_connect_swapped (test_button, "clicked", G_CALLBACK (pref_sound_play), dialog_button);
	gtk_table_attach (GTK_TABLE (table), test_button, 5, 6, i, i+1, 0, 0, 0, 0);
	gtk_widget_show (test_button);

	return dialog_button;
}

static GtkWidget *sound_options_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *label;

	int pos = 0;

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	/* Sound Enable / Disable frame */
	/* Sounds Enable / Disable */

	frame = gtk_frame_new (_("Sound Enable / Disable"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	table = gtk_table_new (2, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_container_add (GTK_CONTAINER (frame), table);

	gtk_widget_show (table);

	/* Sound Enable */

	sound_enable_check_button = gtk_check_button_new_with_label (_("Enable Sound"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sound_enable_check_button), sound_enable);

	gtk_table_attach (GTK_TABLE (table), sound_enable_check_button, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (sound_enable_check_button);

	/* Sound Player */

	label = gtk_label_new (_("Player program"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, 0, 0, 0, 0);
	gtk_widget_show (label);

	sound_player_file_dialog_button = gtk_file_chooser_button_new (_("Select a File"), GTK_FILE_CHOOSER_ACTION_OPEN);
	if (sound_player != NULL && *sound_player != '\0') {
		gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (sound_player_file_dialog_button), sound_player);
	}

	gtk_table_attach_defaults (GTK_TABLE (table), sound_player_file_dialog_button, 2, 4, 1, 2);

	gtk_widget_show (sound_player_file_dialog_button);

	gtk_widget_show (frame);

	/* Sound Files frame */
	/* Sounds preferences -- player and various sounds */

	frame = gtk_frame_new (_("Sound Files"));
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	table = gtk_table_new (2, 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_container_add (GTK_CONTAINER (frame), table);

	sound_xqf_start_file_dialog_button = pref_sound_conf_append(sound_xqf_start, _("XQF Start"), table, pos++);
	sound_xqf_quit_file_dialog_button = pref_sound_conf_append(sound_xqf_quit, _("XQF Quit"), table, pos++);
	sound_update_done_file_dialog_button = pref_sound_conf_append(sound_update_done, _("Update Done"), table, pos++);
	sound_refresh_done_file_dialog_button = pref_sound_conf_append(sound_refresh_done, _("Refresh Done"), table, pos++);
	sound_stop_file_dialog_button = pref_sound_conf_append(sound_stop, _("Stop"), table, pos++);
	sound_server_connect_file_dialog_button = pref_sound_conf_append(sound_server_connect, _("Server Connect"), table, pos++);
	sound_redial_success_file_dialog_button = pref_sound_conf_append(sound_redial_success, _("Redial Success"), table, pos++);

	gtk_widget_show(table);
	gtk_widget_show(frame);

	gtk_widget_show(page_vbox);

	return page_vbox;
}

// constructor for generic_prefs
static struct generic_prefs* new_generic_prefs (void) {
	int i;

	struct generic_prefs* new_genprefs;

	new_genprefs = g_malloc0 (sizeof (struct generic_prefs) * UNKNOWN_SERVER);

	pref_q1_top_color    = default_q1_top_color;
	pref_q1_bottom_color = default_q1_bottom_color;

	pref_qw_top_color    = default_qw_top_color;
	pref_qw_bottom_color = default_qw_bottom_color;

	pref_b_switch     = default_b_switch;
	pref_w_switch     = default_w_switch;
	pref_qw_noskins      = default_qw_noskins;
	pref_q2_noskins      = default_q2_noskins;

	pref_qw_skin      = g_strdup (default_qw_skin);
	pref_q2_skin      = g_strdup (default_q2_skin);

	new_genprefs[Q1_SERVER].add_options_to_notebook = add_q1_options_to_notebook;
	new_genprefs[QW_SERVER].add_options_to_notebook = add_qw_options_to_notebook;
	new_genprefs[Q2_SERVER].add_options_to_notebook = add_q2_options_to_notebook;
	new_genprefs[Q3_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[UN_SERVER].add_options_to_notebook = add_un_options_to_notebook;
	new_genprefs[UT2_SERVER].add_options_to_notebook = add_ut2_options_to_notebook;
	new_genprefs[WO_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[WOET_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[ETL_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[DOOM3_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[Q4_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[ETQW_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[COD_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[CODUO_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[COD2_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[COD4_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[CODWAW_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[JK2_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[JK3_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[SOF2S_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[T2_SERVER].add_options_to_notebook = add_t2_options_to_notebook;
	new_genprefs[EF_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[NEXUIZ_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[XONOTIC_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[WARSOW_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[TREMULOUS_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[TREMULOUSGPP_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[UNVANQUISHED_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[OPENARENA_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[Q3RALLY_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[WOP_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[IOURT_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[REACTION_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[SMOKINGUNS_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[ZEQ2LITE_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[TURTLEARENA_SERVER].add_options_to_notebook = add_q3_options_to_notebook;
	new_genprefs[ALIENARENA_SERVER].add_options_to_notebook = add_q3_options_to_notebook;

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		new_genprefs[i].pref_dir = g_strdup (games[i].dir);
		new_genprefs[i].real_dir = g_strdup (games[i].real_dir);
		g_datalist_init(&new_genprefs[i].games_data);
	}

	return new_genprefs;
}

static void generic_prefs_free(struct generic_prefs* prefs) {
	int i;
	if (!prefs) return;

	g_free(pref_qw_skin);
	g_free(pref_q2_skin);

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		g_free(prefs[i].pref_dir);
		g_free(prefs[i].real_dir);
		g_datalist_clear(&prefs[i].games_data);
	}
	g_free(prefs);
}

void preferences_dialog (int page_num) {
	GValue hborder = G_VALUE_INIT;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *page;
	GtkWidget *button;
	GtkWidget *window;
	int game_num;
	int i;

	debug (5, "preferences_dialog()");

	game_num = page_num / 256;
	page_num = page_num % 256;

	genprefs = new_generic_prefs ();

	window = dialog_create_modal_transient_window (_("XQF: Preferences"), TRUE, FALSE, NULL);
	if (!gtk_widget_get_realized (window)) {
		gtk_widget_realize (window);
	}

	allocate_quake_player_colors (gtk_widget_get_window (window));

	vbox = gtk_vbox_new (FALSE, 8);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	/*
	 *  Notebook
	 */

	pref_notebook = gtk_notebook_new ();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (pref_notebook), GTK_POS_TOP);
	g_value_init (&hborder, G_TYPE_INT);
	g_value_set_int (&hborder, 4);
	g_object_set_property (G_OBJECT (pref_notebook), "tab-hborder", &hborder);
	gtk_box_pack_start (GTK_BOX (vbox), pref_notebook, FALSE, FALSE, 0);

	page = general_options_page ();
	label = gtk_label_new (_("General"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	/*
	   page = player_profile_page ();
	   label = gtk_label_new (_("Player Profile"));
	   gtk_widget_show (label);
	   gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);
	   */

	page = games_config_page (game_num);
	label = gtk_label_new (_("Games"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	page = appearance_options_page ();
	label = gtk_label_new (_("Appearance"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	page = qstat_options_page ();
	label = gtk_label_new (_("QStat"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	page = sound_options_page ();
	label = gtk_label_new (_("Sounds"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	page = scripts_config_page ();
	label = gtk_label_new (_("Scripts"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (pref_notebook), page, label);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (pref_notebook), page_num);

	/* Initialize skins and custom cfgs */

	q1_skin_is_valid = TRUE;
	update_q1_skin ();

	qw_skin_is_valid = TRUE;
	update_qw_skins (pref_qw_skin);

	q2_skin_is_valid = TRUE;
	update_q2_skins (pref_q2_skin);

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		update_cfgs (i, genprefs[i].real_dir, games[i].game_cfg);
	}

	gtk_widget_show (pref_notebook);

	/*
	 *  Buttons at the bottom
	 */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_set_size_request (button, 80, -1);
	g_signal_connect_swapped (G_OBJECT (button), "clicked", G_CALLBACK (gtk_widget_destroy), G_OBJECT (window));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("OK"));
	gtk_widget_set_size_request (button, 80, -1);
	g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (ok_callback), G_OBJECT(window));
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	gtk_widget_set_can_default (button, TRUE);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (vbox);

	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);

	/* clean up */

	generic_prefs_free(genprefs);
	genprefs=NULL;

	if (color_menu) {
		gtk_widget_destroy (color_menu);
		color_menu = NULL;
	}

	qw_skin_preview = NULL;
	q2_skin_preview = NULL;

	if (q1_skin_data) {
		g_free(q1_skin_data);
		q1_skin_data=NULL;
	}

	if (qw_skin_data) {
		g_free(qw_skin_data);
		qw_skin_data = NULL;
	}

	if (q2_skin_data) {
		g_free(q2_skin_data);
		q2_skin_data = NULL;
	}
}


// set some defaults when xqf is called the first time
static void user_fix_defaults (void) {
	char** files = NULL;
	char* suggested_file = NULL;
	char* guessed_dir = NULL;
	char str[256];
	int i;
	int j = 0;

	debug(1, "Setting defaults");

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		files = games[i].command;
		if (!files) continue;
		suggested_file = find_file_in_path_list_relative(files);
		if (!suggested_file) continue;

		j++;

		g_snprintf (str, 256, "/" CONFIG_FILE "/Game: %s/cmd", type2id (games[i].type));
		config_set_string (str, suggested_file);
		debug(1,"set command %s for %s",suggested_file,games[i].name);

		guessed_dir = resolve_path(suggested_file);
		if (!guessed_dir) continue;

		g_snprintf (str, 256, "/" CONFIG_FILE "/Game: %s/dir", type2id (games[i].type));
		config_set_string (str, guessed_dir);
		debug(1,"set command %s for %s",guessed_dir,games[i].name);
	}

	if (j) {
		config_set_string ("/" CONFIG_FILE "/Appearance/show only configured games","true");
		debug(2,"%d games found, set 'show only configured games' to true", j);
	}

	config_set_string ("/" CONFIG_FILE "/Games Config/player name", g_get_user_name ());
}


int fix_qw_player_color (int color) {
	color = color % 16;
	if (color > 13) {
		color = 13;
	}
	return color;
}


int init_user_info (void) {
	if (!g_get_user_name () || !g_get_home_dir () || !g_get_tmp_dir () || !g_get_user_config_dir ()) {
		xqf_error(_("Unable to get user name/home directory/XDG config directory/tmp directory"));
		return FALSE;
	}
	user_rcdir  = file_in_dir (g_get_user_config_dir (), XDG_RC_DIR);
	return TRUE;
}


void free_user_info (void) {
	if (user_rcdir) {
		g_free(user_rcdir);
		user_rcdir = NULL;
	}
}

static void prefs_load_for_game(enum server_type type) {
	char buf[256];
	struct game *g = &games[type];

	if (!g->prefs_load) {
		return;
	}

	g_snprintf (buf, sizeof(buf), "/" CONFIG_FILE "/Game: %s", type2id (type));
	config_push_prefix (buf);

	g->prefs_load(g);

	config_pop_prefix ();
}

void q1_prefs_load(struct game* g) {
	default_q1_name =              config_get_string("player name");
	default_q1_top_color =      config_get_int("top=0");
	default_q1_bottom_color =   config_get_int("bottom=0");
}

void qw_prefs_load(struct game* g) {
	default_qw_name =           config_get_string("player name");
	default_qw_team =           config_get_string("team");
	default_qw_skin =           config_get_string("skin");
	default_qw_top_color =      config_get_int("top=0");
	default_qw_bottom_color =   config_get_int("bottom=0");

	default_qw_rate =           config_get_int("rate=2500");
	default_qw_cl_nodelta =     config_get_int("cl_nodelta=0");
	default_qw_cl_predict =     config_get_int("cl_predict=1");
	default_qw_noskins =        config_get_int("noskins=0");
	default_noaim =             config_get_int("noaim=0");
	default_b_switch =          config_get_int("b_switch=0");
	default_w_switch =          config_get_int("w_switch=0");
	pushlatency_mode =          config_get_int("pushlatency mode=1");
	pushlatency_value =         config_get_int("pushlatency value=-50");
}

void q2_prefs_load(struct game* g) {
	default_q2_name =           config_get_string("player name");
	default_q2_skin =           config_get_string("skin");
	default_q2_rate =           config_get_int("rate=2500");
	default_q2_cl_nodelta =     config_get_int("cl_nodelta=0");
	default_q2_cl_predict =     config_get_int("cl_predict=1");
	default_q2_noskins =        config_get_int("noskins=0");
}

void q3_prefs_load_common(struct game* g) {
	char* tmp;
	enum server_type type = g->type;
	struct q3_common_prefs_s* w;
	char buf[256];

	w = get_pref_widgets_for_game(type);
	g_return_if_fail(w->defproto != NULL);

	g_snprintf(buf, sizeof(buf), "protocol=%s", w->defproto);

	tmp = config_get_string(buf);
	if (strlen(tmp) == 0) {
		g_free(tmp);
		tmp = NULL;
	}
	game_set_attribute(type,"masterprotocol",tmp);

	if (w->flags & Q3_PREF_SETFS_GAME) {
		game_set_attribute(type,"setfs_game",config_get_string("setfs_game=true"));
	}

	if (w->flags & Q3_PREF_PB) {
		game_set_attribute(type,"set_punkbuster",config_get_string("set_punkbuster=true"));
	}

	if (w->flags & Q3_PREF_CONSOLE) {
		game_set_attribute(type,"enable_console",config_get_string("enable_console=true"));
	}
}

void q3_prefs_load(struct game* g) {
	enum server_type type = g->type;

	q3_prefs_load_common(g);

	if ( type == Q3_SERVER ) {
		game_set_attribute(type,"pass_memory_options",config_get_string("pass_memory_options=false"));
		game_set_attribute(type,"com_hunkmegs",config_get_string("com_hunkmegs=56"));
		game_set_attribute(type,"com_zonemegs",config_get_string("com_zonemegs=16"));
		game_set_attribute(type,"com_soundmegs",config_get_string("com_soundmegs=8"));
	}
}

void tribes2_prefs_load(struct game* g) {
	default_t2_name = config_get_string("player name");
}

int prefs_load (void) {
	char *oldversion;
	int i;
	int old_rc_loaded;
	int newversion = FALSE;

	oldversion = config_get_string("/" CONFIG_FILE "/Program/version");

	old_rc_loaded = rc_parse ();
	if (old_rc_loaded == 0) {
		rc_save ();
	}

	if (oldversion) {
		newversion = g_ascii_strcasecmp (oldversion, XQF_VERSION);
		g_free(oldversion);
	}
	else {
		newversion = TRUE;
		if (old_rc_loaded == -1) {
			user_fix_defaults();
		}
	}

	do {
		int def = 0;
		struct rlimit rlim;
		char* coresize = NULL;

		if (getrlimit(RLIMIT_CORE, &rlim)) break;

		coresize = config_get_string_with_default ("/" CONFIG_FILE "/Program/coresize=0",&def);
		if (def || !coresize) break;

		if (!strcmp(coresize,"unlimited")) {
			rlim.rlim_cur = RLIM_INFINITY;
		}
		else {
			char* endptr = NULL;
			long int size = strtol(coresize,&endptr,10);
			if (endptr == coresize || size < 0) break;
			rlim.rlim_cur = size;
		}

		if (rlim.rlim_cur > rlim.rlim_max) {
			rlim.rlim_max = rlim.rlim_cur;
		}

		if (setrlimit(RLIMIT_CORE,&rlim)) {
			xqf_warning("could not set core size %lu: %s",rlim.rlim_cur,strerror(errno));
		}

		g_free(coresize);
	} while (0);

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		prefs_load_for_game (i);
	}

	config_push_prefix ("/" CONFIG_FILE "/Games Config");

	default_nosound =           config_get_bool("nosound=false");
	default_nocdaudio =         config_get_bool("nocdaudio=false");

	config_pop_prefix();

	config_push_prefix ("/" CONFIG_FILE "/Appearance");

	show_hostnames =                        config_get_bool("show hostnames=true");
	show_default_port =                     config_get_bool("show default port=true");
	serverlist_countbots =                  config_get_bool("count bots=true");
	default_refresh_sorts =                 config_get_bool("sort on refresh=true");
	default_refresh_on_update =             config_get_bool("refresh on update=true");
	default_resolve_on_update =             config_get_bool("resolve on update=false");
	default_show_only_configured_games =    config_get_bool("show only configured games=false");
	default_icontheme =                     config_get_string("icontheme");

	config_pop_prefix();

	config_push_prefix ("/" CONFIG_FILE "/General");

	default_terminate =         config_get_bool("terminate=false");
	default_launchinfo =        config_get_bool("launchinfo=true");
	default_prelaunchexec =     config_get_bool("prelaunchexec=false");
	default_save_lists =        config_get_bool("save lists=true");
	default_save_srvinfo =      config_get_bool("save srvinfo=true");
	default_save_plrinfo =      config_get_bool("save players=false");
	default_auto_favorites =    config_get_bool("refresh favorites=false");
	default_auto_maps =         config_get_bool("search maps=false");

	config_pop_prefix();

	config_push_prefix ("/" CONFIG_FILE "/QStat");

	maxretries =                config_get_int("maxretires=3");
	maxsimultaneous =           config_get_int("maxsimultaneous=20");
	qstat_srcport_low =         config_get_int("port_low=0");
	qstat_srcport_high =        config_get_int("port_high=0");
	qstat_srcport_changed =     FALSE;
	qstat_srcip =               config_get_string("srcip");
	qstat_srcip_changed =       FALSE;

	config_pop_prefix();

	config_push_prefix ("/" CONFIG_FILE "/Sounds");

	sound_enable =              config_get_bool("sound_enable=false");
	sound_player =              config_get_string("sound_player");
	sound_xqf_start =           config_get_string("sound_xqf_start");
	sound_xqf_quit =            config_get_string("sound_xqf_quit");
	sound_update_done =         config_get_string("sound_update_done");
	sound_refresh_done =        config_get_string("sound_refresh_done");
	sound_stop =                config_get_string("sound_stop");
	sound_server_connect =      config_get_string("sound_server_connect");
	sound_redial_success =      config_get_string("sound_redial_success");

	config_pop_prefix();

	config_set_string ("/" CONFIG_FILE "/Program/version", XQF_VERSION);
	config_sync();

#ifdef DEBUG
	fprintf (stderr, "prefs_load(): program version %s\n", (newversion)? "changed" : "not changed");
#endif

	/* Convert "dir" -> "real_dir" for all game types */

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
		load_game_defaults (i);

		if (default_auto_maps && !skip_startup_mapscan) {
			scan_maps_for(i);
		}

		if (games[i].cmd_or_dir_changed) {
			games[i].cmd_or_dir_changed(&games[i]);
		}
	}

	return newversion;
}

void prefs_done (void) {
	g_free(default_icontheme);
	default_icontheme = NULL;

	g_free (qstat_srcip);
	qstat_srcip = NULL;

	g_free(sound_player);
	g_free(sound_xqf_start);
	g_free(sound_xqf_quit);
	g_free(sound_update_done);
	g_free(sound_refresh_done);
	g_free(sound_stop);
	g_free(sound_server_connect);
	g_free(sound_redial_success);
	sound_player = NULL;
	sound_xqf_start = NULL;
	sound_xqf_quit = NULL;
	sound_update_done = NULL;
	sound_refresh_done = NULL;
	sound_stop = NULL;
	sound_server_connect = NULL;
	sound_redial_success = NULL;

	g_free(default_q1_name);
	default_q1_name = NULL;

	g_free(default_qw_name);
	g_free(default_qw_team);
	g_free(default_qw_skin);
	default_qw_name = NULL;
	default_qw_team = NULL;
	default_qw_skin = NULL;

	g_free(default_q2_name);
	g_free(default_q2_skin);
	default_q2_name = NULL;
	default_q2_skin = NULL;

	g_free(default_t2_name);
	default_t2_name = NULL;
}

static void scan_maps_for(enum server_type type) {
	// translator: %s = game name, e.g. Quake 3 Arena
	char* msg = g_strdup_printf(_("Searching for %s maps"),games[type].name);

	if (games[type].init_maps) {
		games[type].init_maps(games[type].type);
	}

	g_free(msg);
}

/*
void prefs_save (void) {
	config_push_prefix ("/" CONFIG_FILE "/Preferences");
	config_set_bool ("show hostnames", show_hostnames);
	config_set_bool ("show default port", show_default_port);
	config_pop_prefix();
	config_sync();
}
*/

void game_file_dialog_response_callback (GtkWidget *dialog, int response, gpointer data) {
	enum server_type type = (enum server_type) GPOINTER_TO_INT(data);

	if (type >= UNKNOWN_SERVER) {
		return;
	}

	if (response == GTK_RESPONSE_ACCEPT) {
		GtkFileChooser *chooser = GTK_FILE_CHOOSER (dialog);
		char *filename = gtk_file_chooser_get_filename (chooser);

		gtk_entry_set_text (GTK_ENTRY (genprefs[type].cmd_entry), filename);
		pref_guess_dir (type, filename, TRUE);

		g_free (filename);
	}

	gtk_widget_destroy (dialog);
}

void game_file_activate_callback (enum server_type type) {
	const char* file = gtk_entry_get_text (GTK_ENTRY (genprefs[type].cmd_entry));

	pref_guess_dir (type, file, TRUE);
}

void game_file_dialog(enum server_type type) {
	file_dialog(_("Game Command Selection"), G_CALLBACK(game_file_dialog_response_callback), GINT_TO_POINTER(type));
}

void game_dir_dialog(enum server_type type) {
	if (type >= UNKNOWN_SERVER) {
		return;
	}

	file_dialog_textentry(_("Game Directory Selection"), genprefs[type].dir_entry);
}

/*
void file_dialog_destroy_callback (GtkWidget *widget, gpointer data) {
}
*/

void q1_cmd_or_dir_changed(struct game* g) {
	update_q1_skin();
}

void qw_cmd_or_dir_changed(struct game* g) {
	update_qw_skins (NULL);
}

void q2_cmd_or_dir_changed(struct game* g) {
	update_q2_skins (NULL);
}

void ut2004_cmd_or_dir_changed(struct game* g) {
	ut2004_detect_cdkey(g);
}

void doom3_cmd_or_dir_changed(struct game* g) {
	doom3_detect_version(g);
}

void tremulous_cmd_or_dir_changed(struct game* g) {
	doom3_detect_version(g);
}
