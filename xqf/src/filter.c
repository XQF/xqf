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
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "dialogs.h"
#include "server.h"
#include "config.h"
#include "filter.h"
#include "flt-player.h"
#include "utils.h"

static void server_filter_vars_free(struct server_filter_vars* v);

static int server_pass_filter (struct server *s);
static void server_filter_init (void);

static void server_filter_on_ok();
static void server_filter_on_cancel();
static void server_filter_fill_widgets(guint num);

static void server_filter_save_settings (int number,
      struct server_filter_vars* oldfilter,
      struct server_filter_vars* newfilter);

static void filter_select_callback (GtkWidget *widget, guint number);

void server_filter_print(struct server_filter_vars* f);

// whether the currently displayed filter page has been altered
static gboolean server_filter_changed = FALSE;
static gboolean server_filter_deleted = FALSE;

GArray* server_filters;

static GArray* backup_server_filters; // copy of server_filters, for restoring on cancel

static gboolean cleaned_up = FALSE;

struct filter filters[FILTERS_TOTAL] = {
  {
    N_("Server"),
    N_("S Filter"),
    N_("SF Cfg"),
    server_pass_filter,
    server_filter_init,
    NULL,
    server_filter_on_ok,
    server_filter_on_cancel,
    1,
    FILTER_NOT_CHANGED
  },
  { 
    N_("Player"),
    N_("P Filter"),
    N_("PF Cfg"),
    player_filter,
    player_filter_init,
    player_filter_done,
    player_filter_new_defaults,
    NULL,
    1,
    FILTER_NOT_CHANGED
  }
};


unsigned char cur_filter = 0;

unsigned filter_current_time = 1;

unsigned server_filter_dialog_current_filter = 0;


static  GtkWidget *filter_option_menu;
static  GtkWidget *filter_retries_spinner;
static  GtkWidget *filter_ping_spinner;
static  GtkWidget *filter_not_full_check_button;
static  GtkWidget *filter_not_empty_check_button;
static  GtkWidget *filter_no_cheats_check_button;
static  GtkWidget *filter_no_password_check_button;
static  GtkWidget *game_contains_entry;
static  GtkWidget *filter_game_type_entry;
static  GtkWidget *version_contains_entry;

static struct server_filter_vars* server_filter_vars_new()
{
    struct server_filter_vars* f = g_malloc( sizeof( struct server_filter_vars ));
    if(!f) return NULL;

    f->filter_retries = 2;
    f->filter_ping = 9999;
    f->filter_not_full = 0;
    f->filter_not_empty = 0;
    f->filter_no_cheats = 0;
    f->filter_no_password = 0;
    f->filter_name = NULL;
    f->game_contains = NULL;
    f->version_contains = NULL;
    f->game_type = NULL;

    return f;
}

static void server_filter_vars_free(struct server_filter_vars* v)
{
  if(!v) return;

  g_free(v->filter_name);
  g_free(v->game_contains);
  g_free(v->version_contains);
  g_free(v->game_type);
}

// deep copy of server_filter_vars
static struct server_filter_vars* server_filter_vars_copy(struct server_filter_vars* v)
{
  struct server_filter_vars* f;
 
  if(!v) return NULL;

  f = server_filter_vars_new();
  if(!f) return NULL;

  f->filter_retries     = v->filter_retries;
  f->filter_ping        = v->filter_ping;
  f->filter_not_full    = v->filter_not_full;
  f->filter_not_empty   = v->filter_not_empty;
  f->filter_no_cheats   = v->filter_no_cheats;
  f->filter_no_password = v->filter_no_password;
  f->filter_name        = g_strdup(v->filter_name);
  f->game_contains      = g_strdup(v->game_contains);
  f->version_contains   = g_strdup(v->version_contains);
  f->game_type          = g_strdup(v->game_type);

  return f;
}

void server_filter_print(struct server_filter_vars* f)
{
  printf("Filter: %s\n",f->filter_name);
  printf("  retries: %d\n",f->filter_retries);
  printf("  ping: %d\n",f->filter_ping);
  printf("  not full: %d\n",f->filter_not_full);
  printf("  not empty: %d\n",f->filter_not_empty);
  printf("  no cheats: %d\n",f->filter_no_cheats);
  printf("  no password: %d\n",f->filter_no_password);
  printf("  game: %s\n",f->game_contains);
  printf("  version: %s\n",f->version_contains);
  printf("  game type: %s\n",f->game_type);
}

