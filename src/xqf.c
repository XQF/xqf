/* XQF - Quake server browser and launcher
 * Copyright (C) 1998-2015 XQF Team - https://xqf.github.io
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
#include <stdio.h>
#include <stdlib.h>     /* atoi */
#include <sys/socket.h> /* inet_ntoa */
#include <netinet/in.h> /* inet_ntoa */
#include <arpa/inet.h>  /* inet_ntoa */
#include <time.h>       /* time */
#include <string.h>     /* strlen */
#include <ctype.h>      /* isspace */
#include <getopt.h>

// select, fork, pipe...
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>     /* kill... */
#include <sys/wait.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
#include "callbacks.h"
#include "game.h"
#include "stat.h"
#include "source.h"
#include "pref.h"
#include "filter.h"
#include "flt-player.h"
#include "skin.h"
#include "dialogs.h"
#include "utils.h"
#include "server.h"
#include "launch.h"
#include "host.h"
#include "srv-list.h"
#include "srv-info.h"
#include "srv-prop.h"
#include "rc.h"
#include "pixmaps.h"
#include "rcon.h"
#include "statistics.h"
#include "psearch.h"
#include "addserver.h"
#include "addmaster.h"
#include "sort.h"
#include "system.h"
#include "config.h"
#include "debug.h"
#include "redial.h"
#include "loadpixmap.h"
#include "scripts.h"
#include "memtopixmap.h"
#include "xqf-lists.h"
#include "xqf-server-item.h"
#include "xqf-player-item.h"

#ifdef USE_GEOIP
#include "country-filter.h"
#endif

const char gslisthome[] = "http://aluigi.altervista.org/papers.htm#gslist";

const char required_qstat_version[] = "2.10";

time_t xqf_start_time;

int dontlaunch = 0;

int event_type = 0;

char* qstat_configfile = NULL;

GtkBuilder *builder = NULL;
GtkWidget  *main_window = NULL;
GtkWidget  *source_treeview = NULL;
GtkWidget  *server_view = NULL;
GtkWidget  *player_view = NULL;
GtkWidget  *srvinf_treeview = NULL;



GSList *cur_source = NULL;          /* GSList <struct master *> */
GSList *cur_server_list = NULL;     /* GSList <struct server *> */
GSList *cur_userver_list = NULL;    /* GSList <struct userver *> */

struct server *cur_server = NULL;

struct stat_job *stat_process = NULL;

GtkWidget *main_filter_status_bar = NULL;
GtkWidget *main_progress_bar = NULL;

char *progress_bar_str = NULL;

GArray *server_filter_menu_items;

GtkWidget *player_skin_popup = NULL;
GtkWidget *player_skin_popup_preview = NULL;

gboolean check_launch (struct condef* con);
void refresh_selected_callback (GtkWidget *widget, gpointer data);
void launch_close_handler_part2 (struct condef *con);

void copy_text_to_clipboard (const char* text);

GSList* filter_menu_radio_buttons = NULL;

int redialserver = 0;

void sighandler_debug (int signum) {
	if (signum == SIGUSR1) {
		set_debug_level (get_debug_level () + 1);
	}
	else if (signum == SIGUSR2) {
		set_debug_level (get_debug_level () - 1);
	}

	debug (0, "debug level now at %d", get_debug_level ());
}

// returns 0 if equal, -1 if too old, 1 if have > expected
int compare_qstat_version (const char* have, const char* expected) {
	int have_major, expected_major;
	int have_minor, expected_minor;
	char have_pl=0, expected_pl=0;
	const char* have_pos1=NULL, *expected_pos1=NULL;
	const char* have_pos2=NULL, *expected_pos2=NULL;
	char* buf;

	debug (3, "compare_qstat_version (%s,%s)", have, expected);

	if (!strcmp (have, expected)) {
		return 0;
	}

	/* Newer version string are written this way: v2.16
	so we need to strip the starting v. */
	if (*have == 'v')
	{
		have++;
	}

	have_pos1 = strchr (have, '.');
	expected_pos1 = strchr (expected, '.');

	if (!have_pos1 || !expected_pos1) {
		return -1;
	}

	buf = g_strndup (have, have_pos1 - have);
	have_major = atoi (buf);
	g_free (buf);
	buf = g_strndup (expected, expected_pos1 - expected);
	expected_major = atoi (buf);
	g_free (buf);

	debug (3, "compare_qstat_version -- compare major %d %d", have_major, expected_major);
	if (have_major < expected_major) {
		return -1;
	}
	if (have_major > expected_major) {
		return 1;
	}

	have_pos1++;
	expected_pos1++;

	for (have_pos2 = have_pos1;
			have_pos2 && *have_pos2 && isdigit (*have_pos2);
			have_pos2++);
	for (expected_pos2 = expected_pos1;
			expected_pos2 && *expected_pos2 && isdigit (*expected_pos2);
			expected_pos2++);

	buf = g_strndup (have_pos1, have_pos2 - have_pos1);

	/* Strip extra string after the number, for example 2.16-modified
	becomes 2.16 to get the minor number properly parsed. */
	char* have_dash = strchr (buf, '-');
	if (have_dash)
	{
		*have_dash = '\0';
	}

	have_minor = atoi (buf);
	g_free (buf);
	buf = g_strndup (expected_pos1, expected_pos2 - expected_pos1);
	expected_minor = atoi (buf);
	g_free (buf);

	debug (3, "compare_qstat_version -- compare minor %d %d", have_minor, expected_minor);
	if (have_minor < expected_minor) {
		return -1;
	}
	if (have_minor > expected_minor) {
		return 1;
	}

	if (have_pos2 && *have_pos2) have_pl=*have_pos2;
	if (expected_pos2 && *expected_pos2) expected_pl=*expected_pos2;

	debug (3, "compare_qstat_version -- compare pl %c %c", have_pl, expected_pl);
	if (!have_pl && expected_pl) {
		return -1;
	}
	if (have_pl && expected_pl && have_pl < expected_pl) {
		return -1;
	}

	return 1;
}

void qstat_version_string (struct external_program_connection* conn) {
	const char search_for[] = "qstat version";
	const char *ptr, *version_end;
	char* found_version = NULL;

	if (!conn) {
		return;
	}

	// we already found a valid version string
	if (conn->result) {
		return;
	}
	if (!conn->current_line) {
		return;
	}

	if (!strncmp (conn->current_line, search_for, strlen (search_for))) {
		ptr = conn->current_line + strlen (search_for);
		// skip whitespace
		for (; isspace (*ptr); ++ptr);
		if (!*ptr) {
			conn->result = FALSE;
			return;
		}
		// skip until end of version string
		for (version_end = ptr;
				version_end &&
				*version_end != '\0' && !isspace (*version_end);
				++version_end);
		found_version = g_strndup (ptr, version_end - ptr);
		// debug (0, "found version <%s>", found_version);

		if (compare_qstat_version (found_version, required_qstat_version) >= 0) {
			conn->result = TRUE;
		}

		g_free (found_version);
	}
}

int check_qstat_version () {
	char* cmd[] = {QSTAT_EXEC, NULL};

	return external_program_foreach_line (cmd, qstat_version_string, NULL);
}


int forced_filters_flag = FALSE;


void set_filters (unsigned char mask) {
	unsigned n;
	int i;

	forced_filters_flag = TRUE;

	cur_filter = mask;

	for (i = 0, n = 1; i < FILTERS_TOTAL; i++, n <<= 1) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_buttons[i]),
				((cur_filter & n) != 0)? TRUE : FALSE);
	}

	forced_filters_flag = FALSE;
}


void filter_toggle_callback (GtkWidget *widget, unsigned char mask) {


	if (!forced_filters_flag) {
		cur_filter ^= mask;
		server_list_build_filtered (cur_server_list, FALSE); /* in srv-list.c */
		reset_main_status_bar(builder);
	}
}

void filter_menu_activate_current () {
	/* Filter menu radio buttons removed in GTK4 migration; no-op. */
}


void set_server_filter_menu_list_text (void){

	char status_buf[64];
	char* name;

	/* Show the active filter on the status bar
	   -- Add code to indicate if the filter button is checked.
	*/
	if (current_server_filter == 0) {
		snprintf (status_buf, 64, _("No Server Filter Active"));
	}
	else {
		name = g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1)->filter_name;
		if (name) {
			snprintf (status_buf, 64, _("Server Filter: %s"), name);
		}
		else {
			snprintf (status_buf, 64, _("Server Filter: %d"), current_server_filter);
			xqf_error ("this is a bug");
		}
	}

	print_status (main_filter_status_bar, status_buf);

	reset_main_status_bar(builder);
}


void server_filter_select_callback (GtkWidget *widget, int number) {

	if (!GTK_IS_CHECK_MENU_ITEM (widget)) {
		g_warning ("no check menu item");
		return;
	}

	if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (widget)) == 0) {
		// signal was triggered for deactivation
		return;
	}

	current_server_filter = number;

	filters[FILTER_SERVER].changed = FILTER_CHANGED;
	filters[FILTER_SERVER].last_changed = filter_time_inc ();

	server_list_build_filtered (cur_server_list, FALSE); // in srv-list.c
	set_server_filter_menu_list_text ();

	config_push_prefix ("/" CONFIG_FILE "/Server Filter");
	config_set_int ("current_server_filter", current_server_filter);
	config_pop_prefix ();

	return;
}

