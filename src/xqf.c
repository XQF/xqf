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
#include <stdio.h>
#include <stdlib.h>	/* atoi */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */
#include <time.h>	/* time */
#include <string.h>	/* strlen */
#include <ctype.h>	/* isspace */

// select, fork, pipe ...
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>	/* kill, ... */

#ifdef ENABLE_NLS
#  include <locale.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "i18n.h"
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
#include "debug.h"
#include "redial.h"

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
static GtkWidget *main_filter_status_bar = NULL;
static GtkWidget *main_progress_bar = NULL;

static char *progress_bar_str = NULL;

//static GtkWidget *server_filter_menu = NULL; /* baa */

static GtkWidget *server_menu = NULL;
static GtkWidget *source_menu = NULL;
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
//static GtkWidget *cancel_redial_menu_item = NULL;

static GtkWidget *file_quit_menu_item = NULL;
static GtkWidget *file_statistics_menu_item = NULL;

static GtkWidget *edit_add_menu_item = NULL;
static GtkWidget *edit_favadd_menu_item = NULL;
static GtkWidget *edit_delete_menu_item = NULL;
static GtkWidget *edit_update_master_builtin_menu_item = NULL;
static GtkWidget *edit_add_master_menu_item = NULL;
static GtkWidget *edit_edit_master_menu_item = NULL;
static GtkWidget *edit_delete_master_menu_item = NULL;
static GtkWidget *edit_find_player_menu_item = NULL;
static GtkWidget *edit_find_again_menu_item = NULL;

// rmb popup
static GtkWidget *source_add_master_menu_item = NULL;
static GtkWidget *source_edit_master_menu_item = NULL;
static GtkWidget *source_delete_master_menu_item = NULL;

static GtkWidget *view_refresh_menu_item = NULL;
static GtkWidget *view_refrsel_menu_item = NULL;
static GtkWidget *view_update_menu_item = NULL;

GtkWidget *view_hostnames_menu_item = NULL;
GtkWidget *view_defport_menu_item = NULL;

static GtkWidget *server_connect_menu_item = NULL;
static GtkWidget *server_observe_menu_item = NULL;
static GtkWidget *server_record_menu_item = NULL;
static GtkWidget *server_favadd_menu_item = NULL;
static GtkWidget *server_delete_menu_item = NULL;
static GtkWidget *server_resolve_menu_item = NULL;
static GtkWidget *server_properties_menu_item = NULL;
static GtkWidget *server_rcon_menu_item = NULL;
//static GtkWidget *server_cancel_redial_menu_item = NULL;

static GtkWidget *server_serverfilter_menu_item = NULL;
static GArray* server_filter_menu_items;

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
/*
static GtkWidget *server_filter_1_widget = NULL;
static GtkWidget *server_filter_2_widget = NULL;
static GtkWidget *server_filter_3_widget = NULL;
*/

//XXX GtkWidget *server_filter_widget[MAX_SERVER_FILTERS + 3];

static gboolean launch_redial(struct condef *con);
static void refresh_selected_callback (GtkWidget *widget, gpointer data);
static void launch_close_handler_part2(struct condef *con);

/** build server filter menu for menubar
 */
static GtkWidget* create_filter_menu();
//static GtkWidget* filter_menu = NULL; // need to store that for toggling the checkboxes
static GSList* filter_menu_radio_buttons = NULL; // for finding the widgets to activate

void sighandler_debug(int signum)
{
    if( signum == SIGUSR1)
	set_debug_level(get_debug_level()+1);
    else if( signum == SIGUSR2)
	set_debug_level(get_debug_level()-1);

    debug(0,"debug level now at %d", get_debug_level());
}
  
// returns 0 if equal, -1 if too old, 1 if have > expected
int compare_qstat_version ( const char* have, const char* expected )
{
  int have_major, expected_major;
  int have_minor, expected_minor;
  char have_pl=0, expected_pl=0;
  const char* have_pos1=NULL, *expected_pos1=NULL;
  const char* have_pos2=NULL, *expected_pos2=NULL;
  char* buf;

  debug(3,"compare_qstat_version(%s,%s)",have,expected);

  if(!strcmp(have,expected)) return 0;

  have_pos1 = strchr(have,'.');
  expected_pos1 = strchr(expected,'.');

  if(!have_pos1 || !expected_pos1)
    return -1;
  
  buf = g_strndup(have,have_pos1-have);
  have_major = atoi(buf);
  g_free(buf);
  buf = g_strndup(expected,expected_pos1-expected);
  expected_major = atoi(buf);
  g_free(buf);

  debug(3,"compare_qstat_version -- compare major %d %d",have_major,expected_major);
  if(have_major < expected_major) return -1;
  if(have_major > expected_major) return 1;

  have_pos1++;
  expected_pos1++;

  for(have_pos2=have_pos1;
      have_pos2 && *have_pos2 && isdigit(*have_pos2);
      have_pos2++);
  for(expected_pos2=expected_pos1;
      expected_pos2 && *expected_pos2 && isdigit(*expected_pos2);
      expected_pos2++);

  buf = g_strndup(have_pos1,have_pos2-have_pos1);
  have_minor = atoi(buf);
  g_free(buf);
  buf = g_strndup(expected_pos1,expected_pos2-expected_pos1);
  expected_minor = atoi(buf);
  g_free(buf);
  
  debug(3,"compare_qstat_version -- compare minor %d %d",have_minor,expected_minor);
  if(have_minor < expected_minor) return -1;
  if(have_minor > expected_minor) return 1;

  if(have_pos2 && *have_pos2) have_pl=*have_pos2;
  if(expected_pos2 && *expected_pos2) expected_pl=*expected_pos2;

  debug(3,"compare_qstat_version -- compare pl %c %c",have_pl,expected_pl);
  if(!have_pl && expected_pl) return -1;
  if(have_pl && expected_pl && have_pl < expected_pl) return -1;

  return 1;
}

int start_prog_and_return_fd(char *const argv[], pid_t *pid)
{
  int pipefds[2];

  *pid = -1;

  if (pipe (pipefds) < 0)
  {
    debug(0,"start_prog_and_return_fd -- error creating pipe: %s",strerror(errno));
    return -1;
  }
  *pid = fork();
  if (*pid < (pid_t) 0)
  {
    debug(0,"start_prog_and_return_fd -- fork failed: %s",strerror(errno));
    return -1;
  }
  
  if (*pid == 0)	// child
  {
    close(1);
//    close(2);
    close (pipefds[0]);
    dup2 (pipefds[1], 1);
//    dup2 (pipefds[1], 2);
    close (pipefds[1]);
    
    debug(3,"start_prog_and_return_fd -- child about to exec %s", argv[0]);

    execvp (argv[0], argv);

    debug(0,"start_prog_and_return_fd -- failed to exec %s: %s",argv[0],strerror(errno));

    _exit (1);
  }

  close (pipefds[1]);

  return pipefds[0];
}

