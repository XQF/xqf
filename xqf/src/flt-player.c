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
#include <regex.h>
#include <stdio.h>	/* FILE, fopen, fclose, fprintf... */
#include <string.h>	/* strlen, strcmp */
#include <unistd.h>	/* unlink, close */
#include <sys/stat.h>	/* open */
#include <fcntl.h>	/* open */

#include <gtk/gtk.h>

#include "xqf.h"
#include "pref.h"
#include "pixmaps.h"
#include "dialogs.h"
#include "utils.h"
#include "flt-player.h"
#include "filter.h"


#define REGCOMP_FLAGS 	(REG_EXTENDED | REG_NOSUB | REG_ICASE)


static GSList *players = NULL;	/* GSList <struct player_pattern *> */
static GSList *curplrs = NULL;  /* GSList <struct player_pattern *> */

static GtkWidget *pattern_clist;
static GtkWidget *pattern_entry;
static GtkWidget *comment_text;
static GtkWidget *mode_buttons[3];
static GtkWidget *delete_button;
static GtkWidget *up_button;
static GtkWidget *down_button;

static int current_row = -1;

static void player_filter_save_patterns (void);
static void player_filter_load_patterns (void);


enum {
  TOKEN_INVALID = G_TOKEN_LAST,
  TOKEN_STRING,
  TOKEN_SUBSTR,
  TOKEN_REGEXP
};

static char *mode_symbols[3] = {
  "string",
  "substr",
  "regexp"
};

static const char *mode_names[3] = { 
  "string", 
  "substring",
  "regular expression"
};


int player_filter (struct server *s, struct server_filter_vars *vars) {
  /* The 'vars' is ignored in this function, however, since we need it
     for applying the server filter, we will put it in the arguments. */

  GSList *list;
  GSList *plist;
  struct player_pattern *pp;
  struct player *p;

  s->flags &= ~PLAYER_GROUP_MASK;

  if (!s->players)
    return FALSE;

  for (plist = s->players; plist; plist = plist->next) {
    p = (struct player *) plist->data;
    p->flags &= ~PLAYER_GROUP_MASK;
  }

  if (!players)
    return FALSE;

  for (list = players; list; list = list->next) {
    pp = (struct player_pattern *) list->data;

    for (plist = s->players; plist; plist = plist->next) {
      p = (struct player *) plist->data;

      if (pp->data && !pp->error) {
	if ((pp->mode == PATTERN_MODE_STRING && 
                 g_strcasecmp (p->name, pp->data) == 0) ||
	    (pp->mode == PATTERN_MODE_SUBSTR && 
	         lowcasestrstr (p->name, pp->data)) ||
	    (pp->mode == PATTERN_MODE_REGEXP &&
	         regexec ((regex_t *) pp->data, p->name, 0, NULL, 0) == 0)) {

	  p->flags |= pp->groups;
	  s->flags |= pp->groups;
	}
      }
    }
  }

  return ((s->flags & PLAYER_GROUP_MASK) != 0)? TRUE : FALSE;
}


static void free_player_pattern_compiled_data (struct player_pattern *pp) {

  if (!pp)
    return;

  if (pp->error) {
    g_free (pp->error);
    pp->error = NULL;
  }

  if (pp->data) {
    if (pp->mode == PATTERN_MODE_REGEXP) {
      regfree ((regex_t *) pp->data);
    }
    g_free (pp->data);
    pp->data = NULL;
  }
}


static void free_player_pattern (struct player_pattern *pp) {
  free_player_pattern_compiled_data (pp);
  if (pp->pattern) g_free (pp->pattern);
  if (pp->comment) g_free (pp->comment);
  g_free (pp);
}


static char *get_regerror (int errcode, regex_t *compiled) {
  size_t length;
  char *buffer;

  length = regerror (errcode, compiled, NULL, 0);
  buffer = g_malloc (length);
  regerror (errcode, compiled, buffer, length);
  return buffer;
}


static void player_pattern_compile (struct player_pattern *pp) {
  int res;

  free_player_pattern_compiled_data (pp);

  if (pp->pattern && pp->pattern[0]) {

    switch (pp->mode) {

    case PATTERN_MODE_REGEXP:
      pp->data = g_malloc (sizeof (regex_t));

      res = regcomp ((regex_t *) pp->data, pp->pattern, REGCOMP_FLAGS);
      if (res) {
	pp->error = get_regerror (res, (regex_t *) pp->data);
	regfree ((regex_t *) pp->data);
	g_free (pp->data);
	pp->data = NULL;
      }
      break;

    case PATTERN_MODE_STRING:
    case PATTERN_MODE_SUBSTR:
    default:
      pp->data = g_strdup (pp->pattern);
      g_strdown (pp->data);
      break;

    }

  }
}


