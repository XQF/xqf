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
#include <regex.h>
#include <string.h>	/* strcmp */

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "srv-list.h"
#include "dialogs.h"
#include "sort.h"
#include "utils.h"
#include "history.h"
#include "config.h"
#include "psearch.h"

#define REGCOMP_FLAGS 	(REG_EXTENDED | REG_NOSUB | REG_ICASE)


static struct psearch_data psearch = { NULL, PSEARCH_MODE_SUBSTR, NULL };

static struct history *psearch_history = NULL;

static GtkWidget *psearch_combo;
static GtkWidget *mode_buttons[3];

static int psearch_new_pattern;

static const char *mode_names[3] = { 
  N_("Exact Match"),
  N_("Substring"),
  N_("Regular Expression")
};


static int psearch_test_player (struct player *p) {
  return
    ((psearch.mode == PSEARCH_MODE_STRING && 
          g_strcasecmp (p->name, psearch.data) == 0) ||
     (psearch.mode == PSEARCH_MODE_SUBSTR && 
          lowcasestrstr (p->name, psearch.data)) ||
     (psearch.mode == PSEARCH_MODE_REGEXP &&
          regexec ((regex_t *) psearch.data, p->name, 0, NULL, 0) == 0));
}


static void psearch_free_pattern_data (void) {
  if (psearch.data) {
    if (psearch.mode == PSEARCH_MODE_REGEXP) {
      regfree ((regex_t *) psearch.data);
    }
    g_free (psearch.data);
    psearch.data = NULL;
  }
}


static void psearch_free_pattern (void) {
  if (psearch.pattern) {
    g_free (psearch.pattern);
    psearch.pattern = NULL;
  }
  psearch_free_pattern_data ();
}


static char *get_regerror (int errcode, regex_t *compiled) {
  size_t length;
  char *buffer;

  length = regerror (errcode, compiled, NULL, 0);
  buffer = g_malloc (length);
  regerror (errcode, compiled, buffer, length);
  return buffer;
}


static int psearch_compile_pattern (void) {
  char *error;
  int res;

  psearch_free_pattern_data ();

  if (psearch.pattern && psearch.pattern[0]) {
    switch (psearch.mode) {

    case PSEARCH_MODE_REGEXP:
      psearch.data = g_malloc (sizeof (regex_t));

      res = regcomp ((regex_t *) psearch.data, psearch.pattern, REGCOMP_FLAGS);
      if (res) {
	error = get_regerror (res, (regex_t *) psearch.data);
	psearch_free_pattern ();

	dialog_ok (_("XQF: Error"), 
                   _("Regular Expression Error!\n\n%s\n\n%s."),
                   psearch.pattern, error);
	g_free (error);
	return FALSE;
      }
      break;

    case PSEARCH_MODE_STRING:
    case PSEARCH_MODE_SUBSTR:
    default:
      psearch.data = g_strdup (psearch.pattern);
      g_strdown (psearch.data);
      break;

    }

    return TRUE;
  }

  return FALSE;
}


static void psearch_combo_activate_callback (GtkWidget *widget, 
                                                              gpointer data) {
  psearch_free_pattern ();

  if (GTK_TOGGLE_BUTTON (mode_buttons[PSEARCH_MODE_STRING])->active)
    psearch.mode = PSEARCH_MODE_STRING;
  else if (GTK_TOGGLE_BUTTON (mode_buttons[PSEARCH_MODE_SUBSTR])->active)
    psearch.mode = PSEARCH_MODE_SUBSTR;
  else
    psearch.mode = PSEARCH_MODE_REGEXP;

  config_set_int ("/" CONFIG_FILE "/Find Player/mode", psearch.mode);

  psearch.pattern = strdup_strip (
           gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (psearch_combo)->entry)));

  if (psearch.pattern && psearch.pattern[0]) {
    history_add (psearch_history, psearch.pattern);
    psearch_new_pattern = psearch_compile_pattern ();
  }
}