int check_qstat_version( const char* version )
{

  // qstat output has to fit in here
  enum { QSTAT_BUFFER_SIZE = 4096 };
  char buffer[QSTAT_BUFFER_SIZE] = {0};
  int read_ret;
  int fd;
  pid_t pid;

  char* cmd[] = {QSTAT_EXEC,NULL};

  int ret = FALSE;

  fd_set rfds;
  struct timeval tv;
  int retval;

  int flags;

  fd = start_prog_and_return_fd(cmd,&pid);

  if (fd<0||pid<=0)
    return FALSE;

  flags = fcntl (fd, F_GETFL, 0);
  if (flags < 0 || fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0)
  {
    debug(0,"fcntl failed: %s", strerror(errno));
    return -1;
  }

  FD_ZERO(&rfds);
  FD_SET(fd, &rfds);
  
  // wait up to two seconds
  tv.tv_sec = 2;
  tv.tv_usec = 0;

  retval = select(fd+1, &rfds, NULL, NULL, &tv);

  if (!retval)
  {
    debug(0,"check_qstat_version -- No data within two seconds.");
    return FALSE;
  }

  // -1 because of \0!
  read_ret = read(fd,buffer,QSTAT_BUFFER_SIZE-1);
  // read error
  if (read_ret < 0)
  {
    debug(0,"check_qstat_version -- read failed: %s", strerror(errno));
    return FALSE;
  }
  // read hasn't read anything
  else if(read_ret == 0)
  {
    debug(0,"check_qstat_version -- didn't read anything");
    return FALSE;
  }

  {
    char *search_for = "qstat version";
    char *found_version, *version_end;
    
    found_version = strstr(buffer,search_for);
    
    if(!found_version) return FALSE;
    
    found_version = found_version+strlen(search_for);
    // skip whitespace
    for(;isspace(*found_version);found_version++);
    if(!*found_version) return FALSE;
    // skip until end of version string
    for(version_end=found_version;
	version_end &&
	*version_end != '\0' && !isspace(*version_end);
	version_end++);
    found_version=g_strndup(found_version,version_end-found_version);
    debug(3,"check_qstat_version -- found version <%s>",found_version);

    if(compare_qstat_version(found_version,version)>=0)
      ret=TRUE;

    g_free(found_version);
  }

  return ret;
}

void reset_main_status_bar() {
  // Reset bottom left status bar to show number of servers
  print_status (main_status_bar,
		  ngettext(_("%d server"),_("%d servers"), server_clist->rows),
		  server_clist->rows);
}

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
  gtk_widget_set_sensitive (edit_update_master_builtin_menu_item, sens);
  gtk_widget_set_sensitive (edit_add_master_menu_item, sens);
  gtk_widget_set_sensitive (edit_find_player_menu_item, sens);
  gtk_widget_set_sensitive (edit_find_again_menu_item, sens);
  gtk_widget_set_sensitive (player_filter_menu_item, sens);
  gtk_widget_set_sensitive (source_ctree, sens);

  sens = (!stat_process && selected && !source_is_favorites);

  gtk_widget_set_sensitive (favadd_menu_item, sens);
  gtk_widget_set_sensitive (edit_favadd_menu_item, sens);
  gtk_widget_set_sensitive (server_favadd_menu_item, sens);

  sens = (!stat_process && selected && source_is_favorites);

  gtk_widget_set_sensitive (delete_menu_item, sens);
  gtk_widget_set_sensitive (edit_delete_menu_item, sens);
  gtk_widget_set_sensitive (server_delete_menu_item, sens);

  sens = (!stat_process && masters_to_delete);

  gtk_widget_set_sensitive (edit_delete_master_menu_item, sens);
  gtk_widget_set_sensitive (source_delete_master_menu_item, sens);

  // you can only edit one server a time, no groups and no favorites
  sens = (cur_source && cur_source->next == NULL
	  && ! ((struct master *) cur_source->data)->isgroup
	  && ! source_is_favorites );
  
  gtk_widget_set_sensitive (source_edit_master_menu_item, sens);
  gtk_widget_set_sensitive (edit_edit_master_menu_item, sens);

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

  sens = (stat_process == NULL);

  for (i = 0; i < FILTERS_TOTAL; i++) {
    gtk_widget_set_state(filter_buttons[ i ],GTK_STATE_NORMAL);
    gtk_widget_set_sensitive (filter_buttons[i], sens);
    if(GTK_TOGGLE_BUTTON(filter_buttons[ i ])->active)
      gtk_widget_set_state(filter_buttons[ i ],GTK_STATE_ACTIVE);
  }
#if 0
  // grey out button if not redialing
  sens = (redialserver == 1);
  gtk_widget_set_sensitive (cancel_redial_menu_item, sens);
  gtk_widget_set_sensitive (server_cancel_redial_menu_item, sens);
#endif
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
    server_clist_build_filtered (cur_server_list, FALSE); /* in srv-list.c */
    reset_main_status_bar();
  }
}

// iterate through radio buttons and activate the one for the current server
// filter
static void filter_menu_activate_current()
{
  unsigned int count = 0;
  GSList* rbgroup = filter_menu_radio_buttons;
  GtkWidget* widget = NULL;

  while(rbgroup)
  {
    if(GTK_IS_CHECK_MENU_ITEM(rbgroup->data))
    {
      if(count == current_server_filter)
      {
	widget = GTK_WIDGET(rbgroup->data);
	break;
      }
      count++;
    }
    rbgroup=rbgroup->next;
  }

  if(widget)
  {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(widget), TRUE);
  }
}

void set_server_filter_menu_list_text( void ){

  /* baa -- Set the names of the filters if they have been set in
     the config file. The server_filters is defined in filter.h */

  char status_buf[64];
  char* name;

#if 0
  for (i = 0; i < MAX_SERVER_FILTERS; i++) {

    if( i == 0 ){
      if( current_server_filter == i )
	//server filter
	snprintf( buf, 64, _("None <--"));
      else 
	snprintf( buf, 64, _("None"));
      
    } else {
      if( server_filters[i].filter_name                 &&
	  strlen( server_filters[i].filter_name ) ){
	
	if( current_server_filter == i )
	  snprintf( buf, 64, "%s <--", server_filters[i].filter_name );
	else 
	  snprintf( buf, 64, "%s", server_filters[i].filter_name );
	
      } else {
	
	if( current_server_filter == i )
	  snprintf( buf, 64, _("Filter %d <--"), i );
	else 
	  snprintf( buf, 64, _("Filter %d"), i );
	
      }
    }

    if( server_filter_widget[i + filter_start_index ] && 
	GTK_BIN (server_filter_widget[i + filter_start_index])->child )
      {
	GtkWidget *child = GTK_BIN (server_filter_widget[i + filter_start_index ])->child;
	if (GTK_IS_LABEL (child))
	  gtk_label_set (GTK_LABEL (child), buf );
      }
  }

#endif

  
  /* Show the active filter on the status bar 
     -- Add code to indicate if the filter button is checked.
   */
  if( current_server_filter == 0 )
  {
    snprintf( status_buf, 64, _("No Server Filter Active"));
  }
  else
  {
    name = g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1)->filter_name;
    if(name)
    {
      snprintf( status_buf, 64, _("Server Filter: %s"), name);
    }
    else
    {
      snprintf( status_buf, 64, _("Server Filter: %d"), current_server_filter );
      debug(0,"this is a bug");
    }
  }
  
  print_status (main_filter_status_bar, status_buf); 

  reset_main_status_bar();

}


static void server_filter_select_callback (GtkWidget *widget, int number) {

  current_server_filter = number;

  filters[FILTER_SERVER].changed = FILTER_CHANGED;
  filters[FILTER_SERVER].last_changed = ++filter_current_time;

  server_clist_build_filtered (cur_server_list, FALSE); /* in srv-list.c */ 
  set_server_filter_menu_list_text ();

  config_push_prefix ( "/" CONFIG_FILE "/Server Filter" );
  config_set_int ("current_server_filter", current_server_filter);
  config_pop_prefix ();

  return;
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
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (server_serverfilter_menu_item), create_filter_menu());
    filter_menu_activate_current();
//happes automagically   server_clist_build_filtered (cur_server_list, TRUE);
    player_clist_redraw ();
  }
}