void start_preferences_dialog (GtkWidget *widget, int page_num) {
	preferences_dialog (page_num);
}


void start_filters_cfg_dialog (GtkWidget *widget, int page_num) {
	if (filters_cfg_dialog (page_num)) {
		config_sync ();
		rc_save ();
		filter_menu_activate_current ();

		/* refresh filter status*/
		set_server_filter_menu_list_text ();

		//happes automagically   server_list_build_filtered (cur_server_list, TRUE);
		player_list_redraw ();
	}
}


void update_server_lists_from_selected_source (void) {
	GSList *cur_masters = NULL;

	debug (6, "update_server_lists_from_selected_source() --");
	if (cur_server_list) {
		debug (6, "update_server_lists_from_selected_source() -- Free cur_server_list %lx", cur_server_list);
		server_list_free (cur_server_list);
		cur_server_list = NULL;
	}

	if (cur_userver_list) {
		userver_list_free (cur_userver_list);
		cur_userver_list = NULL;
	}

	master_selection_to_lists (cur_source, &cur_masters, &cur_server_list, &cur_userver_list);
	collate_server_lists (cur_masters, &cur_server_list, &cur_userver_list);
}


// Update ui with acquired data during refresh and when refresh is done

int stat_lists_refresh (struct stat_job *job) {
	int items;

	items = g_slist_length (job->delayed.queued_servers) +
		g_slist_length (job->delayed.queued_hosts);

	debug (6, "stat_lists_refresh() -- Job %lx, items %d", job, items);

	if (items>100) {
		update_server_lists_from_selected_source ();
		server_list_build_filtered (cur_server_list, TRUE);
		job->delayed.queued_servers = NULL;
		job->delayed.queued_hosts = NULL;
	}
	else if (items) {
		g_slist_foreach (job->delayed.queued_servers, (GFunc) server_list_refresh_server, NULL);
		server_list_free (job->delayed.queued_servers);
		job->delayed.queued_servers = NULL;

		g_slist_foreach (job->delayed.queued_hosts, (GFunc) server_list_show_hostname, NULL);
		host_list_free (job->delayed.queued_hosts);
		job->delayed.queued_hosts = NULL;
	}

	if (job->progress.tasks > 0) {
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(main_progress_bar), ((float)job->progress.done) / job->progress.tasks);
	}

	return TRUE;
}


void stat_lists_state_handler (struct stat_job *job, enum stat_state state) {
	if (!main_window) {
		return;
	}

	switch (state) {

		case STAT_UPDATE_SOURCE:
			progress_bar_str = _("Updating lists...");
			break;

		case STAT_RESOLVE_NAMES:
			progress_bar_str = _("Resolving host names: %d/%d");
			break;

		case STAT_REFRESH_SERVERS:
			progress_bar_str = _("Refreshing: %d/%d");
			break;

		case STAT_RESOLVE_HOSTS:
			progress_bar_str = _("Resolving host addresses: %d/%d");
			break;

		default:
			progress_bar_str = NULL;
			break;
	}

	progress_bar_start (main_progress_bar, state == STAT_UPDATE_SOURCE || job->progress.tasks <= 1);

	stat_lists_refresh (job);   /* flush */
}


void stat_lists_close_handler (struct stat_job *job, int killed) {

	if (!main_window) {
		return;
	}

	if (job->need_redraw) {
		update_server_lists_from_selected_source ();
		server_list_build_filtered (cur_server_list, TRUE);
	}

	reset_main_status_bar(builder);

	progress_bar_reset (main_progress_bar);

	stat_process = NULL;
	set_widgets_sensitivity (builder);
}


void stat_lists_server_handler (struct stat_job *job, struct server *s) {
	if (s == cur_server) {
		if (job->delayed.refresh_handler) {
			(*job->delayed.refresh_handler) (job);
		}
		player_list_set_server (s);
		srvinf_treeview_set_server (s);
	}
}


void stat_lists_master_handler (struct stat_job *job, struct master *m) {
	source_treeview_show_node_status (m);
}


void stat_lists (GSList *masters, GSList *names, GSList *servers, GSList *hosts) {

	if (stat_process || (!masters && !names && !servers && !hosts)) {
		return;
	}
	debug_increase_indent ();
	debug (7, "stat_lists() -- Server List %p", servers);

	stat_process = stat_job_create (masters, names, servers, hosts);

	stat_process->delayed.refresh_handler = (GSourceFunc) stat_lists_refresh;

	stat_process->state_handlers = g_slist_prepend (stat_process->state_handlers, stat_lists_state_handler);
	stat_process->close_handlers = g_slist_prepend (stat_process->close_handlers, stat_lists_close_handler);

	stat_process->server_handlers = g_slist_append (stat_process->server_handlers, stat_lists_server_handler);
	stat_process->master_handlers = g_slist_append (stat_process->master_handlers, stat_lists_master_handler);

	stat_start (stat_process);
	set_widgets_sensitivity (builder);
	debug_decrease_indent ();
}


void stat_one_server (struct server *server) {
	GSList *list;

	debug (6, "Server %lx", server);
	if (!stat_process && server) {
		list = server_list_prepend (NULL, server);
		stat_lists (NULL, NULL, list, NULL);
	}
}

void launch_close_handler (struct stat_job *job, int killed) {
	struct condef* con;

	con = (struct condef *) job->data;
	job->data = NULL;

	if (killed) {
		condef_free (con);
		return;
	}

	g_timeout_add (0,(GSourceFunc)check_launch, (gpointer)con);
}

/** called from inside timer, always return FALSE to stop it */
gboolean check_launch (struct condef* con) {
	struct server_props *props;
	gboolean launch = FALSE;

	struct server *s;

	if (!con) {
		return FALSE;
	}

	s = con->s;
	props = properties (s);

	if (props && props->sucks) {
		char* comment = props->comment;
		if (comment && strlen (comment)) {
			comment = g_strdup_printf (_("\n\nThe server sucks for the following reason:\n%s"), props->comment);
		}
		else {
			comment = NULL;
		}
		launch = dialog_yesno (NULL, 1, _("Yes"), _("No"),
				/* translator: %s = optional reason why the server sucks */
				_("You said this servers sucks.\nDo you want to risk a game this time?%s"), comment?comment:"");

		g_free (comment);

		if (!launch) {
			condef_free (con);
			return FALSE;
		}
	}

	if (s->flags & SERVER_INCOMPATIBLE) {
		launch = dialog_yesno (NULL, 1, _("Yes"), _("No"),
				_("This server is not compatible with the version of %s you have"
					" installed.\nLaunch client anyway?"), games[s->type].name);

		if (!launch) {
			condef_free (con);
			return FALSE;
		}
	}

	if (s->ping >= MAX_PING) {
		launch = dialog_yesno (NULL, 1, _("Launch"), _("Cancel"),
				_("Server %s:%d is %s.\n\nLaunch client anyway?"),
				(s->host->name)? s->host->name : inet_ntoa (s->host->ip),
				s->port,
				(s->ping == MAX_PING)? "unreachable" : "down");
		if (!launch) {
			condef_free (con);
			return FALSE;
		}
	}

	if (!launch && !con->spectate && server_need_redial (s, props)) {
		launch = dialog_yesnoredial (NULL, 1, _("Launch"), _("Cancel"), _("Redial"),
				_("Server %s:%d is full.\n\nLaunch client anyway?"),
				(s->host->name)? s->host->name : inet_ntoa (s->host->ip),
				s->port);
		if (!launch) {
			condef_free (con);
			return FALSE;
		}
		else if (launch == 2)
		{
			redialserver = 1;

			launch = redial_dialog (con->s, props);

			if (launch == FALSE) {
				condef_free (con);
				return FALSE;
			}
		}
		else
			redialserver = 0;
	}

	launch_close_handler_part2 (con);

	return FALSE;
}


