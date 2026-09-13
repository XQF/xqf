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
#include <stdlib.h>
#include <string.h>     /* memset, strcmp */
#include <sys/socket.h> /* inet_ntoa */
#include <netinet/in.h> /* inet_ntoa */
#include <arpa/inet.h>  /* inet_ntoa */

#include <glib.h>
#include <glib/gi18n.h>

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

#define PERCENTS(A,B) ((B)? (A)/((B)/100.0) : 0)

#define OS_NUM  5
#define CPU_NUM 6


enum OS { OS_WINDOWS = 0, OS_LINUX, OS_SOLARIS, OS_MACOS, OS_UNKNOWN };
enum CPU { CPU_X86 = 0, CPU_X86_64, CPU_SPARC, CPU_AXP, CPU_PPC, CPU_UNKNOWN };

struct server_stats {
	int players;
	int servers;

	int ok;
	int down;
	int timeout;
	int na;
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
	// The space behind up and down is to make the strings different from
	// those in flt-player.c because their meaning is different in german
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


static void server_stats_create (void) {
#ifdef USE_GEOIP
	unsigned g;

	// HACK: position UNKNOWN_SERVER is used for total number of all games
	unsigned i = (sizeof(struct country_stats) + geoip_num_countries()*sizeof(struct country_num)) * (UNKNOWN_SERVER + 1);

	srv_countries  = g_malloc0 (i);

	// HACK: position UNKNOWN_SERVER is used for total number of all games
	for (g = KNOWN_SERVER_START; g <= UNKNOWN_SERVER; ++g) {
		srv_countries[g].country = (struct country_num*)((void*)srv_countries
				+ sizeof(struct country_stats)*(UNKNOWN_SERVER + 1) + g*geoip_num_countries()*sizeof(struct country_num));
	}
#endif

	// HACK: position UNKNOWN_SERVER is used for total number of all games
	srv_stats = g_malloc0 (sizeof (struct server_stats) * (UNKNOWN_SERVER + 1));

	srv_archs  = g_malloc0 (sizeof (struct arch_stats) * UNKNOWN_SERVER);
	players  = g_malloc0 (sizeof (struct arch_stats) * UNKNOWN_SERVER);

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
	gchar *str;

	str = g_ascii_strdown(versionstr, -1);  /* g_ascii_strdown does implicit strndup */
	if (!str)
		return CPU_UNKNOWN;

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
	gchar *str;

	str = g_ascii_strdown(versionstr, -1);  /* g_ascii_strdown does implicit strndup */
	if (!str)
		return OS_UNKNOWN;

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

enum OS t2_identify_os (struct server *s, char *versionstr) {
	if (!strcmp(versionstr,"1"))
		return OS_LINUX;

	return OS_WINDOWS;
}

#ifdef USE_GEOIP
static int country_stat_compare_func(const void* va, const void* vb) {
	const struct country_num* a = va;
	const struct country_num* b = vb;

	if (a->n > b->n) {
		return -1;
	}
	else if (a->n == b->n) {
		return 0;
	}
	else {
		return 1;
	}
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
	int count_players;

	servers = all_servers (); /* Free at end of this function */

	if (servers) {
		for (tmp = servers; tmp; tmp = tmp->next) {
			s = (struct server *) tmp->data;
			info = s->info;
			cpu = CPU_UNKNOWN;
			os = OS_UNKNOWN;
			countthisserver=0;

			servers_count++;

			if (serverlist_countbots) {
			    count_players = s->curplayers;
			} else {
			    count_players = s->curplayers - s->curbots;
			}

			srv_stats[s->type].servers++;
			srv_stats[s->type].players += count_players;

			players_count += count_players;

			if (s->ping < MAX_PING) {
				if (s->ping >= 0) {
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
			if (s->country_id >= 0 && s->country_id < geoip_num_countries()) {
				if (++srv_countries[s->type].country[s->country_id].n == 1) {
					srv_countries[s->type].country[s->country_id].c = s->country_id;
					++srv_countries[s->type].nonzero;
				}
				// HACK: position UNKNOWN_SERVER is used for total number of all games
				if (++srv_countries[UNKNOWN_SERVER].country[s->country_id].n == 1) {
					srv_countries[UNKNOWN_SERVER].country[s->country_id].c = s->country_id;
					++srv_countries[UNKNOWN_SERVER].nonzero;
				}
			}
#endif

			if (info && games[s->type].arch_identifier) {
				while (info[0]) {
					if (g_ascii_strcasecmp (info[0], games[s->type].arch_identifier) == 0) {
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

				if (countthisserver) {
					srv_archs[s->type].oscpu[os][cpu]++;
					srv_archs[s->type].count++;
					players[s->type].on_os[os] += count_players;
					players[s->type].total += count_players;
				}
			}
		}

		server_list_free (servers);
	}

#ifdef USE_GEOIP
	{
		unsigned g;
		// HACK: position UNKNOWN_SERVER is used for total number of all games
		for (g = KNOWN_SERVER_START; g <= UNKNOWN_SERVER; ++g) {
			qsort(srv_countries[g].country, geoip_num_countries(), sizeof(struct country_num), country_stat_compare_func);
		}
	}
#endif
}


/* GtkColumnView row item: a bare GObject carrying display data via
 * g_object_get/set_data(), same idiom already used for the "All Games"
 * entry in country_stats_page() and for pref.c's custom-args list.
 *
 *   "name"  -> gchar* (owned)        display label
 *   "icon"  -> GdkTexture* (borrowed, same lifetime as games[]/pixmap cache)
 *   "bold"  -> gboolean via GINT_TO_POINTER, for the Servers "Total" row
 *   "stats" -> gchar** (owned, NULL-terminated, freed with g_strfreev)
 */

/* "Game"/"Country" column: an icon + label, built once in setup and
 * updated in bind so list-item recycling doesn't reallocate widgets. */
static void icon_label_col_setup_cb (GtkListItemFactory *f, GtkListItem *item, gpointer d) {
	(void)f; (void)d;
	GtkWidget *hbox  = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	GtkWidget *image = gtk_image_new ();
	GtkWidget *label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0f);
	gtk_box_append (GTK_BOX (hbox), image);
	gtk_box_append (GTK_BOX (hbox), label);
	gtk_list_item_set_child (item, hbox);
}

static void icon_label_col_bind_cb (GtkListItemFactory *f, GtkListItem *item, gpointer d) {
	(void)f; (void)d;
	GObject   *row   = G_OBJECT (gtk_list_item_get_item (item));
	GtkWidget *hbox  = gtk_list_item_get_child (item);
	GtkWidget *image = gtk_widget_get_first_child (hbox);
	GtkWidget *label = gtk_widget_get_next_sibling (image);

	GdkTexture *icon = g_object_get_data (row, "icon");
	const char *name = g_object_get_data (row, "name");
	gboolean bold = GPOINTER_TO_INT (g_object_get_data (row, "bold"));

	gtk_widget_set_visible (image, icon != NULL);
	if (icon)
		gtk_image_set_from_paintable (GTK_IMAGE (image), GDK_PAINTABLE (icon));

	if (bold) {
		gchar *markup = g_markup_printf_escaped ("<b>%s</b>", name);
		gtk_label_set_markup (GTK_LABEL (label), markup);
		g_free (markup);
	}
	else {
		gtk_label_set_text (GTK_LABEL (label), name);
	}
}

/* Numeric stat column: plain right-aligned label, one column per index
 * into the row's "stats" array (passed as the bind callback's user_data). */
static void stat_col_setup_cb (GtkListItemFactory *f, GtkListItem *item, gpointer d) {
	(void)f; (void)d;
	GtkWidget *label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 1.0f);
	gtk_list_item_set_child (item, label);
}

static void stat_col_bind_cb (GtkListItemFactory *f, GtkListItem *item, gpointer d) {
	(void)f;
	int idx = GPOINTER_TO_INT (d);
	GObject *row = G_OBJECT (gtk_list_item_get_item (item));
	gchar **stats = g_object_get_data (row, "stats");
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), stats[idx]);
}

/* Build one Servers-tab row. icon may be NULL (Total row); bold marks the
 * Total row for emphasis. */
static GObject *make_server_stats_row (const char *name, GdkTexture *icon,
                                        gboolean bold, struct server_stats *st) {
	GObject *row = g_object_new (G_TYPE_OBJECT, NULL);

	g_object_set_data_full (row, "name", g_strdup (name), g_free);
	if (icon)
		g_object_set_data (row, "icon", icon);
	if (bold)
		g_object_set_data (row, "bold", GINT_TO_POINTER (1));

	gchar **stats = g_new0 (gchar *, 7);
	stats[0] = g_strdup_printf ("%d (%.2f%%)", st->servers, PERCENTS (st->servers, servers_count));
	stats[1] = g_strdup_printf ("%d (%.2f%%)", st->ok,      PERCENTS (st->ok, st->servers));
	stats[2] = g_strdup_printf ("%d (%.2f%%)", st->timeout, PERCENTS (st->timeout, st->servers));
	stats[3] = g_strdup_printf ("%d (%.2f%%)", st->down,    PERCENTS (st->down, st->servers));
	stats[4] = g_strdup_printf ("%d (%.2f%%)", st->na,      PERCENTS (st->na, st->servers));
	stats[5] = g_strdup_printf ("%d (%.2f%%)", st->players, PERCENTS (st->players, players_count));
	g_object_set_data_full (row, "stats", stats, (GDestroyNotify) g_strfreev);

	return row;
}


static GtkWidget *server_stats_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *scrollwin;
	GtkWidget *column_view;
	GListStore *store;
	int i;

	page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	xqf_widget_set_margin_all (page_vbox, 8);
	gtk_widget_set_hexpand (page_vbox, TRUE);
	gtk_widget_set_vexpand (page_vbox, TRUE);

	store = g_list_store_new (G_TYPE_OBJECT);

	for (i = KNOWN_SERVER_START; i < UNKNOWN_SERVER; i++) {

		// Skip a game if it's not configured and show only configured is enabled
		if (!games[i].cmd && default_show_only_configured_games)
			continue;

		GObject *row = make_server_stats_row (_(games[i].name),
				games[i].pix ? games[i].pix->texture : NULL, FALSE, &srv_stats[i]);
		g_list_store_append (store, row);
		g_object_unref (row);

		// HACK: position UNKNOWN_SERVER is used for total number of all games
		srv_stats[UNKNOWN_SERVER].servers += srv_stats[i].servers;
		srv_stats[UNKNOWN_SERVER].ok      += srv_stats[i].ok;
		srv_stats[UNKNOWN_SERVER].timeout += srv_stats[i].timeout;
		srv_stats[UNKNOWN_SERVER].down    += srv_stats[i].down;
		srv_stats[UNKNOWN_SERVER].na      += srv_stats[i].na;
		srv_stats[UNKNOWN_SERVER].players += srv_stats[i].players;
	}

	{
		GObject *total_row = make_server_stats_row (_("Total"), NULL, TRUE, &srv_stats[UNKNOWN_SERVER]);
		g_list_store_append (store, total_row);
		g_object_unref (total_row);
	}

	column_view = gtk_column_view_new (GTK_SELECTION_MODEL (gtk_no_selection_new (G_LIST_MODEL (store))));
	gtk_widget_set_hexpand (column_view, TRUE);
	gtk_widget_set_vexpand (column_view, TRUE);

	{
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (icon_label_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (icon_label_col_bind_cb),  NULL);
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("Game"), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}

	for (i = 0; i < 6; i++) {
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (stat_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (stat_col_bind_cb),  GINT_TO_POINTER (i));
		GtkColumnViewColumn *col = gtk_column_view_column_new (_(srv_headers[i]), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}

	scrollwin = gtk_scrolled_window_new();
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	/* Size the window to the columns' natural (unscrolled) width; only
	 * fall back to horizontal scrolling if that's wider than the screen. */
	gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scrollwin), TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), column_view);
	gtk_box_append (GTK_BOX (page_vbox), scrollwin);

	gtk_widget_set_visible (column_view, TRUE);
	gtk_widget_set_visible (scrollwin, TRUE);

	gtk_widget_set_visible (page_vbox, TRUE);

	return page_vbox;
}