static void update_server_lists_from_selected_source (void) {
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

  master_selection_to_lists (cur_source, &cur_masters, &cur_server_list, 
                                                           &cur_userver_list);
  collate_server_lists (cur_masters, &cur_server_list, &cur_userver_list);
}


static int stat_lists_refresh (struct stat_job *job) {
  int items;
  int freeze;
 
  debug (6, "stat_lists_refresh() -- Job %lx", job);
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
  /*
  if(redialserver == 1)
    print_status (main_status_bar, _("Waiting to redial server(s)..."));
  else
  */
    print_status (main_status_bar, _("Done."));

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
  debug_increase_indent();
  debug (7, "stat_lists() -- Server List %p", servers);

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
  debug_decrease_indent();
}


static void stat_one_server (struct server *server) {
  GSList *list;

  debug (6, "Server %lx", server);
  if (!stat_process && server) {
    list = server_list_prepend (NULL, server);
    stat_lists (NULL, NULL, list, NULL);
  }
}


static void launch_close_handler (struct stat_job *job, int killed) {
  struct server_props *props;
  gboolean launch = FALSE;

  struct server *s;
  struct condef *con;
  int slots_buffer;

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
      launch = dialog_yesno (NULL, 1, _("Launch"), _("Cancel"),
                       _("Server %s:%d is %s.\n\nLaunch client anyway?"),
                       (s->host->name)? s->host->name : inet_ntoa (s->host->ip),
                       s->port,
                       (s->ping == MAX_PING)? "unreachable" : "down");
      if (!launch) {
        condef_free (con);
        return;
      }
    }
    



    /*pulp*/
    if (props) {
  	  if (props->reserved_slots) {


		slots_buffer=props->reserved_slots;
    	  }

    	  else {
    		slots_buffer=0;
    	  }
    }

    else {
    slots_buffer=0;
    }




    if (!launch && s->curplayers >= (s->maxplayers -slots_buffer)  && !con->spectate) {
  //  if (!launch && s->curplayers != 99 && !con->spectate) {
      launch = dialog_yesnoredial (NULL, 1, _("Launch"), _("Cancel"), _("Redial"), 
  		     _("Server %s:%d is full.\n\nLaunch client anyway?"),
  		     (s->host->name)? s->host->name : inet_ntoa (s->host->ip),
		     s->port);
      if (!launch) {
        condef_free (con);
        return;
      }
      else if (launch==2) 
      {
        redialserver = 1;
 
	/*
        gtk_timeout_add (5000, (GtkFunction)launch_redial, (gpointer) con);

        server_clist_refresh_server (s);

        print_status (main_status_bar, _("Waiting to redial server(s)..."));
        progress_bar_reset (main_progress_bar);
	*/

	launch = redial_dialog(con->s,slots_buffer);/*pulp*/

	if(launch == FALSE)
	{
	    condef_free (con);
	    return;
	}
      }
      else
        redialserver = 0;
    }

//  if(redialserver == 0) // we don't need to redial, so continue
    launch_close_handler_part2(con);
}

static gboolean launch_redial(struct condef *con)
{
  struct server *s;
  s = con->s;

  if(!s /*|| redialserver == 0*/ ) // !s may not happen
  {
    print_status (main_status_bar, _("Done."));
    progress_bar_reset (main_progress_bar);

    return FALSE; // stop redialing, we're done
  }

  stat_one_server(s);

  debug (1, "launch redial server name: s->name:%s",s->name);
  debug (1, "launch redial server ping: s->ping:%d",s->ping);  
  
  if (s->curplayers < s->maxplayers)
 // if (s->curplayers == 99)
  {
    // server not busy!

    print_status (main_status_bar, _("Done."));
    progress_bar_reset (main_progress_bar);

    launch_close_handler_part2(con);
    return FALSE; // stop redialing, we're done   
  }
  else
    return TRUE;
}


static void launch_close_handler_part2(struct condef *con)
{
  struct server_props *props;
  int launch = FALSE; 
  int save = 0;
  FILE *f;
  char *fn;
  char *temp_name;
  char *temp_mod;
  char *temp_game;

  struct server *s;

  char *launchargv[4];
  int pid;

  if (redialserver == 1) // was called from a redial
    play_sound(sound_redial_success, 0);
  else
    play_sound(sound_server_connect, 0);

  redialserver = 0; // Cancel any redialing

  s = con->s;
  props = properties (s);

  debug(3,"rest of launch_close_handler"); // alex

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
                         _("Save Password"), &save, _("Spectator Password:"));
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
                            _("Save Password"), &save, _("Server Password:"));
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

//  con->server = g_strdup_printf ("%s:%5d", inet_ntoa (s->host->ip), s->port);
  con->server = g_strdup_printf ("%s:%d", inet_ntoa (s->host->ip), s->port);
  con->gamedir = g_strdup (s->game);

  if (props && props->rcon_password)
    con->rcon_password = g_strdup (props->rcon_password);

  if (props && props->custom_cfg)
    con->custom_cfg = g_strdup (props->custom_cfg);



  launch = client_launch (con, TRUE);
  condef_free (con);

  if (!launch)
    return;

  // Save address etc to LaunchInfo.txt
  if (default_launchinfo) {
    fn = file_in_dir (user_rcdir, LAUNCHINFO_FILE);

    f = fopen (fn, "w");
    if (f) {

       temp_name = cur_server->name;
       temp_game = cur_server->game;
       temp_mod = cur_server->gametype;
       
       if (!temp_name)
          temp_name = "";

       fprintf (f, "GameType %s\n", games[cur_server->type].name);
       fprintf (f, "ServerName %s\n", temp_name);
       fprintf (f, "ServerAddr %s:%d\n", inet_ntoa (cur_server->host->ip), 
           cur_server->port);

       fprintf (f, "ServerMod ");  
       if(temp_game)
       {
	 fprintf (f, "%s", temp_game);
       }
       if(temp_game&&temp_mod)
       {
	 fprintf (f, ", ");  
       }
       if(temp_mod)
       {
	 fprintf (f, "%s", temp_mod);
       }
       fprintf (f, "\n");  

       fclose (f);
    }
    g_free (fn);
  }

  // Launch pre-launch script
  if (default_prelaunchexec) {

        pid = fork();
        if (pid == 0) {

          launchargv[0] = "sh";
          launchargv[1] = "-c";
          strcpy(launchargv[2],user_rcdir);
          strcat(launchargv[2],"/PreLaunch");
          launchargv[3]= 0;
          debug(0,"launchargv[2] is %s\n",launchargv[2]);
          execv("/bin/sh",launchargv);
        }     
  }

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

  debug (6, "launc_callback() --");
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
      demo = enter_string_with_option_dialog (TRUE, _("Spectator"), &spectate, 
                                                             _("Demo name:"));
    }
    else {
      demo = enter_string_dialog (TRUE, _("Demo name:"));
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
  debug (7, "server_clist_compare_func() --");

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

  event_type=EVENT_UPDATE;

  debug_increase_indent();
  debug (6, "update_source_callback() -- ");
  if (stat_process || !cur_source)
    return;

  master_selection_to_lists (cur_source, &masters, &servers, &uservers);
  if (masters || servers || uservers) {
    stat_lists (masters, uservers, servers, NULL);
  }
  debug_decrease_indent();

}


static void refresh_selected_callback (GtkWidget *widget, gpointer data) {
  GSList *list;

  if (stat_process)
  {
    debug(1,"nope");
    return;
  }

  event_type=EVENT_REFRESH_SELECTED;

  list = server_clist_selected_servers ();

  if (list)
    stat_lists (NULL, NULL, list, NULL);
}


