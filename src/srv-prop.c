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

#include <sys/types.h>	/* chmod */
#include <stdio.h>	/* FILE, fprintf, etc... */
#include <string.h>	/* strchr, strcmp */
#include <unistd.h>	/* unlink */
#include <sys/stat.h>	/* chmod */
#include <time.h>	/* ctime */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */

#include <gtk/gtk.h>

#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "source.h"
#include "host.h"
#include "server.h"
#include "pixmaps.h"
#include "utils.h"
#include "xutils.h"
#include "dialogs.h"
#include "srv-prop.h"


static  GtkWidget *password_entry;
static  GtkWidget *spectator_entry;
static  GtkWidget *rcon_entry;
static  GtkWidget *customcfg_combo;


static GSList *props_list = NULL;


static void props_free (struct server_props *p) {
  if (p) {
    if (p->custom_cfg) g_free (p->custom_cfg);

    if (p->server_password) g_free (p->server_password);
    if (p->spectator_password) g_free (p->spectator_password);
    if (p->rcon_password) g_free (p->rcon_password);

    host_unref (p->host);
    g_free (p);
  }
}


void props_free_all (void) {
  g_slist_foreach (props_list, (GFunc) props_free, NULL);
  g_slist_free (props_list);
  props_list = NULL;
}


static struct server_props *__properties (const struct host *h, 
					  const unsigned short p) {
  struct server_props *res;
  GSList *tmp;

  if (!h || p == 0 || p > 65535)
    return NULL;

  for (tmp = props_list; tmp; tmp = tmp->next) {
    res = (struct server_props *) tmp->data;
    if (res->host == h && res->port == p)
      return res;
  }

  return NULL;
}


struct server_props *properties (struct server *s) {
  return __properties (s->host, s->port);
}


struct server_props *properties_new (struct host *host, unsigned short port) {
  struct server_props *p;

  p = g_malloc (sizeof (struct server_props));

  p->host = host;
  host_ref (host);

  p->port = port;
  p->custom_cfg = NULL;
  p->server_password = NULL;
  p->spectator_password = NULL;
  p->rcon_password = NULL;

  props_list = g_slist_append (props_list, p);

  return p;
}


void props_save (void) {
  FILE *f;
  char *fn;
  GSList *list;
  struct server_props *p;

  fn = file_in_dir (user_rcdir, SERVERS_FILE);

  if (props_list) {
    f = fopen (fn, "w");
    chmod (fn, S_IRUSR | S_IWUSR);
    g_free (fn);
    if (!f)
      return;
  }
  else {
    unlink (fn);
    g_free (fn);
    return;
  }

  for (list = props_list; list; list = list->next) {
    p = (struct server_props *) list->data;

    if (p->custom_cfg || p->server_password || p->spectator_password ||
                                                           p->rcon_password) {
      fprintf (f, "[%s:%d]\n", inet_ntoa (p->host->ip), p->port);

      if (p->custom_cfg)
	fprintf (f, "custom_cfg %s\n", p->custom_cfg);
      if (p->server_password)
	fprintf (f, "password %s\n", p->server_password);
      if (p->spectator_password)
	fprintf (f, "spectator_password %s\n", p->spectator_password);
      if (p->rcon_password)
	fprintf (f, "rcon_password %s\n", p->rcon_password);

      fprintf (f, "\n");
    }
  }

  fclose (f);
}


void props_load (void) {
  FILE *f;
  char *fn;
  struct host *h;
  struct server_props *p = NULL;
  char *addr;
  unsigned short port;
  char buf[1024];
  char *ptr;

  props_free_all ();

  fn = file_in_dir (user_rcdir, SERVERS_FILE);
  f = fopen (fn, "r");
  g_free (fn);
  if (!f)
    return;

  while (!feof (f)) {
    fgets (buf, 1024, f);

    if (buf[0] == '\n') {
      if (p)
	p = NULL;
    }
    else if (buf[0] == '[') {
      if (p)
	continue;

      ptr = strchr (&buf[1], ']');
      if (!ptr)
	continue;
      *ptr = '\0';
    
      if (!parse_address (&buf[1], &addr, &port))
	continue;

      if (port == 0) {
	g_free (addr);
	continue;
      }

      h = host_add (addr);
      g_free (addr);
      if (!h)
	continue;

      p = __properties (h, port);
      if (!p)
	p = properties_new (h, port);
    }
    else {
      if (!p)
	continue;

      ptr = strchr (buf, ' ');
      if (!ptr)
	continue;

      *ptr++ = '\0';

      if (strcmp (buf, "custom_cfg") == 0) {
	if (p->custom_cfg) g_free (p->custom_cfg);
	p->custom_cfg = strdup_strip (ptr);
      }
      else if (strcmp (buf, "password") == 0) {
	if (p->server_password) g_free (p->server_password);
	p->server_password = strdup_strip (ptr);
      }
      else if (strcmp (buf, "spectator_password") == 0) {
	if (p->spectator_password) g_free (p->spectator_password);
	p->spectator_password = strdup_strip (ptr);
      }
      else if (strcmp (buf, "rcon_password") == 0) {
	if (p->rcon_password) g_free (p->rcon_password);
	p->rcon_password = strdup_strip (ptr);
      }
    }
  }

  fclose (f);
}