void apply_filters (unsigned mask, struct server *s) {
  /* This function gets called once per server */

  unsigned flt_time;
  unsigned i;
  int n;

  flt_time = s->flt_last;

  for (n = 0, i = 1; n < FILTERS_TOTAL; n++, i <<= 1) {
    if ((mask & i) == i && (
			    (filters[n].last_changed > s->flt_last) ||
			    ((s->flt_mask & i)  != i)                
			    )
	) {
      
      /* baa --
	 The 'vars' (second param to the func) only matters
	 with the server filter call.  It gets passed to the 
	 player filter call but not used. 
      */
      /* if ((*filters[n].func) (s, vars)) */
      //if ((*filters[n].func) (s, &server_filters[current_server_filter]))
      if ((*filters[n].func) (s))
	s->filters |= i;
      else 
	s->filters &= ~i;

      if (flt_time < filters[n].last_changed)
	flt_time = filters[n].last_changed;
    }
  }


  s->flt_mask |= mask;
  s->flt_last = flt_time;
}


/*
  build_filtered_list -- Return a list of servers that pass the filter
  requirements. Called from server_clist_setlist() and 
  server_clist_build_filtered().
*/

GSList *build_filtered_list (unsigned mask, GSList *server_list) {
  struct server *server;
  GSList *list = NULL;

  while (server_list) {
    server = (struct server *) server_list->data;
    apply_filters (mask | FILTER_PLAYER_MASK, server); /* in filter.c */

    if ((server->filters & mask) == mask) {
      list = g_slist_prepend (list, server);
      debug (6, "build_filtered_list() -- Server %lx added to list", server);
      server_ref (server);
    }

    server_list = server_list->next;
  }

  list = g_slist_reverse (list);
  return list;
}

/* 
  This applies a filter's attributes to a server entry and returns true if it
  passes the filter or false if not.
*/
static int server_pass_filter (struct server *s){
  char **info_ptr;
  struct server_filter_vars* filter;
  
  /* Filter Zero is No Filter */
  if( current_server_filter == 0 ){ return TRUE; }

  filter = g_array_index (server_filters, struct server_filter_vars*, current_server_filter-1);

//  server_filter_print(filter);
    
  if (s->ping == -1)	/* no information */
    return FALSE;

  if(s->retries >= filter->filter_retries)
    return FALSE;

  if(s->ping >= filter->filter_ping)
    return FALSE;

  if(filter->filter_not_full && (s->curplayers >= s->maxplayers))
    return FALSE;

  if(filter->filter_not_empty && (s->curplayers == 0))
    return FALSE;

  if(filter->filter_no_cheats && ((s->flags & SERVER_CHEATS) != 0))
    return FALSE;

  if(filter->filter_no_password && ((s->flags & SERVER_PASSWORD) != 0))
    return FALSE;

  if( filter->game_contains && *filter->game_contains )
  {
    if(!s->game)
      return FALSE;
    else if(!lowcasestrstr(s->game,filter->game_contains))
      return FALSE;
  }

  if( filter->game_type && *filter->game_type )
  {
    if( !s->gametype )
      return FALSE;
    else if(!lowcasestrstr(s->gametype, filter->game_type))
      return FALSE;
  }

  if( filter->version_contains && *filter->version_contains)
  {
    const char* version = NULL;
    /* Filter for the version */
    for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
      if (strcmp (*info_ptr, "version") == 0) {
	version=info_ptr[1];
      }
    }
    if(!version)
      return FALSE;
    else if(!lowcasestrstr( version, filter->version_contains )){
      return FALSE;
    }
  }/*end version check */

  return TRUE;
}