static void refresh_callback (GtkWidget *widget, gpointer data) {
  GSList *servers;
  GSList *uservers;

  if (stat_process)
    return;

  event_type=EVENT_REFRESH;

  debug (7, "refresh_callback() -- Get Server List");

  servers = server_clist_all_servers ();
  uservers = userver_list_copy (cur_userver_list);

  debug (7, "refresh_callback() -- server list %lx", servers);

  if (servers || uservers) {
    stat_lists (NULL, uservers, servers, NULL);
  }
}


static void stop_callback (GtkWidget *widget, gpointer data) {

  event_type = 0; // To prevent sound from stopped action from playing

  if (stat_process) {
    stat_stop (stat_process);
    stat_process = NULL;
  }
  redialserver = 0;  // Reset redialserver so prompt comes up next time 
  play_sound(sound_stop, 0);
  event_type = 0; // To prevent sound from stopped action from playing
}


static void add_to_favorites_callback (GtkWidget *widget, gpointer data) {
  GList *selected = server_clist->selection;
  GSList *list;
  GSList *tmp;
  enum { buflen = 256 };
  char   buf[buflen]; /* if you change this, change the statement below */
  int    server_list_size;

  debug (7, "add_to_favorites_callback() -- ");
  if (stat_process || !selected)
    return;

  list = server_clist_selected_servers ();
  if (list) {
    for (tmp = list; tmp; tmp = tmp->next) {
      server_list_size = g_slist_length (favorites->servers );
      favorites->servers = 
	server_list_append (favorites->servers, (struct server *) tmp->data);
      if( server_list_size < g_slist_length (favorites->servers )){
	snprintf( buf, buflen, "Added Server #%d: '%s'",
	    g_slist_length (favorites->servers ),
	    ((struct server *)tmp->data)->name);
      } else {
	snprintf( buf, buflen, "Server '%s' Already In Favorites",
	    ((struct server *)tmp->data)->name);
      }
      print_status (main_status_bar, buf );
      
    }
    debug (7, "add_to_favorites_callback() -- Saving To Favorites");
    save_favorites ();
    server_list_free (list);
  }
}


static void add_server_real (struct stat_job *job, struct server *s) {
  int row;

  debug (6, "add_server_real() -- Server %lx", s);
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
    dialog_ok (NULL, _("Host %s not found"), us->hostname);
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

  debug (6, "add_server_callback() -- ");
  
  if (stat_process)
    return;

  // FIXME:
  // Set type to 0 - what was is supposed to pass?  The last server type selected in the server list???
  // Alex - 10/14/2002
  type = 0;

  str = add_server_dialog (&type);
  // str = add_server_dialog (0);
  if (!str || !*str)
    return;

  if (!parse_address (str, &addr, &port)) {
    dialog_ok (NULL, _("\"%s\" is not valid host[:port] combination."), str);
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

  debug (7, "del_server_callback() -- ");
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

static void copy_server_callback_plus (GtkWidget *widget, gpointer data) {
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
    g_snprintf (buf, 256, "%i  %s:%d  %s  %s  %i of %i", s->ping, inet_ntoa
       (s->host->ip), s->port, s->name, s->map, s->curplayers, s->maxplayers);
    gtk_editable_insert_text (selection_manager, buf, strlen (buf), &pos);
    gtk_editable_select_region (selection_manager, 0, -1);
    return;

  default:
    for (; selection; selection = selection->next) {
      s = (struct server *) gtk_clist_get_row_data (
                                         server_clist, (int) selection->data);
      g_snprintf (buf, 256, "%i  %s:%d  %s  %s  %i of %i\n", s->ping, inet_ntoa
       (s->host->ip), s->port, s->name, s->map, s->curplayers, s->maxplayers);
      gtk_editable_insert_text (selection_manager, buf, strlen (buf), &pos);
    }
    gtk_editable_select_region (selection_manager, 0, -1);
    break;

  }
}

static void update_master_builtin_callback (GtkWidget *widget, gpointer data) {

  update_master_list_builtin();

}

static void add_master_callback (GtkWidget *widget, gpointer data) {
  struct master *m;

  if (stat_process)
    return;

  m = add_master_dialog(NULL);

  if (m) {
    source_ctree_add_master (source_ctree, m);
    source_ctree_select_source (m);
  }
}

// does only work with one master selected
static void edit_master_callback (GtkWidget *widget, gpointer data) {
  struct master *master_to_edit, *master_to_add;

  if(!cur_source) return;

  master_to_edit = (struct master *) cur_source->data;
  source_ctree_select_source (master_to_edit);
  master_to_add = add_master_dialog(master_to_edit);
  if (master_to_add) {
    source_ctree_add_master (source_ctree, master_to_add);
    source_ctree_select_source (master_to_add);
  }
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
  {
    dialog_ok(NULL,_("You have to select the server you want to delete"));
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
      print_status (main_status_bar, _("Find Player: %s"), pattern);
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

  debug (7, "resolve_callback() --");
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

static void cancelredial_callback (GtkWidget *widget, gpointer data) {

  if (stat_process)
    return;

  redialserver = 0;
  print_status (main_status_bar, _("Done."));
  progress_bar_reset (main_progress_bar);

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
	                          _("Save Password"), &save, _("Server Password:"));

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
  debug (7, "server_clist_select_callback() -- Row %d", row);
  server_clist_sync_selection ();

  if (bevent && bevent->type == GDK_2BUTTON_PRESS && bevent->button == 1)
    launch_callback (NULL, LAUNCH_NORMAL);
}


static void server_clist_unselect_callback (GtkWidget *widget, int row,
                            int column, GdkEvent *event, GtkWidget *button) {
  debug (7, "server_clist_uselect_callback() -- Row %d", row);
  server_clist_sync_selection ();
}


/* Deal with key-presses in the server pane */
static void server_clist_keypress_callback (GtkWidget *widget, GdkEventKey *event)

{
  
  debug (3, "server_clist_keypress_callback() -- CLIST Key %x", event->keyval ); 
  if (event->keyval == GDK_Delete) {
    del_server_callback( widget, event );
  } else if (event->keyval == GDK_Insert ) {
    if (event->state & GDK_SHIFT_MASK ){
      add_to_favorites_callback( widget, event );
    } else {
      add_server_callback (widget, event);
    }
  } else if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter ) {
    launch_callback( widget, LAUNCH_NORMAL );
  }
}

static int source_ctree_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent = (GdkEventButton *) event;
  GList *selection;
  int row;
  GtkCTreeNode *node, *node_under_mouse;
  int node_is_in_selection = 0;
  
  if (event->type == GDK_BUTTON_PRESS &&
                   bevent->window == GTK_CLIST(source_ctree)->clist_window) {

    switch (bevent->button) {

    case 3:
      // lets see which row the cursor is on
      if (gtk_clist_get_selection_info (GTK_CLIST(source_ctree), 
                                          bevent->x, bevent->y, &row, NULL)) {
	// list of selected items
	selection = GTK_CLIST(source_ctree)->selection;
        // XXX: what is the first part of the && good for?
	if (!g_list_find (selection, (gpointer) row) && 
                 (bevent->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == 0) {
	  node_under_mouse = gtk_ctree_node_nth(GTK_CTREE (source_ctree),row);
	  if(node_under_mouse)
	  {
	    // go through all selected masters and search if the one under the
	    // cursor is among them
	    while(selection)
	    {
	      node = GTK_CTREE_NODE(selection->data);
	      if(node == node_under_mouse)
	      {
		node_is_in_selection = 1;
		break;
	      }
	      selection = selection->next;
	    }

	    // clear selection and only select the one under the curser
	    if(!node_is_in_selection)
	    {
	      gtk_ctree_unselect_recursive(GTK_CTREE(source_ctree),NULL);
	      gtk_ctree_select (GTK_CTREE (source_ctree), node_under_mouse);
	    }
	  }
	}

      }

      gtk_menu_popup (GTK_MENU (source_menu), NULL, NULL, NULL, NULL,
	                                        bevent->button, bevent->time);
      return TRUE;

    default:
      return FALSE;
    }

  }
  return FALSE;
}


static int server_clist_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent = (GdkEventButton *) event;
  GList *selection;
  int row;
  
  /* debug (7, "server_clist_event_callback() -- "); */
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

  debug (6, "souce_selection_changed() --");
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
  
  reset_main_status_bar();
}