static struct player_pattern *player_pattern_new (struct player_pattern *src) {
  struct player_pattern *pp;

  pp = g_malloc0 (sizeof (struct player_pattern));

  if (src) {
    pp->mode    = src->mode;
    pp->pattern = g_strdup (src->pattern);
    pp->comment = g_strdup (src->comment);
    pp->groups  = src->groups;
  }
  else {
    pp->mode    = PATTERN_MODE_STRING;
    pp->groups  = PLAYER_GROUP_RED;
  }

  pp->dirty = TRUE;
  return pp;
}


static void pattern_clist_sync_selection (void) {
  GSList *list;
  struct player_pattern *pp;

  if (current_row >= 0) {
    list = g_slist_nth (curplrs, current_row);
    pp = (struct player_pattern *) list->data;

    if (!GTK_WIDGET_REALIZED (comment_text))
      gtk_widget_realize (comment_text);
  
    gtk_text_freeze (GTK_TEXT (comment_text));
    gtk_text_set_point (GTK_TEXT (comment_text), 0);
    gtk_text_forward_delete (GTK_TEXT (comment_text),
                               gtk_text_get_length (GTK_TEXT (comment_text)));
    gtk_text_set_point (GTK_TEXT (comment_text), 0);
    if (pp->comment) {
      gtk_text_insert (GTK_TEXT (comment_text), NULL, NULL, NULL,
                                           pp->comment, strlen (pp->comment));
      gtk_text_set_point (GTK_TEXT (comment_text), 0);
    }
    gtk_text_set_editable (GTK_TEXT (comment_text), TRUE);
    gtk_text_thaw (GTK_TEXT (comment_text));

    gtk_entry_set_text (GTK_ENTRY (pattern_entry), 
                                             (pp->pattern)? pp->pattern : "");
    gtk_entry_set_editable (GTK_ENTRY (pattern_entry), TRUE);

    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_STRING], TRUE);
    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_SUBSTR], TRUE);
    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_REGEXP], TRUE);

    gtk_toggle_button_set_active (
  			    GTK_TOGGLE_BUTTON (mode_buttons[pp->mode]), TRUE);

    gtk_widget_set_sensitive (delete_button, TRUE);
    gtk_widget_set_sensitive (up_button, TRUE);
    gtk_widget_set_sensitive (down_button, TRUE);
  }
  else {
    gtk_text_freeze (GTK_TEXT (comment_text));
    gtk_text_set_point (GTK_TEXT (comment_text), 0);
    gtk_text_forward_delete (GTK_TEXT (comment_text),
                               gtk_text_get_length (GTK_TEXT (comment_text)));
    gtk_text_set_editable (GTK_TEXT (comment_text), FALSE);
    gtk_text_thaw (GTK_TEXT (comment_text));

    gtk_entry_set_text (GTK_ENTRY (pattern_entry), "");
    gtk_entry_set_editable (GTK_ENTRY (pattern_entry), FALSE);

    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_STRING], FALSE);
    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_SUBSTR], FALSE);
    gtk_widget_set_sensitive (mode_buttons[PATTERN_MODE_REGEXP], FALSE);

    gtk_widget_set_sensitive (delete_button, FALSE);
    gtk_widget_set_sensitive (up_button, FALSE);
    gtk_widget_set_sensitive (down_button, FALSE);
  }
}


static void pattern_clist_update_groups (int row, unsigned newstate, 
                                                          unsigned oldstate) {
  int i;
  unsigned mask;

  for (i = 0, mask = 1; i < 3; i++, mask <<= 1) {
    if ((newstate & mask) != 0) {
      if ((oldstate & mask) == 0) {
	gtk_clist_set_pixmap (GTK_CLIST (pattern_clist), row, i, 
                                         group_pix[i].pix, group_pix[i].mask);
      }
    }
    else {
      if ((oldstate & mask) != 0)
	gtk_clist_set_text (GTK_CLIST (pattern_clist), row, i, "");
    }
  }
}


