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

#include <sys/types.h>	/* socket, send, bind, connect */
#include <stdio.h>	/* fprintf */
#include <stdarg.h>	/* va_start, va_end */
#include <sys/socket.h>	/* socket, send, bind, connect */
#include <netinet/in.h>	/* sockaddr_in */
#include <unistd.h>	/* close */
#include <string.h>	/* memset, strcmp */
#include <errno.h>      /* errno */


#include <gtk/gtk.h>

#include "xqf.h"
#include "srv-prop.h"
#include "srv-list.h"
#include "game.h"
#include "dialogs.h"
#include "utils.h"
#include "history.h"
#include "config.h"
#include "rcon.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#define _(string) gettext(string)
#else
#define _(string) (string)
#endif

#define PACKET_MAXSIZE	(64 * 1024)


static int rcon_fd = -1;
static int rcon_tag;
static char *packet = NULL;

static struct history *rcon_history = NULL;

static GtkWidget *rcon_combo = NULL;
static GtkWidget *rcon_text = NULL;


static int failed (char *name) {
  fprintf (stderr, "%s() failed: %s\n", name, g_strerror (errno));
  return TRUE;
}


static void rcon_print (char *fmt, ...) {
  char buf[2048];
  va_list ap;

  if (fmt) {
    va_start (ap, fmt);
    g_vsnprintf (buf, 2048, fmt, ap);
    va_end (ap);

    gtk_text_freeze (GTK_TEXT (rcon_text));
    gtk_text_insert (GTK_TEXT (rcon_text), NULL, NULL, NULL, buf, -1);
    gtk_text_thaw (GTK_TEXT (rcon_text));
    gtk_adjustment_set_value (GTK_TEXT (rcon_text)->vadj,
                              GTK_TEXT (rcon_text)->vadj->upper - 
			      GTK_TEXT (rcon_text)->vadj->page_size);
  }
}


static int open_connection (struct server *s) {
  struct sockaddr_in addr;
  int fd;
  int res;

  if (!s)
    return -1;

  fd = socket (PF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    failed ("socket");
    return -1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = g_htons (0);
  addr.sin_addr.s_addr = g_htonl (INADDR_ANY);
  memset (&addr.sin_zero, 0, sizeof (addr.sin_zero));

  res = bind (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (res < 0) {
    failed ("bind");
    close (fd);
    return -1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons (s->port);
  addr.sin_addr.s_addr = s->host->ip.s_addr;
  memset (&addr.sin_zero, 0, sizeof (addr.sin_zero));

  res = connect (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (res < 0) {
    failed ("connect");
    close (fd);
    return -1;
  }

  return fd;
}


#define RCON_MAX_SIZE	256

static void rcon_combo_activate_callback (GtkWidget *widget, gpointer data) {
  char buf[RCON_MAX_SIZE];
  char *cmd;
  char *passwd = (char *) data;
  int size;
  int res;

  cmd = strdup_strip (
	      gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry)));

  if (cmd) {
    size = g_snprintf (buf, RCON_MAX_SIZE, "\377\377\377\377rcon %s %s", 
                                                                 passwd, cmd);
    if (size == -1)
      size = RCON_MAX_SIZE;
    else
      size++;

    rcon_print ("RCON> %s\n", cmd);

    res = send (rcon_fd, buf, size, 0);
    if (res < 0) {
      rcon_print ("***ERROR*** send() failed: %s\n", g_strerror (errno));
    }

    history_add (rcon_history, cmd);
    combo_set_vals (rcon_combo, rcon_history->items, "");
    g_free (cmd);
  }
  else {
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry), "");
  }
}


static void msg_terminate (char *msg, int *size) {
  if (!msg || size < 0)
    return;

  if (size == 0) {
    msg[0] = '\n';
    msg[1] = '\0';
    return;
  }

  if (msg[*size - 1] != '\0') {
    if (msg[*size - 1] != '\n')
      msg[(*size)++] = '\n';
    msg[*size] = '\0';
  }
  else {
    if (*size >= 2 && msg[*size - 2] != '\n') {
      msg[*size - 1] = '\n';
      msg[*size] = '\0';
    }
  }
}