void launch_close_handler_part2 (struct condef *con) {
	struct server_props *props;
	int launch = FALSE;
	int save = 0;
	FILE *f;
	char *fn;
	char *temp_name;
	char *temp_mod;
	char *temp_game;

	struct server *s;

	if (redialserver == 1) // was called from a redial
		play_sound (sound_redial_success, 0);
	else
		play_sound (sound_server_connect, 0);

	redialserver = 0; // Cancel any redialing

	s = con->s;
	props = properties (s);

	debug (3, "rest of launch_close_handler"); // alex

	if (con->spectate) {
		if ((s->flags & SERVER_SP_PASSWORD) == 0) {
			// XXX: what's this good for? looks useless to me -- ln
			con->spectator_password = g_strdup ("1");
		}
		else {
			if (props && props->spectator_password) {
				con->spectator_password = g_strdup (props->spectator_password);
			}
			else {
				con->spectator_password = enter_string_with_option_dialog (FALSE,
						_("Save Password"), &save, _("Spectator Password:"));
				if (!con->spectator_password) {
					condef_free (con);
					return;
				}
				if (save) {
					if (!props) {
						props = properties_new (s->host, s->port);
					}
					props->spectator_password = g_strdup (con->spectator_password);
					props_save ();
				}
			}
		}
	}
	else {
		if (props && props->server_password) {
			con->password = g_strdup (props->server_password);
		}
		else if ((s->flags & SERVER_PASSWORD) != 0) {
			con->password = enter_string_with_option_dialog (FALSE,
					_("Save Password"), &save, _("Server Password:"));
			if (!con->password) {
				condef_free (con);
				return;
			}
			if (save) {
				if (!props) {
					props = properties_new (s->host, s->port);
				}
				props->server_password = g_strdup (con->password);
				props_save ();
			}
		}
	}

	// con->server = g_strdup_printf ("%s:%5d", inet_ntoa (s->host->ip), s->port);
	con->server = g_strdup_printf ("%s:%d", inet_ntoa (s->host->ip), s->port);
	con->gamedir = g_strdup (s->game);

	if (props && props->rcon_password) {
		con->rcon_password = g_strdup (props->rcon_password);
	}

	if (props && props->custom_cfg) {
		con->custom_cfg = g_strdup (props->custom_cfg);
	}


	launch = client_launch (con, TRUE);
	condef_free (con);

	if (!launch) {
		return;
	}

	// Save address etc to LaunchInfo.txt
	if (default_launchinfo) {
		fn = file_in_dir (user_rcdir, LAUNCHINFO_FILE);

		f = fopen (fn, "w");
		if (f) {

			temp_name = s->name;
			temp_game = s->game;
			temp_mod = s->gametype;

			if (!temp_name) {
				temp_name = "";
			}

			fprintf (f, "GameType %s\n", games[s->type].name);
			fprintf (f, "ServerName %s\n", temp_name);
			fprintf (f, "ServerAddr %s:%d\n", inet_ntoa (s->host->ip), s->port);

			fprintf (f, "ServerMod ");
			if (temp_game) {
				fprintf (f, "%s", temp_game);
			}
			if (temp_game && temp_mod) {
				fprintf (f, ", ");
			}
			if (temp_mod) {
				fprintf (f, "%s", temp_mod);
			}
			fprintf (f, "\n");

			fclose (f);
		}
		g_free (fn);
	}

	// Launch pre-launch script
	if (default_prelaunchexec) {
		if (fork () == 0) {
			char *launchargv[4];
			launchargv[0] = g_strdup_printf ("%s/PreLaunch", user_rcdir);
			launchargv[1] = games[s->type].qstat_str;
			launchargv[2] = g_strdup_printf ("%s:%d", inet_ntoa (s->host->ip), s->port);
			launchargv[3] = NULL;
			execv (launchargv[0], launchargv);
			xqf_error ("PreLaunch failed");
			_exit (EXIT_FAILURE);
		}
	}

	if (main_window && default_terminate) {
		gtk_widget_destroy (main_window);
	}
}


void launch_server_handler (struct stat_job *job, struct server *s) {

	server_list_refresh_server (s);

	if (s == cur_server) {
		player_list_set_server (s);
		srvinf_treeview_set_server (s);
	}

	// no connection defined, maybe because of a hostname instead of ip specified
	// on command line. just take first server to launch
	if (!job->data) {
		job->data = condef_new (s);
	}

	/* Don't spend time on host name lookups */

	if (job->hosts) {
		host_list_free (job->hosts);
		job->hosts = NULL;
	}
}

// Restructure: Front callback calls callback core with specified action, action sits in another function

void launch_core1_callback () {

	debug (6, "launch_callback() --");
	if (stat_process || !cur_server || (games[cur_server->type].flags & GAME_CONNECT) == 0) {
		return;
	}
	if (games[cur_server->type].config_is_valid &&
			!(*games[cur_server->type].config_is_valid) (cur_server)) {
		start_preferences_dialog (NULL, PREF_PAGE_GAMES + cur_server->type * 256);
		return;
	}

}

void launch_core2_callback (int spectate, char *demo, struct condef *con) {

	con = condef_new (cur_server);
	con->demo = demo;
	con->spectate = spectate;

	stat_process = stat_job_create (NULL, NULL, server_list_prepend (NULL, cur_server), NULL);
	stat_process->data = con;

	stat_process->state_handlers = g_slist_prepend (
			stat_process->state_handlers, stat_lists_state_handler);
	stat_process->close_handlers = g_slist_prepend (
			stat_process->close_handlers, stat_lists_close_handler);

	stat_process->server_handlers = g_slist_append (
			stat_process->server_handlers, launch_server_handler);

	/* run last!!! */
	stat_process->close_handlers = g_slist_append (
			stat_process->close_handlers, launch_close_handler);

	stat_start (stat_process);
	set_widgets_sensitivity (builder);

	return;

}

void launch_normal_callback (GtkWidget *widget) {

	int spectate = FALSE;
	char *demo = NULL;
	struct condef *con = NULL;

	launch_core1_callback ();
	launch_core2_callback (spectate, demo, con);

	return;
}

void launch_spectate_callback (GtkWidget *widget) {

	int spectate = FALSE;
	char *demo = NULL;
	struct condef *con = NULL;

	launch_core1_callback ();

	if ((cur_server->flags & SERVER_SPECTATE) == 0) {
		return;
	}

	spectate = TRUE;

	launch_core2_callback (spectate, demo, con);

	return;

}

void launch_record_callback (GtkWidget *widget) {

	int spectate = FALSE;
	char *demo = NULL;
	struct condef *con = NULL;

	launch_core1_callback ();

	if ((games[cur_server->type].flags & GAME_RECORD) == 0) {
		return;
	}

	if ((games[cur_server->type].flags & GAME_SPECTATE) != 0) {
		demo = enter_string_with_option_dialog (TRUE, _("Spectator"), &spectate, _("Demo name:"));
	}

	else {
		demo = enter_string_dialog (TRUE, _("Demo name:"));
	}

	if (!demo) {
		return;
	}

	launch_core2_callback (spectate, demo, con);

	return;

}

int server_list_sort_mode = SORT_SERVER_PING;

void server_list_set_sort_mode (enum ssort_mode mode) {
	server_list_sort_mode = mode;
}



void update_source_callback (GtkWidget *widget, gpointer data) {
	GSList *masters = NULL;
	GSList *servers = NULL;
	GSList *uservers = NULL;

	event_type = EVENT_UPDATE;

	debug_increase_indent ();
	debug (6, "update_source_callback() -- ");
	if (stat_process || !cur_source) {
		return;
	}

	master_selection_to_lists (cur_source, &masters, &servers, &uservers);
	if (masters || servers || uservers) {
		stat_lists (masters, uservers, servers, NULL);
	}
	debug_decrease_indent ();
}


void refresh_selected_callback (GtkWidget *widget, gpointer data) {
	GSList *list;

	if (stat_process) {
		debug (1, "nope");
		return;
	}

	event_type = EVENT_REFRESH_SELECTED;

	list = server_list_selected_servers ();

	if (list) {
		stat_lists (NULL, NULL, list, NULL);
	}
}


void refresh_callback (GtkWidget *widget, gpointer data) {
	GSList *servers;
	GSList *uservers;

	if (stat_process) {
		return;
	}

	event_type = EVENT_REFRESH;

	debug (7, "refresh_callback() -- Get Server List");

	servers = server_list_all_servers ();
	uservers = userver_list_copy (cur_userver_list);

	debug (7, "refresh_callback() -- server list %lx", servers);

	if (servers || uservers) {
		stat_lists (NULL, uservers, servers, NULL);
	}
}

void refresh_n_server (GtkWidget * button, gpointer *data) {
	GSList *list;
	gint number;

	if (stat_process)
		return;

	event_type = EVENT_REFRESH;
	number = GPOINTER_TO_INT (data);

	list = server_list_get_n_servers (number);

	if (list) {
		stat_lists (NULL, NULL, list, NULL);
	}
}

void stop_callback (GtkWidget *widget, gpointer data) {

	event_type = 0;		// To prevent sound from stopped action from playing

	if (stat_process) {
		stat_stop (stat_process);
		stat_process = NULL;
	}
	redialserver = 0;	// Reset redialserver so prompt comes up next time
	play_sound (sound_stop, 0);
	event_type = 0;		// To prevent sound from stopped action from playing
}


void add_to_favorites_callback (GtkWidget *widget, gpointer data) {
	GSList *list;
	GSList *tmp;
	enum { buflen = 256 };
	char buf[buflen];	/* if you change this, change the statement below */
	int server_list_size;

	debug (7, "add_to_favorites_callback() -- ");

	list = server_list_selected_servers ();
	if (stat_process || !list) {
		server_list_free (list);
		return;
	}
	if (list) {
		for (tmp = list; tmp; tmp = tmp->next) {
			server_list_size = g_slist_length (favorites->servers);
			favorites->servers = server_list_append (favorites->servers, (struct server *) tmp->data);
			if (server_list_size < g_slist_length (favorites->servers)){
				snprintf (buf, buflen, "Added Server #%d: '%s'",
						  g_slist_length (favorites->servers),
						  ((struct server *)tmp->data)->name);
			} else {
				snprintf (buf, buflen, "Server '%s' Already In Favorites",
						  ((struct server *)tmp->data)->name);
			}
			print_status (GTK_WIDGET (gtk_builder_get_object (builder, "main-status-bar")), buf);

		}
		debug (7, "add_to_favorites_callback() -- Saving To Favorites");
		save_favorites ();
		server_list_free (list);
	}
}

