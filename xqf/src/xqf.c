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
#include <stdio.h>
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */
#include <time.h>	/* time */
#include <string.h>	/* strlen */

#include <gtk/gtk.h>

#include "xqf.h"
#include "xqf-ui.h"
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
#include "xutils.h"
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
#include "menus.h"
#include "config.h"


time_t xqf_start_time;

GtkWidget *main_window = NULL;
GtkWidget *source_ctree = NULL;
GtkCList  *server_clist = NULL;
GtkCList  *player_clist = NULL;
GtkCTree  *srvinf_ctree = NULL;

GtkEditable *selection_manager = NULL;

GSList *cur_source = NULL;		/*  GSList <struct master *>  */
GSList *cur_server_list = NULL;		/*  GSList <struct server *>  */
GSList *cur_userver_list = NULL;	/*  GSList <struct userver *>  */
struct server *cur_server = NULL;

struct stat_job *stat_process = NULL;

static GtkWidget *main_toolbar = NULL;
static GtkWidget *main_status_bar = NULL;
static GtkWidget *main_progress_bar = NULL;

static char *progress_bar_str = NULL;

static GtkWidget *server_menu = NULL;
static GtkWidget *player_menu = NULL;

static GtkWidget *connect_menu_item = NULL;
static GtkWidget *observe_menu_item = NULL;
static GtkWidget *record_menu_item = NULL;
static GtkWidget *favadd_menu_item = NULL;
static GtkWidget *add_menu_item = NULL;
static GtkWidget *delete_menu_item = NULL;
static GtkWidget *refresh_menu_item = NULL;
static GtkWidget *refrsel_menu_item = NULL;
static GtkWidget *resolve_menu_item = NULL;
static GtkWidget *properties_menu_item = NULL;
static GtkWidget *rcon_menu_item = NULL;

static GtkWidget *file_quit_menu_item = NULL;
static GtkWidget *file_statistics_menu_item = NULL;

static GtkWidget *edit_add_menu_item = NULL;
static GtkWidget *edit_delete_menu_item = NULL;
static GtkWidget *edit_add_master_menu_item = NULL;
static GtkWidget *edit_delete_master_menu_item = NULL;
static GtkWidget *edit_find_player_menu_item = NULL;
static GtkWidget *edit_find_again_menu_item = NULL;

static GtkWidget *view_refresh_menu_item = NULL;
static GtkWidget *view_refrsel_menu_item = NULL;
static GtkWidget *view_update_menu_item = NULL;

GtkWidget *view_hostnames_menu_item = NULL;
GtkWidget *view_defport_menu_item = NULL;

static GtkWidget *server_connect_menu_item = NULL;
static GtkWidget *server_observe_menu_item = NULL;
static GtkWidget *server_record_menu_item = NULL;
static GtkWidget *server_favadd_menu_item = NULL;
static GtkWidget *server_resolve_menu_item = NULL;
static GtkWidget *server_properties_menu_item = NULL;
static GtkWidget *server_rcon_menu_item = NULL;

static GtkWidget *player_filter_menu_item = NULL;

static GtkWidget *update_button = NULL;
static GtkWidget *refresh_button = NULL;
static GtkWidget *refrsel_button = NULL;
static GtkWidget *stop_button = NULL;

static GtkWidget *connect_button = NULL;
static GtkWidget *observe_button = NULL;
static GtkWidget *record_button = NULL;

static GtkWidget *filter_buttons[FILTERS_TOTAL];

static GtkWidget *player_skin_popup = NULL;
static GtkWidget *player_skin_popup_preview = NULL;