/* Build one CPU/OS-tab row: an OS_NUM-wide breakdown plus a trailing
 * "Total" column (same 6-column shape as the Servers tab, reusing
 * stat_col_setup_cb/bind_cb). bold marks the summary "Total"/"Players"
 * rows. */
static GObject *make_arch_stats_row (const char *name, gboolean bold,
                                     const int *values, int base,
                                     int total_val, int total_base) {
	GObject *row = g_object_new (G_TYPE_OBJECT, NULL);
	int i;

	g_object_set_data_full (row, "name", g_strdup (name), g_free);
	if (bold)
		g_object_set_data (row, "bold", GINT_TO_POINTER (1));

	gchar **stats = g_new0 (gchar *, OS_NUM + 2);
	for (i = 0; i < OS_NUM; i++)
		stats[i] = g_strdup_printf ("%d (%.2f%%)", values[i], PERCENTS (values[i], base));
	stats[OS_NUM] = g_strdup_printf ("%d (%.2f%%)", total_val, PERCENTS (total_val, total_base));
	g_object_set_data_full (row, "stats", stats, (GDestroyNotify) g_strfreev);

	return row;
}

static void arch_notebook_page (GtkWidget *notebook,
		enum server_type type, struct arch_stats *arch) {
	GtkWidget *scrollwin;
	GtkWidget *column_view;
	GListStore *store;
	int i, j;

	store = g_list_store_new (G_TYPE_OBJECT);

	for (j = 0; j < CPU_NUM; j++) {
		int values[OS_NUM];
		int cpu_total = 0;

		for (i = 0; i < OS_NUM; i++) {
			values[i] = arch->oscpu[i][j];
			cpu_total += arch->oscpu[i][j];
			if (j > 0)
				arch->oscpu[i][0] += arch->oscpu[i][j];
		}

		GObject *row = make_arch_stats_row (_(cpu_names[j]), FALSE,
				values, arch->count, cpu_total, arch->count);
		g_list_store_append (store, row);
		g_object_unref (row);
	}

	{
		int values[OS_NUM];
		for (i = 0; i < OS_NUM; i++)
			values[i] = arch->oscpu[i][0];
		GObject *row = make_arch_stats_row (_("Total"), TRUE,
				values, arch->count, arch->count, arch->count);
		g_list_store_append (store, row);
		g_object_unref (row);
	}
	{
		int values[OS_NUM];
		for (i = 0; i < OS_NUM; i++)
			values[i] = players[type].on_os[i];
		GObject *row = make_arch_stats_row (_("Players"), TRUE,
				values, players[type].total, srv_stats[type].players, players_count);
		g_list_store_append (store, row);
		g_object_unref (row);
	}

	column_view = gtk_column_view_new (GTK_SELECTION_MODEL (gtk_no_selection_new (G_LIST_MODEL (store))));
	gtk_widget_set_hexpand (column_view, TRUE);
	gtk_widget_set_vexpand (column_view, TRUE);

	{
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (icon_label_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (icon_label_col_bind_cb),  NULL);
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("CPU"), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_column_set_expand (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}

	for (i = 0; i < OS_NUM; i++) {
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (stat_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (stat_col_bind_cb),  GINT_TO_POINTER (i));
		GtkColumnViewColumn *col = gtk_column_view_column_new (_(os_names[i]), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}
	{
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (stat_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (stat_col_bind_cb),  GINT_TO_POINTER (OS_NUM));
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("Total"), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}

	scrollwin = gtk_scrolled_window_new();
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scrollwin), TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), column_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, NULL);

	gtk_widget_set_visible (column_view, TRUE);
	gtk_widget_set_visible (scrollwin, TRUE);
}