static void pattern_clist_update_row (struct player_pattern *pp, int row) {

  pattern_clist_update_groups (row, pp->groups, ~pp->groups); /* update all */

  if (pp->error) {
    gtk_clist_set_pixtext (GTK_CLIST (pattern_clist), row, 3, 
                    mode_symbols[pp->mode], 2, error_pix.pix, error_pix.mask);
  }
  else {
    gtk_clist_set_text (GTK_CLIST (pattern_clist), row, 3, 
                                                      mode_symbols[pp->mode]);
  }

  gtk_clist_set_text (GTK_CLIST (pattern_clist), row, 4, pp->pattern);
}


static void sync_pattern_data (void) {
  struct player_pattern *pp;
  char *pattern;
  char *comment;
  enum pattern_mode mode;
  int update_pattern = FALSE;
  int update_comment = FALSE; 
  GSList *list;

  if (current_row < 0)
    return;

  list = g_slist_nth (curplrs, current_row);
  pp = (struct player_pattern *) list->data;

  if (GTK_TOGGLE_BUTTON (mode_buttons[PATTERN_MODE_STRING])->active)
    mode = PATTERN_MODE_STRING;
  else if (GTK_TOGGLE_BUTTON (mode_buttons[PATTERN_MODE_SUBSTR])->active)
    mode = PATTERN_MODE_SUBSTR;
  else
    mode = PATTERN_MODE_REGEXP;

  pattern = strdup_strip (gtk_entry_get_text (GTK_ENTRY (pattern_entry)));
  comment = gtk_editable_get_chars (GTK_EDITABLE (comment_text), 0, -1);

  update_pattern = (pp->pattern && pp->pattern[0])? 
                    !pattern || !pattern[0] || strcmp (pp->pattern, pattern) :
                    pattern && pattern[0];

  update_comment = (pp->comment && pp->comment[0])? 
                    !comment || !comment[0] || strcmp (pp->comment, comment) :
                    comment && comment[0];

  /* 
   *  Test everything but groups.
   *  They are synchronized by pattern_set_groups()
   */

  if (mode != pp->mode || update_comment || update_pattern) {

    if (!pp->dirty) {
      pp = player_pattern_new (pp);
      list->data = pp;
    }

    if (update_pattern) {
      if (pp->pattern) g_free (pp->pattern);
      pp->pattern = pattern;
    }

    if (update_comment) {
      if (pp->comment) g_free (pp->comment);
      pp->comment = comment;
    }

    if (pp->mode != mode || update_pattern) {
      free_player_pattern_compiled_data (pp);
      pp->mode = mode;
      player_pattern_compile (pp);
    }

    pattern_clist_update_row (pp, current_row);

  }

  if (!update_comment)
    g_free (comment);

  if (!update_pattern)
    g_free (pattern);
}


static int pattern_clist_insert_row (struct player_pattern *pp, int row) {
  char *text[6];

  text[0] = text[1] = text[2] = text[3] = text[4] = text[5] = NULL;

  if (row < 0)
    row = gtk_clist_append (GTK_CLIST (pattern_clist), text);
  else
    gtk_clist_insert (GTK_CLIST (pattern_clist), row, text);

  pattern_clist_update_row (pp, row);
  return row;
}


static void pattern_set_groups (int row, int column, int add) {
  GSList *list;
  struct player_pattern *pp;
  unsigned newstate;
  unsigned oldstate;

  if (column < 0 || column > 2 || row < 0)
    return;

  list = g_slist_nth (curplrs, row);
  pp = (struct player_pattern *) list->data;

  oldstate = pp->groups;
  newstate = 1 << column;

  if (add) {
    if ((pp->groups & ~newstate & PLAYER_GROUP_MASK) != 0)
      newstate ^= pp->groups;
  }

  if (newstate != oldstate) {
    if (!pp->dirty) {
      pp = player_pattern_new (pp);
      list->data = pp;
      player_pattern_compile (pp);
    }
    pp->groups = newstate;
    pattern_clist_update_groups (row, newstate, oldstate);
  }
}


static void show_pattern_error (int row) {
  GSList *list;
  struct player_pattern *pp;

  list = g_slist_nth (curplrs, row);
  pp = (struct player_pattern *) list->data;

  if (pp->error) {
    dialog_ok ("XQF: Error", "Regular Expression Error!\n\n%s\n\n%s.", 
                                                      pp->pattern, pp->error);
  }
}


