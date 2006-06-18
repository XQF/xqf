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

#include <sys/types.h>	/* socket, send, bind, connect */
#include <stdio.h>	/* fprintf */
#include <stdarg.h>	/* va_start, va_end */
#include <sys/socket.h>	/* socket, send, bind, connect */
#include <netinet/in.h>	/* sockaddr_in */
#include <arpa/inet.h>
#include <unistd.h>	/* close */
#include <string.h>	/* memset, strcmp */
#include <errno.h>      /* errno */
#include <ctype.h>      /* isprint */

#include <sys/time.h>   // select

#ifdef RCON_STANDALONE
#  ifdef ENABLE_NLS
#  include <locale.h>
#  endif
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "i18n.h"
#include "xqf.h"
#include "utils.h"
#ifndef RCON_STANDALONE
#include "xqf-ui.h"
#include "srv-prop.h"
#include "srv-list.h"
#include "game.h"
#include "dialogs.h"
#include "history.h"
#include "config.h"
#endif
#include "rcon.h"

enum { PACKET_MAXSIZE = (64 * 1024) };

static int rcon_fd = -1;
static char *packet = NULL;
static const char* rcon_password = NULL;
static char* rcon_challenge = NULL;  // halflife challenge
static enum server_type rcon_servertype;

#ifndef RCON_STANDALONE
static struct history *rcon_history = NULL;

static GtkWidget *rcon_combo = NULL;
static GtkWidget *rcon_text = NULL;
#endif

static char* msg_terminate (char *msg, int size);
static void rcon_print (char *fmt, ...);

static int failed (char *name) {
  fprintf (stderr, "%s() failed: %s\n", name, g_strerror (errno));
  rcon_print ("%s() failed: %s\n", name, g_strerror (errno));
  return TRUE;
}


static void rcon_print (char *fmt, ...) {
#ifndef RCON_STANDALONE
  char buf[2048];
  va_list ap;

  if (fmt) {
    va_start (ap, fmt);
    g_vsnprintf (buf, sizeof(buf), fmt, ap);
    va_end (ap);

    gtk_text_freeze (GTK_TEXT (rcon_text));
    gtk_text_insert (GTK_TEXT (rcon_text), NULL, NULL, NULL, buf, -1);
    gtk_text_thaw (GTK_TEXT (rcon_text));
    gtk_adjustment_set_value (GTK_TEXT (rcon_text)->vadj,
                              GTK_TEXT (rcon_text)->vadj->upper - 
			      GTK_TEXT (rcon_text)->vadj->page_size);
  }
#endif
}


static int wait_read_timeout(int fd, long sec, long usec)
{
    fd_set rfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    return select(rcon_fd+1, &rfds, NULL, NULL, &tv);
}

static int open_connection (struct in_addr *ip, unsigned short port) {
  struct sockaddr_in addr;
  int fd;
  int res;

  if (!ip || !port)
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
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = ip->s_addr;
  memset (&addr.sin_zero, 0, sizeof (addr.sin_zero));

  res = connect (fd, (struct sockaddr *) &addr, sizeof (addr));
  if (res < 0) {
    failed ("connect");
    close (fd);
    return -1;
  }

  return fd;
}