void set_widgets_sensitivity (void) {
  GList *selected = server_clist->selection;
  int sens;
  int i;
  int source_is_favorites;
  int masters_to_update;
  int masters_to_delete;

  /*
   *  Every button with callback that can modify its sensitivity
   *  should be explicitly put to GTK_STATE_NORMAL state before 
   *  changing its insensitivity
   */

  source_is_favorites = (cur_source != NULL && 
			 cur_source->next == NULL &&
			 (struct master *) cur_source->data == favorites);

  if (source_is_favorites) {
    masters_to_update = masters_to_delete = FALSE;
  }
  else {
    masters_to_update = source_has_masters_to_update (cur_source);
    masters_to_delete = source_has_masters_to_delete (cur_source);
  }

  sens = (!stat_process && cur_server);

  gtk_widget_set_sensitive (properties_menu_item, sens);
  gtk_widget_set_sensitive (server_properties_menu_item, sens);

  sens = (!stat_process && cur_server && 
                         (games[cur_server->type].flags & GAME_CONNECT) != 0);

  gtk_widget_set_sensitive (connect_menu_item, sens);
  gtk_widget_set_sensitive (server_connect_menu_item, sens);

  gtk_widget_set_state (connect_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (connect_button, sens);

  sens = (!stat_process && cur_server &&
                       (games[cur_server->type].flags & GAME_SPECTATE) != 0 &&
                                  (cur_server->flags & SERVER_SPECTATE) != 0);

  gtk_widget_set_sensitive (observe_menu_item, sens);
  gtk_widget_set_sensitive (server_observe_menu_item, sens);

  gtk_widget_set_state (observe_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (observe_button, sens);

  sens = (!stat_process && cur_server && 
                          (games[cur_server->type].flags & GAME_RECORD) != 0);

  gtk_widget_set_sensitive (record_menu_item, sens);
  gtk_widget_set_sensitive (server_record_menu_item, sens);

  gtk_widget_set_state (record_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (record_button, sens);

  sens = (!stat_process && cur_server && 
                            (games[cur_server->type].flags & GAME_RCON) != 0);

  gtk_widget_set_sensitive (rcon_menu_item, sens);
  gtk_widget_set_sensitive (server_rcon_menu_item, sens);

  sens = (!stat_process && selected);

  gtk_widget_set_sensitive (refrsel_menu_item, sens);
  gtk_widget_set_sensitive (view_refrsel_menu_item, sens);

  gtk_widget_set_state (refrsel_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (refrsel_button, sens);

  gtk_widget_set_sensitive (resolve_menu_item, sens);
  gtk_widget_set_sensitive (server_resolve_menu_item, sens);

  sens = (!stat_process);

  gtk_widget_set_sensitive (file_statistics_menu_item, sens);
  gtk_widget_set_sensitive (add_menu_item, sens);
  gtk_widget_set_sensitive (edit_add_menu_item, sens);
  gtk_widget_set_sensitive (edit_add_master_menu_item, sens);
  gtk_widget_set_sensitive (edit_find_player_menu_item, sens);
  gtk_widget_set_sensitive (edit_find_again_menu_item, sens);
  gtk_widget_set_sensitive (player_filter_menu_item, sens);
  gtk_widget_set_sensitive (source_ctree, sens);

  sens = (!stat_process && selected && !source_is_favorites);

  gtk_widget_set_sensitive (favadd_menu_item, sens);
  gtk_widget_set_sensitive (server_favadd_menu_item, sens);

  sens = (!stat_process && selected && source_is_favorites);

  gtk_widget_set_sensitive (delete_menu_item, sens);
  gtk_widget_set_sensitive (edit_delete_menu_item, sens);

  sens = (!stat_process && masters_to_delete);

  gtk_widget_set_sensitive (edit_delete_master_menu_item, sens);

  sens = (!stat_process && (server_clist->rows > 0));

  gtk_widget_set_sensitive (refresh_menu_item, sens);
  gtk_widget_set_sensitive (view_refresh_menu_item, sens);

  gtk_widget_set_state (refresh_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (refresh_button, sens);

  gtk_widget_set_sensitive (view_hostnames_menu_item, sens);
  gtk_widget_set_sensitive (view_defport_menu_item, sens);

  sens = (!stat_process && masters_to_update);

  gtk_widget_set_sensitive (view_update_menu_item, sens);

  gtk_widget_set_state (update_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (update_button, sens);

  sens = (stat_process != NULL);

  gtk_widget_set_state (stop_button, GTK_STATE_NORMAL);
  gtk_widget_set_sensitive (stop_button, sens);

  for (i = 0; i < FILTERS_TOTAL; i++)
    gtk_widget_set_sensitive (filter_buttons[i], (stat_process == NULL));
}


static int forced_filters_flag = FALSE;


static void set_filters (unsigned char mask) {
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


static void filter_toggle_callback (GtkWidget *widget, unsigned char mask) {
  if (!forced_filters_flag) {
    cur_filter ^= mask;
    server_clist_build_filtered (cur_server_list, FALSE);
  }
}


static void start_preferences_dialog (GtkWidget *widget, int page_num) {
  preferences_dialog (page_num);
  set_toolbar_appearance (GTK_TOOLBAR (main_toolbar), 
                                 default_toolbar_style, default_toolbar_tips);
}


static void start_filters_cfg_dialog (GtkWidget *widget, int page_num) {
  if (filters_cfg_dialog (page_num)) {
    config_sync ();
    rc_save ();
    server_clist_build_filtered (cur_server_list, TRUE);
    player_clist_redraw ();
  }
}


static void update_server_lists_from_selected_source (void) {
  GSList *cur_masters = NULL;

  if (cur_server_list) {
    server_list_free (cur_server_list);
    cur_server_list = NULL;
  }

  if (cur_userver_list) {
    userver_list_free (cur_userver_list);
    cur_userver_list = NULL;
  }

  master_selection_to_lists (cur_source, &cur_masters, &cur_server_list, 
                                                           &cur_userver_list);
  collate_server_lists (cur_masters, &cur_server_list, &cur_userver_list);
}


static int stat_lists_refresh (struct stat_job *job) {
  int items;
  int freeze;

  items = g_slist_length (job->delayed.queued_servers) + 
                                   g_slist_length (job->delayed.queued_hosts);
  if (items) {
    freeze = (items > 1) || default_refresh_sorts;

    if (freeze)
      gtk_clist_freeze (server_clist);

    g_slist_foreach (job->delayed.queued_servers, 
                                   (GFunc) server_clist_refresh_server, NULL);
    server_list_free (job->delayed.queued_servers);
    job->delayed.queued_servers = NULL;

    g_slist_foreach (job->delayed.queued_hosts, 
                                    (GFunc) server_clist_show_hostname, NULL);
    host_list_free (job->delayed.queued_hosts);
    job->delayed.queued_hosts = NULL;

    if (default_refresh_sorts)
      gtk_clist_sort (server_clist);

    if (freeze)
      gtk_clist_thaw (server_clist);
  }

  print_status (main_status_bar, (progress_bar_str)? progress_bar_str : "", 
                                     job->progress.done, job->progress.tasks);
  if (job->progress.tasks > 0) {
    progress_bar_set_percentage (main_progress_bar, 
                           ((float)job->progress.done) / job->progress.tasks);
  }

  return TRUE;
}


static void stat_lists_state_handler (struct stat_job *job, 
                                                      enum stat_state state) {
  if (!main_window)
    return;

  switch (state) {

  case STAT_UPDATE_SOURCE:
    progress_bar_str = "Updating lists...";
    break;

  case STAT_RESOLVE_NAMES:
    progress_bar_str = "Resolving host names: %d/%d";
    break;

  case STAT_REFRESH_SERVERS:
    progress_bar_str = "Refreshing: %d/%d";
    break;

  case STAT_RESOLVE_HOSTS:
    progress_bar_str = "Resolving host addresses: %d/%d";
    break;

  default:
    progress_bar_str = NULL;
    break;
  }

  progress_bar_start (main_progress_bar, 
                     state == STAT_UPDATE_SOURCE || job->progress.tasks <= 1);

  stat_lists_refresh (job);	/* flush */
}


static void stat_lists_close_handler (struct stat_job *job, int killed) {

  if (!main_window)
    return;

  if (job->need_redraw) {
    update_server_lists_from_selected_source ();
    server_clist_build_filtered (cur_server_list, TRUE);
  }

  print_status (main_status_bar, "Done.");

  progress_bar_reset (main_progress_bar);

  stat_process = NULL;
  set_widgets_sensitivity ();
}


static void stat_lists_server_handler (struct stat_job *job, 
                                                           struct server *s) {
  if (s == cur_server) {
    if (job->delayed.refresh_handler)
      (*job->delayed.refresh_handler) (job);
    player_clist_set_server (s);
    srvinf_ctree_set_server (s);
  }
}


static void stat_lists_master_handler (struct stat_job *job, 
                                                           struct master *m) {
  source_ctree_show_node_status (source_ctree, m);
}


static void stat_lists (GSList *masters, GSList *names, GSList *servers, 
                                                              GSList *hosts) {
  if (stat_process || (!masters && !names && !servers && !hosts))
    return;

  stat_process = stat_job_create (masters, names, servers, hosts);

  stat_process->delayed.refresh_handler = (GtkFunction) stat_lists_refresh;

  stat_process->state_handlers = g_slist_prepend (
                      stat_process->state_handlers, stat_lists_state_handler);
  stat_process->close_handlers = g_slist_prepend (
                      stat_process->close_handlers, stat_lists_close_handler);

  stat_process->server_handlers = g_slist_append (
                    stat_process->server_handlers, stat_lists_server_handler);
  stat_process->master_handlers = g_slist_append (
                    stat_process->master_handlers, stat_lists_master_handler);

  stat_start (stat_process);
  set_widgets_sensitivity ();
}


static void stat_one_server (struct server *s) {
  GSList *list;

  if (!stat_process && s) {
    list = server_list_prepend (NULL, s);
    stat_lists (NULL, NULL, list, NULL);
  }
}


static void launch_close_handler (struct stat_job *job, int killed) {
  struct server_props *props;
  int launch = FALSE;
  int save = 0;
  struct server *s;
  struct condef *con;

  con = (struct condef *) job->data;
  job->data = NULL;

  if (!con)
    return;

  if (killed) {
    condef_free (con);
    return;
  }

  s = con->s;
  props = properties (s);

  if (s->ping >= MAX_PING) {
    launch = dialog_yesno (NULL, 1, "Launch", "Cancel",
                     "Server %s:%d is %s.\n\nLaunch client anyway?",
                     (s->host->name)? s->host->name : inet_ntoa (s->host->ip),
                     s->port,
                     (s->ping == MAX_PING)? "unreachable" : "down");
    if (!launch) {
      condef_free (con);
      return;
    }
  }

  if (!launch && s->curplayers >= s->maxplayers && !con->spectate) {
    launch = dialog_yesno (NULL, 1, "Launch", "Cancel",
		     "Server %s:%d is full.\n\nLaunch client anyway?",
		     (s->host->name)? s->host->name : inet_ntoa (s->host->ip),
		     s->port);
    if (!launch) {
      condef_free (con);
      return;
    }
  }

  if (con->spectate) {
    if ((s->flags & SERVER_SP_PASSWORD) == 0) {
      con->spectator_password = g_strdup ("1");
    }
    else {
      if (props && props->spectator_password) {
	con->spectator_password = g_strdup (props->spectator_password);
      }
      else {
	con->spectator_password = enter_string_with_option_dialog (FALSE,
                               "Save Password", &save, "Spectator Password:");
	if (!con->spectator_password) {
	  condef_free (con);
	  return;
	}
	if (save) {
	  if (!props)
	    props = properties_new (s->host, s->port);
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
                                  "Save Password", &save, "Server Password:");
      if (!con->password) {
	condef_free (con);
	return;
      }
      if (save) {
	if (!props)
	  props = properties_new (s->host, s->port);
	props->server_password = g_strdup (con->password);
	props_save ();
      }
    }
  }

  con->server = g_strdup_printf ("%s:%5d", inet_ntoa (s->host->ip), s->port);
  con->gamedir = g_strdup (s->game);

  if (props && props->rcon_password)
    con->rcon_password = g_strdup (props->rcon_password);

  if (props && props->custom_cfg)
    con->custom_cfg = g_strdup (props->custom_cfg);

  launch = client_launch (con, TRUE);
  condef_free (con);

  if (!launch)
    return;

  if (main_window && default_iconify && !default_terminate)
    iconify_window (main_window->window);

  if (main_window && default_terminate)
    gtk_widget_destroy (main_window);
}


static void launch_server_handler (struct stat_job *job, struct server *s) {

  server_clist_refresh_server (s);

  if (s == cur_server) {
    player_clist_set_server (s);
    srvinf_ctree_set_server (s);
  }

  /* Don't spend time on host name lookups */

  if (job->hosts) {
    host_list_free (job->hosts);
    job->hosts = NULL;
  }
}


static void launch_callback (GtkWidget *widget, enum launch_mode mode) {
  int spectate = FALSE;
  char *demo = NULL;
  struct condef *con = NULL;

  if (stat_process || !cur_server || 
                        (games[cur_server->type].flags & GAME_CONNECT) == 0) {
    return;
  }

  if (games[cur_server->type].config_is_valid && 
                   !(*games[cur_server->type].config_is_valid) (cur_server)) {
    start_preferences_dialog (NULL, PREF_PAGE_GAMES + cur_server->type * 256);
    return;
  }

  switch (mode) {

  case LAUNCH_NORMAL:
    break;

  case LAUNCH_SPECTATE:
    if ((cur_server->flags & SERVER_SPECTATE) == 0)
      return;

    spectate = TRUE;
    break;

  case LAUNCH_RECORD:
    if ((games[cur_server->type].flags & GAME_RECORD) == 0)
      return;

    if ((games[cur_server->type].flags & GAME_SPECTATE) != 0) {
      demo = enter_string_with_option_dialog (TRUE, "Spectator", &spectate, 
                                                                "Demo name:");
    }
    else {
      demo = enter_string_dialog (TRUE, "Demo name:");
    }

    if (!demo)
      return;

    break;

  }

  con = condef_new (cur_server);
  con->demo = demo;
  con->spectate = spectate;

  stat_process = stat_job_create (NULL, NULL, 
                                server_list_prepend (NULL, cur_server), NULL);
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
  set_widgets_sensitivity ();
}


static int server_clist_compare_func (GtkCList *clist, 
                                     gconstpointer ptr1, gconstpointer ptr2) {
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;
  struct server *s1 = (struct server *) row1->data;
  struct server *s2 = (struct server *) row2->data;

  return compare_servers (s1, s2, clist->sort_column);
}


static int player_clist_compare_func (GtkCList *clist, 
                                     gconstpointer ptr1, gconstpointer ptr2) {
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;
  struct player *p1 = (struct player *) row1->data;
  struct player *p2 = (struct player *) row2->data;

  return compare_players (p1, p2, clist->sort_column);
}


static int srvinf_clist_compare_func (GtkCList *clist, 
                                     gconstpointer ptr1, gconstpointer ptr2) {
  GtkCListRow *row1 = (GtkCListRow *) ptr1;
  GtkCListRow *row2 = (GtkCListRow *) ptr2;
  const char **i1 = (const char **) row1->data;
  const char **i2 = (const char **) row2->data;

  return compare_srvinfo (i1, i2, clist->sort_column);
}


static void update_source_callback (GtkWidget *widget, gpointer data) {
  GSList *masters = NULL;
  GSList *servers = NULL;
  GSList *uservers = NULL;

  if (stat_process || !cur_source)
    return;

  master_selection_to_lists (cur_source, &masters, &servers, &uservers);
  if (masters || servers || uservers) {
    stat_lists (masters, uservers, servers, NULL);
  }
}


static void refresh_selected_callback (GtkWidget *widget, gpointer data) {
  GSList *list;

  if (stat_process)
    return;

  list = server_clist_selected_servers ();

  if (list)
    stat_lists (NULL, NULL, list, NULL);
}


static void refresh_callback (GtkWidget *widget, gpointer data) {
  GSList *servers;
  GSList *uservers;

  if (stat_process)
    return;

  servers = server_clist_all_servers ();
  uservers = userver_list_copy (cur_userver_list);

  if (servers || uservers) {
    stat_lists (NULL, uservers, servers, NULL);
  }
}


static void stop_callback (GtkWidget *widget, gpointer data) {
  if (stat_process) {
    stat_stop (stat_process);
    stat_process = NULL;
  }
}


static void add_to_favorites_callback (GtkWidget *widget, gpointer data) {
  GList *selected = server_clist->selection;
  GSList *list;
  GSList *tmp;

  if (stat_process || !selected)
    return;

  list = server_clist_selected_servers ();
  if (list) {
    for (tmp = list; tmp; tmp = tmp->next) {
      favorites->servers = 
	server_list_append (favorites->servers, (struct server *) tmp->data);
    }
    save_favorites ();
    server_list_free (list);
  }
}


static void add_server_real (struct stat_job *job, struct server *s) {
  int row;

  favorites->servers = server_list_append (favorites->servers, s);
  save_favorites ();

  source_ctree_select_source (favorites);

  if (cur_filter != 0) {
    set_filters (0);	/* turn off filters */
    if (job)
      job->need_redraw = FALSE;
  }

  row = gtk_clist_find_row_from_data (server_clist, s);
  if (row >= 0) {
    server_clist_select_one (row);
    server_clist_selection_visible ();
  }
}


static void add_server_name_handler (struct stat_job *job, struct userver *us,
                                                     enum dns_status status) {
  if (us->s) {
    add_server_real (job, us->s);
  }
  else {
    progress_bar_reset (main_progress_bar);
    dialog_ok (NULL, "Host %s not found", us->hostname);
  }
}


static void add_server_callback (GtkWidget *widget, gpointer data) {
  char *str;
  char *addr;
  unsigned short port;
  struct host *h;
  struct server *s = NULL;
  struct userver *us = NULL;
  enum server_type type;

  if (stat_process)
    return;

  str = add_server_dialog (&type);
  if (!str || !*str)
    return;

  if (!parse_address (str, &addr, &port)) {
    dialog_ok (NULL, "\"%s\" is not valid host[:port] combination.", str);
    g_free (str);
    return;
  }

  g_free (str);

  h = host_add (addr);
  if (h) {						/* IP address */
    host_ref (h);
    s = server_add (h, port, type);
    if (s)
      add_server_real (NULL, s);
    host_unref (h);
  }
  else {						/* hostname */
    g_strdown (addr);
    us = userver_add (addr, port, type);
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

    stat_process->server_handlers = g_slist_append (
                    stat_process->server_handlers, stat_lists_server_handler);

    stat_process->name_handlers = g_slist_prepend (
                        stat_process->name_handlers, add_server_name_handler);

    stat_start (stat_process);
    set_widgets_sensitivity ();
  }
}


static void del_server_callback (GtkWidget *widget, gpointer data) {
  GSList *list;
  GSList *tmp;

  if (stat_process || !cur_source || 
                            (struct master *) cur_source->data != favorites) {
    return;
  }

  list = server_clist_selected_servers ();
  if (list) {
    for (tmp = list; tmp; tmp = tmp->next) {
      favorites->servers = 
	server_list_remove (favorites->servers, (struct server *) tmp->data);
    }
    save_favorites ();
    server_list_free (list);

    update_server_lists_from_selected_source ();
    server_clist_build_filtered (cur_server_list, FALSE);
  }
}


static void copy_server_callback (GtkWidget *widget, gpointer data) {
  GList *selection = server_clist->selection;
  struct server *s;
  char buf[256];
  int pos = 0;

  gtk_editable_delete_text (selection_manager, 0, -1);

  switch (g_list_length (selection)) {

  case 0:
    gtk_editable_select_region (selection_manager, 0, 0);
    return;

  case 1:
    s = (struct server *) gtk_clist_get_row_data (
                                         server_clist, (int) selection->data);
    g_snprintf (buf, 256, "%s:%d", inet_ntoa (s->host->ip), s->port);
    gtk_editable_insert_text (selection_manager, buf, strlen (buf), &pos);
    gtk_editable_select_region (selection_manager, 0, -1);
    return;

  default:
    for (; selection; selection = selection->next) {
      s = (struct server *) gtk_clist_get_row_data (
                                         server_clist, (int) selection->data);
      g_snprintf (buf, 256, "%s:%d\n", inet_ntoa (s->host->ip), s->port);
      gtk_editable_insert_text (selection_manager, buf, strlen (buf), &pos);
    }
    gtk_editable_select_region (selection_manager, 0, -1);
    break;

  }
}


static void add_master_callback (GtkWidget *widget, gpointer data) {
  char *str;
  char *desc;
  enum server_type type;
  struct master *m;

  if (stat_process)
    return;

  str = add_master_dialog (&type, &desc);
  if (!str || !*str)
    return;

  m = add_master (str, desc, type, TRUE, FALSE);

  if (m) {
    source_ctree_add_master (source_ctree, m);
    source_ctree_select_source (m);
  }
  else {
    dialog_ok (NULL, "Master address \"%s\" is not valid.", str);
  }

  g_free (str);
  g_free (desc);
}


static void del_master_callback (GtkWidget *widget, gpointer data) {
  struct master *m;
  GSList *masters = NULL;
  char *master_names = NULL;
  GSList *list;
  int delete;
  char *s1;
  char *s2;

  if (stat_process)
    return;

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

  if (!masters)
    return;

  delete = dialog_yesno (NULL, 1, "Delete", "Cancel", 
        "Master%s to delete:\n\n%s", (g_slist_length (masters) > 1)? "s" : "",
        master_names);

  if (delete) {
    for (list = masters; list; list = list->next) {
      m = (struct master *) list->data;
      source_ctree_delete_master (source_ctree, m);
      free_master (m);
    }
  }

  g_free (master_names);
  g_slist_free (masters);
}


static void find_player_callback (GtkWidget *widget, int find_next) {
  char *pattern;

  if (find_next || find_player_dialog ()) {
    if (!psearch_data_is_empty ()) {
      pattern = psearch_lookup_pattern ();
      print_status (main_status_bar, "Find Player: %s", pattern);
      g_free (pattern);

      find_player (find_next);
    }
  }
}


static void show_hostnames_callback (GtkWidget *widget, gpointer data) {

  if (stat_process || !server_clist || server_clist->rows == 0)
    return;

  if (GTK_CHECK_MENU_ITEM (view_hostnames_menu_item)->active != 
                                                             show_hostnames) {
    show_hostnames = GTK_CHECK_MENU_ITEM (view_hostnames_menu_item)->active;
    server_clist_redraw ();
    config_set_bool ("/" CONFIG_FILE "/Appearance/show hostnames", 
		                                              show_hostnames);
  }
}


static void show_default_port_callback (GtkWidget *widget, gpointer data) {

  if (stat_process || !server_clist || server_clist->rows == 0)
    return;

  if (GTK_CHECK_MENU_ITEM (view_defport_menu_item)->active != 
                                                          show_default_port) {
    show_default_port = GTK_CHECK_MENU_ITEM (view_defport_menu_item)->active;
    server_clist_redraw ();
    config_set_bool ("/" CONFIG_FILE "/Appearance/show default port", 
                                                           show_default_port);
  }
}


static void resolve_callback (GtkWidget *widget, gpointer data) {
  GSList *selected;
  GSList *hosts;
  struct server *s;

  if (stat_process)
    return;

  if (!show_hostnames) {
    gtk_check_menu_item_set_active (
                        GTK_CHECK_MENU_ITEM (view_hostnames_menu_item), TRUE);
  }

  selected = server_clist_selected_servers ();
  if (!selected)
    return;

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


static void properties_callback (GtkWidget *widget, gpointer data) {

  if (stat_process)
    return;

  if (cur_server) {
    properties_dialog (cur_server);
    set_widgets_sensitivity ();
  }
}


static void rcon_callback (GtkWidget *widget, gpointer data) {
  struct server_props *sp;
  char *passwd = NULL;
  int save = 0;

  if (stat_process || !cur_server || 
                           (games[cur_server->type].flags & GAME_RCON) == 0) {
    return;
  }

  sp = properties (cur_server);

  if (!sp || !sp->rcon_password) {
    passwd = enter_string_with_option_dialog (FALSE,
	                          "Save Password", &save, "Server Password:");

    if (!passwd)	/* canceled */
      return;

    if (save) {
      if (!sp)
	sp = properties_new (cur_server->host, cur_server->port);
      sp->rcon_password = passwd;
      props_save ();
      passwd = NULL;
    }
  }

  rcon_dialog (cur_server, (passwd)? passwd : sp->rcon_password);

  if (passwd)
    g_free (passwd);
}


static void server_clist_select_callback (GtkWidget *widget, int row,
                            int column, GdkEvent *event, GtkWidget *button) {
  GdkEventButton *bevent = (GdkEventButton *) event;

  server_clist_sync_selection ();

  if (bevent && bevent->type == GDK_2BUTTON_PRESS && bevent->button == 1)
    launch_callback (NULL, LAUNCH_NORMAL);
}


static void server_clist_unselect_callback (GtkWidget *widget, int row,
                            int column, GdkEvent *event, GtkWidget *button) {
  server_clist_sync_selection ();
}


static int server_clist_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent = (GdkEventButton *) event;
  GList *selection;
  int row;

  if (event->type == GDK_BUTTON_PRESS &&
                   bevent->window == server_clist->clist_window) {

    switch (bevent->button) {

    case 2:
      if (gtk_clist_get_selection_info (server_clist, 
                                          bevent->x, bevent->y, &row, NULL)) {
	server_clist_select_one (row);
	stat_one_server (cur_server);
      }
      return TRUE;

    case 3:
      if (gtk_clist_get_selection_info (server_clist, 
                                          bevent->x, bevent->y, &row, NULL)) {
	selection = server_clist->selection;
	if (!g_list_find (selection, (gpointer) row) && 
                 (bevent->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == 0) {
	  server_clist_select_one (row);
	}
      }
      gtk_menu_popup (GTK_MENU (server_menu), NULL, NULL, NULL, NULL,
	                                        bevent->button, bevent->time);
      return TRUE;

    default:
      return FALSE;
    }

  }
  return FALSE;
}


static void source_selection_changed (void) {
  GList *selection = GTK_CLIST (source_ctree)->selection;
  GtkCTreeNode *node;
  struct master *m;

  if (cur_source) {
    g_slist_free (cur_source);
    cur_source = NULL;
  }

  while (selection) {
    node = (GtkCTreeNode *) selection->data;
    m = (struct master *) gtk_ctree_node_get_row_data (
                                              GTK_CTREE (source_ctree), node);
    cur_source = g_slist_append (cur_source, m);
    selection = selection->next;
  }

  update_server_lists_from_selected_source ();
  server_clist_set_list (cur_server_list);

  print_status (main_status_bar, (server_clist->rows == 1) ?
                              "%d server" : "%d servers", server_clist->rows);
}


static void source_ctree_selection_changed_callback (GtkWidget *widget, 
                    int row, int column, GdkEvent *event, GtkWidget *button) {
  source_selection_changed ();
}


static void add_to_player_filter_callback (GtkWidget *widget, unsigned mask) {
  GList *selection = player_clist->selection;
  struct player *p;
  int row;

  if (!selection || !cur_server || stat_process)
    return;

  row = (int) selection->data;
  p = (struct player *) gtk_clist_get_row_data (player_clist, row);

  if (player_filter_add_player (p->name, mask)) {
    server_clist_build_filtered (cur_server_list, TRUE);
    player_clist_redraw ();
  }
}


static void player_skin_preview_popup_show (guchar *skin, 
                                          int top, int bottom, int x, int y) {
  GtkWidget *frame;
  int win_x, win_y, scr_w, scr_h;

  if (!player_skin_popup) {
    player_skin_popup = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_set_policy (GTK_WINDOW (player_skin_popup), FALSE, FALSE, TRUE);
  
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
    gtk_container_add (GTK_CONTAINER (player_skin_popup), frame);
    gtk_widget_show (frame);

    player_skin_popup_preview = gtk_preview_new (GTK_PREVIEW_COLOR);
    gtk_container_add (GTK_CONTAINER (frame), player_skin_popup_preview);
    gtk_preview_size (GTK_PREVIEW (player_skin_popup_preview), 320, 200);
    gtk_widget_show (player_skin_popup_preview);
  }
  else {
    gtk_widget_hide (player_skin_popup);
  }

  gdk_window_get_origin (player_clist->clist_window, &win_x, &win_y);
  x += win_x;
  y += win_y;
  scr_w = gdk_screen_width ();
  scr_h = gdk_screen_height ();
  x = (x + 320 > scr_w)? scr_w - 320 : x;
  y = (y + 200 > scr_h)? scr_h - 200 : y;

  gtk_widget_popup (player_skin_popup, x, y);

  draw_qw_skin (player_skin_popup_preview, skin, top, bottom);
}


static int player_clist_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent = (GdkEventButton *) event;
  guchar *skindata;
  int row, column;
  struct player *p;

  switch (event->type) {

  case GDK_BUTTON_PRESS:
    if (bevent->window == player_clist->clist_window) {
      if (gtk_clist_get_selection_info (player_clist, 
                                       bevent->x, bevent->y, &row, &column)) {
	if (row >= 0 && row < g_slist_length (cur_server->players)) {

	  if ((column == 2 || column == 3) && cur_server &&
                    (games[cur_server->type].flags & GAME_QUAKE1_SKIN) != 0) {

	    p = (struct player *) gtk_clist_get_row_data (player_clist, row);

	    skindata = get_qw_skin (p->skin, games[QW_SERVER].real_dir);

	    gdk_pointer_grab (player_clist->clist_window, FALSE,
			      GDK_POINTER_MOTION_HINT_MASK |
			      GDK_BUTTON1_MOTION_MASK |
			      GDK_BUTTON_RELEASE_MASK,
			      NULL, NULL, bevent->time);

	    player_skin_preview_popup_show (skindata, p->shirt, p->pants,
                                                        bevent->x, bevent->y);
	    if (skindata)
	      g_free (skindata);

	    return TRUE;

	  }
	  else {
	    if (bevent->button == 3) {
	      gtk_clist_select_row (player_clist, row, column);
	      gtk_menu_popup (GTK_MENU (player_menu), NULL, NULL, NULL, NULL,
	                                        bevent->button, bevent->time);
	      return TRUE;
	    }
	  }

	}
      }
    }
    break;

  case GDK_2BUTTON_PRESS:
  case GDK_3BUTTON_PRESS:
    return TRUE;

  case GDK_BUTTON_RELEASE:
    if (player_skin_popup) {
      gdk_pointer_ungrab (bevent->time);
      gtk_widget_hide (player_skin_popup);
    }
    break;

  default:
    break;
  }

  return FALSE;
}


static void statistics_callback (GtkWidget *widget, gpointer data) {

  if (stat_process)
    return;

  statistics_dialog ();
}


static void about_dialog (GtkWidget *widget, gpointer data) {
  dialog_ok ("About XQF", 
	     "X11 Quake/QuakeWorld/Quake2/Quake3 Front-End\n"
	     "Version " XQF_VERSION "\n\n"
	     "Copyright (C) 1998-2000 Roman Pozlevich <roma@botik.ru>\n\n"
	     "http://www.linuxgames.com/xqf/");
}


struct __menuitem {
  char *label;
  char accel_key;
  int accel_mods;
  GtkWidget **widget;
  GtkSignalFunc	callback;
  gpointer user_data;
};

static const struct menuitem srvopt_menu_items[] = {
  { 
    MENU_ITEM,		"Connect",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL,
    &connect_menu_item
  },
  { 
    MENU_ITEM,		"Observe",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE,
    &observe_menu_item
  },
  { 
    MENU_ITEM,		"Record Demo",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD,
    &record_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"Add to Favorites",	0,	0,
    GTK_SIGNAL_FUNC (add_to_favorites_callback), NULL,
    &favadd_menu_item
  },
  { 
    MENU_ITEM,		"Add...",		0,   	0,
    GTK_SIGNAL_FUNC (add_server_callback), NULL,
    &add_menu_item
  },
  { 
    MENU_ITEM,		"Delete",		0,   	0,
    GTK_SIGNAL_FUNC (del_server_callback), NULL,
    &delete_menu_item
  },
  { 
    MENU_ITEM,		"Copy",			0,   	0,
    GTK_SIGNAL_FUNC (copy_server_callback), NULL,
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"Refresh",		0,	0,
    GTK_SIGNAL_FUNC (refresh_callback), NULL,
    &refresh_menu_item
  },
  { 
    MENU_ITEM,		"Refresh Selected",	0,	0,
    GTK_SIGNAL_FUNC (refresh_selected_callback), NULL,
    &refrsel_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"DNS Lookup",		0,	0,
    GTK_SIGNAL_FUNC (resolve_callback), NULL,
    &resolve_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"RCon...",   		0,	0,
    GTK_SIGNAL_FUNC (rcon_callback), NULL,
    &rcon_menu_item
  },
  { 
    MENU_ITEM,		"Properties...",   	0,	0,
    GTK_SIGNAL_FUNC (properties_callback), NULL,
    &properties_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem file_menu_items[] = {
  { 
    MENU_ITEM,		"_Statistics...",	0,	0,
    GTK_SIGNAL_FUNC (statistics_callback), NULL,
    &file_statistics_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"_Exit",		'Q',	GDK_CONTROL_MASK,
    NULL,		NULL,
    &file_quit_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem edit_menu_items[] = {
  { 
    MENU_ITEM,		"_Add Server...",	'N',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (add_server_callback), NULL,
    &edit_add_menu_item
  },
  { 
    MENU_ITEM,		"_Delete",		'D',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (del_server_callback), NULL,
    &edit_delete_menu_item
  },
  { 
    MENU_ITEM,		"_Copy",		'C',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (copy_server_callback), NULL,
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  {
    MENU_ITEM,          "Add _Master...",        'M',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (add_master_callback), NULL,
    &edit_add_master_menu_item
  },
  {
    MENU_ITEM,          "D_elete Master",        0,	0,
    GTK_SIGNAL_FUNC (del_master_callback), NULL,
    &edit_delete_master_menu_item
  },

  { MENU_SEPARATOR,    NULL,                   0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"_Find Player...",	'F',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (find_player_callback), (gpointer) FALSE,
    &edit_find_player_menu_item
  },
  { 
    MENU_ITEM,		"Find A_gain",		'G',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (find_player_callback), (gpointer) TRUE,
    &edit_find_again_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem view_menu_items[] = {
  { 
    MENU_ITEM,		"_Refresh",		'R',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (refresh_callback), NULL,
    &view_refresh_menu_item
  },
  { 
    MENU_ITEM,		"Refresh _Selected",	'S',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (refresh_selected_callback), NULL,
    &view_refrsel_menu_item
  },
  { 
    MENU_ITEM,		"_Update From Master",	'U',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (update_source_callback), NULL,
    &view_update_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_CHECK_ITEM,	"Show _Host Names",	'H',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (show_hostnames_callback), NULL,
    &view_hostnames_menu_item
  },
  { 
    MENU_CHECK_ITEM,	"Show Default _Port",	0,	0,
    GTK_SIGNAL_FUNC (show_default_port_callback), NULL,
    &view_defport_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem server_menu_items[] = {
  { 
    MENU_ITEM,		"_Connect",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL,
    &server_connect_menu_item
  },
  { 
    MENU_ITEM,		"_Observe",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE,
    &server_observe_menu_item
  },
  { 
    MENU_ITEM,		"Record _Demo",		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD,
    &server_record_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"Add to _Favorites",	0,	0,
    GTK_SIGNAL_FUNC (add_to_favorites_callback), NULL,
    &server_favadd_menu_item
  },
  { 
    MENU_ITEM,		"DNS _Lookup",		'L',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (resolve_callback), NULL,
    &server_resolve_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"_RCon...",   		0,	0,
    GTK_SIGNAL_FUNC (rcon_callback), NULL,
    &server_rcon_menu_item
  },
  { 
    MENU_ITEM,		"_Properties...",  	0,	0,
    GTK_SIGNAL_FUNC (properties_callback), NULL,
    &server_properties_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem preferences_menu_items[] = {
  { 
    MENU_ITEM,		"_Player Profile...",	0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_PLAYER + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		"_Games...",		0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog), 
    (gpointer) (PREF_PAGE_GAMES + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		"_Appearance...",	0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_APPEARANCE + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		"_QStat Options...",	0,	0, 
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_QSTAT + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		"_QW/Q2 Options...",		0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_QWQ2 + UNKNOWN_SERVER * 256),
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		"_Server Filter...",	0,	0,
    GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) FILTER_SERVER,
    NULL
  },
  { 
    MENU_ITEM,		"Player _Filter...",	0,	0,
    GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) FILTER_PLAYER,
    NULL
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem help_menu_items[] = {
  { 
    MENU_ITEM,		"_About...",		0,	0,
    GTK_SIGNAL_FUNC (about_dialog), NULL,
    NULL
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};


static const struct menuitem menubar_menu_items[] = {
  {
    MENU_BRANCH,		"_File",	0,	0,
    NULL, &file_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		"_Edit",	0,	0,
    NULL, &edit_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		"_View",	0,	0,
    NULL, &view_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		"_Server",	0,	0,
    NULL, &server_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		"_Preferences",	0,	0,
    NULL, &preferences_menu_items,
    NULL
  },
  {
    MENU_LAST_BRANCH,		"_Help",	0,	0,
    NULL, &help_menu_items,
    NULL
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};


static GtkWidget *create_player_menu_item (char *str, int i) {
  GtkWidget *menu_item;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *pixmap;

  menu_item = gtk_menu_item_new ();

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (menu_item), hbox);

  label = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  pixmap = gtk_pixmap_new (group_pix[i].pix, group_pix[i].mask);
  gtk_box_pack_end (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
  gtk_widget_show (pixmap);

  gtk_widget_show (hbox);

  return menu_item;
}


static GtkWidget *create_player_menu (GtkAccelGroup *accel_group) {
  GtkWidget *menu;
  GtkWidget *marker_menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new ();

  marker_menu = gtk_menu_new ();

  menu_item = create_player_menu_item ("Mark as Red", 0);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
		      (gpointer) PLAYER_GROUP_RED);
  gtk_widget_show (menu_item);

  menu_item = create_player_menu_item ("Mark as Green", 1);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
		      (gpointer) PLAYER_GROUP_GREEN);
  gtk_widget_show (menu_item);

  menu_item = create_player_menu_item ("Mark as Blue", 2);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
                      (gpointer) PLAYER_GROUP_BLUE);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label ("Add to Player Filter");
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), marker_menu);
  gtk_menu_append (GTK_MENU (menu), menu_item);
  gtk_widget_show (menu_item);

  player_filter_menu_item = menu_item;

  return menu;
}


static void populate_main_toolbar (void) {
  GtkWidget *pixmap;
  char buf[128];
  unsigned mask;
  int i;

  pixmap = gtk_pixmap_new (update_pix.pix, update_pix.mask);
  gtk_widget_show (pixmap);

  update_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   "Update", "Update from master", NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (update_source_callback), NULL);

  pixmap = gtk_pixmap_new (refresh_pix.pix, refresh_pix.mask);
  gtk_widget_show (pixmap);

  refresh_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   "Refresh", "Refresh current list", NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (refresh_callback), NULL);

  pixmap = gtk_pixmap_new (refrsel_pix.pix, refrsel_pix.mask);
  gtk_widget_show (pixmap);

  refrsel_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   "Ref.Sel.", "Refresh selected servers", NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (refresh_selected_callback), NULL);

  pixmap = gtk_pixmap_new (stop_pix.pix, stop_pix.mask);
  gtk_widget_show (pixmap);

  stop_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   "Stop", "Stop", NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (stop_callback), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  pixmap = gtk_pixmap_new (connect_pix.pix, connect_pix.mask);
  gtk_widget_show (pixmap);

  connect_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   "Connect", "Connect", NULL,
                   pixmap,
                 GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL);

  pixmap = gtk_pixmap_new (observe_pix.pix, observe_pix.mask);
  gtk_widget_show (pixmap);

  observe_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                  "Observe", "Observe", NULL,
                  pixmap,
               GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE);

  pixmap = gtk_pixmap_new (record_pix.pix, record_pix.mask);
  gtk_widget_show (pixmap);

  record_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                  "Record", "Record Demo", NULL,
                  pixmap,
                 GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD);

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  /*
   *  Filter buttons
   */

  for (i = 0, mask = 1; i < FILTERS_TOTAL; i++, mask <<= 1) {
    g_snprintf (buf, 128, "%s Filter", filters[i].name);

    pixmap = gtk_pixmap_new (filter_pix[i].pix, filter_pix[i].mask);
    gtk_widget_show (pixmap);

    filter_buttons[i] = gtk_toolbar_append_element (
		   GTK_TOOLBAR (main_toolbar),
		   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
                   filters[i].short_name, buf, NULL,
                   pixmap,
                   GTK_SIGNAL_FUNC (filter_toggle_callback), (gpointer) mask);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_buttons[i]), 
                                    ((cur_filter & mask) != 0)? TRUE : FALSE);
  }

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  for (i = 0, mask = 1; i < FILTERS_TOTAL; i++, mask <<= 1) {
    g_snprintf (buf, 128, "%s Filter Configuration", filters[i].name);

    pixmap = gtk_pixmap_new (filter_cfg_pix[i].pix, filter_cfg_pix[i].mask);
    gtk_widget_show (pixmap);

    gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   filters[i].short_cfg_name, buf, NULL,
                   pixmap,
                   GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) i);
  }

  set_toolbar_appearance (GTK_TOOLBAR (main_toolbar), 
                                 default_toolbar_style, default_toolbar_tips);
}


void create_main_window (void) {
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hpaned;
  GtkWidget *hpaned2;
  GtkWidget *vpaned;
  GtkWidget *menu_bar;
  GtkWidget *handlebox;
  GtkWidget *scrollwin;
  GtkAccelGroup *accel_group;
  int i;

  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect (GTK_OBJECT (main_window), "delete_event",
		      GTK_SIGNAL_FUNC (window_delete_event_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (ui_done), NULL);
  gtk_signal_connect (GTK_OBJECT (main_window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_window_set_title (GTK_WINDOW (main_window), "XQF");

  register_window (main_window);

  gtk_widget_realize (main_window);
  init_pixmaps (main_window);

  accel_group = gtk_accel_group_new ();

  server_menu = create_menu (srvopt_menu_items, accel_group);

  player_menu = create_player_menu (accel_group);

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), main_vbox);

  /*  Lazy way to get `copy to clipboard' feature working.
   *  Don't gtk_widget_show() selection_manager widget!
   */

  selection_manager = GTK_EDITABLE (gtk_text_new (NULL, NULL));
  gtk_box_pack_start (GTK_BOX (main_vbox), GTK_WIDGET (selection_manager), 
		                                             FALSE, FALSE, 0);
  gtk_widget_realize (GTK_WIDGET (selection_manager));

  handlebox = gtk_handle_box_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), handlebox, FALSE, FALSE, 0);

  menu_bar = create_menubar (menubar_menu_items, accel_group);

  gtk_signal_connect_object (GTK_OBJECT (file_quit_menu_item), "activate",
	      GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (main_window));

  gtk_check_menu_item_set_active (
              GTK_CHECK_MENU_ITEM (view_hostnames_menu_item), show_hostnames);

  gtk_check_menu_item_set_active (
             GTK_CHECK_MENU_ITEM (view_defport_menu_item), show_default_port);

  gtk_container_add (GTK_CONTAINER (handlebox), menu_bar);
  gtk_widget_show (menu_bar);

  gtk_widget_show (handlebox);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

  main_toolbar = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, 
                                                            GTK_TOOLBAR_BOTH);
  gtk_box_pack_start (GTK_BOX (vbox), main_toolbar, FALSE, FALSE, 0);
  populate_main_toolbar ();
  gtk_widget_show (main_toolbar);

  pane1_widget = hpaned = gtk_hpaned_new ();
  gtk_box_pack_start (GTK_BOX (vbox), hpaned, TRUE, TRUE, 0);

  /*
   *  Sources CTree
   */

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add1 (GTK_PANED (hpaned), scrollwin);

  source_ctree = create_source_ctree (scrollwin);

  gtk_widget_show (source_ctree);

  gtk_signal_connect (GTK_OBJECT (source_ctree), "tree_select_row",
             GTK_SIGNAL_FUNC (source_ctree_selection_changed_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (source_ctree), "tree_unselect_row",
             GTK_SIGNAL_FUNC (source_ctree_selection_changed_callback), NULL);

  gtk_widget_show (scrollwin);

  pane2_widget = vpaned = gtk_vpaned_new ();
  gtk_paned_add2 (GTK_PANED (hpaned), vpaned);

  /*
   *  Server CList
   */

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add1 (GTK_PANED (vpaned), scrollwin);

  server_clist = GTK_CLIST (create_cwidget (scrollwin, &server_clist_def));

  gtk_signal_connect (GTK_OBJECT (server_clist), "click_column",
                  GTK_SIGNAL_FUNC (clist_set_sort_column), &server_clist_def);
  gtk_signal_connect (GTK_OBJECT (server_clist), "event",
                         GTK_SIGNAL_FUNC (server_clist_event_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (server_clist), "select_row",
                        GTK_SIGNAL_FUNC (server_clist_select_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (server_clist), "unselect_row",
                      GTK_SIGNAL_FUNC (server_clist_unselect_callback), NULL);

  gtk_clist_set_compare_func (server_clist,
			     (GtkCListCompareFunc) server_clist_compare_func);

  gtk_widget_show (GTK_WIDGET (server_clist));
  gtk_widget_show (scrollwin);

  pane3_widget = hpaned2 = gtk_hpaned_new ();
  gtk_paned_add2 (GTK_PANED (vpaned), hpaned2);

  /*
   *  Player CList
   */

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add1 (GTK_PANED (hpaned2), scrollwin);

  player_clist = GTK_CLIST (create_cwidget (scrollwin, &player_clist_def));

  gtk_signal_connect (GTK_OBJECT (player_clist), "click_column",
                  GTK_SIGNAL_FUNC (clist_set_sort_column), &player_clist_def);
  gtk_signal_connect (GTK_OBJECT (player_clist), "event",
                         GTK_SIGNAL_FUNC (player_clist_event_callback), NULL);

  gtk_clist_set_compare_func (player_clist,
			     (GtkCListCompareFunc) player_clist_compare_func);

  gtk_widget_show (GTK_WIDGET (player_clist));
  gtk_widget_show (scrollwin);

  /*
   *  Server Info CList
   */

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_paned_add2 (GTK_PANED (hpaned2), scrollwin);

  srvinf_ctree = GTK_CTREE (create_cwidget (scrollwin, &srvinf_clist_def));

  gtk_signal_connect (GTK_OBJECT (srvinf_ctree), "click_column",
                  GTK_SIGNAL_FUNC (clist_set_sort_column), &srvinf_clist_def);

  gtk_clist_set_compare_func (GTK_CLIST (srvinf_ctree),
			     (GtkCListCompareFunc) srvinf_clist_compare_func);

  gtk_widget_show (GTK_WIDGET (srvinf_ctree));
  gtk_widget_show (scrollwin);

  gtk_widget_show (hpaned2);
  gtk_widget_show (vpaned);
  gtk_widget_show (hpaned);

  gtk_widget_ensure_style (GTK_WIDGET (server_clist));
  i = calculate_clist_row_height (GTK_WIDGET (server_clist), q_pix.pix);
  gtk_clist_set_row_height (server_clist, i);
  gtk_clist_set_row_height (player_clist, i);
  gtk_clist_set_row_height (GTK_CLIST (srvinf_ctree), i);
  gtk_clist_set_row_height (GTK_CLIST (source_ctree), i);

  /*
   *  Status Bar & Progress Bar
   */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  main_status_bar = gtk_statusbar_new ();
  gtk_box_pack_start (GTK_BOX (hbox), main_status_bar, TRUE, TRUE, 0);
  gtk_widget_show (main_status_bar);

  main_progress_bar = create_progress_bar ();
  gtk_widget_set_usize (main_progress_bar, 200, -1);
  gtk_box_pack_end (GTK_BOX (hbox), main_progress_bar, FALSE, FALSE, 0);
  gtk_widget_show (main_progress_bar);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  gtk_widget_show (main_vbox);

  restore_main_window_geometry ();

  gtk_widget_show (main_window);

  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
  gtk_accel_group_unref (accel_group);
}


int main (int argc, char *argv[]) {
  char *gtk_config;
  int newversion = FALSE;

  if (!init_user_info ()) {
    return 1;
  }

  if (dns_spawn_helper () < 0) {
    fprintf (stderr, "Unable to start DNS helper\n");
    return 1;
  }

  xqf_start_time = time (NULL);

  gtk_config = file_in_dir (user_rcdir, "gtkrc");
  gtk_rc_add_default_file (gtk_config);
  g_free (gtk_config);

  gtk_init (&argc, &argv);

  dns_gtk_init ();

  rc_check_dir ();

  config_set_base_dir (user_rcdir);
  newversion = prefs_load ();

  gtk_preview_set_gamma (1.5);

  props_load ();
  filters_init ();

  host_cache_load ();
  init_masters (newversion);

  client_init ();
  ignore_sigpipe ();

  add_server_init ();
  add_master_init ();
  psearch_init ();
  rcon_init ();

  create_main_window ();

  source_ctree_select_source (favorites);

  print_status (main_status_bar, NULL);

  if (default_auto_favorites)
    refresh_callback (NULL, NULL);

  gtk_main ();

  unregister_window (main_window);
  main_window = NULL;

  if (stat_process)
    stop_callback (NULL, NULL);

  if (server_menu) {
    gtk_widget_destroy (server_menu);
    server_menu = NULL;
  }

  if (player_menu) {
    gtk_widget_destroy (player_menu);
    player_menu = NULL;
  }

  if (player_skin_popup) {
    gtk_widget_destroy (player_skin_popup);
    player_skin_popup = NULL;
  }

  pixmap_cache_clear (&qw_colors_pixmap_cache, 0);
  pixmap_cache_clear (&server_pixmap_cache, 0);
  free_pixmaps ();

  filters_done ();
  props_free_all ();

  g_slist_free (cur_source);
  server_list_free (cur_server_list);
  userver_list_free (cur_userver_list);

  if (cur_server) {
    server_unref (cur_server);
    cur_server = NULL;
  }

  host_cache_save ();
  host_cache_clear ();

  free_masters ();
  rcon_done ();
  psearch_done ();
  add_server_done ();
  add_master_done ();
  client_detach_all ();
  free_user_info ();

#ifdef DEBUG
  fprintf (stderr, "total servers: %d\n", servers_total ());
  fprintf (stderr, "total uservers: %d\n", uservers_total ());
  fprintf (stderr, "total hosts: %d\n", hosts_total ());
  if (servers_total () > 0) {
    GSList *list = all_servers ();
    
    server_list_fprintf (stderr, list);
    server_list_free (list);
  }
#endif

  dns_helper_shutdown ();

  config_sync ();
  config_drop_all ();

  return 0;
}