static int pattern_clist_event_callback (GtkWidget *widget, GdkEvent *event) {
  GdkEventButton *bevent = (GdkEventButton *) event;
  int row, column;
  int add;

  if ((event->type == GDK_BUTTON_PRESS ||
       event->type == GDK_2BUTTON_PRESS ||
       event->type == GDK_3BUTTON_PRESS) &&
      bevent->window == GTK_CLIST (pattern_clist)->clist_window) {

    if (gtk_clist_get_selection_info (GTK_CLIST (pattern_clist), 
                                       bevent->x, bevent->y, &row, &column)) {
      if (event->type == GDK_BUTTON_PRESS) {

	if (column >= 0) {
	  switch (column) {

	  case 0:
	  case 1:
	  case 2:
	    sync_pattern_data ();
	    add = (bevent->button == 3) || 
	           (bevent->state & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) != 0;
	    pattern_set_groups (row, column, add);
	    return TRUE;

	  case 3:
	    /* sync_pattern_data () is called by "select_row" callback */

	    gtk_clist_select_row (GTK_CLIST (pattern_clist), row, 0);

	    show_pattern_error (row);
	    return TRUE;

	  default:
	    break;

	  }
	}

      }

      if (event->type == GDK_2BUTTON_PRESS || event->type == GDK_3BUTTON_PRESS)
	return TRUE;
    }
  }
  return FALSE;
}


static void pattern_clist_select_row_callback (GtkWidget *widget, 
                                 int row, int column, GdkEventButton *event) {
  sync_pattern_data ();
  current_row = row;

  pattern_clist_sync_selection ();
}


static void new_pattern_callback (GtkWidget *widget, gpointer data) {
  struct player_pattern *pp;
  int row;

  pp = player_pattern_new (NULL);

  if (current_row >= 0) {
    row = current_row;
    current_row++;
  }
  else {
    row = 0;
  }

  curplrs = g_slist_insert (curplrs, pp, row);
  pattern_clist_insert_row (pp, row);

  if (curplrs && curplrs->next)
    gtk_clist_select_row (GTK_CLIST (pattern_clist), row, 0);

  gtk_widget_grab_focus (pattern_entry);
}


static void delete_pattern_callback (GtkWidget *widget, gpointer data) {
  GSList *link;
  struct player_pattern *pp;
  int row;

  if (current_row < 0)
    return;

  link = g_slist_nth (curplrs, current_row);
  pp = (struct player_pattern *) link->data;

  curplrs = g_slist_remove_link (curplrs, link);
  if (pp->dirty)
    free_player_pattern (pp);

  row = current_row;
  current_row = -1;
  gtk_clist_remove (GTK_CLIST (pattern_clist), row);

  if (current_row < 0)
    pattern_clist_sync_selection ();
}


static void pattern_clist_adjust_visibility (int row, int direction) {
  GtkVisibility vis;

  vis = gtk_clist_row_is_visible (GTK_CLIST (pattern_clist), row);
  if (vis != GTK_VISIBILITY_FULL) {
    gtk_clist_moveto (GTK_CLIST (pattern_clist), row, 0, 
                   (direction == 0)? 0.5 : ((direction > 0)? 1.0 : 0.0), 0.0);
  }
}


static void move_up_down_pattern_callback (GtkWidget *widget, int dir) {
  GSList *link;
  struct player_pattern *pp;
  int row = current_row;

  if ((dir != -1 || row <= 0) &&
      (dir != 1  || row < 0 || row == g_slist_length (curplrs) - 1)) {
    return;
  }

  link = g_slist_nth (curplrs, row + dir);
  pp = (struct player_pattern *) link->data;
  curplrs = g_slist_remove_link (curplrs, link);
  curplrs = g_slist_insert (curplrs, pp, row);

  gtk_clist_swap_rows (GTK_CLIST (pattern_clist), row, row + dir);
  current_row += dir;

  pattern_clist_adjust_visibility (current_row, dir);
}


static void pattern_clist_row_move_callback (GtkWidget *widget, 
                                                       int source, int dest) {
  GtkCList *clist = GTK_CLIST (widget);
  GSList *link;
  struct player_pattern *pp;

  if (source < 0 || dest < 0 || source == dest || 
                                 source > clist->rows || dest > clist->rows) {
    return;
  }

  link = g_slist_nth (curplrs, source);
  pp = (struct player_pattern *) link->data;
  curplrs = g_slist_remove_link (curplrs, link);
  curplrs = g_slist_insert (curplrs, pp, dest);

  current_row = dest;
}


