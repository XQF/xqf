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
#include <string.h>	/* strlen, strstr, strcpy */

#include "xqf-ui.h"
#include "i18n.h"
#include "redial.h"
#include "debug.h"
#include "dialogs.h"
#include "stat.h"
#include "host.h"
#include "server.h"
#include "srv-list.h"


static gboolean launchnow;
static GtkWidget* redial_window;

static unsigned countdown;
static const unsigned default_redial_wait = 5;

static int timeoutid;

struct server_props* srv_props;

static void stat_redial_close_handler (struct stat_job *job, int killed);
static void stat_redial_server_handler (struct stat_job *job, struct server *s);
static void set_redial_label(const char* name, gboolean waiting);

static void on_launchbutton_clicked (GtkButton *button, gpointer user_data)
{
    launchnow = TRUE;
    if(timeoutid!=-1)
	gtk_timeout_remove(timeoutid);
    gtk_widget_destroy(redial_window);
}


static void on_cancelbutton_clicked (GtkButton *button, gpointer user_data)
{
    launchnow = FALSE;
    if(timeoutid!=-1)
	gtk_timeout_remove(timeoutid);
    gtk_widget_destroy(redial_window);
}

/**
 * called on every gtk_timeout, updates progressbar until default_redial_wait
 * is reached, starts a new stat_process then
 */
static gboolean redial_countdown (struct server* s)
{
    void* blub = gtk_object_get_data(GTK_OBJECT(redial_window),"secondsprogress");
    struct condef* con;
    countdown++;
    if(countdown>default_redial_wait)
    {
	countdown=0;
	gtk_progress_set_value (GTK_PROGRESS (blub), countdown);
	debug(3,"countdown 0, querying server");
	con = condef_new (s);
	stat_process = stat_job_create (NULL, NULL, 
			server_list_prepend (NULL, con->s), NULL);
	stat_process->data = con;

	stat_process->close_handlers = g_slist_prepend (
		      stat_process->close_handlers, stat_redial_close_handler);

	stat_process->server_handlers = g_slist_append (
			stat_process->server_handlers, stat_redial_server_handler);

	stat_start (stat_process);

	set_redial_label(con->s->name, FALSE);
	timeoutid=-1;
	return FALSE;
    }
    else
    {
	gtk_progress_set_value (GTK_PROGRESS (blub), countdown);
	return TRUE;
    }
}

static void stat_redial_server_handler (struct stat_job *job, struct server *s)
{
  /* Don't spend time on host name lookups */

  debug(3,"server %s refreshed",s->name);

  if (job->hosts) {
    host_list_free (job->hosts);
    job->hosts = NULL;
  }
}


/** set server name for redial dialog */
static void set_redial_label(const char* name, gboolean waiting)
{
    GtkWidget* label = gtk_object_get_data(GTK_OBJECT(redial_window),"label");
    char* text;
    if(waiting)
	text = g_strdup_printf(_("Waiting for free slots on\n%s"),name);
    else
	text = g_strdup_printf(_("Refreshing\n%s"),name);
    gtk_label_set_text(GTK_LABEL(label), text);
    g_free(text);
}

/** stat_process finished, check if slots are free otherwise setup new timeout */
static void stat_redial_close_handler (struct stat_job *job, int killed)
{
    struct condef* con = (struct condef *) job->data;
    job->data = NULL;

    stat_process = NULL;

    debug(3,"stat process done");

    if (!con)
	return;

    if (!killed)
    {
	if (!server_need_redial(con->s, srv_props))
	{
	    // ok, free slot. launch!
	    launchnow = TRUE;
	    gtk_widget_destroy(redial_window);
	}
	else
	{
	    timeoutid = gtk_timeout_add (1000, (GtkFunction)redial_countdown, (gpointer)con->s);
	    set_redial_label(con->s->name, TRUE);
	}
    }
    else
    {
	launchnow = FALSE;
	gtk_widget_destroy(redial_window);
    }
    server_clist_refresh_server (con->s);
    condef_free (con);
    return;
}