int find_player_dialog (void) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *label;
  GSList *group;
  int i;

  psearch_new_pattern = FALSE;

  window = dialog_create_modal_transient_window (_("Find Player"),
                                                           TRUE, FALSE, NULL);
  main_vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 16);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  /* Pattern Entry */

  label = gtk_label_new (_("Find Player:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* ComboBox */

  psearch_combo = gtk_combo_new ();
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (psearch_combo)->entry), 128);
  gtk_widget_set_usize (GTK_COMBO (psearch_combo)->entry, 160, -1);
  gtk_combo_disable_activate (GTK_COMBO (psearch_combo));
  if (psearch_history->items) {
    gtk_combo_set_popdown_strings (GTK_COMBO (psearch_combo), 
                                                      psearch_history->items);
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (psearch_combo)->entry), 
                                       (char *) psearch_history->items->data);
    gtk_entry_select_region (GTK_ENTRY (GTK_COMBO (psearch_combo)->entry), 0, 
                              strlen ((char *) psearch_history->items->data));
  }
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (psearch_combo)->entry), 
         "activate", GTK_SIGNAL_FUNC (psearch_combo_activate_callback), NULL);
  gtk_signal_connect_object (GTK_OBJECT (GTK_COMBO (psearch_combo)->entry), 
       "activate", GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_start (GTK_BOX (hbox), psearch_combo, TRUE, TRUE, 0);
  gtk_widget_grab_focus (GTK_COMBO (psearch_combo)->entry);
  gtk_widget_show (psearch_combo);

  /* OK Button */

  button = gtk_button_new_with_label (_("OK"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
		            GTK_SIGNAL_FUNC (psearch_combo_activate_callback),
			    GTK_OBJECT (psearch_combo));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
	           GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Cancel Button */

  button = gtk_button_new_with_label (_("Cancel"));
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  /* Mode Buttons */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  group = NULL;

  for (i = 0; i < 3; i++) {
    mode_buttons[i] = gtk_radio_button_new_with_label (group,
		                                       _(mode_names[i]));
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (mode_buttons[i]));
    gtk_box_pack_start (GTK_BOX (hbox), mode_buttons[i], FALSE, FALSE, 0);
    gtk_widget_show (mode_buttons[i]);
  }

  gtk_toggle_button_set_active (
                        GTK_TOGGLE_BUTTON (mode_buttons[psearch.mode]), TRUE);

  gtk_widget_show (hbox);

  gtk_widget_show (main_vbox);
  gtk_widget_show (window);

  gtk_main ();

  unregister_window (window);

  return psearch_new_pattern;
}


void psearch_init (void) {
  psearch_history = history_new ("Player Search");
  psearch.mode = config_get_int ("/" CONFIG_FILE "/Find Player/mode=1");
}


void psearch_done (void) {
  if (psearch.pattern) {
    psearch_free_pattern ();
  }
  if (psearch_history) {
    history_free (psearch_history);
    psearch_history = NULL;
  }
}


int psearch_data_is_empty (void) {
  return (psearch.data == NULL);
}


char *psearch_lookup_pattern (void) {
  char *pat;
  int len;

  if (!psearch.pattern || !psearch.pattern[0])
    return NULL;

  len = strlen (psearch.pattern) + 2 + strlen (mode_names[psearch.mode]) + 2;
  pat = g_malloc (len);

  g_snprintf (pat, len, "%s (%s)", psearch.pattern, mode_names[psearch.mode]);

  return pat;
}


static int integer_list_find_minimum (GList *list) {
  int res = 0;

  if (list) {
    res = (int) list->data;
    list = list->next;

    while (list) {
      if (res > (int) list->data) {
	res = (int) list->data;
      }
      list = list->next;
    }
  }

  return res;
}


static int server_has_player (struct server *s) {
  GSList *plist;
  struct player *p;

  for (plist = s->players; plist; plist = plist->next) {
    p = (struct player *) plist->data;
    if (psearch_test_player (p))
      return TRUE;
  }
  return FALSE;
}


static int psearch_next_player (int player) {
  struct player *p;
  GtkVisibility vis;

  while (player < player_clist->rows) {
    p = (struct player *) gtk_clist_get_row_data (player_clist, player);

    if (psearch_test_player (p)) {
      gtk_clist_select_row (player_clist, player, 0);

      vis = gtk_clist_row_is_visible (player_clist, player);
      if (vis != GTK_VISIBILITY_FULL)
	gtk_clist_moveto (player_clist, player, 0, 0.5, 0.0);

      return TRUE;
    }

    player++;
  }

  return FALSE;
}


void find_player (int find_next) {
  struct server *s;
  int player = 0;
  int server = 0;

  if (find_next && server_clist->selection != NULL) {	/* selected one */

    if (server_clist->selection->next == NULL) {
      server = (int) server_clist->selection->data;

      if (player_clist->selection)
	player = (int) player_clist->selection->data + 1;

      if (psearch_next_player (player))
	return;

      server++;
    }
    else {						/* selected many */
      server = integer_list_find_minimum (server_clist->selection);
    }

  }

  while (server < server_clist->rows) {
    s = (struct server *) gtk_clist_get_row_data (server_clist, server);

    if (server_has_player (s)) {
      server_clist_select_one (server);
      psearch_next_player (0);
      return;
    }

    server++;
  }

  if (!find_next || server_clist->selection == NULL) {
    dialog_ok (NULL, _("Player not found."));
  }
  else {
    if (dialog_yesno (NULL, 0, _("OK"), _("Cancel"),
          _("End of server list reached.  Continue from beginning?"))) {
      find_player (FALSE);
    }
  }
}