static GtkWidget *aligned_pixmap (GdkPixmap *pix, GdkBitmap *mask) {
  GtkWidget *pixmap;
  GtkWidget *alignment;

  alignment = gtk_alignment_new (0.5, 0.5, 0, 0);

  pixmap = gtk_pixmap_new (pix, mask);
  gtk_container_add (GTK_CONTAINER (alignment), pixmap);
  gtk_widget_show (pixmap);

  return alignment;
}


static GtkWidget *player_filter_pattern_editor (void) {
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *frame;
  GtkWidget *vscrollbar;
  GSList *group;
  int i;

  vbox = gtk_vbox_new (FALSE, 8);

  frame = gtk_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  table = gtk_table_new (2, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_table_set_col_spacings (GTK_TABLE (table), 4);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);
  gtk_container_add (GTK_CONTAINER (frame), table);

  /* Pattern Entry */

  label = gtk_label_new ("Pattern");
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1, 0, 0, 0, 0);
  gtk_widget_show (label);

  pattern_entry = gtk_entry_new_with_max_length (256);
  gtk_table_attach_defaults (GTK_TABLE (table), pattern_entry, 1, 2, 0, 1);
  gtk_signal_connect (GTK_OBJECT (pattern_entry), "activate",
		                   GTK_SIGNAL_FUNC (sync_pattern_data), NULL);
  gtk_widget_show (pattern_entry);

  /* Mode Buttons */

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 1, 2);

  group = NULL;

  for (i = 0; i < 3; i++) {
    mode_buttons[i] = gtk_radio_button_new_with_label (group, mode_names[i]);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (mode_buttons[i]));

    gtk_signal_connect (GTK_OBJECT (mode_buttons[i]), "clicked",
                                   GTK_SIGNAL_FUNC (sync_pattern_data), NULL);

    gtk_box_pack_start (GTK_BOX (hbox), mode_buttons[i], FALSE, FALSE, 0);
    gtk_widget_show (mode_buttons[i]);
  }

  gtk_widget_show (hbox);

  gtk_widget_show (table);
  gtk_widget_show (frame);

  /* Comment */

  frame = gtk_frame_new ("Pattern Comment");
  gtk_box_pack_end (GTK_BOX (vbox), frame, TRUE, TRUE, 0);

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_container_add (GTK_CONTAINER (frame), hbox);

  comment_text = gtk_text_new (NULL, NULL);
  gtk_widget_set_usize (comment_text, -1, 80);
  gtk_box_pack_start (GTK_BOX (hbox), comment_text, TRUE, TRUE, 0);
  gtk_widget_show (comment_text);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (comment_text)->vadj);
  gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, FALSE, 0);
  gtk_widget_show (vscrollbar);

  gtk_widget_show (hbox);
  gtk_widget_show (frame);

  gtk_widget_show (vbox);

  return vbox;
}


static void player_filter_page_init (void) {
  struct player_pattern *pp;
  GSList *list;

  curplrs = g_slist_copy (players);
  current_row = -1;

  for (list = players; list; list = list->next) {
    pp = (struct player_pattern *) list->data;
    pp->dirty = FALSE;
    pattern_clist_insert_row (pp, -1);
  }

  if (!curplrs)
    pattern_clist_sync_selection ();
}