static void rcon_input_callback (gpointer data, int fd, 
                                                GdkInputCondition condition) {
  struct server *s = (struct server *) data;
  char *msg;
  int size;

  if (!packet)
    packet = g_malloc (PACKET_MAXSIZE);

  size = recv (rcon_fd, packet, PACKET_MAXSIZE, 0);
  if (size < 0) {
    rcon_print ("***ERROR*** recv() failed: %s\n", g_strerror (errno));
  }
  else {
    switch (s->type) {

    case QW_SERVER:
    case HW_SERVER:
      msg = packet + 4 + 1;
      size = size - 4 - 1;
      break;

      /* Q2, Q3 */

    default:	
      msg = packet + 4 + 6;
      size = size - 4 - 6;
      break;
    }
    
    msg_terminate (msg, &size);

    gtk_text_freeze (GTK_TEXT (rcon_text));
    gtk_text_insert (GTK_TEXT (rcon_text), NULL, NULL, NULL, msg, -1);
    gtk_text_thaw (GTK_TEXT (rcon_text));
    gtk_adjustment_set_value (GTK_TEXT (rcon_text)->vadj,
                              GTK_TEXT (rcon_text)->vadj->upper - 
			      GTK_TEXT (rcon_text)->vadj->page_size);
  }
}


static void rcon_status_button_clicked_callback (GtkWidget *w, gpointer data) {
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry), "status");
  rcon_combo_activate_callback (rcon_combo, data);
}


static void rcon_clear_button_clicked_callback (GtkWidget *w, gpointer data) {
  gtk_text_freeze (GTK_TEXT (rcon_text));
  gtk_text_set_point (GTK_TEXT (rcon_text), 0);
  gtk_text_forward_delete (GTK_TEXT (rcon_text), 
			    gtk_text_get_length (GTK_TEXT (rcon_text)));
  gtk_text_thaw (GTK_TEXT (rcon_text));
}


static void rcon_save_geometry (GtkWidget *window, gpointer data) {
  config_push_prefix ("/" CONFIG_FILE "/RCON Window Geometry/");
  config_set_int ("height", window->allocation.height);
  config_set_int ("width", window->allocation.width);
  config_pop_prefix ();
}


static void rcon_restore_geometry (GtkWidget *window) {
  char buf[256];
  int height, width;

  config_push_prefix ("/" CONFIG_FILE "/RCON Window Geometry/");

  g_snprintf (buf, 256, "height=%d", 400);
  height = config_get_int (buf);
  g_snprintf (buf, 256, "width=%d", 460);
  width = config_get_int (buf);
  gtk_window_set_default_size (GTK_WINDOW (window), width, height);

  config_pop_prefix ();
}


