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
#include <string.h>	/* memset, strcmp */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */


#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "server.h"
#include "host.h"
#include "source.h"
#include "dialogs.h"
#include "utils.h"
#include "config.h"
#include "statistics.h"
#include "debug.h"

#define PERCENTS(A,B)	((B)? (A)/((B)/100.0) : 0)

#define	OS_NUM		5
#define CPU_NUM		5


enum OS { OS_WINDOWS = 0, OS_LINUX, OS_SOLARIS, OS_MACOS, OS_UNKNOWN };
enum CPU { CPU_X86 = 0, CPU_SPARC, CPU_AXP, CPU_PPC, CPU_UNKNOWN };

struct server_stats {
  int	players;
  int 	servers;

  int 	ok;
  int 	down;
  int	timeout;
  int	na;
};

struct arch_stats {
  int oscpu[OS_NUM][CPU_NUM];
  int count;
};


static const char *srv_headers[6] = {
  //The space behind up and down is to make the strings different from
  //those in flt-player.c because their meaning is different in german
  N_("Servers"), N_("Up "), N_("Timeout"), N_("Down "), N_("Info n/a"), N_("Players")
};

static const char *os_names[OS_NUM] = {
  "Windows", "Linux", "Solaris", "MacOS", N_("unknown")
};

static const char *cpu_names[CPU_NUM] = {
  "Intel x86", "Sparc", "AXP", "PPC", N_("unknown")
};

const char *srv_label = N_("Servers");
const char *arch_label = N_("OS/CPU");


static struct server_stats *srv_stats;
static struct arch_stats *srv_archs;

struct players_s
{
  int on_os[OS_NUM];
  int total;
};

static struct players_s* players;

static int servers_count;
static int players_count;

static GtkWidget *stat_notebook;
static GtkWidget *arch_notebook;


static void server_stats_create (void) {

  srv_stats = g_malloc0 (sizeof (struct server_stats) * GAMES_TOTAL);
  srv_archs  = g_malloc0 (sizeof (struct arch_stats) * GAMES_TOTAL);
  players  = g_malloc0 (sizeof (struct arch_stats) * GAMES_TOTAL);

  servers_count = 0;
  players_count = 0;
}


static void server_stats_destroy (void) {
  if (srv_stats) {
    g_free (srv_stats);
    srv_stats = NULL;
  }
  if (srv_archs) {
    g_free (srv_archs);
    srv_archs = NULL;
  }
}


enum CPU identify_cpu (struct server *s, const char *versionstr) {
  enum CPU cpu = CPU_UNKNOWN;
  char *str;
  
  if (!(str = g_strdup(versionstr)))
  	return CPU_UNKNOWN;
  g_strdown(str);
  
  if (strstr (str, "x86") || strstr (str, "i386"))
    cpu = CPU_X86;
  else if (strstr (str, "sparc"))
    cpu = CPU_SPARC;
  else if (strstr (str, "axp"))
    cpu = CPU_AXP;
  else if (strstr (str, "ppc"))
    cpu = CPU_PPC;
  else {
    debug (3, "identify_cpu() -- [%s %s:%d] Unknown CPU: %s\n", type2id(s->type), 
                                inet_ntoa (s->host->ip), s->port, versionstr);
  }
  g_free(str);
  return cpu;
}


enum OS identify_os (struct server *s, char *versionstr) {
  enum OS os = OS_UNKNOWN;
  char *str;
  
  if (!(str = g_strdup(versionstr)))
  	return CPU_UNKNOWN;
  g_strdown(str);

  if (strstr (str, "win"))
    os = OS_WINDOWS;
  else if (strstr (str, "linux"))
    os = OS_LINUX;
  else if (strstr (str, "solaris"))
    os = OS_SOLARIS;
  else if (strstr (str, "macos"))
    os = OS_MACOS;
  else {
    debug (3, "identify_os() -- [%s %s:%d] Unknown OS: %s\n", type2id(s->type),
                                inet_ntoa (s->host->ip), s->port, versionstr);
  }
  g_free(str);
  return os;
}

enum OS t2_identify_os (struct server *s, char *versionstr)
{
  if (!strcmp(versionstr,"1"))
    return OS_LINUX;

  return OS_WINDOWS;
}