void player_filter_page (GtkWidget *notebook) {
  GtkWidget *page_hbox;
  GtkWidget *scrollwin;
  GtkWidget *label;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *alignment;
  GtkWidget *pixmap;
  GtkWidget *button;
  GtkWidget *peditor;
  char *titles[5] = { "", "", "", "Mode", "Pattern" };
  int i;

  page_hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (page_hbox), 8);

  label = gtk_label_new ("Player Filter");
  gtk_widget_show (label);

  gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page_hbox, label);

  /* Pattern CList */

  scrollwin = gtk_scrolled_window_new (NULL, NULL);
  gtk_box_pack_start (GTK_BOX (page_hbox), scrollwin, FALSE, FALSE, 0);

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin), 
				  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  pattern_clist = gtk_clist_new_with_titles (5, titles);
  gtk_widget_set_usize (pattern_clist, 260, 200);
  gtk_clist_set_selection_mode (GTK_CLIST (pattern_clist),
                                                        GTK_SELECTION_BROWSE);
  gtk_clist_set_reorderable (GTK_CLIST (pattern_clist), TRUE);

  gtk_signal_connect (GTK_OBJECT (pattern_clist), "event",
                        GTK_SIGNAL_FUNC (pattern_clist_event_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (pattern_clist), "select_row",
                   GTK_SIGNAL_FUNC (pattern_clist_select_row_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (pattern_clist), "row_move",
                     GTK_SIGNAL_FUNC (pattern_clist_row_move_callback), NULL);

  for (i = 0; i < 3; i++) {
    pixmap = aligned_pixmap (group_pix[i].pix, group_pix[i].mask);
    gtk_clist_set_column_width (GTK_CLIST (pattern_clist), i, 
                                             pixmap_width (group_pix[i].pix));
    gtk_clist_set_column_widget (GTK_CLIST (pattern_clist), i, pixmap);
    gtk_widget_show (pixmap);

    gtk_clist_set_column_resizeable (GTK_CLIST (pattern_clist), i, FALSE);
  }

  gtk_clist_set_column_width (GTK_CLIST (pattern_clist), 3, 45);

  gtk_container_add (GTK_CONTAINER (scrollwin), pattern_clist); 
  gtk_clist_column_titles_passive (GTK_CLIST (pattern_clist));

  gtk_widget_show (pattern_clist);

  gtk_widget_show (scrollwin);

  gtk_widget_ensure_style (pattern_clist);
  i = MAX (pixmap_height (group_pix[0].pix), 
	      pattern_clist->style->font->ascent + 
	      pattern_clist->style->font->descent + 1);

  gtk_clist_set_row_height (GTK_CLIST (pattern_clist), i);

  /* Buttons */

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (page_hbox), vbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (" New ");
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                                GTK_SIGNAL_FUNC (new_pattern_callback), NULL);
  gtk_widget_show (button);

  delete_button = gtk_button_new_with_label (" Delete ");
  gtk_box_pack_start (GTK_BOX (vbox), delete_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (delete_button), "clicked",
                             GTK_SIGNAL_FUNC (delete_pattern_callback), NULL);
  gtk_widget_show (delete_button);

  alignment = gtk_alignment_new (0, 0.5, 1, 0);
  gtk_box_pack_end (GTK_BOX (vbox), alignment, TRUE, TRUE, 0);

  vbox2 = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (alignment), vbox2);

  up_button = gtk_button_new_with_label (" Up ");
  gtk_box_pack_start (GTK_BOX (vbox2), up_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (up_button), "clicked",
                GTK_SIGNAL_FUNC (move_up_down_pattern_callback), (void *) -1);
  gtk_widget_show (up_button);

  down_button = gtk_button_new_with_label (" Down ");
  gtk_box_pack_start (GTK_BOX (vbox2), down_button, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (down_button), "clicked",
                 GTK_SIGNAL_FUNC (move_up_down_pattern_callback), (void *) 1);
  gtk_widget_show (down_button);

  gtk_widget_show (vbox2);
  gtk_widget_show (alignment);
  gtk_widget_show (vbox);

  /* Pattern Editor */

  peditor = player_filter_pattern_editor ();
  gtk_box_pack_end (GTK_BOX (page_hbox), peditor, TRUE, TRUE, 0);

  gtk_widget_show (page_hbox);

  player_filter_page_init ();
}


static int strings_are_same (const char *s1, const char *s2) {

  return ((!s1 || *s1 == '\0') && (!s2 || *s2 == '\0')) ||
         (s1 && s2 && strcmp (s1, s2) == 0);
}


/*
 *  'FILTER_DATA_CHANGED' situations that are not catched by 
 *   player_pattern_lists_compare():
 *
 *     - string/substring mode: case change in pattern
 *     - regexp mode: errorneous pattern is modified but 
 *          the error is not corrected
 */

