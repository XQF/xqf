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
#include <stdlib.h>
#include <string.h>	/* memset, strcmp */
#include <sys/socket.h>	/* inet_ntoa */
#include <netinet/in.h>	/* inet_ntoa */
#include <arpa/inet.h>	/* inet_ntoa */


#include "i18n.h"
#include "xqf-ui.h"
#include "game.h"
#include "pref.h"
#include "server.h"
#include "host.h"
#include "source.h"
#include "dialogs.h"
#include "utils.h"
#include "config.h"
#include "statistics.h"
#include "country-filter.h"
#include "debug.h"

#define PERCENTS(A,B)	((B)? (A)/((B)/100.0) : 0)

#define	OS_NUM		5
#define CPU_NUM		6


enum OS { OS_WINDOWS = 0, OS_LINUX, OS_SOLARIS, OS_MACOS, OS_UNKNOWN };
enum CPU { CPU_X86 = 0, CPU_X86_64, CPU_SPARC, CPU_AXP, CPU_PPC, CPU_UNKNOWN };

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
  int notebookpage; // page in notebook
};

struct country_num {
  int c;
  int n;
};
struct country_stats {
  unsigned nonzero;
  int notebookpage; // page in notebook
  struct country_num* country;
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
  "i386", "x86_64", "sparc", "alpha", "ppc", N_("unknown")
};

const char *srv_label = N_("Servers");
const char *arch_label = N_("OS/CPU");
const char *country_label = N_("Country");


static struct server_stats *srv_stats;
static struct arch_stats *srv_archs;
#ifdef USE_GEOIP
static struct country_stats *srv_countries;
#endif

struct players_s
{
  int on_os[OS_NUM];
  int total;
};

static struct players_s* players;

static int servers_count;
static int players_count;

#ifdef USE_GEOIP
static GtkWidget *country_notebook;
#endif
static GtkWidget *stat_notebook;
static GtkWidget *arch_notebook;
static enum server_type selected_type;
static enum server_type selected_country;


static void server_stats_create (void)
{
#ifdef USE_GEOIP
  unsigned g;
  // position GAMES_TOTAL is used for total number of all games
  unsigned i = (sizeof(struct country_stats) + geoip_num_countries()*sizeof(struct country_num)) * (GAMES_TOTAL+1);
  srv_countries  = g_malloc0 (i);

  for(g = 0; g < GAMES_TOTAL+1; ++g)
  {
    srv_countries[g].country = (struct country_num*)((void*)srv_countries
	+ sizeof(struct country_stats)*(GAMES_TOTAL+1) + g*geoip_num_countries()*sizeof(struct country_num));
  }
#endif

  // position GAMES_TOTAL is used for total number of all games
  srv_stats = g_malloc0 (sizeof (struct server_stats) * (GAMES_TOTAL+1));

  srv_archs  = g_malloc0 (sizeof (struct arch_stats) * GAMES_TOTAL);
  players  = g_malloc0 (sizeof (struct arch_stats) * GAMES_TOTAL);

  servers_count = 0;
  players_count = 0;
}


static void server_stats_destroy (void) {
  g_free (srv_stats);
  srv_stats = NULL;
  g_free (srv_archs);
  srv_archs = NULL;
#ifdef USE_GEOIP
  g_free(srv_countries);
  srv_countries = NULL;
#endif
}


enum CPU identify_cpu (struct server *s, const char *versionstr) {
  enum CPU cpu = CPU_UNKNOWN;
  char *str;
  
  if (!(str = g_strdup(versionstr)))
  	return CPU_UNKNOWN;
  g_strdown(str);
  