static void source_ctree_selection_changed_callback (GtkWidget *widget, 
                    int row, int column, GdkEvent *event, GtkWidget *button) {
  debug(6,"source_ctree_selection_changed_callback(%p,%d,%d,%p,%p)",widget,row,column,event,button);
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
  char *intro = _("X11 Quake/QuakeWorld/Quake2/Quake3 Front-End\n");
  char *version = g_strdup_printf(_("Version %s\n\n"),XQF_VERSION);

  /* 8859-1 translators can use the copyright symbol © instead of (C) */
  char *author = _("Copyright (C) 1998-2002 Roman Pozlevich <roma@botik.ru>\n\n");
  char *urls = _("http://www.linuxgames.com/xqf\n"
		"http://sourceforge.net/projects/xqf\n\n");
  char *contrib = _("Significant Contributors:\n\n"
	"Bill Adams <webmaster@evil.inetarena.com>\n"
	"Alex Burger <alex_b@users.sourceforge.net>\n"
	"Jordi Mallach <jordi@sindominio.net>\n"
	"Ludwig Nussel <l-n@users.sourceforge.net>\n\n");
  char *contrib2 = _("Other Contributors:\n\n"
	"Jochen Baier <email@jochen-baier.de>\n\n");
  char *bugs = _("Bug reports:\n\n"
		"http://sourceforge.net/projects/xqf\n\n");
  char *features = _("Feature requests:\n\n"
		"http://www.linuxgames.com/xqf Forums section\n");
  dialog_ok (_("About XQF"), 
	     "%s%s%s%s%s%s%s%s",intro,version,author,urls,contrib,contrib2,bugs,features);
  g_free(version);
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
    MENU_ITEM,		N_("Connect"),		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL,
    &connect_menu_item
  },
  { 
    MENU_ITEM,		N_("Observe"),		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE,
    &observe_menu_item
  },
  { 
    MENU_ITEM,		N_("Record Demo"),	0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD,
    &record_menu_item
  },