static int player_pattern_lists_compare (GSList *list1, GSList *list2) {
  enum filter_status changed = FILTER_NOT_CHANGED;
  struct player_pattern *pp1, *pp2;
  GSList *tmp;

  if (g_slist_length (list1) != g_slist_length (list2))
    return FILTER_CHANGED;

  while (list1) {
    pp1 = (struct player_pattern *) list1->data;
    pp2 = (struct player_pattern *) list2->data;

    if (pp1->mode == pp2->mode && pp1->groups == pp2->groups &&
                              strings_are_same (pp1->pattern, pp2->pattern)) { 
      if (!strings_are_same (pp1->comment, pp2->comment))
	changed = FILTER_DATA_CHANGED;
      list2 = list2->next;
    }
    else {
      changed = FILTER_DATA_CHANGED;

      for (tmp = list2->next; tmp; tmp = tmp->next) {
	pp2 = (struct player_pattern *) tmp->data;

	if (pp1->mode == pp2->mode && pp1->groups == pp2->groups &&
                              strings_are_same (pp1->pattern, pp2->pattern)) { 
	  break;
	}
      }

      if (!tmp)
	return FILTER_CHANGED;
    }

    list1 = list1->next;
  }

  return changed;
}


void player_filter_new_defaults (void) {
  struct player_pattern *pp;
  GSList *list;

  sync_pattern_data ();

  filters[FILTER_PLAYER].changed = 
                              player_pattern_lists_compare (players, curplrs);

  for (list = curplrs; list; list = list->next) {
    pp = (struct player_pattern *) list->data;
    pp->dirty = TRUE;
  }

  for (list = players; list; list = list->next) {
    pp = (struct player_pattern *) list->data;
    if (!pp->dirty)
      free_player_pattern (pp);
  }
  g_slist_free (players);
  players = curplrs;
  curplrs = NULL;

  switch (filters[FILTER_PLAYER].changed) {

  case FILTER_CHANGED:
    filters[FILTER_PLAYER].last_changed = ++filter_current_time;
    /* fall through */

  case FILTER_DATA_CHANGED:
    player_filter_save_patterns ();
    break;

  case FILTER_NOT_CHANGED:
  default:
    break;

  }
}


void player_filter_cfg_clean_up (void) {
  struct player_pattern *pp;
  GSList *list;

  if (curplrs) {
    for (list = curplrs; list; list = list->next) {
      pp = (struct player_pattern *) list->data;
      if (pp->dirty)
	free_player_pattern (pp);
    }
    g_slist_free (curplrs);
    curplrs = NULL;
  }
}


int player_filter_add_player (char *name, unsigned mask) {
  GSList *list;
  struct player_pattern *pp;
  char *pattern;
  int changed = FALSE;

  pattern = strdup_strip (name);

  if (!pattern)
    return FALSE;

  for (list = players; list; list = list->next) {
    pp = (struct player_pattern *) list->data;
    if (pp->mode == PATTERN_MODE_STRING && pp->pattern &&
                                   g_strcasecmp (pattern, pp->pattern) == 0) {
      break;
    }
  }

  if (!list) {
    pp = player_pattern_new (NULL);
    pp->pattern = pattern;
    pp->groups = 0;

    player_pattern_compile (pp);
    players = g_slist_append (players, pp);

    changed = TRUE;
  }

  mask &= PLAYER_GROUP_MASK;

  if ((pp->groups & mask) != mask) {
    pp->groups |= mask;

    filters[FILTER_PLAYER].last_changed = ++filter_current_time;
    player_filter_save_patterns ();

    changed = TRUE;
  }

  return changed;
}


static GScannerConfig patterns_scanner_config = {
  (
   " \t\n"
   )			/* cset_skip_characters */,
  (
   G_CSET_a_2_z
   "_"
   G_CSET_A_2_Z
   )			/* cset_identifier_first */,
  (
   G_CSET_a_2_z
   "_0123456789"
   G_CSET_A_2_Z
   G_CSET_LATINS
   G_CSET_LATINC
   )			/* cset_identifier_nth */,
  ( "#\n" )		/* cpair_comment_single */,
  
  TRUE			/* case_sensitive */,
  
  TRUE			/* skip_comment_multi */,
  TRUE			/* skip_comment_single */,
  FALSE			/* scan_comment_multi */,
  TRUE			/* scan_identifier */,
  FALSE			/* scan_identifier_1char */,
  FALSE			/* scan_identifier_NULL */,
  TRUE			/* scan_symbols */,
  FALSE			/* scan_binary */,
  FALSE			/* scan_octal */,
  FALSE			/* scan_float */,
  TRUE			/* scan_hex */,
  FALSE			/* scan_hex_dollar */,
  FALSE			/* scan_string_sq */,
  TRUE			/* scan_string_dq */,
  TRUE			/* numbers_2_int */,
  FALSE			/* int_2_float */,
  TRUE			/* identifier_2_string */,
  TRUE			/* char_2_token */,
  TRUE			/* symbol_2_token */,
};


