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
#include <stdlib.h>

#include <glib.h>
#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "xqf-ui.h"
#include "dialogs.h"
#include "server.h"
#include "config.h"
#include "filter.h"
#include "flt-player.h"
#include "utils.h"
#include "pref.h"

#ifdef USE_GEOIP
#include "country-filter.h"

static void country_select_button_pressed(GtkWidget * widget, gpointer data);
static void country_add_button(GtkWidget * widget, gpointer data);
static void country_delete_button(GtkWidget * widget, gpointer data);
static void country_clear_list(GtkWidget * widget, gpointer data);
static void country_create_popup_window(void);

// TODO: get rid of global variables
static gint selected_row_left_list=-1;
static gint selected_row_right_list=-1;
static int last_row_right_list = 0;
static int last_row_country_list = 0;
#endif

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

/* QUICK FILTER */

static char* quick_filter_token[8];
static char quick_filter_str[512] = {0};

static int quick_filter (struct server *s);
void filter_quick_set (const char* str);
const char* filter_quick_get(void);
void filter_quick_unset (void);

/* /QUICK FILTER */

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
    FILTER_NOT_CHANGED,
    &filter_pix[0],
    &filter_cfg_pix[0],
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
    FILTER_NOT_CHANGED,
    &filter_pix[0],
    &filter_cfg_pix[0],
  },
  { 
    "not visible",
    "not visible",
    "not visible",
    quick_filter,
    NULL,
    NULL,
    NULL,
    NULL,
    1,
    FILTER_NOT_CHANGED,
    NULL,
    NULL,
  }
};


unsigned char cur_filter = 0;

static unsigned filter_current_time = 1;

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
static  GtkWidget *map_contains_entry;
static  GtkWidget *server_name_contains_entry;
#ifdef USE_GEOIP
static GtkWidget *country_left_list;
static GtkWidget *country_right_list;
static GtkWidget *country_filter_list;
static GtkWidget *scrolledwindow_fcountry;
static GtkWidget *country_selection_button;
static GtkWidget *country_clear_button;
static GtkWidget *country_show_all_check_button;
#endif

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
    f->map_contains = NULL;
    f->server_name_contains=NULL;
#ifdef USE_GEOIP
    f->countries = g_array_new (FALSE, FALSE, sizeof (int));
#endif

    return f;
}

static void server_filter_vars_free(struct server_filter_vars* v)
{
  if(!v) return;

  g_free(v->filter_name);
  g_free(v->game_contains);
  g_free(v->version_contains);
  g_free(v->map_contains);
  g_free(v->server_name_contains);
  g_free(v->game_type);
#ifdef USE_GEOIP
  g_array_free(v->countries,TRUE);
  v->countries=NULL;
#endif
}

// deep copy of server_filter_vars
static struct server_filter_vars* server_filter_vars_copy(struct server_filter_vars* v)
{
  struct server_filter_vars* f;
  unsigned i;
 
  if(!v) return NULL;

  f = server_filter_vars_new();
  if(!f) return NULL;

  f->filter_retries     	= v->filter_retries;
  f->filter_ping        	= v->filter_ping;
  f->filter_not_full    	= v->filter_not_full;
  f->filter_not_empty   	= v->filter_not_empty;
  f->filter_no_cheats   	= v->filter_no_cheats;
  f->filter_no_password 	= v->filter_no_password;
  f->filter_name        	= g_strdup(v->filter_name);
  f->game_contains      	= g_strdup(v->game_contains);
  f->version_contains   	= g_strdup(v->version_contains);
  f->game_type          	= g_strdup(v->game_type);
  f->map_contains       	= g_strdup(v->map_contains);
  f->server_name_contains       = g_strdup(v->server_name_contains);
#ifdef USE_GEOIP

  //FIXME reserve space first, then insert
  for (i =0; i< v->countries->len;i++)
    g_array_append_val (f->countries,g_array_index(v->countries,int,i));
 
#endif

  return f;
}