// add a server to favorites
void new_server_to_favorites (struct stat_job *job, struct server *s) {

	debug (6, "Server %lx, job %p", s, job);
	favorites->servers = server_list_append (favorites->servers, s);
	save_favorites ();

	source_treeview_select_source (favorites);

	if (cur_filter != 0) {
		set_filters (0);	/* turn off filters */
		if (job) {
			job->need_redraw = FALSE;
		}
	}

	{
		guint pos = server_store_find (s);
		if (pos != G_MAXUINT) {
			server_list_select_one ((int) pos);
			server_list_selection_visible ();
		}
	}
}


void add_server_name_handler (struct stat_job *job, struct userver *us,
		enum dns_status status) {
	if (us->s) {
		new_server_to_favorites (job, us->s);
	}
	else {
		progress_bar_reset (main_progress_bar);
		dialog_ok (NULL, _("Host %s not found"), us->hostname);
	}
}

/** check specified address is valid, resolve hostname if needed, stat server,
 * finally call new_server_to_favorites
 * calls free (str)!!!
 */
void prepare_new_server_to_favorites (enum server_type type, char* str, gboolean dolaunch) {
	char *addr;
	unsigned short port;
	struct host *h;
	struct server *s = NULL;
	struct userver *us = NULL;

	if (!str || !*str) {
		return;
	}

	if (!parse_address (str, &addr, &port)) {
		dialog_ok (NULL, _("\"%s\" is not valid host[:port] combination."), str);
		g_free (str);
		return;
	}

	g_free (str);

	h = host_add (addr);
	if (h) {	/* IP address */
		host_ref (h);
		s = server_add (h, port, type);
		if (s) {
			new_server_to_favorites (NULL, s);
		}
		host_unref (h);
	}
	else {		/* hostname */
		us = userver_add (g_ascii_strdown (addr, -1), port, type);
	}
	g_free (addr);

	if (s || us) {
		stat_process = stat_job_create (NULL,
				(us)? userver_list_add (NULL, us) : NULL,
				(s)? server_list_prepend (NULL, s) : NULL,
				NULL);

		stat_process->state_handlers = g_slist_prepend (
				stat_process->state_handlers, stat_lists_state_handler);
		stat_process->close_handlers = g_slist_prepend (
				stat_process->close_handlers, stat_lists_close_handler);

		// stat_process->server_handlers = g_slist_append (stat_process->server_handlers, stat_lists_server_handler);

		stat_process->name_handlers = g_slist_prepend (
				stat_process->name_handlers, add_server_name_handler);

		if (dolaunch) {
			if (s) {
				struct condef* con = condef_new (s);
				stat_process->data = con;
			}

			stat_process->server_handlers = g_slist_append (
					stat_process->server_handlers, launch_server_handler);

			stat_process->close_handlers = g_slist_append (
					stat_process->close_handlers, launch_close_handler);
		}

		stat_start (stat_process);
		set_widgets_sensitivity (builder);
	}
}

void add_server_callback (GtkWidget *widget, gpointer data) {
	char *str = NULL;
	enum server_type type = UNKNOWN_SERVER;

	if (stat_process) {
		return;
	}

	str = add_server_dialog (&type, NULL);

	if (!str) {
		return;
	}

	prepare_new_server_to_favorites (type, str, FALSE);

	return;
}

void del_server_callback (GtkWidget *widget, gpointer data) {
	GSList *selected;
	GSList *l, *c;
	int is_favorites = 0;
	int delete_from_all = 0;

	debug (3, "--");

	if (stat_process || !cur_source) {
		return;
	}

	selected = server_list_selected_servers ();

	if (!selected) {
		return;
	}

	for (c = cur_source; c; c = c->next) {
		struct master* m = (struct master *) c->data;

		if (m == favorites) {
			is_favorites = 1;
		}

		if (!delete_from_all && m->isgroup) {
			delete_from_all = dialog_yesno (NULL, 1, _("Yes"), _("No"), _("Remove selected servers from all lists?"));
		}
	}

	for (l = selected; l; l = l->next) {
		struct server* s = (struct server*) l->data;

		if (delete_from_all) {
			server_remove_from_all (s);
			master_remove_server (favorites, s);
		}
		else {
			for (c = cur_source; c; c = c->next) {
				struct master* m = (struct master *) c->data;

				master_remove_server (m, s);
			}
		}
	}

	if (is_favorites) {
		save_favorites ();
	}

	g_slist_free (selected);

	update_server_lists_from_selected_source ();
	server_list_build_filtered (cur_server_list, FALSE);
}


void copy_server_callback (GtkWidget *widget, gpointer data) {
	GSList *selection = server_list_selected_servers ();
	if (!selection) return;
	GString *str = g_string_new ("");
	for (GSList *cur = selection; cur; cur = cur->next) {
		struct server *s = (struct server *) cur->data;
		if (str->len > 0) g_string_append_c (str, '\n');
		g_string_append_printf (str, "%s:%d", inet_ntoa (s->host->ip), s->port);
	}
	server_list_free (selection);
	gdk_clipboard_set_text (gdk_display_get_clipboard (gdk_display_get_default ()), str->str);
	g_string_free (str, TRUE);
}

void copy_server_info_callback (GtkWidget *widget, gpointer data) {
	srvinf_copy_server_info ();
}

void copy_text_to_clipboard (const char* text) {
	if (text && *text)
		gdk_clipboard_set_text (gdk_display_get_clipboard (gdk_display_get_default ()), text);
}

void copy_server_callback_plus (GtkWidget *widget, gpointer data) {
	GSList *selection = server_list_selected_servers ();
	if (!selection) return;
	GString *str = g_string_new ("");
	for (GSList *cur = selection; cur; cur = cur->next) {
		struct server *s = (struct server *) cur->data;
		unsigned players = s->curplayers;
		if (serverlist_countbots && s->curbots <= players)
			players -= s->curbots;
		if (str->len > 0) g_string_append_c (str, '\n');
		g_string_append_printf (str, "%i  %s:%d  %s  %s  %i of %i", s->ping,
		                        inet_ntoa (s->host->ip), s->port, s->name, s->map,
		                        players, s->maxplayers);
	}
	server_list_free (selection);
	gdk_clipboard_set_text (gdk_display_get_clipboard (gdk_display_get_default ()), str->str);
	g_string_free (str, TRUE);
}

void update_master_builtin_callback (GtkWidget *widget, gpointer data) {

	update_master_list_builtin ();
}

void update_master_gslist_callback (GtkWidget *widget, gpointer data) {
	if (!have_gslist_installed ()) {
		// translator: %s == url
		dialog_ok (NULL, _("For Gslist support you must install the 'gslist' program available from\n%s\n"
					"Don't forget that you need to run 'gslist -u' before you can use it."), gslisthome);
		copy_text_to_clipboard (gslisthome);
		return;
	}
	update_master_gslist_builtin ();
}

void add_master_callback (GtkWidget *widget, gpointer data) {
	struct master *m;

	if (stat_process) {
		return;
	}

	m = add_master_dialog (NULL);

	if (m) {
		source_treeview_add_master (m);
		source_treeview_select_source (m);
	}
}

// does only work with one master selected
void edit_master_callback (GtkWidget *widget, gpointer data) {
	struct master *master_to_edit, *master_to_add;

	if (!cur_source) {
		return;
	}

	master_to_edit = (struct master *) cur_source->data;
	source_treeview_select_source (master_to_edit);
	master_to_add = add_master_dialog (master_to_edit);
	if (master_to_add) {
		source_treeview_add_master (master_to_add);
		source_treeview_select_source (master_to_add);
	}
}

void del_master_callback (GtkWidget *widget, gpointer data) {
	struct master *m;
	GSList *masters = NULL;
	char *master_names = NULL;
	GSList *list;
	int delete;
	char *s1;
	char *s2;

	if (stat_process) {
		return;
	}

	for (list = cur_source; list; list = list->next) {
		m = (struct master *) list->data;
		if (m != favorites && !m->isgroup) {

			masters = g_slist_append (masters, m);
			s1 = g_strdup_printf ("%s (%s)", m->name, games[m->type].name);

			if (!master_names) {
				master_names = s1;
			}
			else {
				s2= master_names;
				master_names = g_strconcat (s2, "\n", s1, NULL);
				g_free (s1);
				g_free (s2);
			}

		}
	}

	if (!masters) {
		dialog_ok (NULL, _("You have to select the server you want to delete"));
		return;
	}

	// FIXME: plural
	delete = dialog_yesno (NULL, 1, _("Delete"), _("Cancel"),
			_("Master%s to delete:\n\n%s"),
			(g_slist_length (masters) > 1)? "s" : "",
			master_names);

	if (delete) {
		for (list = masters; list; list = list->next) {
			m = (struct master *) list->data;
			source_treeview_delete_master (m);
			free_master (m);
		}
	}

	g_free (master_names);
	g_slist_free (masters);
}