/*
  { 
    MENU_ITEM,		N_("Cancel Redial"),   	0,	0,
    GTK_SIGNAL_FUNC (cancelredial_callback), NULL,
    &cancel_redial_menu_item
  },
  */
  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("Add new Server"),		0,   	0,
    GTK_SIGNAL_FUNC (add_server_callback), NULL,
    &add_menu_item
  },
  { 
    MENU_ITEM,		N_("Add to Favorites"),	0,	0,
    GTK_SIGNAL_FUNC (add_to_favorites_callback), NULL,
    &favadd_menu_item
  },
  { 
    MENU_ITEM,		N_("Remove from Favorites"),		0,   	0,
    GTK_SIGNAL_FUNC (del_server_callback), NULL,
    &delete_menu_item
  },
  { 
    MENU_ITEM,		N_("Copy"),		0,   	0,
    GTK_SIGNAL_FUNC (copy_server_callback), NULL,
    NULL
  },
  { 
    MENU_ITEM,		N_("Copy+"),		0,   	0,
    GTK_SIGNAL_FUNC (copy_server_callback_plus), NULL,
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("Refresh"),		0,	0,
    GTK_SIGNAL_FUNC (refresh_callback), NULL,
    &refresh_menu_item
  },
  { 
    MENU_ITEM,		N_("Refresh Selected"),	0,	0,
    GTK_SIGNAL_FUNC (refresh_selected_callback), NULL,
    &refrsel_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("DNS Lookup"),	0,	0,
    GTK_SIGNAL_FUNC (resolve_callback), NULL,
    &resolve_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("RCon"),   	0,	0,
    GTK_SIGNAL_FUNC (rcon_callback), NULL,
    &rcon_menu_item
  },
  { 
    MENU_ITEM,		N_("Properties"),   	0,	0,
    GTK_SIGNAL_FUNC (properties_callback), NULL,
    &properties_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem file_menu_items[] = {
  { 
    MENU_ITEM,		N_("_Statistics"),	0,	0,
    GTK_SIGNAL_FUNC (statistics_callback), NULL,
    &file_statistics_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("_Exit"),		'Q',	GDK_CONTROL_MASK,
    NULL,		NULL,
    &file_quit_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

// appears on right click on a master server
static const struct menuitem source_ctree_popup_menu[] = {
  {
    MENU_ITEM,          N_("Add _Master"),        'M',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (add_master_callback), NULL,
    &source_add_master_menu_item
  },
  {
    MENU_ITEM,          N_("_Rename Master"),        0, 0,
    GTK_SIGNAL_FUNC (edit_master_callback), NULL,
    &source_edit_master_menu_item
  },
  {
    MENU_ITEM,          N_("D_elete Master"),        0,	0,
    GTK_SIGNAL_FUNC (del_master_callback), NULL,
    &source_delete_master_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem edit_menu_items[] = {
  { 
    MENU_ITEM,		N_("_Add new Server"),	'N',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (add_server_callback), NULL,
    &edit_add_menu_item
  },
  { 
    MENU_ITEM,		N_("Add to _Favorites"),	0, 0,
    GTK_SIGNAL_FUNC (add_to_favorites_callback), NULL,
    &edit_favadd_menu_item
  },
  { 
    MENU_ITEM,		N_("_Remove from Favorites"),		'D',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (del_server_callback), NULL,
    &edit_delete_menu_item
  },
  { 
    MENU_ITEM,		N_("_Copy"),		'C',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (copy_server_callback), NULL,
    NULL
  },
  { 
    MENU_ITEM,		N_("_Copy+"),		'O',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (copy_server_callback_plus), NULL,
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  {
    MENU_ITEM,          N_("Add Default Masters" ),        0,	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (update_master_builtin_callback), NULL,
    &edit_update_master_builtin_menu_item
  },

  {
    MENU_ITEM,          N_("Add _Master"),        'M',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (add_master_callback), NULL,
    &edit_add_master_menu_item
  },
  {
    MENU_ITEM,          N_("_Rename Master"),        0, 0,
    GTK_SIGNAL_FUNC (edit_master_callback), NULL,
    &edit_edit_master_menu_item
  },
  {
    MENU_ITEM,          N_("D_elete Master"),        0,	0,
    GTK_SIGNAL_FUNC (del_master_callback), NULL,
    &edit_delete_master_menu_item
  },

  { MENU_SEPARATOR,    NULL,                   0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("_Find Player"),	'F',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (find_player_callback), (gpointer) FALSE,
    &edit_find_player_menu_item
  },
  { 
    MENU_ITEM,		N_("Find A_gain"),		'G',   	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (find_player_callback), (gpointer) TRUE,
    &edit_find_again_menu_item
  },

  { MENU_SEPARATOR,    NULL,                   0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("Properties"),   	0,	0,
    GTK_SIGNAL_FUNC (properties_callback), NULL,
    &properties_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem view_menu_items[] = {
  { 
    MENU_ITEM,		N_("_Refresh"),		'R',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (refresh_callback), NULL,
    &view_refresh_menu_item
  },
  { 
    MENU_ITEM,		N_("Refresh _Selected"),	'S',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (refresh_selected_callback), NULL,
    &view_refrsel_menu_item
  },
  { 
    MENU_ITEM,		N_("_Update From Master"),	'U',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (update_source_callback), NULL,
    &view_update_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_CHECK_ITEM,	N_("Show _Host Names"),	'H',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (show_hostnames_callback), NULL,
    &view_hostnames_menu_item
  },
  { 
    MENU_CHECK_ITEM,	N_("Show Default _Port"),	0,	0,
    GTK_SIGNAL_FUNC (show_default_port_callback), NULL,
    &view_defport_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};


/*
  baa -- Oh, this is kind of bad.  In order to allow
  the number of menus to be changed at compile (and in
  the future, run) time, I have to allocate and set
  up each of the filter menu items.  But that makes
  it no longer a const.
*/

static const struct menuitem server_menu_items[] = {
  {
    MENU_ITEM,		N_("_Server Filters"),	0,	0,
    NULL, 0,
    &server_serverfilter_menu_item
  },
  { 
    MENU_ITEM,		N_("_Connect"),		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL,
    &server_connect_menu_item
  },
  { 
    MENU_ITEM,		N_("_Observe"),		0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE,
    &server_observe_menu_item
  },
  { 
    MENU_ITEM,		N_("Record _Demo"),	0,	0,
    GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD,
    &server_record_menu_item
  },

/*
  { 
    MENU_ITEM,		N_("Cancel Redial"),  	0,	0,
    GTK_SIGNAL_FUNC (cancelredial_callback), NULL,
    &server_cancel_redial_menu_item
  },
*/ 
  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("_Add new Server"),	0, 0,
    GTK_SIGNAL_FUNC (add_server_callback), NULL,
    &edit_add_menu_item
  },
  { 
    MENU_ITEM,		N_("Add to _Favorites"),	0, 0,
    GTK_SIGNAL_FUNC (add_to_favorites_callback), NULL,
    &server_favadd_menu_item
  },
  { 
    MENU_ITEM,		N_("_Remove from Favorites"),		0,   	0,
    GTK_SIGNAL_FUNC (del_server_callback), NULL,
    &server_delete_menu_item
  },

  { 
    MENU_ITEM,		N_("DNS _Lookup"),		'L',	GDK_CONTROL_MASK,
    GTK_SIGNAL_FUNC (resolve_callback), NULL,
    &server_resolve_menu_item
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("_RCon"),   		0,	0,
    GTK_SIGNAL_FUNC (rcon_callback), NULL,
    &server_rcon_menu_item
  },
  { 
    MENU_ITEM,		N_("_Properties"),  	0,	0,
    GTK_SIGNAL_FUNC (properties_callback), NULL,
    &server_properties_menu_item
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem preferences_menu_items[] = {
  { 
    MENU_ITEM,		N_("_General"),	0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_GENERAL + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		N_("_Games"),		0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog), 
    (gpointer) (PREF_PAGE_GAMES + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		N_("_Appearance"),	0,	0,
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_APPEARANCE + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		N_("_QStat Options"),	0,	0, 
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_QSTAT + UNKNOWN_SERVER * 256),
    NULL
  },
  { 
    MENU_ITEM,		N_("_Sound Options"),	0,	0, 
    GTK_SIGNAL_FUNC (start_preferences_dialog),
    (gpointer) (PREF_PAGE_SOUNDS + UNKNOWN_SERVER * 256),
    NULL
  },

  { MENU_SEPARATOR,	NULL,			0, 0, NULL, NULL, NULL },

  { 
    MENU_ITEM,		N_("_Server Filter"),	0,	0,
    GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) FILTER_SERVER,
    NULL
  },
  { 
    MENU_ITEM,		N_("Player _Filter"),	0,	0,
    GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) FILTER_PLAYER,
    NULL
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};

static const struct menuitem help_menu_items[] = {
  { 
    MENU_ITEM,		N_("_About"),		0,	0,
    GTK_SIGNAL_FUNC (about_dialog), NULL,
    NULL
  },

  { MENU_END,		NULL,			0, 0, NULL, NULL, NULL }
};


static const struct menuitem menubar_menu_items[] = {
  {
    MENU_BRANCH,		N_("_File"),	0,	0,
    NULL, &file_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		N_("_Edit"),	0,	0,
    NULL, &edit_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		N_("_View"),	0,	0,
    NULL, &view_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		N_("_Server"),	0,	0,
    NULL, &server_menu_items,
    NULL
  },
  {
    MENU_BRANCH,		N_("_Preferences"),	0,	0,
    NULL, &preferences_menu_items,
    NULL
  },
  {
    MENU_LAST_BRANCH,		N_("_Help"),	0,	0,
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

  menu_item = create_player_menu_item (_("Mark as Red"), 0);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
		      (gpointer) PLAYER_GROUP_RED);
  gtk_widget_show (menu_item);

  menu_item = create_player_menu_item (_("Mark as Green"), 1);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
		      (gpointer) PLAYER_GROUP_GREEN);
  gtk_widget_show (menu_item);

  menu_item = create_player_menu_item (_("Mark as Blue"), 2);
  gtk_menu_append (GTK_MENU (marker_menu), menu_item);
  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
		      GTK_SIGNAL_FUNC (add_to_player_filter_callback),
                      (gpointer) PLAYER_GROUP_BLUE);
  gtk_widget_show (menu_item);

  menu_item = gtk_menu_item_new_with_label (_("Add to Player Filter"));
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
                   _("Update"), _("Update from master"), NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (update_source_callback), NULL);

  pixmap = gtk_pixmap_new (refresh_pix.pix, refresh_pix.mask);
  gtk_widget_show (pixmap);

  refresh_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   _("Refresh"), _("Refresh current list"), NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (refresh_callback), NULL);

  pixmap = gtk_pixmap_new (refrsel_pix.pix, refrsel_pix.mask);
  gtk_widget_show (pixmap);

  refrsel_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   _("Ref.Sel."), _("Refresh selected servers"), NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (refresh_selected_callback), NULL);

  pixmap = gtk_pixmap_new (stop_pix.pix, stop_pix.mask);
  gtk_widget_show (pixmap);

  stop_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   _("Stop"), _("Stop"), NULL,
                   pixmap,
		   GTK_SIGNAL_FUNC (stop_callback), NULL);

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  pixmap = gtk_pixmap_new (connect_pix.pix, connect_pix.mask);
  gtk_widget_show (pixmap);

  connect_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   _("Connect"), _("Connect"), NULL,
                   pixmap,
                 GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_NORMAL);

  pixmap = gtk_pixmap_new (observe_pix.pix, observe_pix.mask);
  gtk_widget_show (pixmap);

  observe_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                  _("Observe"), _("Observe"), NULL,
                  pixmap,
               GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_SPECTATE);

  pixmap = gtk_pixmap_new (record_pix.pix, record_pix.mask);
  gtk_widget_show (pixmap);

  record_button = gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                  _("Record"), _("Record Demo"), NULL,
                  pixmap,
                 GTK_SIGNAL_FUNC (launch_callback), (gpointer) LAUNCH_RECORD);

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  /*
   *  Filter buttons
   */

  for (i = 0, mask = 1; i < FILTERS_TOTAL; i++, mask <<= 1) {
    // Translators: e.g. Server Filter
    g_snprintf (buf, 128, _("%s Filter Enable / Disable"), _(filters[i].name));

    pixmap = gtk_pixmap_new (filter_pix[i].pix, filter_pix[i].mask);
    gtk_widget_show (pixmap);

    filter_buttons[i] = gtk_toolbar_append_element (
		   GTK_TOOLBAR (main_toolbar),
		   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
                   _(filters[i].short_name), buf, NULL,
                   pixmap,
                   GTK_SIGNAL_FUNC (filter_toggle_callback), (gpointer) mask);

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (filter_buttons[i]), 
                                    ((cur_filter & mask) != 0)? TRUE : FALSE);
  }
  

  gtk_toolbar_append_space (GTK_TOOLBAR (main_toolbar));

  for (i = 0, mask = 1; i < FILTERS_TOTAL; i++, mask <<= 1) {
    // Translators: e.g. Server Filter Configuration
    g_snprintf (buf, 128, _("%s Filter Configuration"), _(filters[i].name));

    pixmap = gtk_pixmap_new (filter_cfg_pix[i].pix, filter_cfg_pix[i].mask);
    gtk_widget_show (pixmap);

    gtk_toolbar_append_item (GTK_TOOLBAR (main_toolbar), 
                   _(filters[i].short_cfg_name), buf, NULL,
                   pixmap,
                   GTK_SIGNAL_FUNC (start_filters_cfg_dialog), (gpointer) i);
  }

  set_toolbar_appearance (GTK_TOOLBAR (main_toolbar), 
                                 default_toolbar_style, default_toolbar_tips);
}