  if (strstr (str, "x86_64") || strstr (str, "amd64") || strstr(str, "x64"))
    cpu = CPU_X86_64;
  else if (strstr (str, "x86") || strstr (str, "i386"))
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

#ifdef USE_GEOIP
static int country_stat_compare_func(const void* va, const void* vb)
{
  const struct country_num* a = va;
  const struct country_num* b = vb;

  if(a->n > b->n)
    return -1;
  else if(a->n == b->n)
    return 0;
  else
    return 1;
}
#endif

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

#ifdef USE_GEOIP
      if(s->country_id >= 0 && s->country_id < geoip_num_countries())
      {
	if(++srv_countries[s->type].country[s->country_id].n == 1)
	{
	  srv_countries[s->type].country[s->country_id].c = s->country_id;
	  ++srv_countries[s->type].nonzero;
	}
	if(++srv_countries[GAMES_TOTAL].country[s->country_id].n == 1)
	{
	  srv_countries[GAMES_TOTAL].country[s->country_id].c = s->country_id;
	  ++srv_countries[GAMES_TOTAL].nonzero;
	}
      }
#endif

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

#ifdef USE_GEOIP
  {
    unsigned g;
    for(g = 0; g < GAMES_TOTAL+1; ++g)
    {
      qsort(srv_countries[g].country, geoip_num_countries(), sizeof(struct country_num), country_stat_compare_func);
    }
  }
#endif
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
  GtkWidget *table;
  GtkWidget *game_label;
  GtkWidget *scrollwin;
  int i;
  int row = 0;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (page_vbox), scrollwin, TRUE, TRUE, 0);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin), alignment);

  table = gtk_table_new (GAMES_TOTAL + 2, 7, FALSE);
  gtk_container_add (GTK_CONTAINER (alignment), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);

  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 20);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 20);
  gtk_table_set_col_spacing (GTK_TABLE (table), 5, 20);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 12);
  gtk_table_set_row_spacing (GTK_TABLE (table), GAMES_TOTAL, 12);

  for (i = 0; i < 6; i++)
    put_label_to_table (table, _(srv_headers[i]), 1.0, i + 1, 0);

  for (i = 0, row = 1; i < GAMES_TOTAL; i++) {
  
    // Skip a game if it's not configured and show only configured is enabled
    if (!games[i].cmd && default_show_only_configured_games)
      continue;
              
    game_label = game_pixmap_with_label (i);
    gtk_table_attach_defaults (GTK_TABLE (table), 
				 game_label, 
				 0, 1, row, row+1);

    put_server_stats (table, i, row);

    {
      srv_stats[GAMES_TOTAL].servers += srv_stats[i].servers;
      srv_stats[GAMES_TOTAL].ok      += srv_stats[i].ok;
      srv_stats[GAMES_TOTAL].timeout += srv_stats[i].timeout;
      srv_stats[GAMES_TOTAL].down    += srv_stats[i].down;
      srv_stats[GAMES_TOTAL].na      += srv_stats[i].na;
      srv_stats[GAMES_TOTAL].players += srv_stats[i].players;
    }
    
    row++;
  }

  put_label_to_table (table, _("Total"), 0.0, 0, row + 1);

  put_server_stats (table, GAMES_TOTAL, row + 1);

  gtk_widget_show (table);
  gtk_widget_show (scrollwin);

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

  table = gtk_table_new (CPU_NUM + 2, OS_NUM + 3, FALSE);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), table, NULL);

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

gboolean create_server_type_menu_filter_hasharch(enum server_type type)
{
    if ((!games[type].cmd && default_show_only_configured_games)
	  || !games[type].arch_identifier)
	return FALSE;
    else
	return TRUE;
}

static void select_server_type_callback(GtkWidget *widget, enum server_type type)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (arch_notebook), srv_archs[type].notebookpage );
  selected_type = type;
}