void find_player_callback (GtkWidget *widget, int find_next) {
	char *pattern;

	if (find_next || find_player_dialog ()) {
		if (!psearch_data_is_empty ()) {
			pattern = psearch_lookup_pattern ();
			print_status (GTK_WIDGET (gtk_builder_get_object (builder, "main-status-bar")), _("Find Player: %s"), pattern);
			g_free (pattern);

			find_player (find_next);
		}
	}
}


void show_hostnames_callback (GSimpleAction *action, GVariant *parameter, gpointer data) {
	GVariant *state;
	gboolean new_val;

	if (stat_process || !server_store ||
	    g_list_model_get_n_items (G_LIST_MODEL (server_store)) == 0) {
		return;
	}

	state = g_action_get_state (G_ACTION (action));
	new_val = !g_variant_get_boolean (state);
	g_variant_unref (state);
	g_simple_action_set_state (action, g_variant_new_boolean (new_val));

	if (new_val != show_hostnames) {
		show_hostnames = new_val;
		server_list_redraw ();
		config_set_bool ("/" CONFIG_FILE "/Appearance/show hostnames", show_hostnames);
	}
}


void show_default_port_callback (GSimpleAction *action, GVariant *parameter, gpointer data) {
	GVariant *state;
	gboolean new_val;

	if (stat_process || !server_store ||
	    g_list_model_get_n_items (G_LIST_MODEL (server_store)) == 0) {
		return;
	}

	state = g_action_get_state (G_ACTION (action));
	new_val = !g_variant_get_boolean (state);
	g_variant_unref (state);
	g_simple_action_set_state (action, g_variant_new_boolean (new_val));

	if (new_val != show_default_port) {
		show_default_port = new_val;
		server_list_redraw ();
		config_set_bool ("/" CONFIG_FILE "/Appearance/show default port", show_default_port);
	}
}


static GSimpleActionGroup *_win_ag;
static GMainLoop *_xqf_app_loop;

/* GAction "activate" callback — quit from menu/keyboard */
static void app_quit_cb (GSimpleAction *a, GVariant *p, gpointer d) {
	(void)a; (void)p; (void)d;
	if (_xqf_app_loop) g_main_loop_quit (_xqf_app_loop);
}

/* close-request handler for the main window.
 * Returns FALSE so GTK4 proceeds to destroy the window normally,
 * which fires "destroy" → ui_done saves geometry, then we quit. */
static gboolean main_window_close_cb (GtkWidget *w, gpointer d) {
	(void)w; (void)d;
	if (_xqf_app_loop) g_main_loop_quit (_xqf_app_loop);
	return FALSE;
}

void resolve_callback (GtkWidget *widget, gpointer data) {
	GSList *selected;
	GSList *hosts;
	struct server *s;

	debug (7, "resolve_callback() --");
	if (stat_process) {
		return;
	}

	if (!show_hostnames) {
		GSimpleAction *a = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (_win_ag), "show-hostnames"));
		if (a) g_simple_action_set_state (a, g_variant_new_boolean (TRUE));
		show_hostnames = TRUE;
	}

	selected = server_list_selected_servers ();
	if (!selected) {
		return;
	}

	if (selected->next) {
		hosts = merge_hosts_to_resolve (NULL, selected);
	}
	else {

		/* always resolve if only one host is asked to */

		s = (struct server *) selected->data;
		hosts = host_list_add (NULL, s->host);
	}

	server_list_free (selected);

	stat_lists (NULL, NULL, NULL, hosts);
}


void properties_callback (GtkWidget *widget, gpointer data) {

	if (stat_process) {
		return;
	}

	if (cur_server) {
		properties_dialog (cur_server);
		set_widgets_sensitivity (builder);
	}
}

void rcon_callback (GtkWidget *widget, gpointer data) {
	struct server_props *sp;
	char *passwd = NULL;
	int save = 0;

	if (stat_process || !cur_server || (games[cur_server->type].flags & GAME_RCON) == 0) {
		return;
	}

	sp = properties (cur_server);

	if (!sp || !sp->rcon_password) {
		passwd = enter_string_with_option_dialog (FALSE,
				_("Save Password"), &save, _("Server Password:"));

		if (!passwd)	/* canceled */
			return;

		if (save) {
			if (!sp) {
				sp = properties_new (cur_server->host, cur_server->port);
			}
			sp->rcon_password = passwd;
			props_save ();
			passwd = NULL;
		}
	}

	rcon_dialog (cur_server, (passwd)? passwd : sp->rcon_password);

	if (passwd) {
		g_free (passwd);
	}
}


void server_view_select_cb (GtkWidget *widget, int row,
		int column, GdkEvent *event, GtkWidget *button) {
	(void)event; (void)column; (void)button;
	debug (7, "server_view_select_cb() -- Row %d", row);
	server_list_sync_selection ();
}

void server_view_unselect_cb (GtkWidget *widget, int row,
		int column, GdkEvent *event, GtkWidget *button) {
	debug (7, "server_view_uselect_callback() -- Row %d", row);
	server_list_sync_selection ();
}

/* Double-click on server row → connect */
static void server_view_double_click_cb (GtkGestureClick *gesture G_GNUC_UNUSED,
                                          int n_press, double x G_GNUC_UNUSED,
                                          double y G_GNUC_UNUSED, gpointer data G_GNUC_UNUSED) {
	if (n_press == 2)
		launch_normal_callback (NULL);
}

/* Key-press on server list: Enter=connect, Space=refresh-selected */
static gboolean server_view_key_cb (GtkEventControllerKey *ctrl G_GNUC_UNUSED,
                                     guint keyval, guint keycode G_GNUC_UNUSED,
                                     GdkModifierType state G_GNUC_UNUSED,
                                     gpointer data G_GNUC_UNUSED) {
	switch (keyval) {
	case GDK_KEY_Return:
	case GDK_KEY_KP_Enter:
		launch_normal_callback (NULL);
		return TRUE;
	case GDK_KEY_space:
		refresh_selected_callback (NULL, NULL);
		return TRUE;
	}
	return FALSE;
}

/* Right-click on server list → popover context menu */
static GtkWidget *server_context_popover = NULL;

static void server_view_right_click_cb (GtkGestureClick *gesture,
                                         int n_press G_GNUC_UNUSED,
                                         double x, double y,
                                         gpointer data G_GNUC_UNUSED) {
	if (!cur_server) return;
	GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));

	if (!server_context_popover) {
		GMenu *menu = g_menu_new ();
		g_menu_append (menu, _("Connect"),           "win.connect");
		g_menu_append (menu, _("Observe"),           "win.observe");
		g_menu_append (menu, _("Record Demo"),       "win.record-demo");
		g_menu_append (menu, _("Refresh Selected"),  "win.refresh-selected");
		g_menu_append (menu, _("Server Properties"), "win.properties");
		g_menu_append (menu, _("Add to Favorites"),  "win.add-to-favorites");
		g_menu_append (menu, _("DNS Lookup"),        "win.dns-lookup");
		g_menu_append (menu, _("RCON"),              "win.rcon");
		server_context_popover = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
		g_object_unref (menu);
		gtk_widget_set_parent (server_context_popover, widget);
	}

	GdkRectangle rect = { (int)x, (int)y, 1, 1 };
	gtk_popover_set_pointing_to (GTK_POPOVER (server_context_popover), &rect);
	gtk_popover_popup (GTK_POPOVER (server_context_popover));
}

/* Right-click on player list → popover context menu */
static GtkWidget *player_context_popover = NULL;

static void player_view_right_click_cb (GtkGestureClick *gesture,
                                         int n_press G_GNUC_UNUSED,
                                         double x, double y,
                                         gpointer data G_GNUC_UNUSED) {
	GtkWidget *widget = gtk_event_controller_get_widget (GTK_EVENT_CONTROLLER (gesture));

	if (!player_context_popover) {
		GMenu *menu = g_menu_new ();
		g_menu_append (menu, _("Add to Red filter"),   "win.player-filter-red");
		g_menu_append (menu, _("Add to Green filter"), "win.player-filter-green");
		g_menu_append (menu, _("Add to Blue filter"),  "win.player-filter-blue");
		player_context_popover = gtk_popover_menu_new_from_model (G_MENU_MODEL (menu));
		g_object_unref (menu);
		gtk_widget_set_parent (player_context_popover, widget);
	}

	GdkRectangle rect = { (int)x, (int)y, 1, 1 };
	gtk_popover_set_pointing_to (GTK_POPOVER (player_context_popover), &rect);
	gtk_popover_popup (GTK_POPOVER (player_context_popover));
}

GtkWidget *server_mapshot_popup = NULL;
GtkWidget *server_mapshot_popup_image = NULL;

