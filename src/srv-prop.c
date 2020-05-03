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

#include <sys/types.h>  /* chmod */
#include <stdio.h>      /* FILE, fprintf, etc... */
#include <string.h>     /* strchr, strcmp */
#include <unistd.h>     /* unlink */
#include <sys/stat.h>   /* chmod */
#include <time.h>       /* ctime */
#include <sys/socket.h> /* inet_ntoa */
#include <netinet/in.h> /* inet_ntoa */
#include <arpa/inet.h>  /* inet_ntoa */
#include <stdlib.h>     /* atoi */

#include <glib.h>
#include <glib/gi18n.h>

#include "xqf-ui.h"
#include "game.h"
#include "pref.h"
#include "source.h"
#include "host.h"
#include "server.h"
#include "pixmaps.h"
#include "utils.h"
#include "dialogs.h"
#include "srv-prop.h"
#include "country-filter.h"

static  GtkWidget *password_entry;
static  GtkWidget *spectator_entry;
static  GtkWidget *rcon_entry;
static  GtkWidget *customcfg_combo;
static  GtkWidget *sucks_check_button;
static  GtkWidget *comment_text;

static GtkTextBuffer *comment_text_buffer;
GtkTextIter start, end;

/*pulp*/
static  GtkWidget *spinner;
static  GtkAdjustment *adj;

static GSList *props_list = NULL;