gboolean create_server_type_menu_filter_hasharch(enum server_type type) {
	if ((!games[type].cmd && default_show_only_configured_games)
			|| !games[type].arch_identifier)
		return FALSE;
	else
		return TRUE;
}

static void select_server_type_callback(GtkWidget *widget, enum server_type type) {
	gtk_notebook_set_current_page (GTK_NOTEBOOK (arch_notebook), srv_archs[type].notebookpage);
	selected_type = type;
}

static GtkWidget *archs_stats_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *option_menu;
	GtkWidget *hbox;
	int pagenum = 0;
	enum server_type type = Q2_SERVER;
	enum server_type to_activate = UNKNOWN_SERVER;

	page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	xqf_widget_set_margin_all (page_vbox, 8);
	gtk_widget_set_hexpand (page_vbox, TRUE);
	gtk_widget_set_vexpand (page_vbox, TRUE);

	arch_notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (arch_notebook), FALSE);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (arch_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(arch_notebook), FALSE);
	gtk_widget_set_hexpand (arch_notebook, TRUE);
	gtk_widget_set_vexpand (arch_notebook, TRUE);

	to_activate = config_get_int("/" CONFIG_FILE "/Statistics/game");

	for (type = KNOWN_SERVER_START; type < UNKNOWN_SERVER; ++type) {
		if (!create_server_type_menu_filter_hasharch(type))
			continue;

		srv_archs[type].notebookpage=pagenum++;

		arch_notebook_page (arch_notebook, type, &srv_archs[type]);
	}

	// the notebook must exist to allow activate events of the menu
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (GTK_BOX (page_vbox), hbox);

	option_menu = create_server_type_menu (to_activate,
			create_server_type_menu_filter_hasharch,
			G_CALLBACK(select_server_type_callback));


	gtk_box_append (GTK_BOX (hbox), option_menu);
	gtk_widget_set_visible (option_menu, TRUE);

	gtk_widget_set_visible (hbox, TRUE);

	gtk_box_append (GTK_BOX (page_vbox), arch_notebook);

	gtk_widget_set_visible (arch_notebook, TRUE);
	gtk_widget_set_visible (page_vbox, TRUE);

	return page_vbox;
}