void server_filter_print(struct server_filter_vars* f)
{
#ifdef USE_GEOIP 
 unsigned i;
#endif

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
  printf("  map: %s\n",f->map_contains);
  printf("  server name: %s\n",f->server_name_contains);
#ifdef USE_GEOIP
  
  for (i =0; i< f->countries->len;i++)
  	printf("country id: %d ",g_array_index(f->countries,int,i));
  printf("\n");
    
#endif
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
  int players = s->curplayers;
  
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

  if(serverlist_countbots && s->curbots <= players)
    players-=s->curbots;

  if(filter->filter_not_full && (players >= s->maxplayers))
    return FALSE;

  if(filter->filter_not_empty && (players == 0))
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

  if( filter->map_contains && *filter->map_contains )
  {
    if( !s->map )
      return FALSE;
    else if(!lowcasestrstr(s->map, filter->map_contains))
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
  
#ifdef USE_GEOIP
  if (filter->countries->len > 0 ) {
    gboolean have_country=FALSE;
    unsigned i;

    if (!s->country_id)
      return FALSE;
    else {  
      for (i = 0; i < filter->countries->len; ++i)
        if (g_array_index(filter->countries,int,i)==s->country_id)
	  have_country=TRUE;
      
      if (!have_country)       
        return FALSE;
    }
  } 

#endif

  if( filter->server_name_contains && *filter->server_name_contains )
  {
    if( !s->name )
      return FALSE;
    else if(!lowcasestrstr(s->name, filter->server_name_contains))
      return FALSE;
  }



  return TRUE;
}

/** initialize server_filters array from config file */
static void server_filter_init (void) {
  int i;
  char config_section[64];
  int isdefault = FALSE; // used to determine whether key exists in config
  char* filtername;

#ifdef USE_GEOIP
  char *str = NULL;
#endif


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

    filter->filter_name        		= filtername;
    filter->filter_retries    		= config_get_int  ("retries=2");
    filter->filter_ping       		= config_get_int  ("ping=9999");
    filter->filter_not_full   		= config_get_bool ("not full=false");
    filter->filter_not_empty   		= config_get_bool ("not empty=false");
    filter->filter_no_cheats   		= config_get_bool ("no cheats=false");
    filter->filter_no_password 		= config_get_bool ("no password=false");
    filter->game_contains      		= config_get_string("game_contains");
    filter->version_contains   		= config_get_string("version_contains");
    filter->game_type          		= config_get_string("game_type");
    filter->map_contains       		= config_get_string("map_contains");
    filter->server_name_contains	= config_get_string("server_name_contains");
#ifdef USE_GEOIP

    /*country filter ids*/
    str = config_get_string("server_country_contains");
    if(str)
    {
      int nr;
      gchar **buf = NULL;
      buf = g_strsplit(str," ",0);

      for(nr = 0; buf && buf[nr]; ++nr)
      {
	int flag_nr = geoip_id_by_code(buf[nr]);
	if(flag_nr > 0)
	  g_array_append_val (filter->countries,flag_nr);
      }

      g_strfreev(buf);
    }
      
#endif
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
  unsigned i;
  struct server_filter_vars* filter;

  for(i=0;i<server_filters->len;++i)
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
#ifdef USE_GEOIP
 int country_nr;
 int i;
#endif

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
  filter->map_contains = gtk_editable_get_chars (GTK_EDITABLE (map_contains_entry), 0, -1 );
  filter->server_name_contains = gtk_editable_get_chars (GTK_EDITABLE (server_name_contains_entry), 0, -1 );

#ifdef USE_GEOIP

  for (i = 0; i < last_row_country_list; ++i) {
    country_nr=GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(country_filter_list), i));
    g_array_append_val(filter->countries,country_nr);
  }
  
#endif

  return filter;
}