/** build server filter menu for menubar
 */
static GtkWidget* create_filter_menu()
{
  unsigned int i;
  GtkWidget *menu;
  GtkWidget *menu_item;
//  GtkWidget *radiobutton;
  struct server_filter_vars* filter = NULL;
  GSList* rbgroup = NULL;

  filter_menu_radio_buttons = NULL;

  menu = gtk_menu_new();
//  menu_item = gtk_menu_item_new_with_label(_("None"));
//  menu_item = gtk_check_menu_item_new_with_label(_("None"));
//  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), (current_server_filter == 0));
//  gtk_menu_append (GTK_MENU (menu), menu_item);
//  gtk_widget_show (menu_item);
//  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
//	   GTK_SIGNAL_FUNC (server_filter_select_callback), 0);

  for (i = 0;i<=server_filters->len;i++)
  {
    char* name = NULL;
    if(i == 0)
    {
      filter = NULL;
      name = _("None");
    }
    else
    {
      filter = g_array_index (server_filters, struct server_filter_vars*, i-1);
      name = filter->filter_name;
    }
//      menu_item = gtk_menu_item_new_with_label(name);
//    menu_item = gtk_menu_item_new();
//    radiobutton = gtk_radio_button_new_with_label(rbgroup,name);
//    rbgroup = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
//    gtk_container_add(GTK_CONTAINER(menu_item),radiobutton);
//    gtk_widget_show(radiobutton);
    
//    menu_item = gtk_check_menu_item_new_with_label(name);
//    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), (current_server_filter == i));
    // that doesn't do what the docu says, right?
//    gtk_check_menu_item_set_show_toggle ( GTK_CHECK_MENU_ITEM (menu_item), FALSE);

    menu_item = gtk_radio_menu_item_new_with_label(rbgroup,name);
    rbgroup = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), FALSE);
    filter_menu_radio_buttons = g_slist_append(filter_menu_radio_buttons,menu_item);

    gtk_menu_append (GTK_MENU (menu), menu_item);
    gtk_widget_show (menu_item);

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	   GTK_SIGNAL_FUNC (server_filter_select_callback), (gpointer)i); // array starts from zero but filters from 1

    // add separator
    if(i == 0)
    {
      menu_item = gtk_menu_item_new ();
      gtk_widget_set_sensitive (menu_item, FALSE);
      gtk_menu_append (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
    }
  }

//  filter_menu = menu;
  return menu;
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
//  char *buf;

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

  /* 
     Before we create the right-click server menu, we
     need to set up all of the needed filter entries.  This
     is a terrible thing to do because in menus.c, the function
     that builds the menu expects a const.  However, we are not going
     go g_free or otherwise reuse the memory we g_malloc so we should be
     okay.

     If you change the number of menu items before the various filters, 
     you need to change the server_filter_widget line in xqf.h.
  */

#if 0
  if ((server_filter_menu_items = g_malloc( sizeof( struct menuitem ) *  ( MAX_SERVER_FILTERS + 4 )))) { 
    i = 0;
    j = 0;
    server_filter_menu_items[i].type       = MENU_ITEM;
    server_filter_menu_items[i].label      = _("Filters");
    server_filter_menu_items[i].accel_key  = 0;
    server_filter_menu_items[i].accel_mods = 0;
    server_filter_menu_items[i].callback   = NULL;
    server_filter_menu_items[i].user_data  = NULL;
    server_filter_menu_items[i].widget     = &server_filter_widget[i];
    i++;

    server_filter_menu_items[i].type       = MENU_SEPARATOR;
    server_filter_menu_items[i].label      = NULL;
    server_filter_menu_items[i].accel_key  = 0;
    server_filter_menu_items[i].accel_mods = 0;
    server_filter_menu_items[i].callback   = NULL;
    server_filter_menu_items[i].user_data  = NULL;
    server_filter_menu_items[i].widget     = &server_filter_widget[i];
    i++;

    /* Start of the filters */
    filter_start_index = i;
    server_filter_menu_items[i].type       = MENU_ITEM;
    server_filter_menu_items[i].label      = _("None");
    server_filter_menu_items[i].accel_key  = 0;
    server_filter_menu_items[i].accel_mods = 0;
    server_filter_menu_items[i].callback   = GTK_SIGNAL_FUNC (server_filter_select_callback);
    server_filter_menu_items[i].user_data  = (gpointer) j;
    server_filter_menu_items[i].widget     = &server_filter_widget[i];

    i++; 
    j++;

    for( ; i < (MAX_SERVER_FILTERS + filter_start_index); i++, j++ ){
      buf = g_malloc( sizeof( char ) * ( 16 )); 
      sprintf( buf, "Filter %d", j );
      server_filter_menu_items[i].type       = MENU_ITEM;
      server_filter_menu_items[i].label      = buf;
      server_filter_menu_items[i].accel_key  = 0;
      server_filter_menu_items[i].accel_mods = 0;
      server_filter_menu_items[i].callback   = GTK_SIGNAL_FUNC (server_filter_select_callback);
      server_filter_menu_items[i].user_data  = (gpointer) j;
      server_filter_menu_items[i].widget     = &server_filter_widget[i];
    }

    server_filter_menu_items[i].type       = MENU_END;
    server_filter_menu_items[i].label      = NULL;
    server_filter_menu_items[i].accel_key  = 0;
    server_filter_menu_items[i].accel_mods = 0;
    server_filter_menu_items[i].callback   = NULL;
    server_filter_menu_items[i].user_data  = NULL;
    server_filter_menu_items[i].widget     = NULL;
  }

  /* Depeding on where you want the filters to appear... */
#if 0
  srvopt_menu_items[0].user_data = &server_filter_menu_items[0];
#else
  server_menu_items[0].user_data = &server_filter_menu_items[0];