/** initialize server_filters array from config file */
static void server_filter_init (void) {
  int i;
  char config_section[64];
  int isdefault = FALSE; // used to determine whether key exists in config
  char* filtername;

  struct server_filter_vars* filter;
  
    /* This is called before the window has been created so
       we cannot set the filter names in the pulldown yet. */
    
  server_filters = g_array_new( FALSE,FALSE, sizeof(struct server_filter_vars*));
  if(!server_filters) return;

  i = 1;  
  snprintf( config_section, 64, "/" CONFIG_FILE "/Server Filter/%d", i );
  config_push_prefix (config_section );

  filtername = config_get_string_with_default ("filter_name",&isdefault);

  while(!isdefault)
  {
    filter = server_filter_vars_new();
    if(!filter) break;

    filter->filter_name        = filtername;
    filter->filter_retries     = config_get_int  ("retries=2");
    filter->filter_ping        = config_get_int  ("ping=1000");
    filter->filter_not_full    = config_get_bool ("not full=false");
    filter->filter_not_empty   = config_get_bool ("not empty=false");
    filter->filter_no_cheats   = config_get_bool ("no cheats=false");
    filter->filter_no_password = config_get_bool ("no password=false");
    filter->game_contains      = config_get_string("game_contains");
    filter->version_contains   = config_get_string("version_contains");
    filter->game_type          = config_get_string("game_type");

    g_array_append_val(server_filters,filter);
    
    config_pop_prefix ();

    i++;
    snprintf( config_section, 64, "/" CONFIG_FILE "/Server Filter/%d", i );
    config_push_prefix (config_section );
    filtername = config_get_string_with_default ("filter_name",&isdefault);

  }

  config_pop_prefix ();

  debug(3,"number of server filters: %d",server_filters->len);

  sprintf( config_section, "/" CONFIG_FILE "/Server Filter" );
  config_push_prefix (config_section );
  current_server_filter = config_get_int  ("current_server_filter=0");

  if (current_server_filter<0 || current_server_filter>server_filters->len)
    current_server_filter = 0;

  config_pop_prefix ();
}

static void server_filter_on_cancel()
{
  // restore server filter array
  int i;
  struct server_filter_vars* filter;

  for(i=0;i<server_filters->len;i++)
  {
    filter = g_array_index (server_filters, struct server_filter_vars*, i);
    server_filter_vars_free(filter);
  }
  g_array_free(server_filters, FALSE);

  server_filters = backup_server_filters;

  filters[FILTER_SERVER].changed = FILTER_CHANGED;
}

// query widgets and put values in a new struct
static struct server_filter_vars* server_filter_new_from_widgets()
{
  struct server_filter_vars* filter = server_filter_vars_new();
  if(!filter) return NULL;

  filter->filter_retries = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (filter_retries_spinner));
  filter->filter_ping = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (filter_ping_spinner));
  filter->filter_not_full = GTK_TOGGLE_BUTTON (filter_not_full_check_button)->active;
  filter->filter_not_empty = GTK_TOGGLE_BUTTON (filter_not_empty_check_button)->active;
  filter->filter_no_password = GTK_TOGGLE_BUTTON (filter_no_password_check_button)->active;
  filter->filter_no_cheats = GTK_TOGGLE_BUTTON (filter_no_cheats_check_button)->active;
  filter->game_type = gtk_editable_get_chars (GTK_EDITABLE (filter_game_type_entry), 0, -1 );
  filter->version_contains = gtk_editable_get_chars (GTK_EDITABLE (version_contains_entry), 0, -1 );
  filter->game_contains = gtk_editable_get_chars (GTK_EDITABLE (game_contains_entry), 0, -1 );

  return filter;
}

static void server_filter_on_ok ()
{
//  struct server_filter_vars** oldfilter = NULL;
//  struct server_filter_vars* newfilter = NULL;

  int i;
  struct server_filter_vars* filter;

  if( server_filter_dialog_current_filter == 0 && server_filter_deleted == FALSE ){ return; }
/*
  oldfilter = &g_array_index (server_filters, struct server_filter_vars*, server_filter_dialog_current_filter-1);
  if(!oldfilter || !*oldfilter)
  {
    debug(0,"Bug in " __FUNCTION__ ": filter is NULL");
    return;
  }
*/
  filter_select_callback(NULL,server_filter_dialog_current_filter);

/*
  newfilter = server_filter_vars_new();
  if(!newfilter)
    return;

  debug(0,__FUNCTION__);
  filters[FILTER_SERVER].changed = FILTER_CHANGED;

  newfilter = server_filter_new_from_widgets();
  newfilter->filter_name = g_strdup((*oldfilter)->filter_name);

  server_filter_save_settings ( server_filter_dialog_current_filter, *oldfilter, newfilter);

  server_filter_vars_free(*oldfilter);
  *oldfilter = newfilter;

  if(server_filter_deleted == TRUE)*/
  { /* as some filter in the middle could have changed, clear all of them and
       set all new
     */
    guint i;
    struct server_filter_vars* oldfilter = server_filter_vars_new();
    struct server_filter_vars* newfilter = NULL;

    config_clean_section("/" CONFIG_FILE "/Server Filter");

    for(i=0;i<server_filters->len;i++)
    {
      newfilter = g_array_index (server_filters, struct server_filter_vars*, i);
      server_filter_save_settings(i+1,oldfilter,newfilter);
    }

    server_filter_vars_free(oldfilter);
  }

  current_server_filter = server_filter_dialog_current_filter;

#ifdef DEBUG
  {
    int i;
    struct server_filter_vars* filter = NULL;
    for(i=0;i<server_filters->len;i++)
    {
      filter = g_array_index (server_filters, struct server_filter_vars*, i);
      server_filter_print(filter);
    }
  }
#endif

  { // save number of current server filters
    enum { buflen = 64 };
    char config_section[buflen];
    sprintf( config_section, "/" CONFIG_FILE "/Server Filter" );
    config_push_prefix (config_section );
    config_set_int  ("current_server_filter", current_server_filter);
    config_pop_prefix();
  }


  for(i=0;i<backup_server_filters->len;i++)
  {
    filter = g_array_index (backup_server_filters, struct server_filter_vars*, i);
    server_filter_vars_free(filter);
  }
  g_array_free(backup_server_filters,FALSE);
}