static void props_free (struct server_props *p) {
	if (p) {
		if (p->custom_cfg) g_free (p->custom_cfg);

		if (p->server_password) g_free (p->server_password);
		if (p->spectator_password) g_free (p->spectator_password);
		if (p->rcon_password) g_free (p->rcon_password);

		g_free (p->comment);

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

	if (!h || p == 0 /* || p > 65535 */)
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
	p->reserved_slots =0;
	p->sucks = 0;
	p->comment = NULL;

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

		if (p->custom_cfg || p->server_password || p->spectator_password || p->rcon_password || p->reserved_slots
				|| p->sucks || (p->comment && strlen(p->comment))) {
			fprintf (f, "[%s:%d]\n", inet_ntoa (p->host->ip), p->port);

			if (p->custom_cfg)
				fprintf (f, "custom_cfg %s\n", p->custom_cfg);
			if (p->server_password)
				fprintf (f, "password %s\n", p->server_password);
			if (p->spectator_password)
				fprintf (f, "spectator_password %s\n", p->spectator_password);
			if (p->rcon_password)
				fprintf (f, "rcon_password %s\n", p->rcon_password);
			if (p->reserved_slots)
				fprintf (f, "reserved_slots %d\n", p->reserved_slots);
			if (p->sucks)
				fprintf (f, "sucks %d\n", p->sucks);

			if (p->comment && strlen(p->comment)) {
				char* s = p->comment;

				// strip trailing \n, otherwise one is added each time the server is saved
				while (s[strlen(s)-1] == '\n')
					s[strlen(s)-1] = '\0';

				fputs("comment ", f);
				while (*s) {
					switch(*s) {
						case '\n':
							fputs("\\n", f);
							break;
						case '\\':
							fputs("\\\\", f);
							break;
						default:
							fputc(*s, f);
							break;
					}
					++s;
				}
				fputc('\n', f);
			}

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
				g_free (p->custom_cfg);
				p->custom_cfg = strdup_strip (ptr);
			}
			else if (strcmp (buf, "password") == 0) {
				g_free (p->server_password);
				p->server_password = strdup_strip (ptr);
			}
			else if (strcmp (buf, "spectator_password") == 0) {
				g_free (p->spectator_password);
				p->spectator_password = strdup_strip (ptr);
			}
			else if (strcmp (buf, "rcon_password") == 0) {
				g_free (p->rcon_password);
				p->rcon_password = strdup_strip (ptr);
			}
			else if (strcmp (buf, "reserved_slots") == 0) {
				p->reserved_slots= atoi(ptr);
			}
			else if (strcmp (buf, "sucks") == 0) {
				p->sucks = atoi(ptr);
			}
			else if (strcmp (buf, "comment") == 0) {
				unsigned di, si;
				size_t slen = strlen(ptr);
				int quote = 0;

				g_free (p->comment);
				p->comment = g_malloc0 (slen + 1);
				for (si = 0, di = 0; si < slen; ++si) {
					if (quote) {
						quote = 0;
						if (ptr[si] == 'n')
							p->comment[di++] = '\n';
						else if (ptr[si] == '\\')
							p->comment[di++] = '\\';
						else
							xqf_warning("unknown control sequence \\%c", ptr[si]);
					}
					else if (ptr[si] == '\\') {
						quote = 1;
					}
					else {
						p->comment[di++] = ptr[si];
					}
				}
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
	int   reserved;
	int   sucks;
	char* comment = NULL;

	customcfg = strdup_strip (gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (customcfg_combo)->entry)));

	srvpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (password_entry)));
	spectpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (spectator_entry)));
	rconpwd = strdup_strip (gtk_entry_get_text (GTK_ENTRY (rcon_entry)));
	reserved = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (spinner));
	sucks = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (sucks_check_button));

	gtk_text_buffer_get_iter_at_offset (comment_text_buffer, &start, 0);
	gtk_text_buffer_get_iter_at_offset (comment_text_buffer, &end, 12);
	comment = gtk_text_buffer_get_text (comment_text_buffer, &start, &end, FALSE);

	props = properties (s);

	if (props) {
		if (customcfg || srvpwd || spectpwd || rconpwd || reserved || sucks || comment) {
			g_free (props->custom_cfg);
			props->custom_cfg = customcfg;

			g_free (props->server_password);
			props->server_password = srvpwd;

			g_free (props->spectator_password);
			props->spectator_password = spectpwd;

			g_free (props->rcon_password);
			props->rcon_password = rconpwd;

			props->reserved_slots = reserved;

			props->sucks = sucks;

			g_free(props->comment);
			props->comment = comment;
		}
		else {
			props_list = g_slist_remove (props_list, props);
			props_free (props);
		}
	}
	else {
		if (customcfg || srvpwd || spectpwd || rconpwd || reserved || sucks || comment) {
			props = properties_new (s->host, s->port);
			props->custom_cfg = customcfg;
			props->server_password = srvpwd;
			props->spectator_password = spectpwd;
			props->rcon_password = rconpwd;
			props->reserved_slots = reserved;
			props->sucks = sucks;
			props->comment = comment;
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
	int slots_buffer;
	guint row = 0;



	props = properties (s);

	page_vbox = gtk_vbox_new (FALSE, 8);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	/* Address */

	table = gtk_table_new (6, 4, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 4);
	gtk_table_set_col_spacings (GTK_TABLE (table), 8);
	gtk_box_pack_start (GTK_BOX (page_vbox), table, FALSE, FALSE, 0);

	gtk_table_set_col_spacing (GTK_TABLE (table), 1, 16);

	label = gtk_label_new (_("IP Address:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	label = gtk_label_new (inet_ntoa (s->host->ip));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 2, row, row+1);
	gtk_widget_show (label);

	label = gtk_label_new (_("Port:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 2, 3, row, row+1);
	gtk_widget_show (label);

	g_snprintf (buf, 32, "%d", s->port);

	label = gtk_label_new (buf);
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 3, 4, row, row+1);
	gtk_widget_show (label);

	row++;

	label = gtk_label_new (_("Host Name:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	if (s->host->name) {
		label = gtk_label_new (s->host->name);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 4, row, row+1);
		gtk_widget_show (label);
	}

	row++;

	label = gtk_label_new (_("Country:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

#ifdef USE_GEOIP
	if (geoip_name_by_id(s->country_id)) {
		GtkWidget* hbox = gtk_hbox_new (FALSE, 4);
		struct pixmap* pix = get_pixmap_for_country(s->country_id);
		if (pix) {
			GtkWidget *pixmap = gtk_pixmap_new(pix->pix,pix->mask);
			gtk_box_pack_start (GTK_BOX (hbox), pixmap, FALSE, FALSE, 0);
			gtk_widget_show (pixmap);
		}

		label = gtk_label_new (geoip_name_by_id(s->country_id));
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_widget_show (label);

		gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 4, row, row+1);
		gtk_widget_show (hbox);
	}
#endif

	row++;

	label = gtk_label_new (_("Refreshed:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	if (s->refreshed) {
		char* str = timet2string(&s->refreshed);

		label = gtk_label_new (str);
		g_free(str);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 4, row, row+1);
		gtk_widget_show (label);
	}

	row++;

	// translator: last time and date the server answered the query
	label = gtk_label_new (_("Last answer:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);

	if (s->last_answer) {
		GtkStyle *style;
		GdkColor color;
		time_t max_days = 3; // XXX: hardcoded, has to be configurable some time
		char* str = timet2string(&s->last_answer);

		label = gtk_label_new (str);
		g_free(str);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 4, row, row+1);

		if (s->last_answer + max_days*24*60*60 < s->refreshed) {
			// XXX: I don't know if that is the correct way, it's undocumented :-(
			style = gtk_widget_get_style(label);
			gdk_color_parse("red",&color);

			style->fg [GTK_STATE_NORMAL]   = color;
			style->fg [GTK_STATE_ACTIVE]   = color;
			style->fg [GTK_STATE_PRELIGHT] = color;
			style->fg [GTK_STATE_SELECTED] = color;
			style->fg [GTK_STATE_INSENSITIVE] = color;

			gtk_widget_set_style (label, style);
		}

		gtk_widget_show (label);
	}

	row++;

	/*pulp*/ /*Reserved Slots spin widget*/


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



	label = gtk_label_new (_("Reserved Slots:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, row, row+1);
	gtk_widget_show (label);


	adj = (GtkAdjustment *) gtk_adjustment_new (slots_buffer, 0, 9, 1, 2,0);
	spinner = gtk_spin_button_new (adj, 0, 0);
	gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (spinner), TRUE);
	gtk_spin_button_set_update_policy(GTK_SPIN_BUTTON (spinner), GTK_UPDATE_IF_VALID);
	gtk_table_attach_defaults (GTK_TABLE (table), spinner, 1, 2, row, row+1);
	gtk_widget_show (spinner);



	gtk_widget_show (table);

	/* Sources */

	frame = gtk_frame_new (_("Sources"));
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	gtk_container_add (GTK_CONTAINER (frame), vbox);

	sources = references_to_server (s);

	for (list = sources; list; list = list->next) {
		m = (struct master *) list->data;

		label = gtk_label_new (_(m->name));
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

		cfgs = (*games[s->type].custom_cfgs) (&games[s->type], NULL, s->game);

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

	label = gtk_label_new (_("Custom CFG:"));
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

	password_entry = passwd_entry (_("Server Password"), (props)? props->server_password : NULL,
			table, 0);

	if ((games[s->type].flags & GAME_PASSWORD) == 0)
		gtk_widget_set_sensitive (password_entry, FALSE);

	spectator_entry = passwd_entry (_("Spectator Password"),
			(props)? props->spectator_password : NULL,
			table, 1);

	if ((games[s->type].flags & GAME_SPECTATE) == 0)
		gtk_widget_set_sensitive (spectator_entry, FALSE);

	rcon_entry = passwd_entry (_("RCon/Admin Password"),
			(props)? props->rcon_password : NULL,
			table, 2);

	if ((games[s->type].flags & GAME_RCON) == 0 && (games[s->type].flags & GAME_ADMIN) == 0)
		gtk_widget_set_sensitive (rcon_entry, FALSE);

	gtk_widget_show (table);

	gtk_widget_show (page_vbox);

	return page_vbox;
}

static GtkWidget *server_comment_page (struct server *s) {
	GtkWidget *page_vbox;
	GtkWidget *scrollwin;
	struct server_props *props;
	int sucks = 0;
	char* comment = NULL;

	props = properties (s);
	if (props) {
		sucks = props->sucks;
		comment = props->comment;
	}

	page_vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

	sucks_check_button = gtk_check_button_new_with_label (_("This server sucks"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sucks_check_button), sucks);
	gtk_box_pack_start (GTK_BOX (page_vbox), sucks_check_button, FALSE, FALSE, 0);
	gtk_widget_show (sucks_check_button);

	scrollwin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (page_vbox), scrollwin, TRUE, TRUE, 0);


	comment_text_buffer = gtk_text_buffer_new (NULL);
	comment_text = gtk_text_view_new_with_buffer (comment_text_buffer);
	if (comment) {
		gtk_text_buffer_set_text (comment_text_buffer, comment, strlen (comment));
	}
	//  gtk_widget_set_usize (comment_text, -1, 80);
	gtk_container_add (GTK_CONTAINER (scrollwin), comment_text);
	gtk_widget_show (comment_text);

	gtk_widget_show (scrollwin);

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

	window = dialog_create_modal_transient_window (_("Properties"),
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
	label = gtk_label_new (_("Info"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	page = server_passwords_page (s);
	label = gtk_label_new (_("Passwords"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	page = server_comment_page(s);
	label = gtk_label_new (_("Comment"));
	gtk_widget_show (label);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);

	gtk_notebook_set_page (GTK_NOTEBOOK (notebook), 0);

	gtk_widget_show (notebook);

	/*
	 *  Buttons at the bottom
	 */

	hbox = gtk_hbox_new (FALSE, 8);
	gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_label (_("Cancel"));
	gtk_widget_set_usize (button, 80, -1);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_show (button);

	button = gtk_button_new_with_label (_("OK"));
	gtk_widget_set_usize (button, 80, -1);
	gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
	g_signal_connect (GTK_OBJECT (button), "clicked",
			G_CALLBACK (set_new_properties), (gpointer) s);
	g_signal_connect_swapped (GTK_OBJECT (button), "clicked",
			G_CALLBACK (gtk_widget_destroy), GTK_OBJECT (window));
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	gtk_widget_show (hbox);

	gtk_widget_show (main_vbox);

	gtk_widget_show (window);

	gtk_main ();

	unregister_window (window);
}


void combo_set_vals (GtkWidget *combo, GList *strlist, const char *str) {

	g_return_if_fail(GTK_IS_COMBO(combo));

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
	else {
		gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), "");
	}
}