static int rcon_send(const char* cmd)
{
  char* buf = NULL;
  size_t bufsize = 0;
  int ret = -1;
  
  if(rcon_servertype == HL_SERVER && rcon_challenge == NULL)
  {
    enum { cbufsize = 1024 };
    char cbuf[cbufsize];
    char* msg = NULL;
    char* mustresponse = "\377\377\377\377challenge rcon ";
    int size;
    buf = "\377\377\377\377challenge rcon";
    bufsize = strlen(buf)+1;
    send (rcon_fd, buf, bufsize, 0);

    if (wait_read_timeout(rcon_fd, 5, 0) <= 0)
    {
      rcon_print ("*** timeout waiting for challenge\n");
      errno = ETIMEDOUT;
      return -1;
    }

    size = recv (rcon_fd, cbuf, cbufsize, 0);
    if(size < 0)
    {
      rcon_print ("*** error in challenge response\n");
      return -1;
    }
    
    msg = msg_terminate(cbuf, size);

    if(strlen(msg) < strlen(mustresponse) || strncmp(msg,mustresponse,strlen(mustresponse)))
    {
      rcon_print ("*** error in challenge response\n");
      return -1;
    }

    rcon_challenge = g_strstrip(msg+strlen(mustresponse));
  }

  if(rcon_servertype == HL_SERVER)
  {
    if(!rcon_challenge || !*rcon_challenge)
    {
      rcon_print ("*** invalid challenge\n");
      return -1;
    }

    buf = g_strdup_printf("\377\377\377\377rcon %s %s %s",rcon_challenge, rcon_password, cmd);
    bufsize = strlen(buf)+1;
  }
  else if (rcon_servertype == DOOM3_SERVER || rcon_servertype == Q4_SERVER)
  {
    const char prefix[] = "\377\377rcon";
    bufsize = sizeof(prefix) +strlen(rcon_password) +1 +strlen(cmd) +1;
    buf = g_new0(char, bufsize);
    strcpy(buf, prefix);
    strcpy(buf+sizeof(prefix), rcon_password);
    strcpy(buf+sizeof(prefix)+strlen(rcon_password)+1, cmd);
  }
  else
  {
    buf = g_strdup_printf("\377\377\377\377rcon %s %s",rcon_password, cmd);
    bufsize = strlen(buf)+1;
  }

  rcon_print ("RCON> %s\n", cmd);

  ret = send (rcon_fd, buf, bufsize, 0);
  g_free(buf);
  return ret;
}

#ifndef RCON_STANDALONE
static void rcon_combo_activate_callback (GtkWidget *widget, gpointer data) {
  char *cmd;
  int res;

  cmd = strdup_strip (
	      gtk_entry_get_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry)));

  if (cmd)
  {
    res = rcon_send(cmd);

    if (res < 0) {
      failed("send");
    }

    history_add (rcon_history, cmd);
    combo_set_vals (rcon_combo, rcon_history->items, "");
    g_free (cmd);
  }
  else {
    gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry), "");
  }
}
#endif

/**
  ensure msg is terminated by \n\0. return string that fullfilles this
  criteria. returned string must be freed afterwards
 */
static char* msg_terminate (char *msg, int size)
{

  char *newmsg = NULL;
  int newsize = size;
  enum { nothing = 0, append_0 = 1, append_nl = 2 } what = nothing;

  if (!msg || size <= 0)
    return g_strdup("\n");

  if(size == 1)
  {
   if(msg[0] != '\0')
     what |= append_0;
   if(msg[0] != '\n')
     what |= append_nl;
  }
  else if(msg[size-1] != '\0')
  {
    // not null terminated, check for newline
    if(msg[size-1] != '\n')
      what |= append_nl;

    what |= append_0;
  }
  else if(msg[size-2] != '\n')
  {
    // null terminated but no newline
    what |= append_nl;
  }

  if( what & append_0 )
    newsize++;
  if( what & append_nl )
    newsize++;

  newmsg = (char*)g_malloc(newsize);
  newmsg = strncpy(newmsg,msg,size);

  if((what & append_0) || (what & append_nl) )
  {
    newmsg[newsize-1] = '\0';
  }
  if( what & append_nl )
  {
    newmsg[newsize-2] = '\n';
  }

  return newmsg;
}