#ifdef USE_GEOIP
static gboolean create_server_type_menu_filter_hascountries(enum server_type type) {
	return (srv_countries[type].nonzero != 0);
}

static void country_notebook_page (GtkWidget *notebook,
		enum server_type type, struct country_stats *stats) {
	GtkWidget *scrollwin;
	GtkWidget *column_view;
	GListStore *store;
	unsigned c;

	store = g_list_store_new (G_TYPE_OBJECT);

	for (c = 0; c < stats->nonzero; ++c) {
		int id = stats->country[c].c;
		unsigned numservers = (type == UNKNOWN_SERVER) ? servers_count : srv_stats[type].ok;
		struct pixmap *pix = get_pixmap_for_country_with_fallback (id);

		GObject *row = g_object_new (G_TYPE_OBJECT, NULL);
		g_object_set_data_full (row, "name", g_strdup (geoip_name_by_id (id)), g_free);
		if (pix && pix->texture)
			g_object_set_data (row, "icon", pix->texture);

		gchar **count = g_new0 (gchar *, 2);
		count[0] = g_strdup_printf ("%u (%.2f%%)", stats->country[c].n,
				PERCENTS (stats->country[c].n, numservers));
		g_object_set_data_full (row, "stats", count, (GDestroyNotify) g_strfreev);

		g_list_store_append (store, row);
		g_object_unref (row);
	}

	column_view = gtk_column_view_new (GTK_SELECTION_MODEL (gtk_no_selection_new (G_LIST_MODEL (store))));
	gtk_widget_set_hexpand (column_view, TRUE);
	gtk_widget_set_vexpand (column_view, TRUE);

	{
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (stat_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (stat_col_bind_cb),  GINT_TO_POINTER (0));
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("Count"), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}
	{
		GtkListItemFactory *f = gtk_signal_list_item_factory_new ();
		g_signal_connect (f, "setup", G_CALLBACK (icon_label_col_setup_cb), NULL);
		g_signal_connect (f, "bind",  G_CALLBACK (icon_label_col_bind_cb),  NULL);
		GtkColumnViewColumn *col = gtk_column_view_column_new (_("Country"), f);
		gtk_column_view_column_set_resizable (col, TRUE);
		/* This tab is only 2 columns wide, but the notebook (and so this
		 * page) is sized to the widest tab (Servers, 7 columns) -- expand
		 * this column so the header reaches the right edge instead of
		 * leaving a bare gap. */
		gtk_column_view_column_set_expand (col, TRUE);
		gtk_column_view_append_column (GTK_COLUMN_VIEW (column_view), col);
		g_object_unref (col);
	}

	scrollwin = gtk_scrolled_window_new();
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	gtk_scrolled_window_set_propagate_natural_width (GTK_SCROLLED_WINDOW (scrollwin), TRUE);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrollwin), column_view);

	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), scrollwin, NULL);

	gtk_widget_set_visible (column_view, TRUE);
	gtk_widget_set_visible (scrollwin, TRUE);
}