static void set_new_properties (GtkWidget *widget, struct server *s) {
  struct server_props *props;
  char *customcfg;
  char *srvpwd;
  char *spectpwd;
  char *rconpwd;

  customcfg = strdup_strip (
        gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (customcfg_combo)->entry)));

  srvpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (password_entry)));
  spectpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (spectator_entry)));
  rconpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (rcon_entry)));

  props = properties (s);

  if (props) {
    if (customcfg || srvpwd || spectpwd || rconpwd) {
      if (props->custom_cfg) g_free (props->custom_cfg);
      props->custom_cfg = customcfg;

      if (props->server_password) g_free (props->server_password);
      props->server_password = srvpwd;

      if (props->spectator_password) g_free (props->spectator_password);
      props->spectator_password = spectpwd;

      if (props->rcon_password) g_free (props->rcon_password);
      props->rcon_password = rconpwd;
    }
    else {
      props_list = g_slist_remove (props_list, props);
      props_free (props);
    }
  }
  else {
    if (customcfg || srvpwd || spectpwd || rconpwd) {
      props = properties_new (s->host, s->port);
      props->custom_cfg = customcfg;
      props->server_password = srvpwd;
      props->spectator_password = spectpwd;
      props->rcon_password = rconpwd;
    }
  }

  props_save ();
}


static GtkWidget *server_info_page (struct server *s) {
  GtkWidget *page_vbox;
  GtkWidget *table;
  GtkWidget *frame;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label;
  GSList *sources;
  GSList *list;
  struct master *m;
  struct server_props *props;
  char buf[32];
  GList *cfgs;
  char *time_str;
  char *tmp;

  props = properties (s);

  page_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  /* Address */

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_box_pack_start (GTK_BOX (page_vbox), table, FALSE, FALSE, 0);

  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 16);

  label = gtk_label_new ("IP Address:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new (inet_ntoa (s->host->ip));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("Port:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, 0, 1);
  gtk_widget_show (label);

  g_snprintf (buf, 32, "%d", s->port);

  label = gtk_label_new (buf);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, 0, 1);
  gtk_widget_show (label);

  label = gtk_label_new ("Host Name:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
  gtk_widget_show (label);

  if (s->host->name) {
    label = gtk_label_new (s->host->name);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 4, 1, 2);
    gtk_widget_show (label);
  }

  label = gtk_label_new ("Refreshed:");
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
  gtk_widget_show (label);

  if (s->refreshed) {
    time_str = ctime (&s->refreshed);

    tmp = strchr (time_str, '\n');
    if (tmp)
      *tmp = '\0';

    label = gtk_label_new (time_str);
    gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 4, 2, 3);
    gtk_widget_show (label);
  }

  gtk_widget_show (table);

  /* Sources */

  frame = gtk_frame_new ("Sources");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  sources = references_to_server (s);

  for (list = sources; list; list = list->next) {
    m = (struct master *) list->data;

    label = gtk_label_new (m->name);
    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
    gtk_widget_show (label);
  }

  g_slist_free (sources);

  gtk_widget_show (vbox);
  gtk_widget_show (frame);

  /* Custom CFG */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, FALSE, 0);

  customcfg_combo = gtk_combo_new ();
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (customcfg_combo)->entry), 
                                                                         256);
  gtk_widget_set_usize (GTK_COMBO (customcfg_combo)->entry, 112, -1);

  if ((games[s->type].flags & GAME_CONNECT) != 0 && 
                                                 games[s->type].custom_cfgs) {

    cfgs = (*games[s->type].custom_cfgs) (NULL, s->game);

    combo_set_vals (customcfg_combo, cfgs, 
                      (props && props->custom_cfg)? props->custom_cfg : NULL);
    if (cfgs) {
      g_list_foreach (cfgs, (GFunc) g_free, NULL);
      g_list_free (cfgs);
    }
  }
  else {
    gtk_widget_set_sensitive (customcfg_combo, FALSE);
  }

  gtk_box_pack_end (GTK_BOX (hbox), customcfg_combo, FALSE, FALSE, 0);
  gtk_widget_show (customcfg_combo);

  label = gtk_label_new ("Custom CFG:");
  gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