/** save settings of current server filter and detect if filter has changed
 */
static void server_filter_save_settings (int number,
      struct server_filter_vars* oldfilter,
      struct server_filter_vars* newfilter)
{
  int text_changed;
  enum { buflen = 64 };
  char config_section[buflen];

  debug(3,__FUNCTION__);

  if( number == 0 ){ return; }

  snprintf( config_section, buflen, "/" CONFIG_FILE "/Server Filter/%d", number );
  config_push_prefix (config_section );

  config_set_string ("filter_name", newfilter->filter_name );


  if (oldfilter->filter_retries != newfilter->filter_retries) {
    config_set_int  ("retries", oldfilter->filter_retries = newfilter->filter_retries);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  if (oldfilter->filter_ping != newfilter->filter_ping) {
    config_set_int  ("ping", oldfilter->filter_ping = newfilter->filter_ping);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  /* GAMECONTAINS string values -- baa */
  text_changed = 0;
  if(newfilter->game_contains && strlen(newfilter->game_contains )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (oldfilter->game_contains){
      if( strcmp(newfilter->game_contains, oldfilter->game_contains )) text_changed = 1;
      g_free( oldfilter->game_contains );
    } else {
      text_changed = 1;
    }
    oldfilter->game_contains = g_strdup(newfilter->game_contains );
    if (text_changed) {
      config_set_string ("game_contains", oldfilter->game_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (oldfilter->game_contains){
      text_changed = 1; /* From something to nothing */
      g_free( oldfilter->game_contains );
      config_set_string ("game_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    oldfilter->game_contains = NULL;
  } 


  /* Version string values -- baa */
  text_changed = 0;
  if( newfilter->version_contains && strlen( newfilter->version_contains )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (oldfilter->version_contains){
      if( strcmp( newfilter->version_contains, oldfilter->version_contains )) text_changed = 1;
      g_free( oldfilter->version_contains );
    } else {
      text_changed = 1;
    }
    oldfilter->version_contains = g_strdup( newfilter->version_contains );
    if (text_changed) {
      config_set_string ("version_contains", oldfilter->version_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (oldfilter->version_contains){
      text_changed = 1; /* From something to nothing */
      g_free( oldfilter->version_contains );
      config_set_string ("version_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    oldfilter->version_contains = NULL;
  } 


  /* GAMETYPE string values -- baa */
  text_changed = 0;
  if( newfilter->game_type && strlen( newfilter->game_type )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (oldfilter->game_type){
      if( strcmp( newfilter->game_type, oldfilter->game_type )) text_changed = 1;
      g_free( oldfilter->game_type );
    } else {
      text_changed = 1;
    }
    oldfilter->game_type = g_strdup( newfilter->game_type );
    if (text_changed) {
      config_set_string ("game_type", oldfilter->game_type );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (oldfilter->game_type){
      text_changed = 1; /* From something to nothing */
      g_free( oldfilter->game_type );
      config_set_string ("game_type", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    oldfilter->game_type = NULL;
  } 
  /* end game_type filter */
  

  if (oldfilter->filter_not_full != newfilter->filter_not_full) {
    config_set_bool ("not full", oldfilter->filter_not_full = newfilter->filter_not_full);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  if (oldfilter->filter_not_empty != newfilter->filter_not_empty) {
    config_set_bool ("not empty", oldfilter->filter_not_empty = newfilter->filter_not_empty);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  if (oldfilter->filter_no_cheats != newfilter->filter_no_cheats) {
    config_set_bool ("no cheats", oldfilter->filter_no_cheats = newfilter->filter_no_cheats);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  if (oldfilter->filter_no_password != newfilter->filter_no_password) {
    config_set_bool ("no password", oldfilter->filter_no_password = newfilter->filter_no_password);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }
#if 0
  /* This from the Gtk FAQ, mostly. */
  if( server_filter_widget[current_server_filter + filter_start_index] && 
      GTK_BIN (server_filter_widget[current_server_filter + filter_start_index])->child)
    {
      GtkWidget *child = GTK_BIN (server_filter_widget[current_server_filter + filter_start_index])->child;
      
      /* do stuff with child */
      if (GTK_IS_LABEL (child))
	{
	  if( filter->filter_name != NULL &&
	      strlen( filter->filter_name )){
	    gtk_label_set (GTK_LABEL (child), filter->filter_name );
	  } else {
	    /* Reuse the config_secion var */
	    sprintf( config_section, "Filter %d", current_server_filter );
	    gtk_label_set (GTK_LABEL (child), config_section );
	  }
	}
    }
#endif
  
  config_pop_prefix ();

  if (filters[FILTER_SERVER].changed == FILTER_CHANGED)
    filters[FILTER_SERVER].last_changed = ++filter_current_time;
}

// store changed widget values
static void filter_select_callback (GtkWidget *widget, guint number)
{
  struct server_filter_vars** filter = NULL;
  char* name;

  if(server_filter_changed && server_filter_dialog_current_filter > 0 )
  {
    /*
    int cont = dialog_yesno(_("Server filter changed"), 1,
	_("Continue"),
	_("Cancel"),
	_("Your changes will be lost if you change server filters now"));
    if(!cont)
    {
      gtk_option_menu_set_history(GTK_OPTION_MENU(filter_option_menu), server_filter_dialog_current_filter-1);
      return FALSE;
    }
    */
    filter = &g_array_index (server_filters,
	struct server_filter_vars*, server_filter_dialog_current_filter-1);
    name = g_strdup((*filter)->filter_name);
    server_filter_vars_free(*filter);
    *filter = server_filter_new_from_widgets();
    (*filter)->filter_name = name;
  }
  server_filter_dialog_current_filter = number;
  server_filter_fill_widgets(number);
  return;
}

static GtkWidget *create_filter_menu()
{
  GtkWidget *menu;
  GtkWidget *menu_item;
  guint i;
  struct server_filter_vars* filter;

  menu = gtk_menu_new ();

//  gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
//                 GTK_SIGNAL_FUNC (callback), (gpointer) 0);

  for (i = 0;i<server_filters->len;i++)
  {
    filter = g_array_index (server_filters, struct server_filter_vars*, i);
    if(!filter)
    {
      debug(0,"Bug in " __FUNCTION__ ": filter is NULL");
      continue;
    }
    menu_item = gtk_menu_item_new_with_label(filter->filter_name?filter->filter_name:"(null)");
    gtk_menu_append (GTK_MENU (menu), menu_item);
    gtk_widget_show (menu_item);

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	   GTK_SIGNAL_FUNC (filter_select_callback), (gpointer)i+1); // array starts from zero but filters from 1
  }

  gtk_widget_show (menu);

  return menu;
}

/** create new filter if number == 0, rename current filter number if number >0
 */
static void filter_new_rename_callback (int number)
{
  char *str = NULL;
  struct server_filter_vars* filter = NULL;

  debug(3,__FUNCTION__ " %s %d",str, number);

  // renaming the none filter is not possible
  if(number && server_filter_dialog_current_filter < 1) return;

  // remember changes
  if(!number)
  {
      filter_select_callback(NULL,server_filter_dialog_current_filter);
  }

  str = enter_string_dialog(TRUE,_("Enter filter name"));

  if(str && *str)
  {
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
    if(!number)
    {
      filter = server_filter_vars_new();
      filter->filter_name = str;
      g_array_append_val(server_filters,filter);
      server_filter_dialog_current_filter = server_filters->len;
    }
    else
    {
      filter = g_array_index (server_filters, struct server_filter_vars*, server_filter_dialog_current_filter-1);
      filter->filter_name = str;
    }

    gtk_option_menu_set_menu (GTK_OPTION_MENU (filter_option_menu), create_filter_menu());

    gtk_option_menu_set_history(GTK_OPTION_MENU(filter_option_menu), server_filter_dialog_current_filter-1);
    server_filter_fill_widgets(server_filter_dialog_current_filter);
    
    server_filter_changed = TRUE;
  }
}

static void filter_delete_callback (void* dummy)
{
  struct server_filter_vars* filter = NULL;
  int cont = 0;
  
  if(server_filter_dialog_current_filter == 0) return;

  filter = g_array_index (server_filters, struct server_filter_vars*, server_filter_dialog_current_filter-1);
  if(!filter) return;

  cont = dialog_yesno(_("Delete server filter"), 1,
      _("Yes"),
      _("No"),
      _("Really delete server filter \"%s\"?"),filter->filter_name);

  if(!cont)
    return;

  filters[FILTER_SERVER].changed = FILTER_CHANGED;
  server_filter_deleted = TRUE;

  server_filter_vars_free(filter);
  g_array_remove_index (server_filters,server_filter_dialog_current_filter-1);
  server_filter_dialog_current_filter = server_filters->len;
  debug(3,"number of filters: %d",server_filter_dialog_current_filter);

  gtk_option_menu_set_menu (GTK_OPTION_MENU (filter_option_menu), create_filter_menu());
  gtk_option_menu_set_history(GTK_OPTION_MENU(filter_option_menu), server_filter_dialog_current_filter-1);
  server_filter_fill_widgets(server_filter_dialog_current_filter);
/*
  for(cont=0;cont<server_filters->len;cont++)
  {
    filter = g_array_index (server_filters, struct server_filter_vars*, cont);
    server_filter_print(filter);
  }
*/
}

/** set all server filter widgets sensitive
 */
static void server_filter_set_widgets_sensitive(gboolean sensitive)
{
  gtk_widget_set_sensitive(filter_game_type_entry,sensitive);
  gtk_widget_set_sensitive(version_contains_entry,sensitive);
  gtk_widget_set_sensitive(game_contains_entry,sensitive);
  gtk_widget_set_sensitive(filter_ping_spinner,sensitive);
  gtk_widget_set_sensitive(filter_retries_spinner,sensitive);
  gtk_widget_set_sensitive(filter_not_full_check_button,sensitive);
  gtk_widget_set_sensitive(filter_not_empty_check_button,sensitive);
  gtk_widget_set_sensitive(filter_not_full_check_button,sensitive);
  gtk_widget_set_sensitive(filter_no_cheats_check_button,sensitive);
  gtk_widget_set_sensitive(filter_no_password_check_button,sensitive);
}

static void server_filter_fill_widgets(guint num)
{
  struct server_filter_vars* filter = NULL;

  gboolean dofree = FALSE;

  if(num > 0)
  {
    filter = g_array_index (server_filters, struct server_filter_vars*, num-1);
    if(!filter)
    {
      debug(0,"Bug in " __FUNCTION__ ": filter is NULL");
      filter = server_filter_vars_new();
      dofree = TRUE;
    }
    server_filter_set_widgets_sensitive(TRUE);
  }
  else
  {
    server_filter_set_widgets_sensitive(FALSE);
    filter = server_filter_vars_new();
    dofree = TRUE;
  }

  gtk_entry_set_text (GTK_ENTRY (filter_game_type_entry), filter->game_type?filter->game_type:"" );
  gtk_entry_set_text (GTK_ENTRY (version_contains_entry), filter->version_contains?filter->version_contains:"" );
  gtk_entry_set_text (GTK_ENTRY (game_contains_entry), filter->game_contains?filter->game_contains:"" );

  gtk_adjustment_set_value(gtk_spin_button_get_adjustment(
	GTK_SPIN_BUTTON(filter_ping_spinner)),filter->filter_ping);
  gtk_adjustment_set_value(gtk_spin_button_get_adjustment(
	GTK_SPIN_BUTTON(filter_retries_spinner)),filter->filter_retries);

  gtk_toggle_button_set_active (
	    GTK_TOGGLE_BUTTON (filter_not_full_check_button), filter->filter_not_full);
  gtk_toggle_button_set_active (
	  GTK_TOGGLE_BUTTON (filter_not_empty_check_button), filter->filter_not_empty);
  gtk_toggle_button_set_active (
	  GTK_TOGGLE_BUTTON (filter_no_cheats_check_button), filter->filter_no_cheats);
  gtk_toggle_button_set_active (
      GTK_TOGGLE_BUTTON (filter_no_password_check_button), filter->filter_no_password);

  server_filter_changed = FALSE;

  if(dofree == TRUE)
    server_filter_vars_free(filter);
}

// set changed flag to status
static void server_filter_set_changed_callback(int status)
{
  server_filter_changed = status;
}

static void server_filter_page (GtkWidget *notebook) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkObject *adj;

//  char label_buf[64];
  int row = 0; /* for auto inserting entry boxes */

  cleaned_up = FALSE;
  
  /* One cannot edit the "None" filter */
  if( current_server_filter < 0 )
  {
    current_server_filter = 0;
    debug(0,__FUNCTION__ ": invalid filter nr %d", server_filter_dialog_current_filter );
  }
  else if ( current_server_filter > server_filters->len )
  {
    current_server_filter = 1;
    debug(0,__FUNCTION__ ": invalid filter nr %d", server_filter_dialog_current_filter );
  }
  
  server_filter_deleted = FALSE;

  server_filter_dialog_current_filter = current_server_filter;

  {// back up server filter array
    int i;
    struct server_filter_vars* filter;
    backup_server_filters = g_array_new( FALSE,FALSE, sizeof(struct server_filter_vars*));
//    g_array_set_size(backup_server_filters,server_filters->len);

    for(i=0;i<server_filters->len;i++)
    {
      filter = server_filter_vars_copy(g_array_index (server_filters, struct server_filter_vars*, i));
      g_array_append_val(backup_server_filters, filter );
    }
  }

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  label = gtk_label_new (_("Server Filter"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_vbox, label);

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
  
  filter_option_menu = gtk_option_menu_new ();
  gtk_box_pack_start (GTK_BOX (hbox), filter_option_menu, FALSE, FALSE, 0);
  gtk_option_menu_set_menu (GTK_OPTION_MENU (filter_option_menu), 
					       create_filter_menu());
  gtk_widget_show(filter_option_menu);
  
  button = gtk_button_new_with_label (_("New"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (filter_new_rename_callback), (gpointer)0);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label (_("Rename"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (filter_new_rename_callback), (gpointer)1);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  button = gtk_button_new_with_label (_("Delete"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (filter_delete_callback), NULL);
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show(button);

  frame = gtk_frame_new (_("Server would pass filter if"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (frame), alignment);

  table = gtk_table_new (6, 5, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (alignment), table);

  /* max ping */

  label = gtk_label_new (_("ping is less than"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL,
                                                                         0, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (MAX_PING, 0.0, MAX_PING, 100.0, 1000.0, 0.0);

  filter_ping_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (filter_ping_spinner), 
                                                            GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (filter_ping_spinner, 64, -1);
  gtk_signal_connect_object (GTK_OBJECT (filter_ping_spinner), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_ping_spinner, 1, 2, 0, 1);
  gtk_widget_show (filter_ping_spinner);

  /* max timeouts */

  label = gtk_label_new (_("the number of retries is fewer than"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 
                                                                         0, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (2, 0.0, MAX_RETRIES, 1.0, 1.0, 0.0);

  filter_retries_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_widget_set_usize (filter_retries_spinner, 64, -1);
  gtk_signal_connect_object (GTK_OBJECT (filter_retries_spinner), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_retries_spinner, 
                                                                   1, 2, 1, 2);
  gtk_widget_show (filter_retries_spinner);

  row++;

  /* GAMECONTAINS Filter -- baa */
  /* http://developer.gnome.org/doc/API/gtk/gtktable.html */
    
  label = gtk_label_new (_("the game contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 
		    0, 0);
  gtk_widget_show (label);
  game_contains_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (game_contains_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (game_contains_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (game_contains_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), game_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (game_contains_entry);
  row++;

  /* GAMETYPE Filter -- baa */
  /* http://developer.gnome.org/doc/API/gtk/gtktable.html */
    
  label = gtk_label_new (_("the game type contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 
		    0, 0);
  gtk_widget_show (label);
  filter_game_type_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (filter_game_type_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (filter_game_type_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (filter_game_type_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), filter_game_type_entry, 4, 5, row, row+1);
  gtk_widget_show (filter_game_type_entry);
  row++;


  /* Version Filter -- baa */
    
  label = gtk_label_new (_("the version contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 
		    0, 0);
  gtk_widget_show (label);
  version_contains_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (version_contains_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (version_contains_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (version_contains_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), version_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (version_contains_entry);


  /* not full */

  filter_not_full_check_button = 
                       gtk_check_button_new_with_label (_("it is not full"));
  gtk_signal_connect_object (GTK_OBJECT (filter_not_full_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_not_full_check_button, 
                                                                   0, 2, 2, 3);
  gtk_widget_show (filter_not_full_check_button);

  /* not empty */

  filter_not_empty_check_button = 
                        gtk_check_button_new_with_label (_("it is not empty"));
  gtk_signal_connect_object (GTK_OBJECT (filter_not_empty_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_not_empty_check_button, 
                                                                   0, 2, 3, 4);
  gtk_widget_show (filter_not_empty_check_button);

  /* no cheats */

  filter_no_cheats_check_button = 
                  gtk_check_button_new_with_label (_("cheats are not allowed"));
  gtk_signal_connect_object (GTK_OBJECT (filter_no_cheats_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_cheats_check_button, 
                                                                   0, 2, 4, 5);
  gtk_widget_show (filter_no_cheats_check_button);

  /* no password */

  filter_no_password_check_button = 
                    gtk_check_button_new_with_label (_("no password required"));
  gtk_signal_connect_object (GTK_OBJECT (filter_no_password_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_password_check_button, 
                                                                   0, 2, 5, 6);
  gtk_widget_show (filter_no_password_check_button);

  gtk_widget_show (table);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
  gtk_widget_show (page_vbox);

  gtk_option_menu_set_history(GTK_OPTION_MENU(filter_option_menu), server_filter_dialog_current_filter-1);
  server_filter_fill_widgets(server_filter_dialog_current_filter);
}

static void filters_on_ok (void)
{
  int i;
  cleaned_up = TRUE;
  for (i = 0; i < FILTERS_TOTAL; i++) {
    if (filters[i].filter_on_ok)
      (*filters[i].filter_on_ok) ();
  }
}

static void filters_on_cancel (void)
{
  int i;

  // ok was pressed 
  if(cleaned_up) return;

  for (i = 0; i < FILTERS_TOTAL; i++) {
    if (filters[i].filter_on_cancel)
      (*filters[i].filter_on_cancel) ();
  }
}

int filters_cfg_dialog (int page_num) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *notebook;
  GtkWidget *button;
  GtkWidget *window;
  int changed = FALSE;
  int i;

#ifdef DEBUG
  const char *flt_status[3] = { "not changed", "changed", "data changed" };
#endif

  window = dialog_create_modal_transient_window (_("XQF: Filters"), 
                                                            TRUE, TRUE, filters_on_cancel);
  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_container_add (GTK_CONTAINER (window), vbox);
         
  /*
   *  Notebook
   */

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (notebook), 4);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  server_filter_page (notebook);

  player_filter_page (notebook);

  gtk_widget_show (notebook);

  /* 
   *  Buttons at the bottom
   */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_set_usize (button, 80, -1);
/*  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (filters_on_cancel), NULL);
*/
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("OK"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (filters_on_ok), NULL);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  gtk_widget_show (window);

  for (i = 0; i < FILTERS_TOTAL; i++)
    filters[i].changed = FILTER_NOT_CHANGED;

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), page_num);

  gtk_main ();

  unregister_window (window);

  player_filter_cfg_clean_up ();

#ifdef DEBUG
  for (i = 0; i < FILTERS_TOTAL; i++) {
    fprintf (stderr, "%s Filter (%d): %s\n", filters[i].name, i,
	                                      flt_status[filters[i].changed]);
  }
#endif

  for (i = 0; i < FILTERS_TOTAL; i++) {
    if (filters[i].changed == FILTER_CHANGED) {
      changed = TRUE;
      break;
    }
  }

  return changed;
}


void filters_init (void) {
  int i;

  for (i = 0; i < FILTERS_TOTAL; i++) {
    if (filters[i].filter_init)
      (*filters[i].filter_init) ();
  }
}


void filters_done (void) {
  int i;

  for (i = 0; i < FILTERS_TOTAL; i++) {
    if (filters[i].filter_done)
      (*filters[i].filter_done) ();
  }
}