static void collect_statistics (void) {
  GSList *servers;
  GSList *tmp;
  struct server *s;
  char **info;
  enum OS os;
  enum CPU cpu;
  int countthisserver;

  servers = all_servers (); /* Free at end of this function */

  if (servers) {
    for (tmp = servers; tmp; tmp = tmp->next) {
      s = (struct server *) tmp->data;
      info = s->info;
      cpu = CPU_UNKNOWN;
      os = OS_UNKNOWN;
      countthisserver=0;

      servers_count++;

      srv_stats[s->type].servers++;
      srv_stats[s->type].players += s->curplayers;

      players_count += s->curplayers;

      if (s->ping < MAX_PING) {
	if (s->ping >= 0)
	{
	  srv_stats[s->type].ok++;
	  countthisserver=1;
	}
	else
	  srv_stats[s->type].na++;
      }
      else {
	if (s->ping == MAX_PING)
	  srv_stats[s->type].timeout++;
	else
	  srv_stats[s->type].down++;
      }

      if (info && games[s->type].arch_identifier)
      {
	while (info[0]) {
	  if (g_strcasecmp (info[0], games[s->type].arch_identifier) == 0)
	  {
	    if (!info[1])
	      break;

	    if (games[s->type].identify_cpu)
	      cpu = games[s->type].identify_cpu(s, info[1]); 
	    else
	      cpu = CPU_UNKNOWN;

	    if (games[s->type].identify_os)
	      os = games[s->type].identify_os(s, info[1]); 
	    else
	      os = OS_UNKNOWN;

	    break;
	  }
	  info += 2;
	}

	if(countthisserver)
	{
	  srv_archs[s->type].oscpu[os][cpu]++;
	  srv_archs[s->type].count++;
	  players[s->type].on_os[os] += s->curplayers;
	  players[s->type].total += s->curplayers;
	}
      }
    }

    server_list_free (servers);
  }
}


static void put_label_to_table (GtkWidget *table, const char *str, 
                                            float justify, int col, int row) {
  GtkWidget *label;

  if (str) {
    label = gtk_label_new (str);
    gtk_misc_set_alignment (GTK_MISC (label), justify, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), label, 
                                                  col, col + 1, row, row + 1);
    gtk_widget_show (label);
  }
}


static void put_server_stats (GtkWidget *table, int num, int row) {
  char buf1[16], buf2[16], buf3[16], buf4[16], buf5[16], buf6[16];
  const char *strings[6];
  int i;

  strings[0] = buf1;
  strings[1] = buf2;
  strings[2] = buf3;
  strings[3] = buf4;
  strings[4] = buf5;
  strings[5] = buf6;

  g_snprintf (buf1, 16, "%d (%.2f%%)", srv_stats[num].servers, 
                            PERCENTS (srv_stats[num].servers, servers_count));

  g_snprintf (buf2, 16, "%d (%.2f%%)", srv_stats[num].ok,
                        PERCENTS (srv_stats[num].ok, srv_stats[num].servers));

  g_snprintf (buf3, 16, "%d (%.2f%%)", srv_stats[num].timeout,
                   PERCENTS (srv_stats[num].timeout, srv_stats[num].servers));

  g_snprintf (buf4, 16, "%d (%.2f%%)", srv_stats[num].down,
                      PERCENTS (srv_stats[num].down, srv_stats[num].servers));

  g_snprintf (buf5, 16, "%d (%.2f%%)", srv_stats[num].na,
                        PERCENTS (srv_stats[num].na, srv_stats[num].servers));

  g_snprintf (buf6, 16, "%d (%.2f%%)", srv_stats[num].players,
                            PERCENTS (srv_stats[num].players, players_count));

  for (i = 0; i < 6; i++)
    put_label_to_table (table, strings[i], 1.0, i + 1, row);
}


static GtkWidget *server_stats_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *game_label;
  int i;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (alignment), frame);

  table = gtk_table_new (GAMES_TOTAL + 2, 7, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);

  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 20);
  gtk_table_set_col_spacing (GTK_TABLE (table), 5, 20);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), GAMES_TOTAL, 12);

  for (i = 0; i < 6; i++)
    put_label_to_table (table, _(srv_headers[i]), 1.0, i + 1, 0);

  for (i = 0; i < GAMES_TOTAL; i++) {
    game_label = game_pixmap_with_label (i);
    gtk_table_attach_defaults (GTK_TABLE (table), 
				 game_label, 
				 0, 1, i + 1, i + 2);

    put_server_stats (table, i, i + 1);

    if (i > 0) {
      srv_stats[0].servers += srv_stats[i].servers;
      srv_stats[0].ok      += srv_stats[i].ok;
      srv_stats[0].timeout += srv_stats[i].timeout;
      srv_stats[0].down    += srv_stats[i].down;
      srv_stats[0].na      += srv_stats[i].na;
      srv_stats[0].players += srv_stats[i].players;
    }
  }

  put_label_to_table (table, _("Total"), 0.0, 0, GAMES_TOTAL + 1);

  put_server_stats (table, 0, GAMES_TOTAL + 1);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  gtk_widget_show (alignment);
  gtk_widget_show (page_vbox);

  return page_vbox;
}


static void put_arch_stats (GtkWidget *table, int val, int total, 
                                                           int row, int col) {
  char buf[16];

  g_snprintf (buf, 16, "%d (%.2f%%)", val, PERCENTS (val, total));
  put_label_to_table (table, buf, 1.0, row, col);
}