static char* rcon_receive()
{
  char *msg = "\n";
  ssize_t t;
  ssize_t size;

  if (!packet)
    packet = g_malloc (PACKET_MAXSIZE);

  size = recv (rcon_fd, packet, PACKET_MAXSIZE, 0);
  if (size < 0)
  {
    if(errno != EWOULDBLOCK) failed("recv");
  }
  else
  {
    switch (rcon_servertype) {

    case QW_SERVER:
    case HW_SERVER:
    case HL_SERVER:
      // "\377\377\377\377<some character>"
      msg = packet + 4 + 1;
      size = size - 4 - 1;

      for(t = 0; t < size && msg[t]; t++)      // filter QW KTPRO status list
      {
        if((unsigned char)msg[t] == 141)
          msg[t] = '>';

        msg[t]&=0x7f;

        if(msg[t] < 32)
        {
          if(msg[t] >= 0x12 && msg[t] <= 0x12 + 9)
            msg[t]+='0' - 0x12;    // yellow numbers
          else if(msg[t] == 16)
            msg[t] = '[';
          else if(msg[t] == 17)
            msg[t] = ']';
          else if(msg[t] != '\n' && msg[t] != '\r')
            msg[t] = '.';        // unprintable
        }
      }
      break;

    case DOOM3_SERVER:
    case Q4_SERVER:
      // "\377\377print\0????\0"
      if(size > 2+6+4+1)
      {
	char* ptr = msg = packet+2+6+4;
	while(ptr && ptr < packet + size - 1)
	{
	  if(*ptr == '\n' || isprint(*ptr))
	    ++ptr;
	  else
	  {
	    *ptr = '.';
	    ++ptr;
	  }
	}
      }
      break;

      /* Q2, Q3 */

    default:	
      // "\377\377\377\377print\n"
      msg = packet + 4 + 6;
      size = size - 4 - 6;
      break;
    }

    msg = msg_terminate (msg, size);
  }

  return msg;
}


#ifndef RCON_STANDALONE
static void rcon_input_callback (gpointer data, int fd, 
                                                GdkInputCondition condition) {
    char* msg = rcon_receive();

    gtk_text_freeze (GTK_TEXT (rcon_text));
    gtk_text_insert (GTK_TEXT (rcon_text), NULL, NULL, NULL, msg, -1);
    gtk_text_thaw (GTK_TEXT (rcon_text));
    gtk_adjustment_set_value (GTK_TEXT (rcon_text)->vadj,
                              GTK_TEXT (rcon_text)->vadj->upper - 
			      GTK_TEXT (rcon_text)->vadj->page_size);
    g_free(msg);
}
#endif


#ifndef RCON_STANDALONE
static void rcon_status_button_clicked_callback (GtkWidget *w, gpointer data) {
  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (rcon_combo)->entry), "status");
  rcon_combo_activate_callback (rcon_combo, data);
}
#endif


#ifndef RCON_STANDALONE
static void rcon_clear_button_clicked_callback (GtkWidget *w, gpointer data) {
  gtk_text_freeze (GTK_TEXT (rcon_text));
  gtk_text_set_point (GTK_TEXT (rcon_text), 0);
  gtk_text_forward_delete (GTK_TEXT (rcon_text), 
			    gtk_text_get_length (GTK_TEXT (rcon_text)));
  gtk_text_thaw (GTK_TEXT (rcon_text));
}
#endif


#ifndef RCON_STANDALONE
static void rcon_save_geometry (GtkWidget *window, gpointer data) {
  config_push_prefix ("/" CONFIG_FILE "/RCON Window Geometry/");
  config_set_int ("height", window->allocation.height);
  config_set_int ("width", window->allocation.width);
  config_pop_prefix ();
}
#endif


#ifndef RCON_STANDALONE
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
#endif