static void server_filter_on_ok ()
{
//  struct server_filter_vars** oldfilter = NULL;
//  struct server_filter_vars* newfilter = NULL;

  unsigned i;
  struct server_filter_vars* filter;

  if( server_filter_dialog_current_filter == 0 && server_filter_deleted == FALSE ){ return; }
/*
  oldfilter = &g_array_index (server_filters, struct server_filter_vars*, server_filter_dialog_current_filter-1);
  if(!oldfilter || !*oldfilter)
  {
    debug(0,"Bug: filter is NULL");
    return;
  }
*/
  filter_select_callback(NULL,server_filter_dialog_current_filter);

/*
  newfilter = server_filter_vars_new();
  if(!newfilter)
    return;

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
    unsigned i;
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
 * oldfilter is currently dummy
 */
static void server_filter_save_settings (int number,
      struct server_filter_vars* oldfilter,
      struct server_filter_vars* newfilter)
{
  int text_changed;
  enum { buflen = 64 };
  char config_section[buflen];

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
  
  
  /* map string values */
  text_changed = 0;
  if( newfilter->map_contains && strlen( newfilter->map_contains )){
    /*
      First case, the user entered something.  See if the value
      is different
    */
    if (oldfilter->map_contains){
      if( strcmp( newfilter->map_contains, oldfilter->map_contains )) text_changed = 1;
      g_free( oldfilter->map_contains);
    } else {
      text_changed = 1;
    }
    oldfilter->map_contains = g_strdup( newfilter->map_contains );
    if (text_changed) {
      config_set_string ("map_contains", oldfilter->map_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (oldfilter->map_contains){
      text_changed = 1; /* From something to nothing */
      g_free( oldfilter->map_contains );
      config_set_string ("map_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    oldfilter->map_contains= NULL;
  }  /* end of map filter */
 
  /* servername string values */
  text_changed = 0;
  if( newfilter->server_name_contains && strlen( newfilter->server_name_contains )){
    /*
      First case, the user entered something.  See if the value
      is different
    */
    if (oldfilter->server_name_contains){
      if( strcmp( newfilter->server_name_contains, oldfilter->server_name_contains )) text_changed = 1;
      g_free( oldfilter->server_name_contains);
    } else {
      text_changed = 1;
    }
    oldfilter->server_name_contains = g_strdup( newfilter->server_name_contains );
    if (text_changed) {
      config_set_string ("server_name_contains", oldfilter->server_name_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (oldfilter->server_name_contains){
      text_changed = 1; /* From something to nothing */
      g_free( oldfilter->server_name_contains );
      config_set_string ("server_name_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    oldfilter->server_name_contains = NULL;
  }  /* end of server filter */

#ifdef USE_GEOIP

  /* country string values */

  if (newfilter->countries->len > 0)
  {
    unsigned i;
    char* buf = NULL;
    buf = g_new(char,newfilter->countries->len*3);

    for (i = 0; i < newfilter->countries->len; ++i)
    {
      const char* code = geoip_code_by_id(g_array_index(newfilter->countries,int,i));
      if(strlen(code)!=2) code = "  "; // may not happen
      buf[i*3]=code[0];
      buf[i*3+1]=code[1];
      buf[i*3+2]=' ';
    }
    buf[i*3-1]='\0';
	      
    config_set_string ("server_country_contains", buf);

    g_free(buf);

    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

#endif


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
    filters[FILTER_SERVER].last_changed = filter_time_inc();
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
      debug(0,"Bug: filter is NULL");
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

  debug(3,"%s %d",str, number);

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
      gtk_widget_grab_focus (GTK_WIDGET (game_contains_entry));
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
  gtk_widget_set_sensitive(map_contains_entry,sensitive);
  gtk_widget_set_sensitive(server_name_contains_entry,sensitive);
  gtk_widget_set_sensitive(filter_ping_spinner,sensitive);
  gtk_widget_set_sensitive(filter_retries_spinner,sensitive);
  gtk_widget_set_sensitive(filter_not_full_check_button,sensitive);
  gtk_widget_set_sensitive(filter_not_empty_check_button,sensitive);
  gtk_widget_set_sensitive(filter_not_full_check_button,sensitive);
  gtk_widget_set_sensitive(filter_no_cheats_check_button,sensitive);
  gtk_widget_set_sensitive(filter_no_password_check_button,sensitive);
#ifdef USE_GEOIP
  if (geoip_is_working()) {
    gtk_widget_set_sensitive(country_filter_list, sensitive);
    gtk_widget_set_sensitive(scrolledwindow_fcountry, sensitive);
    gtk_widget_set_sensitive(country_selection_button, sensitive);
    gtk_widget_set_sensitive(country_clear_button, sensitive);
    if (!sensitive) {
      gtk_clist_clear(GTK_CLIST(country_filter_list));
    }
  }
#endif
}

static void server_filter_fill_widgets(guint num)
{
  struct server_filter_vars* filter = NULL;

  gboolean dofree = FALSE;
#ifdef USE_GEOIP
  char buf[3];
  int f_number;
  int rw = 0;
  
  struct pixmap* countrypix = NULL;
  buf[2]='\0';
#endif

  if(num > 0)
  {
    filter = g_array_index (server_filters, struct server_filter_vars*, num-1);
    if(!filter)
    {
      debug(0,"Bug: filter is NULL");
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
  gtk_entry_set_text (GTK_ENTRY (map_contains_entry), filter->map_contains?filter->map_contains:"" );
  gtk_entry_set_text (GTK_ENTRY (server_name_contains_entry), filter->server_name_contains?filter->server_name_contains:"" );
#ifdef USE_GEOIP
  gtk_clist_clear(GTK_CLIST(country_filter_list));
 
  last_row_country_list=0;
  
  /*fill the country_filter_list from filter->countries*/
  if (geoip_is_working()) {
    unsigned i;
    gchar buf[64] = {0};
    gchar *text[1] = {buf};
  
    if (filter->countries != NULL) {
    
      for (i = 0; i < filter->countries->len; ++i) {
    
        f_number=g_array_index(filter->countries,int,i);

	// gtk_clist_insert third parameter is not const!
	strncpy(buf,geoip_name_by_id(f_number),sizeof(buf));
	gtk_clist_insert(GTK_CLIST(country_filter_list), rw, text);
        countrypix = get_pixmap_for_country(f_number);
	if(countrypix)
	{
	  gtk_clist_set_pixtext(GTK_CLIST(country_filter_list), rw, 0,geoip_name_by_id(f_number), 4,
	      countrypix->pix, countrypix->mask);
	}
        gtk_clist_set_row_data(GTK_CLIST(country_filter_list),rw,GINT_TO_POINTER(f_number));

	last_row_country_list++;
        ++rw;
      }
    } 
  }
  
#endif

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

#ifdef USE_GEOIP
  GtkWidget *vbuttonbox1;
#endif
  

//  char label_buf[64];
  int row = 0; /* for auto inserting entry boxes */

  cleaned_up = FALSE;
  
  /* One cannot edit the "None" filter */
  if( current_server_filter < 0 )
  {
    current_server_filter = 0;
    debug(0,"invalid filter nr %d", server_filter_dialog_current_filter );
  }
  else if ( current_server_filter > server_filters->len )
  {
    current_server_filter = 1;
    debug(0,"invalid filter nr %d", server_filter_dialog_current_filter );
  }
  
  server_filter_deleted = FALSE;

  server_filter_dialog_current_filter = current_server_filter;

  {// back up server filter array
    unsigned i;
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
  gtk_option_menu_set_menu (GTK_OPTION_MENU (filter_option_menu), create_filter_menu());
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

  #ifdef USE_GEOIP
    table = gtk_table_new (8, 5, FALSE);
  #else
    table = gtk_table_new (6, 5, FALSE);
  #endif

  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (alignment), table);

  /*row=0..1 */

  /* max ping */
  label = gtk_label_new (_("ping is less than"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL,
                                                                         0, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (MAX_PING, 0.0, MAX_PING, 100.0, 1000.0, 0.0);

  filter_ping_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (filter_ping_spinner), 
                                                            GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (filter_ping_spinner, 64, -1);
  gtk_signal_connect_object (GTK_OBJECT (filter_ping_spinner), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_ping_spinner, 1, 2, row, row+1);
  gtk_widget_show (filter_ping_spinner);


  /* GAMECONTAINS Filter -- baa */
  /* http://developer.gnome.org/doc/API/gtk/gtktable.html */
    
  label = gtk_label_new (_("the game contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  game_contains_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (game_contains_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (game_contains_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (game_contains_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), game_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (game_contains_entry);
  row++;


  /*row=1..2*/

  /* max timeouts */
  label = gtk_label_new (_("the number of retries is fewer than"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, row, row+1, GTK_FILL, GTK_FILL, 
                                                                         0, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (2, 0.0, MAX_RETRIES, 1.0, 1.0, 0.0);

  filter_retries_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_widget_set_usize (filter_retries_spinner, 64, -1);
  gtk_signal_connect_object (GTK_OBJECT (filter_retries_spinner), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_retries_spinner, 
                                                                   1, 2, row, row+1);
  gtk_widget_show (filter_retries_spinner);

  /* GAMETYPE Filter -- baa */
  /* http://developer.gnome.org/doc/API/gtk/gtktable.html */
    
  label = gtk_label_new (_("the game type contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  filter_game_type_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (filter_game_type_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (filter_game_type_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (filter_game_type_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), filter_game_type_entry, 4, 5, row, row+1);
  gtk_widget_show (filter_game_type_entry);
  row++;

  /*row=2..3*/

  /*not full */
  filter_not_full_check_button =gtk_check_button_new_with_label(_("it is not full"));
  gtk_signal_connect_object(GTK_OBJECT(filter_not_full_check_button),"toggled",
    GTK_SIGNAL_FUNC(server_filter_set_changed_callback),(gpointer) TRUE);
  gtk_table_attach_defaults(GTK_TABLE(table),filter_not_full_check_button, 0, 2, row, row+1);
  gtk_widget_show(filter_not_full_check_button);

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
  row++;
 
   /*row=3..4*/
 
  /* not empty */
  filter_not_empty_check_button = 
                        gtk_check_button_new_with_label (_("it is not empty"));
  gtk_signal_connect_object (GTK_OBJECT (filter_not_empty_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_not_empty_check_button, 
                                                                   0, 2, row, row+1);
  gtk_widget_show (filter_not_empty_check_button);


  /* Map filter*/
  label = gtk_label_new (_("the map contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  map_contains_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (map_contains_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (map_contains_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (map_contains_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table), map_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (map_contains_entry);
  row++;

  /*row=4..5*/

  /* no cheats */
  filter_no_cheats_check_button = 
                  gtk_check_button_new_with_label (_("cheats are not allowed"));
  gtk_signal_connect_object (GTK_OBJECT (filter_no_cheats_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_cheats_check_button, 
                                                                   0, 2, row, row+1);
  gtk_widget_show (filter_no_cheats_check_button);


  /* Server name filter*/
  label = gtk_label_new (_("the server name contains the string"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  server_name_contains_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (server_name_contains_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (server_name_contains_entry), TRUE);
  gtk_signal_connect_object (GTK_OBJECT (server_name_contains_entry), "changed",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);

  gtk_table_attach_defaults (GTK_TABLE (table),server_name_contains_entry , 4, 5, row, row+1);
  gtk_widget_show (server_name_contains_entry);
  row++;

  /*row=5..6*/

  /* no password */
  filter_no_password_check_button = 
                    gtk_check_button_new_with_label (_("no password required"));
  gtk_signal_connect_object (GTK_OBJECT (filter_no_password_check_button), "toggled",
                 GTK_SIGNAL_FUNC (server_filter_set_changed_callback), (gpointer) TRUE);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_password_check_button, 
                                                                   0, 2, row, row+1);
  gtk_widget_show (filter_no_password_check_button);

  row++;

   /*country list */
#ifdef USE_GEOIP 
  label = gtk_label_new(_("Country filter:"));
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 6, 7);
  gtk_misc_set_padding(GTK_MISC(label), 0, 15);
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_widget_show(label);

  scrolledwindow_fcountry = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_sensitive(scrolledwindow_fcountry,FALSE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW
         (scrolledwindow_fcountry),
         GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  country_filter_list = gtk_clist_new(1);
  gtk_clist_set_shadow_type(GTK_CLIST(country_filter_list),GTK_SHADOW_NONE);
  gtk_widget_set_sensitive(country_filter_list,FALSE);
  
  
  gtk_clist_set_column_justification(GTK_CLIST(country_filter_list), 0,
             GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_width(GTK_CLIST(country_filter_list), 0, 100);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
          (scrolledwindow_fcountry),
          country_filter_list);

  gtk_widget_set_usize(scrolledwindow_fcountry, 100, 100);

  gtk_table_attach_defaults(GTK_TABLE(table), scrolledwindow_fcountry, 0,
          1, 7, 8);
  gtk_widget_show(scrolledwindow_fcountry);
  gtk_widget_show(country_filter_list);

  /*select and clear buttons */
  vbuttonbox1 = gtk_vbutton_box_new();
  gtk_widget_show(vbuttonbox1);
  gtk_table_attach_defaults(GTK_TABLE(table), vbuttonbox1, 1, 2, 7, 8);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox1),
          GTK_BUTTONBOX_START);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(vbuttonbox1), 1);
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(vbuttonbox1), 80, 0);
  gtk_button_box_set_child_ipadding(GTK_BUTTON_BOX(vbuttonbox1), 5, -1);

  country_selection_button = gtk_button_new_with_label(_("select..."));
  gtk_widget_set_sensitive(country_selection_button,FALSE);
  gtk_widget_show(country_selection_button);
  gtk_container_add(GTK_CONTAINER(vbuttonbox1),
        country_selection_button);
  gtk_widget_set_usize(country_selection_button, 80, -1);
  gtk_signal_connect(GTK_OBJECT(country_selection_button), "clicked",
         GTK_SIGNAL_FUNC(country_select_button_pressed), NULL);
  
  gtk_signal_connect_object(GTK_OBJECT(country_selection_button), "clicked",
          GTK_SIGNAL_FUNC(server_filter_set_changed_callback),(gpointer) TRUE);
          
  
  country_clear_button = gtk_button_new_with_label(_("clear"));
  gtk_widget_set_sensitive(country_clear_button,FALSE);
  gtk_signal_connect(GTK_OBJECT(country_clear_button), "clicked",
         GTK_SIGNAL_FUNC(country_clear_list), NULL);
  gtk_signal_connect_object(GTK_OBJECT(country_clear_button), "clicked",
          GTK_SIGNAL_FUNC(server_filter_set_changed_callback),(gpointer) TRUE);
          
  
  gtk_widget_show(country_clear_button);
  gtk_container_add(GTK_CONTAINER(vbuttonbox1), country_clear_button);
  gtk_widget_set_usize(country_clear_button, 80, -1);
#endif

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

#ifdef USE_GEOIP
/*
*
*       country filter stuff
*
*/

/*callback: row selection left list*/
void country_selection_left_list(GtkWidget * list,
          gint row_select,
          gint column,
          GdkEventButton * event, gpointer data)
{
  selected_row_left_list = row_select;
  return;
}

/*callback: row selection right list*/
void country_selection_right_list(GtkWidget * list,
          gint row_select,
          gint column,
          GdkEventButton * event, gpointer data)
{
  selected_row_right_list = row_select;
  return;
}

/*callback: no row is selected*/
void country_unselection_right_list(GtkWidget * list,
            gint row_select,
            gint column,
            GdkEventButton * event, gpointer data)
{
  selected_row_right_list = -1;
  return;
}

/*show the country selection popup window*/
static void country_select_button_pressed(GtkWidget * widget, gpointer data)
{
  country_create_popup_window();
}

/*callback: clear button*/
static void country_clear_list(GtkWidget * widget, gpointer data)
{
  gtk_clist_clear(GTK_CLIST(country_filter_list));
  last_row_country_list=0;
}

/** add the country selected in the left list to the right list */
static void country_add_selection_to_right_list()
{
  gint i;
  gint flag_id;
  gchar buf[64] = {0};
  gchar *text[1] = {buf};
  struct pixmap* countrypix = NULL;

  if (selected_row_left_list < 0)
    return;
  
  flag_id = GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(country_left_list),
	selected_row_left_list));
  
  /* do nothing if country is already in right list */
  for (i = 0; i < last_row_right_list; i++)
    if (flag_id == GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(country_right_list), i)))
      return;
  
  countrypix = get_pixmap_for_country(flag_id);

  // gtk_clist_insert third parameter is not const!
  strncpy(buf,geoip_name_by_id(flag_id),sizeof(buf));
  gtk_clist_append(GTK_CLIST(country_right_list), text);
  if(countrypix)
  {
    gtk_clist_set_pixtext(GTK_CLIST(country_right_list), last_row_right_list, 0,
	geoip_name_by_id(flag_id), 4, countrypix->pix, countrypix->mask);
  }

  gtk_clist_set_row_data(GTK_CLIST(country_right_list), last_row_right_list,
      GINT_TO_POINTER(flag_id));

  ++last_row_right_list;
}

/*callback: >> button*/
static void country_add_button(GtkWidget * widget, gpointer data)
{
  country_add_selection_to_right_list();
}

/*callback: << button*/
static void country_delete_button(GtkWidget * widget, gpointer data)
{

  if ((selected_row_right_list != -1) && (last_row_right_list > 0)) {
    gtk_clist_remove(GTK_CLIST(country_right_list), selected_row_right_list);
    last_row_right_list--;
  }
}


/** callback: double click on row */
gint country_mouse_click_left_list(GtkWidget * widget,
          GdkEventButton * event,
          gpointer func_data)
{
  if ((event->type == GDK_2BUTTON_PRESS) && (event->button==1))
  {
    country_add_selection_to_right_list();
  }

  return FALSE;
}

/*callback: double click on row*/
gint country_mouse_click_right_list(GtkWidget * widget,
          GdkEventButton * event,
          gpointer func_data)
{

  if ((event->type == GDK_2BUTTON_PRESS) && (event->button==1))
  {
    country_delete_button(NULL,NULL);
  }

  return FALSE;
}

/*callback: ready with country selection*/
static void country_selection_on_ok(void)
{
  int i;
  gint country_nr;
  gchar buf[64] = {0};
  gchar *text[1] = {buf};

  struct pixmap* countrypix = NULL;
  
  selected_row_right_list=-1;
  gtk_clist_freeze(GTK_CLIST(country_filter_list));
  gtk_clist_clear(GTK_CLIST(country_filter_list));
  
  for (i = 0; i < last_row_right_list; ++i)
  {
    country_nr = GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(country_right_list), i));
  
    countrypix = get_pixmap_for_country(country_nr);
  
    // gtk_clist_insert third parameter is not const!
    strncpy(buf,geoip_name_by_id(country_nr),sizeof(buf));
    gtk_clist_insert(GTK_CLIST(country_filter_list), i, text);
    if(countrypix)
    {
      gtk_clist_set_pixtext(GTK_CLIST(country_filter_list), i, 0,
            text[0], 4,countrypix->pix,countrypix->mask);
    }
    gtk_clist_set_row_data(GTK_CLIST(country_filter_list),i,GINT_TO_POINTER(country_nr));
    
  }
  gtk_clist_thaw(GTK_CLIST(country_filter_list));
  last_row_country_list=last_row_right_list;
}

static void country_selection_on_cancel(void)
{
  selected_row_right_list=-1;
}

/** populate a clist with country names&flags
 * @param clist the clist
 * @param all if false only show countries that have a flag
 */
static void populate_country_clist(GtkWidget* clist, gboolean all)
{
    struct pixmap* countrypix = NULL;
    int row_number = -1;
    unsigned i;
    gchar buf[64] = {0};
    gchar *text[1] = {buf};

    g_return_if_fail(GTK_IS_CLIST(clist));

    gtk_clist_freeze(GTK_CLIST(clist));

    gtk_clist_clear(GTK_CLIST(clist));

    for (i = 0; i <= MaxCountries; ++i)
    {
	countrypix = get_pixmap_for_country(i);

	if ( !all && !countrypix )
	    continue;

	++row_number;

	// gtk_clist_insert third parameter is not const!
	strncpy(buf,geoip_name_by_id(i),sizeof(buf));
	gtk_clist_insert(GTK_CLIST(clist), row_number, text);
	if(countrypix)
	{
	    gtk_clist_set_pixtext(GTK_CLIST(clist), row_number, 0,
		geoip_name_by_id(i), 4,
		countrypix->pix,
		countrypix->mask);
	}

	/*save the flag number*/
	gtk_clist_set_row_data(GTK_CLIST(clist),
		row_number, GINT_TO_POINTER(i));
    }

    gtk_clist_thaw(GTK_CLIST(clist));
}

static void country_show_all_changed_callback (GtkWidget *widget, GtkWidget *clist)
{
  populate_country_clist(clist,gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*country selection window*/
static void country_create_popup_window(void)
{
  GtkWidget *country_popup_window;
  GtkWidget *vbox1;
  GtkWidget *vbox2;
  GtkWidget *frame1;
  GtkWidget *hbox1;
  GtkWidget *scrolledwindow1;


  GtkWidget *vbuttonbox1;
  GtkWidget *button3;
  GtkWidget *button4;
  GtkWidget *scrolledwindow2;


  GtkWidget *hbuttonbox1;
  GtkWidget *button1;
  GtkWidget *button2;

  int i;
  int flag_nr;
  gchar buf[64] = {0};
  gchar *text[1] = {buf};
  
  struct pixmap* countrypix = NULL;

  /* window caption for country filter */
  country_popup_window = dialog_create_modal_transient_window(_("Configure Country Filter"),
             TRUE, TRUE,
         country_selection_on_cancel);
  gtk_widget_set_usize(GTK_WIDGET(country_popup_window), 480, 320);

  vbox1 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox1);
  gtk_container_add(GTK_CONTAINER(country_popup_window), vbox1);

  frame1 = gtk_frame_new(_("Country filter:"));
  gtk_widget_show(frame1);
  gtk_box_pack_start(GTK_BOX(vbox1), frame1, TRUE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(frame1), 4);

  vbox2 = gtk_vbox_new(FALSE, 0);
  gtk_widget_show(vbox2);
  gtk_container_add(GTK_CONTAINER(frame1), vbox2);
  gtk_container_set_border_width(GTK_CONTAINER(frame1), 4);


  hbox1 = gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox1);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox1, TRUE, TRUE, 0);

  /* left clist */
  scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow1), 8);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1),
         GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
  country_left_list = gtk_clist_new(1);
  gtk_clist_set_shadow_type(GTK_CLIST(country_left_list),GTK_SHADOW_NONE);
  gtk_clist_set_selection_mode(GTK_CLIST(country_left_list), GTK_SELECTION_SINGLE);
  gtk_clist_set_column_justification(GTK_CLIST(country_left_list), 0,
             GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_width(GTK_CLIST(country_left_list), 0, 100);
  

  gtk_signal_connect(GTK_OBJECT(country_left_list), "select_row",
         GTK_SIGNAL_FUNC(country_selection_left_list),
         NULL);


  gtk_signal_connect(GTK_OBJECT(country_left_list), "button_press_event",
         GTK_SIGNAL_FUNC(country_mouse_click_left_list),
         NULL);
  
  /* fill the list with all countries if the flag is available*/

  populate_country_clist(country_left_list, FALSE);

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolledwindow1),
	  GTK_WIDGET(country_left_list));
  gtk_box_pack_start(GTK_BOX(hbox1), scrolledwindow1, TRUE, TRUE, 0);
  gtk_widget_show(scrolledwindow1);
  gtk_widget_show(country_left_list);

  /* >> and << buttons */

  /*>> */
  vbuttonbox1 = gtk_vbutton_box_new();
  gtk_widget_show(vbuttonbox1);
  gtk_box_pack_start(GTK_BOX(hbox1), vbuttonbox1, FALSE, TRUE, 0);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(vbuttonbox1),
          GTK_BUTTONBOX_SPREAD);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(vbuttonbox1), 0);
//  gtk_button_box_set_child_size(GTK_BUTTON_BOX(vbuttonbox1), 63, -1);

  button3 = gtk_button_new_with_label(">>");
  gtk_signal_connect(GTK_OBJECT(button3), "clicked",
         GTK_SIGNAL_FUNC(country_add_button), NULL);
  gtk_widget_show(button3);
  gtk_container_add(GTK_CONTAINER(vbuttonbox1), button3);
  GTK_WIDGET_SET_FLAGS(button3, GTK_CAN_DEFAULT);


  /*<< */
  button4 = gtk_button_new_with_label("<<");
  gtk_signal_connect(GTK_OBJECT(button4), "clicked",
         GTK_SIGNAL_FUNC(country_delete_button), NULL);
  gtk_widget_show(button4);
  gtk_container_add(GTK_CONTAINER(vbuttonbox1), button4);
  GTK_WIDGET_SET_FLAGS(button4, GTK_CAN_DEFAULT);


  /*right clist */
  scrolledwindow2 = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_set_border_width(GTK_CONTAINER(scrolledwindow2), 8);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow2),
         GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  country_right_list = gtk_clist_new(1);
  gtk_clist_set_shadow_type(GTK_CLIST(country_right_list),GTK_SHADOW_NONE);
  gtk_clist_set_selection_mode(GTK_CLIST(country_right_list), GTK_SELECTION_SINGLE);

  gtk_clist_set_column_justification(GTK_CLIST(country_right_list), 0,
             GTK_JUSTIFY_LEFT);
  gtk_clist_set_column_width(GTK_CLIST(country_right_list), 0, 100);
  
  
  gtk_signal_connect(GTK_OBJECT(country_right_list), "select_row",
         GTK_SIGNAL_FUNC(country_selection_right_list),
         NULL);
         
  gtk_signal_connect(GTK_OBJECT(country_right_list), "unselect_row",
         GTK_SIGNAL_FUNC(country_unselection_right_list),
         NULL);
  
  gtk_signal_connect(GTK_OBJECT(country_right_list), "button_press_event",
         GTK_SIGNAL_FUNC(country_mouse_click_right_list),
         NULL);

    
  /*fill the clist with the same countries as in country_filter_list */
   
  for (i = 0; i<last_row_country_list;i++) {
  
    flag_nr=GPOINTER_TO_INT(gtk_clist_get_row_data(GTK_CLIST(country_filter_list), i));
   
    countrypix = get_pixmap_for_country(flag_nr);
  
    // gtk_clist_insert third parameter is not const!
    strncpy(buf,geoip_name_by_id(flag_nr),sizeof(buf));
    gtk_clist_insert(GTK_CLIST(country_right_list),i, text);
    if(countrypix)
    {
      gtk_clist_set_pixtext(GTK_CLIST(country_right_list),i, 0,
          geoip_name_by_id(flag_nr), 4,
          countrypix->pix,countrypix->mask);
    }
    
    gtk_clist_set_row_data(GTK_CLIST(country_right_list),i,GINT_TO_POINTER(flag_nr));
      
  }

  last_row_right_list=last_row_country_list;
  

  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW
          (scrolledwindow2), country_right_list);
  gtk_box_pack_start(GTK_BOX(hbox1), scrolledwindow2, TRUE, TRUE, 0);
  gtk_widget_show(scrolledwindow2);
  gtk_widget_show(country_right_list);

  country_show_all_check_button = gtk_check_button_new_with_label(_("Show all countries"));
  gtk_signal_connect(GTK_OBJECT(country_show_all_check_button),"toggled",
	GTK_SIGNAL_FUNC(country_show_all_changed_callback), (gpointer)country_left_list);
  gtk_widget_show(country_show_all_check_button);
  gtk_box_pack_start(GTK_BOX(vbox2), country_show_all_check_button, FALSE, FALSE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(country_show_all_check_button), 4);

  /*OK and Cancel buttons */
  hbuttonbox1 = gtk_hbutton_box_new();


  gtk_widget_show(hbuttonbox1);
  gtk_box_pack_start(GTK_BOX(vbox1), hbuttonbox1, FALSE, TRUE, 0);
  gtk_container_set_border_width(GTK_CONTAINER(hbuttonbox1), 4);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox1),
          GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(hbuttonbox1), 1);
//  gtk_button_box_set_child_size(GTK_BUTTON_BOX(hbuttonbox1), 79, 38);

  button1 = gtk_button_new_with_label(_("OK"));
//  gtk_widget_set_usize(button1, 80, -1);
  gtk_signal_connect(GTK_OBJECT(button1), "clicked",
         GTK_SIGNAL_FUNC(country_selection_on_ok), NULL);
  gtk_signal_connect_object(GTK_OBJECT(button1), "clicked",
          GTK_SIGNAL_FUNC(gtk_widget_destroy),
          GTK_OBJECT(country_popup_window));

  gtk_widget_show(button1);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1), button1);
  GTK_WIDGET_SET_FLAGS(button1, GTK_CAN_DEFAULT);

  button2 = gtk_button_new_with_label(_("Cancel"));
//  gtk_widget_set_usize(button2, 80, -1);
  gtk_signal_connect_object(GTK_OBJECT(button2), "clicked",
          GTK_SIGNAL_FUNC(gtk_widget_destroy),
          GTK_OBJECT(country_popup_window));

  gtk_widget_show(button2);
  gtk_container_add(GTK_CONTAINER(hbuttonbox1), button2);
  GTK_WIDGET_SET_FLAGS(button2, GTK_CAN_DEFAULT);

  gtk_widget_show(country_popup_window);

  gtk_main();

  unregister_window(country_popup_window);

}
#endif

unsigned filter_time_inc()
{
  ++filter_current_time;
  if(!filter_current_time)
  {
    struct server* s;
    printf("CONGRATULATION! You managed to filter more than %u times\n", UINT_MAX);
    GSList* list = all_servers();
    for(;list; list = list->next)
    {
      s = (struct server *) list->data;
      s->flt_last = 0;
    }
    ++filter_current_time;
  }
  return filter_current_time;
}

static int quick_filter (struct server *s)
{
  unsigned i;
  size_t max = sizeof(quick_filter_token)/sizeof(quick_filter_token[0]);

  if(!s || !*quick_filter_str) return TRUE;

  for(i = 0; i < max && quick_filter_token[i]; ++i)
  {
    if(s->map && strstr(s->map, quick_filter_token[i]))
      continue;

    if(s->game && lowcasestrstr(s->game, quick_filter_token[i]))
      continue;

    if(s->gametype && lowcasestrstr(s->gametype, quick_filter_token[i]))
      continue;

    if(s->name && lowcasestrstr(s->name, quick_filter_token[i]))
      continue;

    if(s->host && s->host->name && lowcasestrstr(s->host->name, quick_filter_token[i]))
      continue;

    {
      gboolean match = FALSE;
      char **info_ptr;
      for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2)
      {
	if(lowcasestrstr(info_ptr[1], quick_filter_token[i]))
	{
	  match = TRUE;
	  break;
	}
      }
      if(!match)
	return FALSE;
    }
  }

  return TRUE;
}

void filter_quick_set (const char* str)
{
  if(str)
  {
    unsigned num;
    size_t max = sizeof(quick_filter_token)/sizeof(quick_filter_token[0]);
    strncpy(quick_filter_str, str, sizeof(quick_filter_str));
    num = tokenize(quick_filter_str, quick_filter_token, max, " ");
    if(num < max)
      quick_filter_token[num] = NULL;
  }
  else
  {
    quick_filter_str[0] = '\0';
    quick_filter_token[0] = NULL;
  }
}

const char* filter_quick_get(void)
{
  if(!*quick_filter_token)
    return NULL;
  return quick_filter_str;
}

void filter_quick_unset (void)
{
  quick_filter_str[0] = '\0';
  quick_filter_token[0] = NULL;
}