void server_mapshot_preview_popup_show (guchar *imagedata, size_t len, int x, int y, gushort overBrightBits) {
	(void)imagedata; (void)len; (void)x; (void)y; (void)overBrightBits;
}

int server_view_event_cb (GtkWidget *widget, GdkEvent *event) {
	(void)widget; (void)event;
	return FALSE;
}


void source_selection_changed (void) {
	GtkTreeSelection *sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (source_treeview));
	GtkTreeModel *model;
	GList *rows, *l;

	debug (6, "source_selection_changed() --");
	if (cur_source) {
		g_slist_free (cur_source);
		cur_source = NULL;
	}

	rows = gtk_tree_selection_get_selected_rows (sel, &model);
	for (l = rows; l; l = l->next) {
		GtkTreeIter iter;
		gpointer mp = NULL;
		gtk_tree_model_get_iter (model, &iter, (GtkTreePath *) l->data);
		gtk_tree_model_get (model, &iter, 0 /* SOURCE_COL_MASTER */, &mp, -1);
		if (mp)
			cur_source = g_slist_append (cur_source, mp);
	}
	g_list_free_full (rows, (GDestroyNotify) gtk_tree_path_free);

	update_server_lists_from_selected_source ();
	server_list_set_list (cur_server_list);

	reset_main_status_bar(builder);
}


static void source_selection_changed_callback (GtkTreeSelection *sel G_GNUC_UNUSED,
                                               gpointer data G_GNUC_UNUSED) {
	source_selection_changed ();
}

void source_selection_clear_master_servers (void) {
	struct master *m;
	GSList* source = NULL;

	for (source = cur_source; source; source=source->next) {
		m = (struct master *) source->data;

		if (m == favorites || m->isgroup) {
			continue;
		}

		server_list_free (m->servers);
		m->servers = NULL;
	}

	update_server_lists_from_selected_source ();
	server_list_set_list (cur_server_list);

	reset_main_status_bar(builder);
}

void clear_master_servers_callback (GtkWidget *widget,
	int row, int column, GdkEvent *event, GtkWidget *button) {
	source_selection_clear_master_servers ();
}

void add_to_player_filter_red_callback (GtkWidget *widget) {
	add_to_player_filter (PLAYER_GROUP_RED);
}

void add_to_player_filter_green_callback (GtkWidget *widget) {
	add_to_player_filter (PLAYER_GROUP_GREEN);
}

void add_to_player_filter_blue_callback (GtkWidget *widget) {
	add_to_player_filter (PLAYER_GROUP_BLUE);
}

void add_to_player_filter (unsigned mask) {
	struct player *p;

	if (!cur_server || stat_process || !player_selection)
		return;

	guint pos = gtk_single_selection_get_selected (
		GTK_SINGLE_SELECTION (player_selection));
	if (pos == GTK_INVALID_LIST_POSITION)
		return;

	XqfPlayerItem *item = XQF_PLAYER_ITEM (
		g_list_model_get_item (G_LIST_MODEL (player_selection), pos));
	if (!item) return;
	p = xqf_player_item_get (item);
	g_object_unref (item);

	if (player_filter_add_player (p->name, mask)) {
		server_list_build_filtered (cur_server_list, TRUE);
		player_list_redraw ();
	}
}


/* TODO: re-implement with GTK4 GtkPopover (Phase 1) */
void player_skin_preview_popup_show (guchar *skin, int top, int bottom, int x, int y) {
	(void)skin; (void)top; (void)bottom; (void)x; (void)y;
}


void statistics_callback (GtkWidget *widget) {

	if (stat_process) {
		return;
	}

	statistics_dialog ();
}

void populate_main_toolbar (void) {
	char buf[128];
	unsigned mask;
	int i;
	GtkWidget *toolbar = GTK_WIDGET (gtk_builder_get_object (builder, "main-toolbar"));

	// Filter toggle buttons (added dynamically after toolbar buttons)
	for (i = 0, mask = 1; i < FILTERS_TOTAL; i++, mask <<= 1) {
		if (!filters[i].pix) {
			filter_buttons[i] = NULL;
			continue;
		}

		filter_buttons[i] = gtk_toggle_button_new_with_label (_(filters[i].short_name));
		g_signal_connect (G_OBJECT (filter_buttons[i]), "toggled",
		                  G_CALLBACK (filter_toggle_callback), GINT_TO_POINTER (mask));

		g_snprintf (buf, 128, _("%s Filter Enable / Disable"), _(filters[i].name));
		gtk_widget_set_tooltip_text (filter_buttons[i], buf);

		gtk_widget_show (filter_buttons[i]);
		gtk_box_append (GTK_BOX (toolbar), filter_buttons[i]);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_buttons[i]),
		                              ((cur_filter & mask) != 0) ? TRUE : FALSE);
	}

	set_toolbar_appearance (toolbar);
}


void quick_filter_entry_changed (GtkWidget* entry, gpointer data) {
	const char* text = gtk_entry_get_text (GTK_ENTRY (entry));
	int mask = 0;

	debug (3, "%d <%s>", strlen (text), text);

	if (!text || !*text) {
		if (filter_quick_get ()) {
			mask = FILTER_QUICK_MASK;
		}
		filter_quick_set (NULL);
	}
	else {
		if (!filter_quick_get ()) {
			mask = FILTER_QUICK_MASK;
		}
		filter_quick_set (text);
	}

	filters[FILTER_QUICK].last_changed = filter_time_inc ();

	filter_toggle_callback (NULL, mask);
}

void quickfilter_delete_button_clicked (GtkWidget *widget, GtkWidget* entry) {
	gtk_editable_delete_text (GTK_EDITABLE (entry), 0, -1);
	gtk_widget_grab_focus (entry);
}

/* Adaptor: call old-style (GtkWidget*, gpointer) callback from GSimpleAction */
static void action_activate_cb (GSimpleAction *action, GVariant *parameter, gpointer user_data) {
	void (*fn)(GtkWidget *, gpointer) = user_data;
	fn (NULL, NULL);
}

/* GSimpleActionGroup holds all "win.*" actions; inserted into the window widget */
GActionGroup *win_action_group (void) {
	return G_ACTION_GROUP (_win_ag);
}

static GSimpleAction *add_win_action (GtkWindow *win, const char *name, GCallback cb, gpointer data) {
	(void)win;
	GSimpleAction *a = g_simple_action_new (name, NULL);
	if (cb)
		g_signal_connect (a, "activate", cb, data);
	g_action_map_add_action (G_ACTION_MAP (_win_ag), G_ACTION (a));
	g_object_unref (a);
	return G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (_win_ag), name));
}

static GSimpleAction *add_win_toggle (GtkWindow *win, const char *name, GCallback cb, gpointer data) {
	(void)win;
	GSimpleAction *a = g_simple_action_new_stateful (name, NULL, g_variant_new_boolean (FALSE));
	if (cb)
		g_signal_connect (a, "activate", cb, data);
	g_action_map_add_action (G_ACTION_MAP (_win_ag), G_ACTION (a));
	g_object_unref (a);
	return G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (_win_ag), name));
}

extern void about_dialog (GtkWidget *widget, gpointer data);

static void register_window_actions (GtkWindow *win) {
	_win_ag = g_simple_action_group_new ();
	/* Regular actions using old-style callback adaptor */
	add_win_action (win, "statistics",         G_CALLBACK (action_activate_cb), statistics_callback);
	add_win_action (win, "quit",               G_CALLBACK (app_quit_cb), NULL);
	add_win_action (win, "add-server",         G_CALLBACK (action_activate_cb), add_server_callback);
	add_win_action (win, "add-to-favorites",   G_CALLBACK (action_activate_cb), add_to_favorites_callback);
	add_win_action (win, "remove-server",      G_CALLBACK (action_activate_cb), del_server_callback);
	add_win_action (win, "copy-server",        G_CALLBACK (action_activate_cb), copy_server_callback);
	add_win_action (win, "copy-server-plus",   G_CALLBACK (action_activate_cb), copy_server_callback_plus);
	add_win_action (win, "add-default-masters",G_CALLBACK (action_activate_cb), update_master_builtin_callback);
	add_win_action (win, "add-gslist-masters", G_CALLBACK (action_activate_cb), update_master_gslist_callback);
	add_win_action (win, "add-master",         G_CALLBACK (action_activate_cb), add_master_callback);
	add_win_action (win, "rename-master",      G_CALLBACK (action_activate_cb), edit_master_callback);
	add_win_action (win, "delete-master",      G_CALLBACK (action_activate_cb), del_master_callback);
	add_win_action (win, "clear-servers",      G_CALLBACK (action_activate_cb), clear_master_servers_callback);
	add_win_action (win, "find-player",        G_CALLBACK (action_activate_cb), find_player_callback);
	add_win_action (win, "find-again",         G_CALLBACK (action_activate_cb), find_player_callback);
	add_win_action (win, "properties",         G_CALLBACK (action_activate_cb), properties_callback);
	add_win_action (win, "preferences",        G_CALLBACK (action_activate_cb), start_preferences_dialog);
	add_win_action (win, "refresh",            G_CALLBACK (action_activate_cb), refresh_callback);
	add_win_action (win, "refresh-selected",   G_CALLBACK (action_activate_cb), refresh_selected_callback);
	add_win_action (win, "update-from-master", G_CALLBACK (action_activate_cb), update_source_callback);
	add_win_action (win, "dns-lookup",         G_CALLBACK (action_activate_cb), resolve_callback);
	add_win_action (win, "rcon",               G_CALLBACK (action_activate_cb), rcon_callback);
	add_win_action (win, "about",              G_CALLBACK (action_activate_cb), about_dialog);
	add_win_action (win, "configure-filters",  G_CALLBACK (action_activate_cb), start_filters_cfg_dialog);
	add_win_action (win, "connect",            G_CALLBACK (action_activate_cb), launch_normal_callback);
	add_win_action (win, "observe",            G_CALLBACK (action_activate_cb), launch_spectate_callback);
	add_win_action (win, "record-demo",        G_CALLBACK (action_activate_cb), launch_record_callback);
	add_win_action (win, "copy-server-info",   G_CALLBACK (action_activate_cb), copy_server_info_callback);
	add_win_action (win, "player-filter-red",  G_CALLBACK (action_activate_cb), add_to_player_filter_red_callback);
	add_win_action (win, "player-filter-green",G_CALLBACK (action_activate_cb), add_to_player_filter_green_callback);
	add_win_action (win, "player-filter-blue", G_CALLBACK (action_activate_cb), add_to_player_filter_blue_callback);

	/* Stateful toggle actions */
	add_win_toggle (win, "show-hostnames",    G_CALLBACK (show_hostnames_callback),    NULL);
	add_win_toggle (win, "show-default-port", G_CALLBACK (show_default_port_callback), NULL);

	gtk_widget_insert_action_group (GTK_WIDGET (win), "win", G_ACTION_GROUP (_win_ag));
	g_object_unref (_win_ag);
}