static void select_country_server_type_callback(GtkWidget *widget, enum server_type type) {
	gtk_notebook_set_current_page (GTK_NOTEBOOK (country_notebook), srv_countries[type].notebookpage);
	selected_country = type;
}

static GtkWidget *country_stats_page (void) {
	GtkWidget *page_vbox;
	GtkWidget *option_menu;
	GtkWidget *hbox;
	int pagenum = 0;
	enum server_type type = Q2_SERVER;
	enum server_type to_activate = UNKNOWN_SERVER;

	page_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	xqf_widget_set_margin_all (page_vbox, 8);
	gtk_widget_set_hexpand (page_vbox, TRUE);
	gtk_widget_set_vexpand (page_vbox, TRUE);

	country_notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (country_notebook), FALSE);
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (country_notebook), GTK_POS_TOP);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(country_notebook), FALSE);
	gtk_widget_set_hexpand (country_notebook, TRUE);
	gtk_widget_set_vexpand (country_notebook, TRUE);

	selected_country = to_activate = config_get_int("/" CONFIG_FILE "/Statistics/country");

	// HACK: position UNKNOWN_SERVER is used for total number of all games
	for (type = KNOWN_SERVER_START; type <= UNKNOWN_SERVER; ++type) {
		if (!srv_countries[type].nonzero)
			continue;

		srv_countries[type].notebookpage=pagenum++;

		country_notebook_page (country_notebook, type, &srv_countries[type]);
	}

	// the notebook must exist to allow activate events of the menu
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (GTK_BOX (page_vbox), hbox);

	option_menu = create_server_type_menu (to_activate == UNKNOWN_SERVER?-1:to_activate,
			create_server_type_menu_filter_hascountries,
			G_CALLBACK(select_country_server_type_callback));
	{
		GListStore *store = G_LIST_STORE (gtk_drop_down_get_model (GTK_DROP_DOWN (option_menu)));
		GObject *item = g_object_new (G_TYPE_OBJECT, NULL);
		g_object_set_data (item, "server-type", GINT_TO_POINTER (UNKNOWN_SERVER));
		g_object_set_data (item, "name", _("All Games"));
		g_list_store_insert (store, 0, item);
		g_object_unref (item);

		if (to_activate == UNKNOWN_SERVER)
			gtk_drop_down_set_selected (GTK_DROP_DOWN (option_menu), 0);
	}

	gtk_box_append (GTK_BOX (hbox), option_menu);
	gtk_widget_set_visible (option_menu, TRUE);

	gtk_widget_set_visible (hbox, TRUE);

	gtk_box_append (GTK_BOX (page_vbox), country_notebook);

	gtk_widget_set_visible (country_notebook, TRUE);
	gtk_widget_set_visible (page_vbox, TRUE);

	return page_vbox;
}
#endif