#ifndef RCON_STANDALONE
void rcon_dialog (const struct server *s, const char *passwd) {
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
  int rcon_tag;

  if (!s || !passwd)
    return;

  if(rcon_challenge)
    g_free(rcon_challenge);
  rcon_challenge = NULL;
  rcon_password = passwd;
  rcon_servertype = s->type;
  rcon_fd = open_connection (&s->host->ip, s->port);
  if (rcon_fd < 0)
    return;

  if(set_nonblock(rcon_fd) == -1)
    failed("fcntl");

  assemble_server_address (srv, 256, s);
  g_snprintf (buf, 256, "Remote Console [%s]", srv);

  window = dialog_create_modal_transient_window (buf, TRUE, TRUE, 
       						         GTK_SIGNAL_FUNC(rcon_save_geometry));
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
                      GTK_SIGNAL_FUNC (rcon_combo_activate_callback), NULL);
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
                      GTK_SIGNAL_FUNC (rcon_combo_activate_callback), NULL);
  GTK_WIDGET_UNSET_FLAGS (button, GTK_CAN_FOCUS);
  gtk_box_pack_start (GTK_BOX (hbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  /* Status Button */

  button = gtk_button_new_with_label (_("Status"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
               GTK_SIGNAL_FUNC (rcon_status_button_clicked_callback), NULL);
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
                                   (GdkInputFunction) rcon_input_callback, NULL);

  gtk_main ();

  gdk_input_remove (rcon_tag);
  close (rcon_fd);
  rcon_fd = -1;

  if (packet) {
    g_free (packet);
    packet = NULL;
  }
  
  g_free(rcon_challenge);
  rcon_challenge = NULL;

  unregister_window (window);
}
#endif


void rcon_init (void) {
#ifndef RCON_STANDALONE
  rcon_history = history_new ("RCON");
#endif
}


void rcon_done (void) {
#ifndef RCON_STANDALONE
  if (rcon_history) {
    history_free (rcon_history);
    rcon_history = NULL;
  }
#endif
}

#ifdef RCON_STANDALONE
int main(int argc, char* argv[])
{
  struct in_addr ip;
  int argpos = 1;
  unsigned short port = 0;
  char* arg_ip = NULL;
  char* arg_port = NULL;

  char prompt[] = "RCON> ";

  char* buf = NULL;
  int res;

#ifdef ENABLE_NLS
  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);
#endif

  rcon_servertype = Q3_SERVER;

  if(argc>1)
  {
    if(!strcmp(argv[argpos],"--qws"))
    {
      rcon_servertype = QW_SERVER;
      argpos++;
    }
    else if(!strcmp(argv[argpos],"--hls"))
    {
      rcon_servertype = HL_SERVER;
      argpos++;
    }
    else if(!strcmp(argv[argpos],"--hws"))
    {
      rcon_servertype = HW_SERVER;
      argpos++;
    }
    else if(!strcmp(argv[argpos],"--dm3s"))
    {
      rcon_servertype = DOOM3_SERVER;
      argpos++;
    }
  }

  if( argc-argpos<2 || !strcmp(argv[argpos],"--help"))
  {
      printf(_("Usage: %s [server type] <ip> <port>\n"),argv[0]);
      printf(_("  server type is either --qws, --hws, --hls or --dm3s.\n"));
      printf(_("  If no server type is specified, Q3 style rcon is assumed.\n"));
      return 1;
  }

  arg_ip = argv[argpos++];
  arg_port = argv[argpos++];

  if(!inet_aton(arg_ip,&ip))
  {
    printf("invalid ip: %s\n",arg_ip);
    return 1;
  }

  if((port = atoi(arg_port)) == 0)
  {
    printf("invalid port: %s\n",arg_port);
    return 1;
  }

  rcon_fd = open_connection(&ip,port);
  if(rcon_fd < 0)
  {
    return 1;
  }

  if(set_nonblock(rcon_fd) == -1)
    failed("fcntl");

  if(getenv("XQF_RCON_PASSWORD"))
  {
    rcon_password = g_strdup(getenv("XQF_RCON_PASSWORD"));
  }
  else
  {
    while(!buf || !*buf)
    {
      // translator: readline prompt
      buf = readline(_("Password: "));
    }
    rcon_password = g_strdup(buf);
  }
  
  buf = readline(prompt);
  while(buf)
  {
    res = rcon_send(buf);
    if (res < 0) {
      failed("send");
    }
    else
    {
      if (wait_read_timeout(rcon_fd, 5, 0) <= 0)
      {
	printf ("*** timeout waiting for reply\n");
      }
      else
      {
	char* msg;
	/* see if more packets arrive within a short interval */
	while(wait_read_timeout(rcon_fd, 0, 50000) > 0)
	{
	  msg = rcon_receive();
	  printf("%s", msg);
	  g_free(msg);
	}
      }
    }
    if(*buf)
      add_history(buf);
    free(buf);
    buf = readline(prompt);
  }
  return 0;
}
#endif