#endif
#endif

  server_menu = create_menu (srvopt_menu_items, accel_group);


  source_menu = create_menu (source_ctree_popup_menu, accel_group);

  /* We will call set_server_filter_menu_list_text (); below after we 
     have the filter status bar. It used to be here -baa  */

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


  // add server filters to menu
  server_filter_menu_items = g_array_new(FALSE,FALSE,sizeof(GtkWidget*));
  gtk_menu_item_set_submenu (GTK_MENU_ITEM (server_serverfilter_menu_item), create_filter_menu());

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
  gtk_signal_connect (GTK_OBJECT (source_ctree), "event",
                         GTK_SIGNAL_FUNC (source_ctree_event_callback), NULL);

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

  gtk_signal_connect (GTK_OBJECT (server_clist), "key_press_event",
		      GTK_SIGNAL_FUNC (server_clist_keypress_callback), NULL);


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

  main_filter_status_bar = gtk_statusbar_new ();
  gtk_widget_set_usize (main_filter_status_bar, 100, -1);
  gtk_box_pack_start (GTK_BOX (hbox), main_filter_status_bar, TRUE, TRUE, 0);
  gtk_widget_show (main_filter_status_bar);

  main_progress_bar = create_progress_bar ();
  gtk_widget_set_usize (main_progress_bar, 200, -1);
  gtk_box_pack_end (GTK_BOX (hbox), main_progress_bar, FALSE, FALSE, 0);
  gtk_widget_show (main_progress_bar);

  /* Make sure the current filter is dispalyed and applied if needed */
  set_server_filter_menu_list_text (); 

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  gtk_widget_show (main_vbox);

  restore_main_window_geometry ();

  gtk_widget_show (main_window);

  gtk_window_add_accel_group (GTK_WINDOW (main_window), accel_group);
  gtk_accel_group_unref (accel_group);
  
  // Set tooltips - also in prefs_load
  tooltips = gtk_tooltips_new ();
  if (default_toolbar_tips)
    gtk_tooltips_enable(tooltips);
  else
    gtk_tooltips_disable(tooltips);
                
}

void play_sound (const char *sound, const int override)
{
  char *launchargv[3];
  int pid;

  if(!sound_enable && !override) {
    debug(2,"sound disabled - not playing");
    return;
  }

  if(!sound || !sound_player) {
    debug(2,"sound player or sound file not defined - not playing anything");
    return;
  }

  if(!strlen(sound) || !strlen(sound_player))
  {
      return;
  }


  pid = fork();
  if (pid == 0)
  {
    launchargv[0] = sound_player;

    if(sound[0] != '/'){
      // Does not start with a / so prepend user_rcdir
      debug(1,"Prepending user_rcdir to sound file");
      launchargv[1] = g_malloc(strlen(sound)+strlen(user_rcdir)+2); // maybe should be the filename only?
      strcpy(launchargv[1],user_rcdir);
      strcat(launchargv[1],"/");
      strcat(launchargv[1],sound);
    }
    else
    {
      launchargv[1] = g_malloc(strlen(sound)+1); // maybe should be the filename only?
      strcpy(launchargv[1],sound);
    }    

    launchargv[2]=0;

    debug(1,"sound player (program): %s",launchargv[0]);
    debug(1,"sound to play: %s",launchargv[1]);
    execvp(launchargv[0],launchargv);

    g_free (launchargv[1]);
    
    _exit (1);
  }
}

int main (int argc, char *argv[]) {
  char *gtk_config;
  int newversion = FALSE;
  char required_qstat_version[]="2.4e";

  int i,j; /* For parsing the command line. */

  xqf_start_time = time (NULL);

  redialserver=0;
  
#ifdef ENABLE_NLS
#  ifdef HAVE_LC_MESSAGES
  setlocale(LC_CTYPE, "");
  setlocale(LC_MESSAGES, "");
#  else
  setlocale(LC_ALL, "");
#  endif
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  set_debug_level (DEFAULT_DEBUG_LEVEL);
  debug (5, "main() -- Debug Level Default Set at %d", DEFAULT_DEBUG_LEVEL);

  if (!init_user_info ()) {
    return 1;
  }

  if (dns_spawn_helper () < 0) {
    fprintf (stderr, "Unable to start DNS helper\n");
    return 1;
  }

  gtk_config = file_in_dir (user_rcdir, "gtkrc");
  gtk_rc_add_default_file (gtk_config);
  g_free (gtk_config);

  gtk_init (&argc, &argv);

  /* Parse the command line.  Really should use getops lib
     but we only have one option right now.  Start
     at pos. 1 since location 0 has the program name. */
  for (i = 1; i < argc; i++ ) {
    if (strcmp (argv[i], "-d") == 0){
      if ((i+1) < argc ) {
	j = atoi (argv[i+1]);
	if (j) i++;
	else  j = 1; /* In case it was not a number. */

      } else {
	j = 1;
      }
      set_debug_level (j);
      debug( 1, "main() -- Debug level set to %d", get_debug_level());
    } else {
      fprintf (stderr, "main() -- Unknown Option '%s' (only -d N is valid, N>5 == Lots of output)\n", argv[i]);
    }
  }
      

  dns_gtk_init ();

  rc_check_dir ();

  init_games();

  config_set_base_dir (user_rcdir);
  newversion = prefs_load ();

  gtk_preview_set_gamma (1.5);

  props_load ();
  filters_init ();

  host_cache_load ();
  init_masters (newversion);

  client_init ();
  ignore_sigpipe ();

  on_sig(SIGUSR1, sighandler_debug);
  on_sig(SIGUSR2, sighandler_debug);

  add_server_init ();
  add_master_init ();
  psearch_init ();
  rcon_init ();

  play_sound(sound_xqf_start, 0);

  create_main_window ();

  source_ctree_select_source (favorites);
  filter_menu_activate_current();

  print_status (main_status_bar, NULL);

  if (default_auto_favorites)
    refresh_callback (NULL, NULL);

  if(check_qstat_version(required_qstat_version) == FALSE)
  {
    dialog_ok(NULL,
	_("You need at least qstat version %s for xqf to function properly"),
	required_qstat_version);
  }

  debug(1,"startup time %ds", time(NULL)-xqf_start_time);

  gtk_main ();

  play_sound(sound_xqf_quit, 0);

  unregister_window (main_window);
  main_window = NULL;

  if (stat_process)
    stop_callback (NULL, NULL);

  debug (1, "total servers: %d", servers_total ());
  debug (1, "total uservers: %d", uservers_total ());
  debug (1, "total hosts: %d", hosts_total ());
#if 0
  if (servers_total () > 0) {
    GSList *list = all_servers (); /* Debug code, free done in two lines */
    
    server_list_fprintf (stderr, list);
    server_list_free (list);
  }
#endif


  if (server_menu) {
    debug( 6, "EXIT: destroy server_menu");
    gtk_widget_destroy (server_menu);
    server_menu = NULL;
  }

  if (player_menu) {
    debug( 6, "EXIT: destroy player_menu");
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

  debug( 6, "EXIT: Free Server Lists");
  g_slist_free (cur_source);
  server_list_free (cur_server_list);
  userver_list_free (cur_userver_list);

  if (cur_server) {
    server_unref (cur_server);
    cur_server = NULL;
  }

  host_cache_save ();
  host_cache_clear ();

  debug( 6, "EXIT: Free Master Lists");
  free_masters ();

  debug( 6, "EXIT: Call rcon_done.");
  rcon_done ();
  psearch_done ();
  add_server_done ();
  add_master_done ();
  client_detach_all ();
  free_user_info ();

  dns_helper_shutdown ();

  config_sync ();
  config_drop_all ();
  debug( 6, "EXIT: Done.");


  return 0;
}