static void grab_defaults (GtkWidget *w, gpointer data) {
	config_set_int ("/" CONFIG_FILE "/Statistics/country", selected_country);
	config_set_int ("/" CONFIG_FILE "/Statistics/game", selected_type);

	config_set_int ("/" CONFIG_FILE "/Statistics/page",
			gtk_notebook_get_current_page (GTK_NOTEBOOK (stat_notebook)));
}

static void statistics_save_geometry (GtkWidget *window, gpointer data) {
	config_push_prefix ("/" CONFIG_FILE "/Statistics Window Geometry/");
	config_set_int ("height", gtk_widget_get_height (window));
	config_set_int ("width", gtk_widget_get_width (window));
	config_pop_prefix ();
}

static void statistics_restore_geometry (GtkWidget *window) {
	int height, width;

	config_push_prefix ("/" CONFIG_FILE "/Statistics Window Geometry/");
	height = config_get_int ("height");
	width  = config_get_int ("width");
	config_pop_prefix ();

	if (!height)
		height = 750;

	/* Width is left to the columns' natural size (see
	 * gtk_scrolled_window_set_propagate_natural_width() in
	 * server_stats_page()/country_notebook_page()) unless the user
	 * resized the window before, in which case that's respected. */
	gtk_window_set_default_size (GTK_WINDOW (window), width ? width : -1, height);
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
			TRUE, TRUE, G_CALLBACK(statistics_save_geometry));

	statistics_restore_geometry(window);

	main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
	xqf_widget_set_margin_all (main_vbox, 8);
	gtk_window_set_child (GTK_WINDOW (window), main_vbox);

	label = gtk_label_new (_("Statistics"));
	gtk_box_append (GTK_BOX (main_vbox), label);
	gtk_widget_set_visible (label, TRUE);

	stat_notebook = gtk_notebook_new ();
	gtk_notebook_set_tab_pos (GTK_NOTEBOOK (stat_notebook), GTK_POS_TOP);
	gtk_widget_set_hexpand (stat_notebook, TRUE);
	gtk_widget_set_vexpand (stat_notebook, TRUE);
	gtk_box_append (GTK_BOX (main_vbox), stat_notebook);

	page = server_stats_page ();
	label = gtk_label_new (_(srv_label));
	gtk_widget_set_visible (label, TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

	page = archs_stats_page ();
	label = gtk_label_new (_(arch_label));
	gtk_widget_set_visible (label, TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);

#ifdef USE_GEOIP
	page = country_stats_page ();
	label = gtk_label_new (_(country_label));
	gtk_widget_set_visible (label, TRUE);
	gtk_notebook_append_page (GTK_NOTEBOOK (stat_notebook), page, label);
#endif

	page_num = config_get_int ("/" CONFIG_FILE "/Statistics/page");

	gtk_notebook_set_current_page (GTK_NOTEBOOK (stat_notebook), page_num);

	gtk_widget_set_visible (stat_notebook, TRUE);

	/* Close Button */

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
	gtk_box_append (GTK_BOX (main_vbox), hbox);

	button = gtk_button_new_with_label (_("Close"));
	gtk_box_append (GTK_BOX (hbox), button);
	gtk_widget_set_size_request (button, 80, -1);
	g_signal_connect (button, "clicked", G_CALLBACK (grab_defaults), NULL);
	g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_window_destroy), GTK_WINDOW (window));
	gtk_widget_set_visible (button, TRUE);

	gtk_widget_set_visible (hbox, TRUE);
	gtk_widget_set_visible (main_vbox, TRUE);
	gtk_widget_set_visible (window, TRUE);

	dialog_run_modal (window);

	unregister_window (window);

	server_stats_destroy ();
}