static GtkWidget *archs_stats_page (void) {
  GtkWidget *page_vbox;
  GtkWidget *alignment;
  GtkWidget *option_menu;
  GtkWidget *hbox;
  int pagenum = 0;
  enum server_type type = Q2_SERVER;
  enum server_type to_activate = UNKNOWN_SERVER;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);


  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);

  arch_notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (arch_notebook), FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (arch_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (arch_notebook), 4);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(arch_notebook), FALSE);
  gtk_container_add (GTK_CONTAINER (alignment), arch_notebook);

  to_activate = config_get_int("/" CONFIG_FILE "/Statistics/game");

  for (type = 0; type < GAMES_TOTAL; ++type)
  {
    if(!create_server_type_menu_filter_hasharch(type))
      continue;

    srv_archs[type].notebookpage=pagenum++;

    arch_notebook_page (arch_notebook, type, &srv_archs[type]);
  }

  // the notebook must exist to allow activate events of the menu
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, TRUE, 0);

    option_menu = create_server_type_menu (to_activate,
		    create_server_type_menu_filter_hasharch,
		    GTK_SIGNAL_FUNC(select_server_type_callback));


    gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, FALSE, 0);
    gtk_widget_show (option_menu);

  gtk_widget_show(hbox);

  gtk_box_pack_start (GTK_BOX (page_vbox), alignment, TRUE, TRUE, 0);

  gtk_widget_show (arch_notebook);
  gtk_widget_show (alignment);
  gtk_widget_show (page_vbox);

  return page_vbox;
}

#ifdef USE_GEOIP
static gboolean create_server_type_menu_filter_hascountries(enum server_type type)
{
  return (srv_countries[type].nonzero != 0);
}

static void country_notebook_page (GtkWidget *notebook, 
                             enum server_type type, struct country_stats *stats) {
  GtkWidget *table;
  GtkWidget *scrollwin;
  GtkWidget *alignment;
  unsigned c;
  char buf[16] = {0};

  scrollwin = gtk_scrolled_window_new (NULL, NULL);

  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin), alignment);

  table = gtk_table_new (stats->nonzero+2, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_table_set_row_spacings (GTK_TABLE (table), 4);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);

  gtk_container_add (GTK_CONTAINER (alignment), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, NULL);

  for(c = 0; c < stats->nonzero; ++c)
  {
    int id;
    unsigned numservers;
    id = stats->country[c].c;
    if(type == UNKNOWN_SERVER)
    {
      numservers = servers_count;
    }
    else
    {
      numservers = srv_stats[type].ok;
    }

    snprintf(buf, sizeof(buf), "%u (%.2f%%)",
	stats->country[c].n,
	PERCENTS(stats->country[c].n, numservers));
    put_label_to_table (table, buf , 1.0, 0, c);

    {
      GtkWidget* label;
      GtkWidget* hbox = gtk_hbox_new (FALSE, 4);
      struct pixmap* pix = get_pixmap_for_country_with_fallback(id);
      if(pix)
      {
	GtkWidget *pixmap = gtk_pixmap_new(pix->pix,pix->mask);
	gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
	gtk_widget_show (pixmap);
      }

      label = gtk_label_new (geoip_name_by_id(id));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      gtk_widget_show (label);

      gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, c, c+1);
      gtk_widget_show (hbox);
    }
  }

  gtk_widget_show(table);
  gtk_widget_show(scrollwin);
  gtk_widget_show(alignment);
}

static void select_country_server_type_callback(GtkWidget *widget, enum server_type type)
{
  gtk_notebook_set_page (GTK_NOTEBOOK (country_notebook), srv_countries[type].notebookpage );
  selected_country = type;
}