void rcon_dialog (struct server *s, char *passwd) {
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *hbox2;
  GtkWidget *label;
  GtkWidget *pixmap;
  GtkWidget *button;
  GtkWidget *vscrollbar;
  GtkWidget *hseparator;
  char srv[256];
  char buf[256];

  if (!s || !passwd)
    return;

  rcon_fd = open_connection (s);
  if (rcon_fd < 0)
    return;

  assemble_server_address (srv, 256, s);
  g_snprintf (buf, 256, "Remote Console [%s]", srv);

  window = dialog_create_modal_transient_window (buf, TRUE, TRUE, 
       						         &rcon_save_geometry);
  gtk_window_set_policy (GTK_WINDOW (window), TRUE, TRUE, TRUE);
  rcon_restore_geometry (window);

  main_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

  vbox = gtk_vbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);

  /* Dialog Title */

  hbox = gtk_hbox_new (TRUE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  hbox2 = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  if (games[s->type].pix) {
    pixmap = gtk_pixmap_new (games[s->type].pix->pix, 
			     games[s->type].pix->mask);
    gtk_box_pack_start (GTK_BOX (hbox2), pixmap, FALSE, FALSE, 0);
    gtk_widget_show (pixmap);
  }

  label = gtk_label_new (buf);
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  gtk_widget_show (hbox2);
  gtk_widget_show (hbox);

  /* Text */

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  rcon_text = gtk_text_new (NULL, NULL);
  gtk_text_set_editable (GTK_TEXT (rcon_text), FALSE);
  GTK_WIDGET_UNSET_FLAGS (rcon_text, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox), rcon_text, TRUE, TRUE, 0);
  gtk_widget_show (rcon_text);

  vscrollbar = gtk_vscrollbar_new (GTK_TEXT (rcon_text)->vadj);
  GTK_WIDGET_UNSET_FLAGS (vscrollbar, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, FALSE, 0);
  gtk_widget_show (vscrollbar);

  gtk_widget_show (hbox);

  hbox = gtk_hbox_new (FALSE, 4);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

  /* Message */

  label = gtk_label_new (_("Cmd:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  /* Entry */

  rcon_combo = gtk_combo_new ();
  gtk_entry_set_max_length (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry), 256);
  gtk_combo_set_case_sensitive (GTK_COMBO (rcon_combo), TRUE);
  gtk_combo_set_use_arrows_always (GTK_COMBO (rcon_combo), TRUE);
  gtk_combo_disable_activate (GTK_COMBO (rcon_combo));
  gtk_signal_connect (GTK_OBJECT (GTK_COMBO (rcon_combo)->entry), "activate",
                      GTK_SIGNAL_FUNC (rcon_combo_activate_callback), passwd);
  GTK_WIDGET_SET_FLAGS (GTK_COMBO (rcon_combo)->entry, GTK_CAN_FOCUS);
  GTK_WIDGET_UNSET_FLAGS (GTK_COMBO (rcon_combo)->button, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox), rcon_combo, TRUE, TRUE, 0);
  gtk_widget_grab_focus (GTK_COMBO (rcon_combo)->entry);
  gtk_widget_show (rcon_combo);

  if (rcon_history->items)
    combo_set_vals (rcon_combo, rcon_history->items, "");

  hbox2 = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_end (GTK_BOX (hbox), hbox2, FALSE, FALSE, 0);

  /* Send Button */

  button = gtk_button_new_with_label (_("Send"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                      GTK_SIGNAL_FUNC (rcon_combo_activate_callback), passwd);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Status Button */

  button = gtk_button_new_with_label (_("Status"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
               GTK_SIGNAL_FUNC (rcon_status_button_clicked_callback), passwd);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Clear Button */

  button = gtk_button_new_with_label (_("Clear"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
                  GTK_SIGNAL_FUNC (rcon_clear_button_clicked_callback), NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_widget_show (hbox2);
  gtk_widget_show (hbox);

  gtk_widget_show (vbox);

  hseparator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (main_vbox), hseparator, FALSE, FALSE, 0);
  gtk_widget_show (hseparator);

  /* Close Button */

  hbox = gtk_hbox_new (FALSE, 8);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 8);
  gtk_box_pack_start (GTK_BOX (main_vbox), hbox, FALSE, FALSE, 0);

  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_end (GTK_BOX (hbox), button, FALSE, FALSE, 0);
  gtk_widget_set_usize (button, 80, -1);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
                   GTK_SIGNAL_FUNC (gtk_widget_destroy), GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  gtk_widget_show (hbox);

  gtk_widget_show (main_vbox);
  gtk_widget_show (window);

  rcon_tag = gdk_input_add (rcon_fd, GDK_INPUT_READ, 
                                   (GdkInputFunction) rcon_input_callback, s);

  gtk_main ();

  gdk_input_remove (rcon_tag);
  close (rcon_fd);
  rcon_fd = -1;

  if (packet) {
    g_free (packet);
    packet = NULL;
  }

  unregister_window (window);
}


void rcon_init (void) {
  rcon_history = history_new ("RCON");
}


void rcon_done (void) {
  if (rcon_history) {
    history_free (rcon_history);
    rcon_history = NULL;
  }
}