static gboolean create_main_window (void) {
	GError *error = NULL;

	builder = gtk_builder_new_from_file (g_build_filename (xqf_PACKAGE_DATA_DIR, "ui", "xqf.ui", NULL));

	if (G_UNLIKELY (error != NULL)) {
		fprintf (stderr, "Could not load UI: %s\n", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	main_window = GTK_WIDGET (gtk_builder_get_object (builder, "main-window"));
	register_window_actions (GTK_WINDOW (main_window));
	g_signal_connect (main_window, "close-request", G_CALLBACK (main_window_close_cb), NULL);
	g_signal_connect (main_window, "destroy", G_CALLBACK (ui_done), NULL);
	gtk_window_set_title (GTK_WINDOW (main_window), "XQF");

	register_window (main_window);

	gtk_widget_realize (main_window);
	return TRUE;
}

void populate_main_window (void) {
	GtkWidget *hbox;
	GtkWidget *entry;
	GtkWidget *button;
	GtkWidget *image;
	int i;

	// Initialize action states for toggle menu items
	{
		GSimpleAction *a;
		a = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (_win_ag), "show-hostnames"));
		if (a) g_simple_action_set_state (a, g_variant_new_boolean (show_hostnames));
		a = G_SIMPLE_ACTION (g_action_map_lookup_action (G_ACTION_MAP (_win_ag), "show-default-port"));
		if (a) g_simple_action_set_state (a, g_variant_new_boolean (show_default_port));
	}

	server_filter_menu_items = g_array_new (FALSE, FALSE, sizeof (GtkWidget*));

	populate_main_toolbar ();

	pane1_widget = GTK_WIDGET (gtk_builder_get_object (builder, "hpaned"));

	// Sources TreeView

	source_treeview = create_source_treeview (GTK_WIDGET (gtk_builder_get_object (builder, "scrollwin-sources")));

	gtk_widget_show (source_treeview);

	/* Restore per-group expand/collapse state from config; fill_source_treeview()
	 * couldn't do this because source_treeview was NULL when it ran. */
	source_treeview_restore_expand_state ();

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (source_treeview)),
	                  "changed", G_CALLBACK (source_selection_changed_callback), NULL);

	gtk_widget_show (GTK_WIDGET (gtk_builder_get_object (builder, "scrollwin-sources")));

	pane2_widget = GTK_WIDGET (gtk_builder_get_object (builder, "vpaned"));

	// Server CList

	hbox = GTK_WIDGET (gtk_builder_get_object (builder, "hbox"));

	button = GTK_WIDGET (gtk_builder_get_object (builder, "button"));
	image = gtk_image_new_from_pixbuf (delete_pix.pixbuf);
	gtk_button_set_child (GTK_BUTTON (button), image);
	gtk_widget_show_all (button);

	entry = GTK_WIDGET (gtk_builder_get_object (builder, "entry"));
	g_signal_connect (entry, "changed", G_CALLBACK (quick_filter_entry_changed), NULL);

	g_signal_connect (button, "clicked", G_CALLBACK (quickfilter_delete_button_clicked), entry);

	server_view = create_server_column_view (
		GTK_WIDGET (gtk_builder_get_object (builder, "scrollwin-server")));
	gtk_widget_show (server_view);

	{
		GtkGestureClick *dbl = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
		gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (dbl), 1);
		g_signal_connect (dbl, "pressed", G_CALLBACK (server_view_double_click_cb), NULL);
		gtk_widget_add_controller (server_view, GTK_EVENT_CONTROLLER (dbl));

		GtkGestureClick *rclick = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
		gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (rclick), 3);
		g_signal_connect (rclick, "pressed", G_CALLBACK (server_view_right_click_cb), NULL);
		gtk_widget_add_controller (server_view, GTK_EVENT_CONTROLLER (rclick));

		GtkEventControllerKey *key = GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
		g_signal_connect (key, "key-pressed", G_CALLBACK (server_view_key_cb), NULL);
		gtk_widget_add_controller (server_view, GTK_EVENT_CONTROLLER (key));
	}

	pane3_widget = GTK_WIDGET (gtk_builder_get_object (builder, "paned1"));

	// Player ColumnView

	player_view = create_player_column_view (
		GTK_WIDGET (gtk_builder_get_object (builder, "scrollwin-player")));
	gtk_widget_show (player_view);

	{
		GtkGestureClick *rclick = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
		gtk_gesture_single_set_button (GTK_GESTURE_SINGLE (rclick), 3);
		g_signal_connect (rclick, "pressed", G_CALLBACK (player_view_right_click_cb), NULL);
		gtk_widget_add_controller (player_view, GTK_EVENT_CONTROLLER (rclick));
	}

	// Server Info TreeView

	srvinf_treeview = srvinf_treeview_new (
		GTK_WIDGET (gtk_builder_get_object (builder, "scrollwin-server-info")));
	gtk_widget_show (srvinf_treeview);

	(void) calculate_row_height (GTK_WIDGET (server_view), games[Q1_SERVER].pix);

	// Status Bar & Progress Bar

	main_filter_status_bar = GTK_WIDGET (gtk_builder_get_object (builder, "main-filter-status-bar"));

	if (main_filter_status_bar)
		gtk_widget_set_size_request (main_filter_status_bar, 100, -1);

	main_progress_bar = GTK_WIDGET (gtk_builder_get_object (builder, "main-progress-bar"));
	if (!main_progress_bar) {
		main_progress_bar = create_progress_bar ();
		gtk_widget_show (main_progress_bar);
	}
	gtk_widget_set_size_request (main_progress_bar, 200, -1);

	// Make sure the current filter is displayed and applied if needed
	set_server_filter_menu_list_text ();

	restore_main_window_geometry ();

	gtk_window_set_default_icon_name ("xqf");

	gtk_widget_grab_focus (entry);
}

void play_sound (const char *sound, gboolean override) {
	play_sound_with (sound_player, sound, override);
}

void play_sound_with (const char* player, const char *sound, gboolean override) {
	int pid;

	if (!sound || !*sound) {
		return;
	}

	if (!sound_enable && !override) {
		debug (2, "sound disabled - not playing");
		return;
	}

	if (!player || !*player) {
		xqf_warning (_("no sound player configured"));
		return;
	}

	pid = fork ();
	if (pid == 0) {
		char *argv[3];

		argv[0] = g_strdup (player);

		if (sound[0] != '/') {
			// Does not start with a / so prepend user_rcdir
			debug (1, "Prepending user_rcdir to sound file");
			argv[1] = file_in_dir (user_rcdir, sound);
		}
		else {
			argv[1] = g_strdup (sound);
		}

		argv[2] = NULL;

		debug (1, "sound player (program): %s", argv[0]);
		debug (1, "sound to play: %s", argv[1]);
		execvp (argv[0], argv);

		g_free (argv[1]);

		_exit (1);
	}
}

void cmdlinehelp () {
	puts ("XQF Version " PACKAGE_VERSION);
	puts (_(
				"Usage:\n"
				"\txqf [OPTIONS]\n"
				"\n"
				"OPTIONS:\n"
				"\t--launch \"[SERVERTYPE] IP\"\tlaunch game on specified server\n"
				"\t--add    \"[SERVERTYPE] IP\"\tadd specified server to favorites\n"
				"\t--debug <level>\t\t\tset debug level\n"
				"\t--version\t\t\tprint version and exit\n"));
	exit (0);
}