static GtkWidget *country_stats_page (void)
{
  GtkWidget *page_vbox;
  GtkWidget *option_menu;
  GtkWidget *hbox;
  int pagenum = 0;
  enum server_type type = Q2_SERVER;
  enum server_type to_activate = UNKNOWN_SERVER;

  page_vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  country_notebook = gtk_notebook_new ();
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (country_notebook), FALSE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (country_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (country_notebook), 4);
  gtk_notebook_set_show_border(GTK_NOTEBOOK(country_notebook), FALSE);

  selected_country = to_activate = config_get_int("/" CONFIG_FILE "/Statistics/country");

  for (type = 0; type <= GAMES_TOTAL; ++type)
  {
    if(!srv_countries[type].nonzero)
      continue;

    srv_countries[type].notebookpage=pagenum++;

    country_notebook_page (country_notebook, type, &srv_countries[type]);
  }

  // the notebook must exist to allow activate events of the menu
  hbox = gtk_hbox_new(FALSE,0);
  gtk_box_pack_start (GTK_BOX (page_vbox), hbox, FALSE, TRUE, 0);

    option_menu = create_server_type_menu (to_activate==GAMES_TOTAL?-1:to_activate,
		    create_server_type_menu_filter_hascountries,
		    GTK_SIGNAL_FUNC(select_country_server_type_callback));
    {
      GtkWidget* menu_item = gtk_menu_item_new ();
      GtkWidget* label = gtk_label_new(_("All Games"));
      GtkWidget* menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(option_menu));

      // separator
      gtk_widget_set_sensitive (menu_item, FALSE);
      gtk_menu_prepend (GTK_MENU (menu), menu_item);
      gtk_widget_show (menu_item);
      
      menu_item = gtk_menu_item_new ();
      gtk_menu_prepend (GTK_MENU (menu), menu_item);
      gtk_container_add (GTK_CONTAINER (menu_item), label);

      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
	    GTK_SIGNAL_FUNC (select_country_server_type_callback), (gpointer)GAMES_TOTAL);

      gtk_widget_show (menu_item);
      gtk_widget_show (label);

      if(to_activate == GAMES_TOTAL)
      {
	gtk_menu_item_activate (GTK_MENU_ITEM (menu_item)); 
	gtk_option_menu_set_history (GTK_OPTION_MENU (option_menu), 0);
      }
    }

    gtk_box_pack_start (GTK_BOX (hbox), option_menu, TRUE, FALSE, 0);
    gtk_widget_show (option_menu);

  gtk_widget_show(hbox);

  gtk_box_pack_start (GTK_BOX (page_vbox), country_notebook, TRUE, TRUE, 0);

  gtk_widget_show (country_notebook);
  gtk_widget_show (page_vbox);

  return page_vbox;
}
#endif

static void grab_defaults (GtkWidget *w, gpointer data)
{
  config_set_int ("/" CONFIG_FILE "/Statistics/country", selected_country);
  config_set_int ("/" CONFIG_FILE "/Statistics/game", selected_type);

  config_set_int ("/" CONFIG_FILE "/Statistics/page", 
        gtk_notebook_get_current_page (GTK_NOTEBOOK (stat_notebook)));
}

static void statistics_save_geometry (GtkWidget *window, gpointer data) {
  config_push_prefix ("/" CONFIG_FILE "/Statistics Window Geometry/");
  config_set_int ("height", window->allocation.height);
  config_set_int ("width", window->allocation.width);
  config_pop_prefix ();
}

static void statistics_restore_geometry (GtkWidget *window) {
  int height, width;

  config_push_prefix ("/" CONFIG_FILE "/Statistics Window Geometry/");

  height = config_get_int ("height");
  width = config_get_int ("width");
  if(!width || !height)
    return;

  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  config_pop_prefix ();
}

void statistics_dialog (void) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *page;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;
  int page_num = 0;

  server_stats_create ();
  collect_statistics ();

  window = dialog_create_modal_transient_window (_("Statistics"), 
				   TRUE, TRUE, GTK_SIGNAL_FUNC(statistics_save_geometry));

  statistics_restore_geometry(window);

  main_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 8);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  label = gtk_label_new (_("Statistics"));
  gtk_box_pack_start (GTK_BOX (main_vbox), label, FALSE, FALSE, 8);
  gtk_widget_show (label);

  stat_notebook = gtk_notebook_new ();
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (stat_notebook), GTK_POS_TOP);
  gtk_notebook_set_tab_hborder (GTK_NOTEBOOK (stat_notebook), 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), stat_notebook, TRUE, TRUE, 0);

  page = server_stats_page ();
  label = gtk_label_new (_(srv_label));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

  page = archs_stats_page ();
  label = gtk_label_new (_(arch_label));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

#ifdef USE_GEOIP
  page = country_stats_page ();
  label = gtk_label_new (_(country_label));
  gtk_widget_show (label);
  gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);
#endif

  page_num = config_get_int ("/" CONFIG_FILE "/Statistics/page");

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
