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

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "dialogs.h"
#include "server.h"
#include "config.h"
#include "filter.h"
#include "flt-player.h"
#include "utils.h"

static int server_pass_filter (struct server *s, struct server_filter_vars *vars);
static void server_filter_init (void);


struct filter filters[FILTERS_TOTAL] = {
  {
    N_("Server"),
    N_("S Filter"),
    N_("SF Cfg"),
    server_pass_filter,
    server_filter_init,
    NULL,
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
    1,
    FILTER_NOT_CHANGED
  }
};


unsigned char cur_filter = 0;

unsigned filter_current_time = 1;


static  GtkWidget *filter_retries_spinner;
static  GtkWidget *filter_ping_spinner;
static  GtkWidget *filter_not_full_check_button;
static  GtkWidget *filter_not_empty_check_button;
static  GtkWidget *filter_no_cheats_check_button;
static  GtkWidget *filter_no_password_check_button;
static  GtkWidget *server_filter_name_entry;
static  GtkWidget *game_contains_entry;
static  GtkWidget *filter_game_type_entry;
static  GtkWidget *version_contains_entry;




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
      if ((*filters[n].func) (s, &server_filters[current_server_filter]))
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
static int server_pass_filter (struct server *s, struct server_filter_vars
    *vars){

  char **info_ptr;
  /* Filter Zero is No Filter */
  if( current_server_filter == 0 ){ return TRUE; }

  if (s->ping == -1)	/* no information */
    return FALSE;

  /* So that we do not get a core dump in the check below we 
     will first check that if we are filtering on a string that
     the string is actually set in the server structure.
  */
  if( server_filters[current_server_filter].game_contains && 
      strlen( server_filters[current_server_filter].game_contains )  && ! s->game )
    return FALSE;

  if( server_filters[current_server_filter].game_type && 
      strlen( server_filters[current_server_filter].game_type )  && ! s->gametype )
    return FALSE;

  if( server_filters[current_server_filter].version_contains && 
      strlen (server_filters[current_server_filter].version_contains)) {
    /* Filter for the version */
    for (info_ptr = s->info; info_ptr && *info_ptr; info_ptr += 2) {
      if (strcmp (*info_ptr, "version") == 0) {
	if( !strstr( info_ptr[1], server_filters[current_server_filter].version_contains )){
	  return FALSE;
	}
      }
    }
  }/*end version check */

  if (s->retries < server_filters[current_server_filter].filter_retries && 
      s->ping < server_filters[current_server_filter].filter_ping &&

      ( !server_filters[current_server_filter].game_contains || 
	!strlen( server_filters[current_server_filter].game_contains ) || 
	( s->game && lowcasestrstr( s->game, server_filters[current_server_filter].game_contains ))) &&

      ( !server_filters[current_server_filter].game_type || 
	!strlen( server_filters[current_server_filter].game_type ) || 
	( s->gametype && strstr( s->gametype, server_filters[current_server_filter].game_type ))) &&

      
      (!server_filters[current_server_filter].filter_not_full    || s->curplayers != s->maxplayers) && 
      (!server_filters[current_server_filter].filter_not_empty   || s->curplayers != 0) &&
      (!server_filters[current_server_filter].filter_no_cheats   || (s->flags & SERVER_CHEATS) == 0) &&
      (!server_filters[current_server_filter].filter_no_password || (s->flags & SERVER_PASSWORD) == 0))
    return TRUE;

  return FALSE;
}


static void server_filter_init (void) {
  int i;
  char config_section[64];

  // 0 is no filter
  for (i = 1; i <= MAX_SERVER_FILTERS; i++) {
    snprintf( config_section, 64, "/" CONFIG_FILE "/Server Filter/%d", i );
    config_push_prefix (config_section );
    /* server_filters[i] = g_malloc( sizeof( struct server_filter_vars )); */
    server_filters[i].filter_retries     = config_get_int  ("retries=2");
    server_filters[i].filter_ping        = config_get_int  ("ping=1000");
    server_filters[i].filter_not_full    = config_get_bool ("not full=false");
    server_filters[i].filter_not_empty   = config_get_bool ("not empty=false");
    server_filters[i].filter_no_cheats   = config_get_bool ("no cheats=false");
    server_filters[i].filter_no_password = config_get_bool ("no password=false");
    server_filters[i].filter_name        = config_get_string_with_default ("filter_name",NULL);
    server_filters[i].game_contains      = config_get_string_with_default ("game_contains",NULL);
    server_filters[i].version_contains   = config_get_string_with_default ("version_contains",NULL);
    server_filters[i].game_type          = config_get_string_with_default ("game_type",NULL);
    
    /* This is called before the window has been created so
       we cannot set the filter names in the pulldown yet. */
    
    config_pop_prefix ();
  }

  sprintf( config_section, "/" CONFIG_FILE "/Server Filter" );
  config_push_prefix (config_section );
  current_server_filter = config_get_int  ("current_server_filter=0");
  config_pop_prefix ();

}


