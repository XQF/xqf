/* XQF - Quake server browser and launcher
 * Splash screen
 * Copyright (C) 2002 Ludwig Nussel <l-n@users.sourceforge.net>
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
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "splash.h"
#include "xqf-ui.h"
#include "loadpixmap.h"
#include "i18n.h"
#include "pref.h"

static GtkWidget* splashscreen;

void destroy_splashscreen(void)
{
  if(!splashscreen) return;

  gtk_widget_destroy(splashscreen);
  splashscreen = NULL;
}

void create_splashscreen (void)
{
  GtkWidget *vbox1;
  GtkWidget *logo;
  GtkWidget *entry;
  GtkWidget *progress;

  if(splashscreen) return;

  if(!default_show_splash) return;

  splashscreen = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_object_set_data (GTK_OBJECT (splashscreen), "splashscreen", splashscreen);
  gtk_window_set_title (GTK_WINDOW (splashscreen), _("XQF: Loading"));
  gtk_window_set_position (GTK_WINDOW (splashscreen), GTK_WIN_POS_CENTER);
  gtk_window_set_modal (GTK_WINDOW (splashscreen), TRUE);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_ref (vbox1);
  gtk_object_set_data_full (GTK_OBJECT (splashscreen), "vbox1", vbox1,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (splashscreen), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 3);

  logo = load_pixmap (splashscreen, "xqflogo.png");
  gtk_widget_ref (logo);
  gtk_object_set_data_full (GTK_OBJECT (splashscreen), "logo", logo,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (logo);
  gtk_box_pack_start (GTK_BOX (vbox1), logo, TRUE, TRUE, 0);

  entry = gtk_entry_new ();
  gtk_widget_ref (entry);
  gtk_object_set_data_full (GTK_OBJECT (splashscreen), "entry", entry,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (entry);
  gtk_box_pack_start (GTK_BOX (vbox1), entry, FALSE, FALSE, 0);
  gtk_entry_set_editable (GTK_ENTRY (entry), FALSE);
  gtk_entry_set_text (GTK_ENTRY (entry), _("Loading ..."));

  progress = gtk_progress_bar_new ();
  gtk_widget_ref (progress);
  gtk_object_set_data_full (GTK_OBJECT (splashscreen), "progress", progress,
                            (GtkDestroyNotify) gtk_widget_unref);
  gtk_widget_show (progress);
  gtk_box_pack_start (GTK_BOX (vbox1), progress, FALSE, FALSE, 0);
  gtk_progress_configure (GTK_PROGRESS (progress), 42, 0, 100);

  gtk_signal_connect (GTK_OBJECT (splashscreen), "destroy",
		      GTK_SIGNAL_FUNC (destroy_splashscreen), NULL);

  gtk_widget_show (splashscreen);

  // force it to draw now.
  gdk_flush();

  // go into main loop, processing events.
  while (gtk_events_pending() || !GTK_WIDGET_REALIZED(logo))
    gtk_main_iteration();

  // test
  while (gdk_events_pending())
    gdk_flush();

}

static guint current_progress;

/* set percentage on splash screen, thanks to lopster for this code */
void splash_set_progress(const char* message, guint per)
{
  GtkProgressBar* progress;
  GtkEntry* entry;

  if (!splashscreen) return;

  if (per > 100) per = 100;

  current_progress = per;

  progress = GTK_PROGRESS_BAR(lookup_widget(splashscreen, "progress"));
  entry = GTK_ENTRY(lookup_widget(splashscreen, "entry"));
  if (message)
    gtk_entry_set_text(entry, message);
  gtk_progress_bar_update(progress, per*1.0/100);

  while (gtk_events_pending())
    gtk_main_iteration();
}

void splash_increase_progress(const char* message, guint per)
{
  if(!splashscreen) return;
  splash_set_progress(message,current_progress+per);
}