static int skip_to_right_curly_bracket (GScanner *scanner) {
  int token;

  token = g_scanner_cur_token (scanner);

  while (1) {

    switch (token) {

    case G_TOKEN_EOF:
    case G_TOKEN_ERROR:
      return FALSE;

    case G_TOKEN_RIGHT_CURLY:
      return TRUE;

    }

    token = g_scanner_get_next_token (scanner);
  }
}


static int player_pattern_parse_statement (GScanner *scanner, char *fn) {
  struct player_pattern *pp;
  int token;

  token = g_scanner_get_next_token (scanner);

  switch (token) {

  case TOKEN_STRING:
  case TOKEN_SUBSTR:
  case TOKEN_REGEXP:

    pp = player_pattern_new (NULL);

    switch (token) {
    case TOKEN_REGEXP:
      pp->mode = PATTERN_MODE_REGEXP;
      break;

    case TOKEN_SUBSTR:
      pp->mode = PATTERN_MODE_SUBSTR;
      break;

    case TOKEN_STRING:
    default:
      pp->mode = PATTERN_MODE_STRING;
      break;
    }

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_LEFT_CURLY) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_STRING) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    pp->pattern = strdup_strip (scanner->value.v_string);

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_COMMA) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_INT) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    pp->groups = scanner->value.v_int & PLAYER_GROUP_MASK;

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_COMMA) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_STRING) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    pp->comment = strdup_strip (scanner->value.v_string);

    token = g_scanner_get_next_token (scanner);
    if (token != G_TOKEN_RIGHT_CURLY) {
	free_player_pattern (pp);
	return skip_to_right_curly_bracket (scanner);
    }

    player_pattern_compile (pp);
    players = g_slist_append (players, pp);
    filters[FILTER_PLAYER].changed = FILTER_CHANGED;
    break;

  case G_TOKEN_EOF:
    return FALSE;

  default:
    fprintf (stderr, "%s[%d:%d] parse error\n\n", fn, scanner->line,
                                                           scanner->position);
    return skip_to_right_curly_bracket (scanner);
  }

  return TRUE;
}


static void player_filter_load_patterns (void) {
  char *fn;
  int fd;
  GScanner *scanner;
  int retval;
  int i;

  fn = file_in_dir (user_rcdir, PLAYERS_FILE);
  fd = open (fn, O_RDONLY);

  if (fd < 0) {
    g_free (fn);
    return;
  }

  scanner = g_scanner_new (&patterns_scanner_config);

  for (i = TOKEN_STRING; i <= TOKEN_REGEXP; i++) {
    g_scanner_add_symbol (scanner, mode_symbols[i - TOKEN_STRING], 
                                                                (gpointer) i);
  }

  g_scanner_input_file (scanner, fd);

  do {
    retval = player_pattern_parse_statement (scanner, fn);
  } while (retval);

  g_scanner_destroy (scanner);
  g_free (fn);
  close (fd);

  if (filters[FILTER_PLAYER].changed != FILTER_NOT_CHANGED)
    filters[FILTER_PLAYER].last_changed = ++filter_current_time;
}


static void player_filter_save_patterns (void) {
  GSList *list;
  struct player_pattern *pp;
  FILE *f;
  char *fn;

  fn = file_in_dir (user_rcdir, PLAYERS_FILE);

  if (!players) {
    unlink (fn);
  }
  else {
    f = fopen (fn, "w");
    if (!f) {
      g_free (fn);
      return;
    }

    for (list = players; list; list = list->next) {
      pp = (struct player_pattern *) list->data;

      fprintf (f, "%s {\n  ", mode_symbols[pp->mode]);
      print_dq_string (f, pp->pattern);
      fprintf (f, ",\n  0x%02X,\n  ", pp->groups);
      print_dq_string (f, pp->comment);
      fprintf (f, "\n}\n\n");
    }

    fclose (f);
  }

  g_free (fn);
}


void player_filter_init (void) {
  player_filter_done ();
  player_filter_load_patterns ();
}


void player_filter_done (void) {
  g_slist_foreach (players, (GFunc) free_player_pattern, NULL);
  g_slist_free (players);
  players = NULL;
}