/** glade function */
static GtkWidget* create_redialwindow (void)
{
  GtkWidget *redialwindow;
  GtkWidget *vbox1;
  GtkWidget *label;
  GtkWidget *secondsprogress;
  GtkWidget *hbuttonbox1;
  GtkWidget *launchbutton;
  GtkWidget *cancelbutton;

  /** window code modified from glade */
  redialwindow = dialog_create_modal_transient_window(_("XQF: Redialing"), TRUE, FALSE, NULL);

#if 0
  redialwindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (redialwindow), "redialwindow", redialwindow);
  gtk_window_set_title (GTK_WINDOW (redialwindow), _("XQF: Redialing"));
  gtk_window_set_modal (GTK_WINDOW (redialwindow), TRUE);
#endif

  vbox1 = gtk_vbox_new (FALSE, 5);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (redialwindow), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 14);

  label = gtk_label_new (_("***\n***"));
  gtk_widget_ref (label);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "label", label,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (label);
  gtk_box_pack_start (GTK_BOX (vbox1), label, FALSE, FALSE, 0);

  secondsprogress = gtk_progress_bar_new ();
  gtk_widget_ref (secondsprogress);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "secondsprogress", secondsprogress,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (secondsprogress);
  gtk_box_pack_start (GTK_BOX (vbox1), secondsprogress, FALSE, FALSE, 10);
  gtk_progress_configure (GTK_PROGRESS (secondsprogress), 4, 0, 10);

  hbuttonbox1 = gtk_hbutton_box_new ();
  gtk_widget_ref (hbuttonbox1);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "hbuttonbox1", hbuttonbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (hbuttonbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbuttonbox1, TRUE, FALSE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox1), GTK_BUTTONBOX_SPREAD);

  launchbutton = gtk_button_new_with_label (_("Launch now"));
  gtk_widget_ref (launchbutton);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "launchbutton", launchbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (launchbutton);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), launchbutton);
  GTK_WIDGET_SET_FLAGS (launchbutton, GTK_CAN_DEFAULT);

  cancelbutton = gtk_button_new_with_label (_("Cancel"));
  gtk_widget_ref (cancelbutton);
  gtk_object_set_data_full (GTK_OBJECT (redialwindow), "cancelbutton", cancelbutton,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (cancelbutton);
  gtk_container_add (GTK_CONTAINER (hbuttonbox1), cancelbutton);
  GTK_WIDGET_SET_FLAGS (cancelbutton, GTK_CAN_DEFAULT);

  gtk_signal_connect (GTK_OBJECT (launchbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_launchbutton_clicked),
                      NULL);
  gtk_signal_connect (GTK_OBJECT (cancelbutton), "clicked",
                      GTK_SIGNAL_FUNC (on_cancelbutton_clicked),
                      NULL);

  gtk_widget_grab_focus (cancelbutton);
  gtk_widget_grab_default (cancelbutton);
  return redialwindow;
}

/** /glade function */

/** open redial dialog, return true if game should be launched, false otherwise */

gboolean redial_dialog (struct server* s, struct server_props* props)
{
    GtkWidget* progress;

    if(!s)
	return FALSE;

    launchnow = FALSE;
    timeoutid = -1;

    countdown = 0;
    srv_props = props;

    redial_window = create_redialwindow();
    progress = gtk_object_get_data(GTK_OBJECT(redial_window),"secondsprogress");
    gtk_progress_configure (GTK_PROGRESS (progress), 0, 0, default_redial_wait);
    
    set_redial_label(s->name, TRUE);
    
    gtk_widget_show(redial_window);

    timeoutid = gtk_timeout_add (1000, (GtkFunction)redial_countdown, (gpointer)s);

    gtk_main ();

    unregister_window (redial_window);

    return launchnow;
}