char* cmdline_add_server = NULL;
gboolean cmdline_launch = FALSE;
gboolean cmdline_newversion = FALSE;

// must always return FALSE to stop g_timeout
gboolean check_cmdline_launch (gpointer nothing) {
	char* token[2] = {0};
	enum server_type type = UNKNOWN_SERVER;
	unsigned n = 0;
	char* addrstring = NULL; // must point to a copy

	if (!cmdline_add_server) {
		return FALSE;
	}

	n = tokenize_bychar (cmdline_add_server, token, 2, ' ');

	if (n == 2) // type and address given
	{
		type = id2type (token[0]);

		if (type == UNKNOWN_SERVER) {
			addrstring = add_server_dialog (&type, token[1]);
		}
		else {
			addrstring = g_strdup (token[1]);
		}
	}
	else if (n == 1) // only address
	{
		char *addr;
		unsigned short port;
		unsigned matches = 0;

		if (!parse_address (token[0], &addr, &port)) {
			dialog_ok (NULL, _("\"%s\" is not valid host[:port] combination."), token[0]);
			g_free (cmdline_add_server);
			return FALSE;
		}

		if (port) // guess the type from the port
		{
			unsigned i = 0;
			for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {
				if (games[i].default_port == port) {
					++matches;
					if (type == UNKNOWN_SERVER) type = i;
				}
			}
		}

		if (!port || type == UNKNOWN_SERVER || matches > 1) {
			addrstring = add_server_dialog (&type, token[0]);
		}
		else {
			addrstring = g_strdup (cmdline_add_server);
		}
	}

	prepare_new_server_to_favorites (type, addrstring, cmdline_launch);

	g_free (cmdline_add_server);
	return FALSE;
}

struct option long_options[] =
{
	{"launch", 1, 0, 'l'},
	{"add", 1, 0, 'a'},
	{"debug", 1, 0, 'd'},
	{"dontlaunch", 0, 0, 128},
	{"version", 0, 0, 'v'},
	{"help", 0, 0, 'h'},
	{"newversion", 0, 0, 129},
	{"nomapscan", 0, 0, 130},
	{0, 0, 0, 0}
};

void parse_commandline (int argc, char* argv[]) {
	while (1) {
		int c;
		int option_index = 0;

		c = getopt_long (argc, argv, "d:l:h", long_options, &option_index);
		if (c == -1) {
			break;
		}

		switch (c) {
			case 'd':
				set_debug_level (atoi (optarg));
				break;
			case 'l':
				g_free (cmdline_add_server);
				cmdline_add_server = g_strdup (optarg);
				cmdline_launch = TRUE;
				break;
			case 'a':
				g_free (cmdline_add_server);
				cmdline_add_server = g_strdup (optarg);
				break;
			case 'h':
				cmdlinehelp ();
				break;
			case 'v':
				puts ("XQF Version " PACKAGE_VERSION);
				exit (0);
				break;
			case 128:
				dontlaunch = TRUE;
				break;
			case 129:
				cmdline_newversion = TRUE;
				break;
			case 130:
				skip_startup_mapscan = TRUE;
				break;
			case '?':
			case ':':
				exit (1);
				break;
			default:
				xqf_warning ("getopt error, starting anyway ...");
				return;
		}
	}
}

void add_pixmap_path_for_theme (const char* theme) {
	char dir[PATH_MAX];
	snprintf (dir, sizeof (dir), "%s/%s", xqf_PACKAGE_DATA_DIR, theme);
	add_pixmap_directory (dir);
	snprintf (dir, sizeof (dir), "%s/.local/share/xqf/%s", g_get_home_dir (), theme);
	add_pixmap_directory (dir);
}

void init_config_path () {
	char dir[PATH_MAX];
	config_add_dir (xqf_PACKAGE_DATA_DIR);
	snprintf (dir, sizeof (dir), "%s/.local/share/xqf", g_get_home_dir ());
	config_add_dir (dir);
	config_add_dir (user_rcdir);
}

void init_scripts_path () {
	char dir[PATH_MAX];
	snprintf (dir, sizeof (dir), "%s/scripts", xqf_PACKAGE_DATA_DIR);
	scripts_add_dir (dir);
	snprintf (dir, sizeof (dir), "%s/.local/share/xqf/scripts", g_get_home_dir ());
	scripts_add_dir (dir);
	snprintf (dir, sizeof (dir), "%s/scripts", user_rcdir);
	scripts_add_dir (dir);
}

int main (int argc, char *argv[]) {
	char* var = NULL;
	int newversion = FALSE;

	xqf_start_time = time (NULL);

	redialserver = 0;

#if defined(USE_RELATIVE_PREFIX)
	setDefaultDirs();
#endif

	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, xqf_LOCALEDIR);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	set_debug_level (DEFAULT_DEBUG_LEVEL);
	debug (5, "main() -- Debug Level Default Set at %d", DEFAULT_DEBUG_LEVEL);

	// migrate config directory to follow XDG  Base Directory Specification
	// http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
	// https://developer.gnome.org/basedir-spec/
	// https://developer.gnome.org/glib/2.37/glib-Miscellaneous-Utility-Functions.html#g-get-user-config-dir

	if (!rc_migrate_dir ()) {
		return 1;
	}

	if (!init_user_info ()) {
		return 1;
	}

	gtk_init (&argc, &argv);

	parse_commandline (argc, argv);

	if (dns_spawn_helper () < 0) {
		xqf_error ("Unable to start DNS helper");
		return 1;
	}

	add_pixmap_path_for_theme ("default");
	add_pixmap_directory (xqf_PACKAGE_DATA_DIR);

	qstat_configfile = g_build_filename (xqf_PACKAGE_DATA_DIR, "qstat.cfg", NULL);

	dns_gtk_init ();

	rc_check_dir ();

	init_games ();

	init_config_path ();

	newversion = prefs_load () | cmdline_newversion;

	if (default_icontheme) {
		add_pixmap_path_for_theme (default_icontheme);
	}

	init_scripts_path ();
	scripts_load ();

#ifdef USE_GEOIP
	geoip_init ();
#endif

	props_load ();
	filters_init ();

	host_cache_load ();
	init_masters (newversion);

	client_init ();
	ignore_sigpipe ();

	on_sig (SIGUSR1, sighandler_debug);
	on_sig (SIGUSR2, sighandler_debug);

	add_server_init ();
	add_master_init ();
	psearch_init ();
	rcon_init ();

	if (check_qstat_version () == FALSE) {
		dialog_ok (NULL, _("You need at least qstat version %s for xqf to function properly"), required_qstat_version);
	}

	if (!create_main_window ())
		return 1;

	init_pixmaps (main_window);

	play_sound (sound_xqf_start, 0);

	script_action_start ();

	populate_main_window ();

	gtk_widget_show (main_window);

	source_treeview_select_source (favorites);
	filter_menu_activate_current ();

	print_status (GTK_WIDGET (gtk_builder_get_object (builder, "main-status-bar")), NULL);

	if (default_auto_favorites && !cmdline_add_server) {
		refresh_callback (NULL, NULL);
	}

	g_timeout_add (0, check_cmdline_launch, NULL);

	debug (1, "startup time %ds", time (NULL) - xqf_start_time);

	_xqf_app_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (_xqf_app_loop);
	g_main_loop_unref (_xqf_app_loop);
	_xqf_app_loop = NULL;

	play_sound (sound_xqf_quit, 0);
	script_action_quit ();

	unregister_window (main_window);
	main_window = NULL;

	if (stat_process) {
		stop_callback (NULL, NULL);
	}

	debug (1, "total servers: %d", servers_total ());
	debug (1, "total uservers: %d", uservers_total ());
	debug (1, "total hosts: %d", hosts_total ());


	if (player_skin_popup) {
		gtk_widget_destroy (player_skin_popup);
	}

	if (server_mapshot_popup) {
		gtk_widget_destroy (server_mapshot_popup);
	}

	pixmap_cache_clear (&qw_colors_pixmap_cache, 0);
	pixmap_cache_clear (&server_pixmap_cache, 0);
	free_pixmaps ();

	filters_done ();
	props_free_all ();

	debug (6, "EXIT: Free Server Lists");
	g_slist_free (cur_source);
	server_list_free (cur_server_list);
	userver_list_free (cur_userver_list);

	if (cur_server) {
		server_unref (cur_server);
		cur_server = NULL;
	}

	host_cache_save ();
	host_cache_clear ();

	debug (6, "EXIT: Free Master Lists");
	free_masters ();

	debug (6, "EXIT: Call rcon_done.");
	rcon_done ();
	psearch_done ();
	add_server_done ();
	add_master_done ();
	client_detach_all ();
	free_user_info ();

	dns_helper_shutdown ();

	config_sync ();
	config_drop_all ();

#ifdef USE_GEOIP
	geoip_done ();
#endif

	games_done ();
	prefs_done ();

	debug (6, "EXIT: Done.");

	return 0;
}