static GtkWidget *passwd_entry (char *str, char *passwd,
                                                  GtkWidget *table, int pos) {
  GtkWidget *label;
  GtkWidget *entry;

  label = gtk_label_new (str);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, pos, pos + 1);
  gtk_widget_show (label);

  entry = gtk_entry_new_with_max_length (32);
  gtk_widget_set_usize (entry, 112, -1);
  if (passwd) {
    gtk_entry_set_text (GTK_ENTRY (entry), passwd);
  }
  gtk_table_attach_defaults (GTK_TABLE (table), entry, 1, 2, pos, pos + 1);
  gtk_widget_show (entry);

  return entry;
}


static GtkWidget *server_passwords_page (struct server *s) {
  GtkWidget *page_vbox;
  GtkWidget *table;
  struct server_props *props;

  props = properties (s);

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  table = gtk_table_new (3, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_box_pack_start (GTK_BOX (page_vbox), table, FALSE, FALSE, 0);

  password_entry = passwd_entry ("Server Password",
				 (props)? props->server_password : NULL,
				 table, 0);

  if ((games[s->type].flags & GAME_PASSWORD) == 0)
    gtk_widget_set_sensitive (password_entry, FALSE);

  spectator_entry = passwd_entry ("Spectator Password",
				  (props)? props->spectator_password : NULL,
				  table, 1);

  if ((games[s->type].flags & GAME_SPECTATE) == 0)
    gtk_widget_set_sensitive (spectator_entry, FALSE);

  rcon_entry = passwd_entry ("RCon Password",
			     (props)? props->rcon_password : NULL,
			     table, 2);

  if ((games[s->type].flags & GAME_RCON) == 0)
    gtk_widget_set_sensitive (rcon_entry, FALSE);

  gtk_widget_show (table);

  gtk_widget_show (page_vbox);

  return page_vbox;
}


void properties_dialog (struct server *s) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *notebook;
  GtkWidget *page;
  GtkWidget *button;
  GtkWidget *pixmap;
  GtkWidget *label;
  char buf[256];

  window = dialog_create_modal_transient_window ("Properties", 
                                                           TRUE, FALSE, NULL);
  main_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  /*
   *  Server Name 
   */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 8);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, TRUE, FALSE, 0);

  if (games[s->type].pix) {
    pixmap = gtk_pixmap_new (games[s->type].pix->pix, 
                                                    games[s->type].pix->mask);
    gtk_box_pack_start (GTK_BOX (hbox2), pixmap, FALSE, FALSE, 0);
    gtk_widget_show (pixmap);
  }

  if (s->name) {
    label = gtk_label_new (s->name);
  }
  else {
    g_snprintf (buf, 256, "%s:%d", inet_ntoa (s->host->ip), s->port);
    label = gtk_label_new (buf);
  }
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox2);
  gtk_widget_show (hbox);

  /*
   *  Notebook 
   */

  notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_box_pack_start (GTK_BOX (main_vbox), notebook, FALSE, FALSE, 0);

  page = server_info_page (s);
  label = gtk_label_new (" Info ");
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  page = server_passwords_page (s);
  label = gtk_label_new (" Passwords ");
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);

  gtk_widget_show (notebook);

  /* 
   *  Buttons at the bottom
   */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("Cancel");
  gtk_widget_set_usize (button, 80, -1);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                    GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("OK");
  gtk_widget_set_usize (button, 80, -1);
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                          GTK_SIGNAL_FUNC (set_new_properties), (gpointer) s);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  gtk_widget_show (main_vbox);

  gtk_widget_show (window);

  gtk_main ();

  unregister_window (window);
}


void combo_set_vals (GtkWidget *combo, GList *strlist, char *str) {
  if (!strlist) {
    gtk_list_clear_items (GTK_LIST (GTK_COMBO (combo)->list), 0, -1);
  } else {
    /*
     *  gtk_combo_set_popdown_strings (actually gtk_list_insert_items) 
     *  automatically selects the first one and puts it into entry.  
     *  That should not happen.  Drop selection mode for a moment 
     *  to prevent that.
     */

    gtk_list_set_selection_mode (GTK_LIST (GTK_COMBO (combo)->list),
                                                        GTK_SELECTION_SINGLE);
    gtk_combo_set_popdown_strings (GTK_COMBO (combo), strlist);
    gtk_list_set_selection_mode (GTK_LIST (GTK_COMBO (combo)->list),
                                                        GTK_SELECTION_BROWSE);
  }

  if (str) {
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), str);
    gtk_entry_set_position (GTK_ENTRY (GTK_COMBO (combo)->entry), 0);
  }
}