static void server_filter_new_defaults (void) {
  int i;
  gchar *gcptr;
  int text_changed;
  char config_section[64];

  if( current_server_filter == 0 ){ return; }

  sprintf( config_section, "/" CONFIG_FILE "/Server Filter/%d", current_server_filter );
  config_push_prefix (config_section );

  i = gtk_spin_button_get_value_as_int (
                                    GTK_SPIN_BUTTON (filter_retries_spinner));
  if (server_filters[current_server_filter].filter_retries != i) {
    config_set_int  ("retries", server_filters[current_server_filter].filter_retries = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  i = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (filter_ping_spinner));
  if (server_filters[current_server_filter].filter_ping != i) {
    config_set_int  ("ping", server_filters[current_server_filter].filter_ping = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  /* Note: The getting of text could be put in some kind of loop. */

  /* Filter Name values -- baa */
  gcptr = gtk_editable_get_chars (GTK_EDITABLE (server_filter_name_entry), 0, -1 );
  text_changed = 0;
  if( strlen( gcptr )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (server_filters[current_server_filter].filter_name){
      if( strcmp( gcptr, server_filters[current_server_filter].filter_name )) text_changed = 1;
      g_free( server_filters[current_server_filter].filter_name );
    } else {
      text_changed = 1;
    }
    server_filters[current_server_filter].filter_name = g_strdup( gcptr );
    if (text_changed) {
      config_set_string ("filter_name", server_filters[current_server_filter].filter_name );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (server_filters[current_server_filter].filter_name){
      text_changed = 1; /* From something to nothing */
      g_free( server_filters[current_server_filter].filter_name );
      config_set_string ("filter_name", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    server_filters[current_server_filter].filter_name = NULL;
  } 
  g_free( gcptr );


  /* GAMECONTAINS string values -- baa */
  gcptr = gtk_editable_get_chars (GTK_EDITABLE (game_contains_entry), 0, -1 );
  text_changed = 0;
  if( strlen( gcptr )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (server_filters[current_server_filter].game_contains){
      if( strcmp( gcptr, server_filters[current_server_filter].game_contains )) text_changed = 1;
      g_free( server_filters[current_server_filter].game_contains );
    } else {
      text_changed = 1;
    }
    server_filters[current_server_filter].game_contains = g_strdup( gcptr );
    if (text_changed) {
      config_set_string ("game_contains", server_filters[current_server_filter].game_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (server_filters[current_server_filter].game_contains){
      text_changed = 1; /* From something to nothing */
      g_free( server_filters[current_server_filter].game_contains );
      config_set_string ("game_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    server_filters[current_server_filter].game_contains = NULL;
  } 
  g_free( gcptr );


  /* Version string values -- baa */
  gcptr = gtk_editable_get_chars (GTK_EDITABLE (version_contains_entry), 0, -1 );
  text_changed = 0;
  if( strlen( gcptr )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (server_filters[current_server_filter].version_contains){
      if( strcmp( gcptr, server_filters[current_server_filter].version_contains )) text_changed = 1;
      g_free( server_filters[current_server_filter].version_contains );
    } else {
      text_changed = 1;
    }
    server_filters[current_server_filter].version_contains = g_strdup( gcptr );
    if (text_changed) {
      config_set_string ("version_contains", server_filters[current_server_filter].version_contains );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (server_filters[current_server_filter].version_contains){
      text_changed = 1; /* From something to nothing */
      g_free( server_filters[current_server_filter].version_contains );
      config_set_string ("version_contains", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    server_filters[current_server_filter].version_contains = NULL;
  } 
  g_free( gcptr );


  /* GAMETYPE string values -- baa */
  gcptr = gtk_editable_get_chars (GTK_EDITABLE (filter_game_type_entry), 0, -1 );
  text_changed = 0;
  if( strlen( gcptr )){
    /*
      First case, the user entered something.  See if the value
      is different 
    */
    if (server_filters[current_server_filter].game_type){
      if( strcmp( gcptr, server_filters[current_server_filter].game_type )) text_changed = 1;
      g_free( server_filters[current_server_filter].game_type );
    } else {
      text_changed = 1;
    }
    server_filters[current_server_filter].game_type = g_strdup( gcptr );
    if (text_changed) {
      config_set_string ("game_type", server_filters[current_server_filter].game_type );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
  } else {
    if (server_filters[current_server_filter].game_type){
      text_changed = 1; /* From something to nothing */
      g_free( server_filters[current_server_filter].game_type );
      config_set_string ("game_type", "" );
      filters[FILTER_SERVER].changed = FILTER_CHANGED;
    }
    server_filters[current_server_filter].game_type = NULL;
  } 
  g_free( gcptr );
  /* end game_type filter */
  

  i = GTK_TOGGLE_BUTTON (filter_not_full_check_button)->active;
  if (server_filters[current_server_filter].filter_not_full != i) {
    config_set_bool ("not full", server_filters[current_server_filter].filter_not_full = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  i = GTK_TOGGLE_BUTTON (filter_not_empty_check_button)->active;
  if (server_filters[current_server_filter].filter_not_empty != i) {
    config_set_bool ("not empty", server_filters[current_server_filter].filter_not_empty = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  i = GTK_TOGGLE_BUTTON (filter_no_cheats_check_button)->active;
  if (server_filters[current_server_filter].filter_no_cheats != i) {
    config_set_bool ("no cheats", server_filters[current_server_filter].filter_no_cheats = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  i = GTK_TOGGLE_BUTTON (filter_no_password_check_button)->active;
  if (server_filters[current_server_filter].filter_no_password != i) {
    config_set_bool ("no password", server_filters[current_server_filter].filter_no_password = i);
    filters[FILTER_SERVER].changed = FILTER_CHANGED;
  }

  /* This from the Gtk FAQ, mostly. */
  if( server_filter_widget[current_server_filter + filter_start_index] && 
      GTK_BIN (server_filter_widget[current_server_filter + filter_start_index])->child)
    {
      GtkWidget *child = GTK_BIN (server_filter_widget[current_server_filter + filter_start_index])->child;
      
      /* do stuff with child */
      if (GTK_IS_LABEL (child))
	{
	  if( server_filters[current_server_filter].filter_name != NULL &&
	      strlen( server_filters[current_server_filter].filter_name )){
	    gtk_label_set (GTK_LABEL (child), server_filters[current_server_filter].filter_name );
	  } else {
	    /* Reuse the config_secion var */
	    sprintf( config_section, "Filter %d", current_server_filter );
	    gtk_label_set (GTK_LABEL (child), config_section );
	  }
	}
    }
  
  config_pop_prefix ();

  if (filters[FILTER_SERVER].changed == FILTER_CHANGED)
    filters[FILTER_SERVER].last_changed = ++filter_current_time;
}


static void server_filter_page (GtkWidget *notebook) {
  GtkWidget *page_hbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *label;
  GtkObject *adj;

  char label_buf[64];
  int row; /* for auto inserting entry boxes */

  /* One cannot edit the "None" filter */
  if( current_server_filter == 0 ){ current_server_filter = 1; }

  page_hbox = gtk_hbox_new (TRUE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_hbox), 8);

  label = gtk_label_new (_("Server Filter"));
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_hbox, label);

  frame = gtk_frame_new (_("Server would pass filter if"));
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_hbox), frame, FALSE, FALSE, 0);

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

  adj = gtk_adjustment_new (server_filters[current_server_filter].filter_ping, 0.0, MAX_PING, 100.0, 1000.0, 0.0);

  filter_ping_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_spin_button_set_update_policy (GTK_SPIN_BUTTON (filter_ping_spinner), 
                                                            GTK_UPDATE_ALWAYS);
  gtk_widget_set_usize (filter_ping_spinner, 64, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_ping_spinner, 1, 2, 0, 1);
  gtk_widget_show (filter_ping_spinner);

  /* max timeouts */

  label = gtk_label_new (_("the number of retires is fewer than"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 
                                                                         0, 0);
  gtk_widget_show (label);

  adj = gtk_adjustment_new (server_filters[current_server_filter].filter_retries,
			    0.0, MAX_RETRIES, 1.0, 1.0, 0.0);

  filter_retries_spinner = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 0, 0);
  gtk_widget_set_usize (filter_retries_spinner, 64, -1);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_retries_spinner, 
                                                                   1, 2, 1, 2);
  gtk_widget_show (filter_retries_spinner);


  /* Filter Name -- baa */
  row = 0;
  sprintf( label_buf, "Filter %d", current_server_filter + 1 );
  
  label = gtk_label_new (_("Filter Name (For Menu):"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 0, 0);
  gtk_widget_show (label);
  server_filter_name_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (server_filter_name_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (server_filter_name_entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (server_filter_name_entry), 
		      (server_filters[current_server_filter].filter_name) ?
		      server_filters[current_server_filter].filter_name : label_buf );

  gtk_table_attach_defaults (GTK_TABLE (table), server_filter_name_entry, 4, 5, row, row+1);
  gtk_widget_show (server_filter_name_entry);
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
  gtk_entry_set_text (GTK_ENTRY (game_contains_entry), 
		      (server_filters[current_server_filter].game_contains) ?
		      server_filters[current_server_filter].game_contains : "");

  gtk_table_attach_defaults (GTK_TABLE (table), game_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (game_contains_entry);
  row++;

  /* GAMETYPE Filter -- baa */
  /* http://developer.gnome.org/doc/API/gtk/gtktable.html */
    
  label = gtk_label_new (_("The Game Type Contains the String:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 3, 4, row, row+1, GTK_FILL, GTK_FILL, 
		    0, 0);
  gtk_widget_show (label);
  filter_game_type_entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (filter_game_type_entry, 64, -1);
  gtk_entry_set_editable (GTK_ENTRY (filter_game_type_entry), TRUE);
  gtk_entry_set_text (GTK_ENTRY (filter_game_type_entry), 
		      (server_filters[current_server_filter].game_type) ?
		      server_filters[current_server_filter].game_type : "");

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
  gtk_entry_set_text (GTK_ENTRY (version_contains_entry), 
		      (server_filters[current_server_filter].version_contains) ? 
		      server_filters[current_server_filter].version_contains : "");

  gtk_table_attach_defaults (GTK_TABLE (table), version_contains_entry, 4, 5, row, row+1);
  gtk_widget_show (version_contains_entry);


  /* not full */

  filter_not_full_check_button = 
                       gtk_check_button_new_with_label (_("it is not full"));
  gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (filter_not_full_check_button), server_filters[current_server_filter].filter_not_full);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_not_full_check_button, 
                                                                   0, 2, 2, 3);
  gtk_widget_show (filter_not_full_check_button);

  /* not empty */

  filter_not_empty_check_button = 
                        gtk_check_button_new_with_label (_("it is not empty"));
  gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (filter_not_empty_check_button), server_filters[current_server_filter].filter_not_empty);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_not_empty_check_button, 
                                                                   0, 2, 3, 4);
  gtk_widget_show (filter_not_empty_check_button);

  /* no cheats */

  filter_no_cheats_check_button = 
                  gtk_check_button_new_with_label (_("cheats are not allowed"));
  gtk_toggle_button_set_active (
          GTK_TOGGLE_BUTTON (filter_no_cheats_check_button), 
	  server_filters[current_server_filter].filter_no_cheats);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_cheats_check_button, 
                                                                   0, 2, 4, 5);
  gtk_widget_show (filter_no_cheats_check_button);

  /* no password */

  filter_no_password_check_button = 
                    gtk_check_button_new_with_label (_("no password required"));
  gtk_toggle_button_set_active (
      GTK_TOGGLE_BUTTON (filter_no_password_check_button), 
      server_filters[current_server_filter].filter_no_password);
  gtk_table_attach_defaults (GTK_TABLE (table), filter_no_password_check_button, 
                                                                   0, 2, 5, 6);
  gtk_widget_show (filter_no_password_check_button);

  gtk_widget_show (table);
  gtk_widget_show (alignment);
  gtk_widget_show (frame);
  gtk_widget_show (page_hbox);
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
                                                            TRUE, TRUE, NULL);
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
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("OK"));
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (server_filter_new_defaults), NULL);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (player_filter_new_defaults), NULL);
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