static void arch_notebook_page (GtkWidget *notebook, 
                             enum server_type type, struct arch_stats *arch) {
  GtkWidget *table;
  int i, j;
  int cpu_total;

  table = gtk_table_new (OS_NUM + 2, CPU_NUM + 3, FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, 
                                               game_pixmap_with_label (type));

  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);

  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
  gtk_table_set_col_spacing (GTK_TABLE (table), OS_NUM, 20);
  gtk_table_set_row_spacing (GTK_TABLE (table), CPU_NUM, 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), CPU_NUM+1, 12);

  put_label_to_table (table, _("CPU  \\  OS"), 0.5, 0, 0);

  for (i = 0; i < OS_NUM; i++) {
    put_label_to_table (table, _(os_names[i]), 1.0, i + 1, 0);
  }
  put_label_to_table (table, _("Total"), 1.0, OS_NUM + 1, 0);

  for (i = 0; i < CPU_NUM; i++) {
    put_label_to_table (table, _(cpu_names[i]), 0.0, 0, i + 1);
  }
  put_label_to_table (table, _("Total"), 0.0, 0, CPU_NUM + 1);
  put_label_to_table (table, _("Players"), 0.0, 0, CPU_NUM + 2);

  for (j = 0; j < CPU_NUM; j++) {
    cpu_total = 0;
    for (i = 0; i < OS_NUM; i++) {
      put_arch_stats (table, arch->oscpu[i][j], arch->count, i + 1, j + 1);

      cpu_total += arch->oscpu[i][j];

      if (j > 0)
	arch->oscpu[i][0] += arch->oscpu[i][j];
    }
    put_arch_stats (table, cpu_total, arch->count, OS_NUM + 1, j + 1);
  }

  for (i = 0; i < OS_NUM; i++) {
    put_arch_stats (table, arch->oscpu[i][0], arch->count, i + 1, CPU_NUM + 1);
    put_arch_stats (table, players[type].on_os[i],
	players[type].total, i + 1, CPU_NUM + 2);
  }
  put_arch_stats (table, arch->count, arch->count, OS_NUM + 1, CPU_NUM + 1);
  put_arch_stats (table, srv_stats[type].players, players_count, OS_NUM + 1, CPU_NUM + 2);

  gtk_widget_show (table);
}


static GtkWidget *archs_stats_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  int pagenum;
  enum server_type type = Q2_SERVER;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, TRUE, TRUE, 0);

  arch_notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (arch_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (arch_notebook), 4);
  gtk_container_add (GTK_CONTAINER (alignment), arch_notebook);

  for (type = 0; type < GAMES_TOTAL; type++)
  {
    if(games[type].arch_identifier)
    {
      arch_notebook_page (arch_notebook, type, &srv_archs[type]);
    }
  }
  pagenum = config_get_int_with_default ("/" CONFIG_FILE "/Statistics/game", 0);

  gtk_notebook_set_page (GTK_NOTEBOOK (arch_notebook), pagenum );

  gtk_widget_show (arch_notebook);
  gtk_widget_show (alignment);
  gtk_widget_show (page_vbox);

  return page_vbox;
}


static void grab_defaults (GtkWidget *w, gpointer data) {
  int pagenum;

  pagenum = gtk_notebook_get_current_page (GTK_NOTEBOOK (arch_notebook));

  config_set_int ("/" CONFIG_FILE "/Statistics/game", pagenum);

  config_set_string ("/" CONFIG_FILE "/Statistics/page", 
        (gtk_notebook_get_current_page (GTK_NOTEBOOK (stat_notebook)) == 0) ?
                                                      srv_label : arch_label);
}


void statistics_dialog (void) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *page;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  char *saved_page;
  int page_num = 0;

  server_stats_create ();
  collect_statistics ();

  window = dialog_create_modal_transient_window (_("Statistics"), 
                                                           TRUE, FALSE, NULL);
  main_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  label = gtk_label_new (_("Statistics"));
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 8);
  gtk_widget_show (label);

  stat_notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (stat_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (stat_notebook), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), stat_notebook, FALSE, FALSE, 0);

  page = server_stats_page ();
  label = gtk_label_new (_(srv_label));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

  page = archs_stats_page ();
  label = gtk_label_new (_(arch_label));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

  saved_page = config_get_string ("/" CONFIG_FILE "/Statistics/page");
  if (saved_page) {
    if (strcmp (saved_page, arch_label) == 0)
      page_num = 1;
    g_free (saved_page);
  }
  gtk_notebook_set_page (GTK_NOTEBOOK (stat_notebook), page_num);

  gtk_widget_show (stat_notebook);

  /* Close Button */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		                       GTK_SIGNAL_FUNC (grab_defaults), NULL);
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

  server_stats_destroy ();
}

