/* XQF - Quake server browser and launcher
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
.*
.*
.* trayicon.c:
.*
 *  Copyright (C) Jochen Baier <email@jochen-baier.de>
.*
.* based on:
.*
.*   eggtrayicon.c api
.*
 *   Copyright (C) Anders Carlsson <andersca@gnu.org>
.*
.*   magic transparent KDE icon:
.*
.*   Copyright (C) Jochen Baier <email@jochen-baier.de>
.*
 */


#include <gtk/gtk.h>
#include "gnuconfig.h"

#ifndef USE_GTK2
/*dummy functions for gtk 1.x, this avoid a lot ifdef´s*/
void tray_init (GtkWidget* window)
{
  gtk_widget_show(window);
};
void tray_delete_event_hook (void) {/**/};
void tray_icon_set_tooltip (gchar *tip) {/**/};
void tray_icon_stop_animation (void) {/**/};
void tray_icon_start_animation (void) {/**/};
extern void tray_done (void) {/**/};
extern gboolean tray_icon_work(void) {return FALSE;};
#else

#include <stdio.h>
#include <gtk/gtkplug.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <X11/Xatom.h>
#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <stdlib.h>

#include "i18n.h"
#include "pref.h"
#include "xqf.h"
#include "debug.h"
#include "loadpixmap.h"
#include "srv-list.h"
#include "source.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define SYSTEM_TRAY_BEGIN_MESSAGE 1
#define SYSTEM_TRAY_CANCEL_MESSAGE 2

#define EGG_TYPE_TRAY_ICON (egg_tray_icon_get_type ())
#define EGG_TRAY_ICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), EGG_TYPE_TRAY_ICON, EggTrayIcon))
#define EGG_TRAY_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), EGG_TYPE_TRAY_ICON, EggTrayIconClass))
#define EGG_IS_TRAY_ICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), EGG_TYPE_TRAY_ICON))
#define EGG_IS_TRAY_ICON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), EGG_TYPE_TRAY_ICON))
#define EGG_TRAY_ICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), EGG_TYPE_TRAY_ICON, EggTrayIconClass))

#define EVENT_REFRESH 2
#define EVENT_UPDATE 3
#define N_SERVER 100
#define ANI_TIME 10

static const char kde_window_manger[]="KWin";

typedef struct _EggTrayIcon EggTrayIcon;
typedef struct _EggTrayIconClass EggTrayIconClass;

struct _EggTrayIcon
{
  GtkPlug parent_instance;
  guint stamp;
  Atom selection_atom;
  Atom manager_atom;
  Atom system_tray_opcode_atom;
  Window manager_window;
  GtkWidget *box;
  GtkWidget *background;
  GtkWidget *image;
  GdkPixbuf *default_pix;
  gboolean ready;
};

struct _EggTrayIconClass
{
  GtkPlugClass parent_class;
};

GType egg_tray_icon_get_type (void);
static GtkPlugClass *parent_class = NULL;

EggTrayIcon *egg_tray_icon_new (const gchar *name, GdkPixbuf *pix);
guint egg_tray_icon_send_message (EggTrayIcon *icon, gint timeout, const char *message, gint len);
void egg_tray_icon_cancel_message (EggTrayIcon *icon, guint id);
static void egg_tray_icon_init (EggTrayIcon *icon);
static void egg_tray_icon_class_init (EggTrayIconClass *klass);
static void egg_tray_icon_unrealize (GtkWidget *widget);
static void egg_tray_icon_update_manager_window (EggTrayIcon *icon);
GdkPixbuf *kde_dock_background(EggTrayIcon *icon);

GtkTooltips *tray_icon_tips = NULL;
EggTrayIcon *tray_icon = NULL;

GdkPixbuf *frame_basic;

GtkWidget *menu = NULL;
GtkWidget *refresh_item;
GtkWidget *update_item;
GtkWidget *stop_item;
GtkWidget *show_item;
GtkWidget *hide_item;
GtkWidget *exit_item; 

typedef struct _frame frame;

struct _frame
{
  GdkPixbuf *pix;
  gint delay;
};

typedef struct _animation animation;

struct _animation
{
  GArray *array;
  frame current_frame;
  gint frame_counter;
  gint time_counter;
  gboolean loop;
};

static animation *busy_ani;
static animation *ready_ani;

static GtkWidget *window; /* copy of the main window */
static gint x_pos = 50;
static gint y_pos = 50;

static gboolean animation_running = FALSE;
static gboolean refresh_update=FALSE;

static gint animation_timer=0;
static gint animation_callback (gpointer nothing);

gboolean tray_icon_work(void)
{
  if (!tray_icon)
    return FALSE;
  
  return tray_icon->ready;
}

/*user close main window-> hide it*/
void tray_delete_event_hook(void)
{
  gtk_window_get_position(GTK_WINDOW(window), &x_pos, &y_pos);
  gtk_widget_hide(window);
}

void show_main_window_call(GtkWidget * button, void *data)
{
  static gboolean first_call = TRUE;

  if (first_call) {
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_widget_show(window);
    first_call = FALSE;
  } else {
    gtk_window_move(GTK_WINDOW(window), x_pos, y_pos);
    gtk_widget_show(window);
   }
}

void hide_main_window_call(GtkWidget * button, void *data)
{
  gtk_window_get_position(GTK_WINDOW(window), &x_pos, &y_pos);
  gtk_widget_hide(window);
}

void exit_call(GtkWidget * button, void *data)
{
  gtk_widget_destroy(GTK_WIDGET(tray_icon));
  gtk_main_quit();
}

void set_menu_sens(void)
{
  gtk_widget_set_sensitive (show_item, !GTK_WIDGET_VISIBLE(window));
  gtk_widget_set_sensitive (hide_item, GTK_WIDGET_VISIBLE(window));
  gtk_widget_set_sensitive (stop_item, refresh_update);
}

static void
tray_icon_pressed(GtkWidget * button, GdkEventButton * event, EggTrayIcon * icon)
{

  /*right click */
  if (event->button == 1) {
    if (GTK_WIDGET_VISIBLE(window)) {
      gtk_window_get_position(GTK_WINDOW(window), &x_pos, &y_pos);
      gtk_widget_hide(window);
    } else {
      show_main_window_call(NULL, NULL);
    }
  }

  /*left click */
  if (event->button == 3) {
  
  set_menu_sens ();

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 0,
       gtk_get_current_event_time());
  }
}

void tray_icon_set_tooltip(gchar * tip)
{
  if (!tray_icon || !tray_icon->ready )
    return;

  if(!tray_icon_tips)
    tray_icon_tips = gtk_tooltips_new();
  
  gtk_tooltips_set_tip(GTK_TOOLTIPS(tray_icon_tips),
      GTK_WIDGET(tray_icon), tip, tip);
}

static gint animation_callback(gpointer _ani)
{
  animation *ani= _ani;
  
  if (ani->frame_counter==0 && ani->time_counter==0) {
    ani->current_frame=g_array_index (ani->array, frame, ani->frame_counter);
    gtk_image_set_from_pixbuf(GTK_IMAGE(tray_icon->image), ani->current_frame.pix);
  }
  
  if (ani->time_counter < ani->current_frame.delay) {
    ani->time_counter++;
    return TRUE;
  }
  
  if (ani->frame_counter == ani->array->len-1) {
    ani->frame_counter= 0;
    ani->time_counter =0;
  
    if (ani->loop) {
      return TRUE;
    }
    else {
       animation_running=FALSE;
      return FALSE;
    }
  }
  
  ani->frame_counter++;
  ani->current_frame=g_array_index (ani->array, frame, ani->frame_counter);
  gtk_image_set_from_pixbuf(GTK_IMAGE(tray_icon->image), ani->current_frame.pix);
  ani->time_counter=0;

  return TRUE;
}

void tray_icon_start_animation(void)
{

  if (!tray_icon_work())
    return;

  refresh_update=TRUE;

  if (animation_running) {
    if (animation_timer) g_source_remove(animation_timer);
  }
  
  busy_ani->frame_counter = 0;
  busy_ani->time_counter = 0;
  
  animation_timer = g_timeout_add(ANI_TIME, (GtkFunction) animation_callback, busy_ani);
  animation_running = TRUE;
}
  
void tray_icon_stop_animation(void)
{
    if (!tray_icon_work())
      return;

  refresh_update=FALSE;
  set_menu_sens ();
  
  if (animation_running) {
    if (animation_timer) 
      g_source_remove(animation_timer);
  
    ready_ani->frame_counter = 0;
    ready_ani->time_counter = 0;

    animation_timer = g_timeout_add(ANI_TIME, (GtkFunction) animation_callback, ready_ani);
  }
}

static animation *tray_icon_load_animation (gchar *name, gboolean loop)
{

  gint i=0;
  gint line_number=0;
  gboolean eof=FALSE;
  gchar *ani_file;
  gchar *content;
  gchar *begin;
  gchar *line;
  gchar *png_start;
  gchar *delay_string=NULL;
  gchar *png_filename;

  animation *ani;
  frame tmp_frame;

  if(!name) return NULL;

  {
    char* tmp = g_strconcat("trayicon", G_DIR_SEPARATOR_S, name, NULL);
    ani_file = find_pixmap_directory(tmp);
    g_free(tmp);
  }

  if(!ani_file || !g_file_get_contents (ani_file, &content, NULL, NULL))
  {
    xqf_warning("Could not load animation file '%s'", name);
    g_free(ani_file);
    return NULL;
  }

  ani=g_new(animation, 1);
  ani->array = g_array_new (FALSE, FALSE, sizeof (frame));
  ani->frame_counter = 0;
  ani->time_counter = 0;
  ani->loop = loop;
  
  begin=content;
  
  while (!eof) {
  
      if  ((content[i] == '\n') || (content[i] == '\0') ) {
      
      if ( content[i-1] != '\n') {
      
      line_number++;
      
      do {
      
        line=  g_strndup (begin, &content[i]-begin);
        if (strlen (line) <= 1) {
          xqf_warning (_("Error in file: %s line: %d\n"), ani_file, line_number);
          break;
        }

        png_start =g_strrstr (line, ".png");
        if (!png_start) {
          xqf_warning (_("Error in file: %s line: %d\n"), ani_file, line_number);
          break;
        }
        
        delay_string=g_strdup (png_start+4);
        g_strstrip(delay_string);
        
        if (strlen (delay_string) < 1) {
          xqf_warning (_("Error in file: %s line: %d\n"), ani_file, line_number);
          break;
        }

        tmp_frame.delay= strtol(delay_string, NULL, 10);
        if (tmp_frame.delay== 0) {
         xqf_warning (_("Error in file: %s line: %d\n"), ani_file, line_number);
          break;
        }
        
        if (tmp_frame.delay != 1)
          tmp_frame.delay=tmp_frame.delay / 2;

        *(png_start+4)=0; /*cut anything behind ".png" */

        png_filename= g_strconcat(PACKAGE_DATA_DIR, G_DIR_SEPARATOR_S,"default",
          G_DIR_SEPARATOR_S, "trayicon", G_DIR_SEPARATOR_S, line, NULL);
        
        tmp_frame.pix = load_pixmap_as_pixbuf (png_filename);
        if (tmp_frame.pix == NULL) {
          xqf_warning (_("Error in file: %s line: %d\n"), ani_file, line_number);
          break;
        }
        
        if (png_filename)
          g_free (png_filename);
        
        g_array_append_val (ani->array, tmp_frame);
      
      } while (0);
      
      if (line)
        g_free(line);
      if (delay_string)
        g_free(delay_string);
      
      }
    
    if (content[i] == '\0' ||  (content[i] == '\n'  && content[i+1] == '\0' ))
      eof=TRUE;
    else 
      begin=&content[i]+1;
    
    }
  i++;
  }

  if (content)
    g_free(content);
  if (ani_file)
    g_free(ani_file);
  
  return ani;
}

void tray_create_menu (void)
{

  GtkWidget *separator1 = NULL;

  menu = gtk_menu_new();
    
  refresh_item = gtk_menu_item_new_with_label(_("Refresh"));
  g_signal_connect(G_OBJECT(refresh_item), "activate",
   G_CALLBACK(refresh_n_server), GINT_TO_POINTER(N_SERVER));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), refresh_item);
  
  update_item= gtk_menu_item_new_with_label(_("Update"));
  g_signal_connect(G_OBJECT(update_item), "activate",
   G_CALLBACK(update_source_callback), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), update_item);
  
  stop_item = gtk_menu_item_new_with_label(_("Stop"));
  g_signal_connect(G_OBJECT(stop_item), "activate",
   G_CALLBACK(stop_callback), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), stop_item);
  
  separator1 = gtk_menu_item_new();
  gtk_widget_show(separator1);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), separator1);
  gtk_widget_set_sensitive(separator1, FALSE);
  
  // translator: show main window
  show_item = gtk_menu_item_new_with_label(_("Show"));
  g_signal_connect(G_OBJECT(show_item), "activate",
   G_CALLBACK(show_main_window_call), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), show_item);
  
  // translator: hide main window
  hide_item= gtk_menu_item_new_with_label(_("Hide"));
  g_signal_connect(G_OBJECT(hide_item), "activate",
   G_CALLBACK(hide_main_window_call), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), hide_item);
  
  exit_item = gtk_menu_item_new_with_label(_("Exit"));
  g_signal_connect(G_OBJECT(exit_item), "activate",
   G_CALLBACK(exit_call), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), exit_item);

  gtk_widget_show_all(menu);
}

void tray_init(GtkWidget * main_window)
{
  gdk_pixbuf_xlib_init (GDK_DISPLAY(), DefaultScreen (GDK_DISPLAY()));

  /* local copy */
  window = main_window;
  
  gtk_window_get_position(GTK_WINDOW(window), &x_pos, &y_pos);

  tray_create_menu();

  busy_ani = tray_icon_load_animation ("busy.ani", TRUE);
  ready_ani = tray_icon_load_animation("ready.ani", FALSE);

  frame_basic = load_pixmap_as_pixbuf("trayicon/frame_basic.png");
   
  if(frame_basic)
    tray_icon = egg_tray_icon_new ("xqf", frame_basic);
  
  if (tray_icon && tray_icon->ready)
  {
    g_signal_connect(tray_icon, "button_press_event",
      G_CALLBACK(tray_icon_pressed),tray_icon);

    gtk_widget_hide(window);
  }
  else
    gtk_widget_show(window);
}

void tray_done (void)
{
  if (busy_ani)
  {
    if (busy_ani->array)
      g_array_free (busy_ani->array, TRUE);
    
    if (ready_ani->array)
       g_array_free (ready_ani->array, TRUE);

    g_free(busy_ani);
  }
  
  g_free(ready_ani);
  
  //FIXME
  //if (tray_icon)
  //gtk_widget_destroy(GTK_WIDGET(tray_icon));
}

/*eggtrayicon stuff*/
GType
egg_tray_icon_get_type (void)
{
  static GType our_type = 0;
  
  our_type = g_type_from_name ("EggTrayIcon");
  
  if (our_type == 0) {
      static const GTypeInfo our_info =
      {
    sizeof (EggTrayIconClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) egg_tray_icon_class_init,
    NULL, /* class_finalize */
    NULL, /* class_data */
    sizeof (EggTrayIcon),
    0,    /* n_preallocs */
    (GInstanceInitFunc) egg_tray_icon_init
      };
    
      our_type = g_type_register_static (GTK_TYPE_PLUG, "EggTrayIcon", &our_info, 0);
  }
  else if (parent_class == NULL) {
    /* we're reheating the old class from a previous instance -  engage ugly hack =( */
    egg_tray_icon_class_init ((EggTrayIconClass *)g_type_class_peek (our_type));
  }

  return our_type;
}

static void
egg_tray_icon_init (EggTrayIcon *icon)
{
  icon->stamp = 1;
  gtk_widget_add_events (GTK_WIDGET (icon), GDK_PROPERTY_CHANGE_MASK);
}

static void
egg_tray_icon_class_init (EggTrayIconClass *klass)
{
  GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;

  parent_class = g_type_class_peek_parent (klass);
  widget_class->unrealize = egg_tray_icon_unrealize;
}

static GdkFilterReturn
egg_tray_icon_manager_filter (GdkXEvent *xevent, GdkEvent *event, gpointer user_data)
{
  EggTrayIcon *icon = user_data;
  XEvent *xev = (XEvent *)xevent;

  if (xev->xany.type == ClientMessage &&
      xev->xclient.message_type == icon->manager_atom &&
      xev->xclient.data.l[1] == icon->selection_atom)
    {
      egg_tray_icon_update_manager_window (icon);
    }
  else if (xev->xany.window == icon->manager_window)
    {
      if (xev->xany.type == DestroyNotify)
    {
      egg_tray_icon_update_manager_window (icon);
    }
    }
  
  return GDK_FILTER_CONTINUE;
}

static void
egg_tray_icon_unrealize (GtkWidget *widget)
{
  EggTrayIcon *icon = EGG_TRAY_ICON (widget);
  GdkWindow *root_window=NULL;

  if (icon->manager_window != None)
    {
      gdk_window_remove_filter (root_window, egg_tray_icon_manager_filter, icon);
    
      if (GTK_WIDGET_CLASS (parent_class)->unrealize)
        (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
  }
}

static void
egg_tray_icon_send_manager_message (EggTrayIcon *icon,
                    long         message,
                    Window       window,
                    long         data1,
                    long         data2,
                    long         data3)
{
  XClientMessageEvent ev;
  Display *display;
  
  ev.type = ClientMessage;
  ev.window = window;
  ev.message_type = icon->system_tray_opcode_atom;
  ev.format = 32;
  ev.data.l[0] = gdk_x11_get_server_time (GTK_WIDGET (icon)->window);
  ev.data.l[1] = message;
  ev.data.l[2] = data1;
  ev.data.l[3] = data2;
  ev.data.l[4] = data3;

  display = gdk_display;

  gdk_error_trap_push ();
  XSendEvent (display,
          icon->manager_window, False, NoEventMask, (XEvent *)&ev);
  XSync (display, False);
  gdk_error_trap_pop ();

  icon->ready=TRUE;
}

static void
egg_tray_icon_send_dock_request (EggTrayIcon *icon)
{

  egg_tray_icon_send_manager_message (icon,
                      SYSTEM_TRAY_REQUEST_DOCK,
                      icon->manager_window,
                      gtk_plug_get_id (GTK_PLUG (icon)),
                      0, 0);
}

/*from gdk-pixbuf-xlib-drawabel.c*/
static gboolean
xlib_window_is_viewable (Window w)
{
  XWindowAttributes wa;
  
  while (w != 0) {
    Window parent, root, *children;
    int nchildren;
    
    XGetWindowAttributes (GDK_DISPLAY(), w, &wa);
    
    if (wa.map_state != IsViewable) {
      return FALSE;
    }

    if (!XQueryTree (GDK_DISPLAY(), w, &root,
    &parent, &children, &nchildren))
      return FALSE;
    
    if (nchildren > 0)
      XFree (children);
    
    if ((parent == root) || (w == root))
      return TRUE;
    
    w = parent;
  }
  return FALSE;
}

gboolean kde_dock (EggTrayIcon *icon)
{

  Window win;
  Window data;
  Atom kde_tray_atom = XInternAtom (GDK_DISPLAY(), "_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR", False);
  Atom kde_dock_atom = XInternAtom (GDK_DISPLAY(), "KWM_DOCKWINDOW", False); //KDE 2 support not tested

  win = GDK_WINDOW_XID (GTK_WIDGET (icon)->window);
  
  XSetWindowBackgroundPixmap(GDK_DISPLAY(), win, ParentRelative);
  
  data = 1;
  XChangeProperty (GDK_DISPLAY(), win, kde_dock_atom, XA_WINDOW, 32,
  PropModeReplace, (unsigned char *)&data, 1);
  data = GDK_WINDOW_XID (window->window);
  XChangeProperty (GDK_DISPLAY(), win, kde_tray_atom, XA_WINDOW, 32,
  PropModeReplace, (unsigned char *)&data, 1);
  
  XMapWindow (GDK_DISPLAY(), win);
  XFlush (GDK_DISPLAY());
  
  return (xlib_window_is_viewable(win));
}

GdkPixbuf *kde_dock_background(EggTrayIcon *icon)
{

  Window win;
  XWindowAttributes wa;
  GdkPixbuf *background;

  win = GDK_WINDOW_XID (GTK_WIDGET (icon)->window);
  XGetWindowAttributes (GDK_DISPLAY(),win, &wa);

  background=gdk_pixbuf_xlib_get_from_drawable (NULL, win, 0, NULL,
    0, 0, 0, 0, wa.width, wa.height);
  
  return background;
}

static void
egg_tray_icon_update_manager_window (EggTrayIcon *icon)
{

  static GdkPixbuf *background_pixbuf;
  const gchar *window_manager=NULL;
  
  if (icon->manager_window != None) {
    GdkWindow *gdkwin;
    gdkwin = gdk_window_lookup (icon->manager_window);
    gdk_window_remove_filter (gdkwin, egg_tray_icon_manager_filter, icon);
  }
  
  XGrabServer (GDK_DISPLAY());
  
  icon->manager_window = XGetSelectionOwner (GDK_DISPLAY(),
  icon->selection_atom);
  
  if (icon->manager_window != None)
    XSelectInput (GDK_DISPLAY(), icon->manager_window, StructureNotifyMask);
  
  XUngrabServer (GDK_DISPLAY());
  XFlush (GDK_DISPLAY());
  
  if (icon->manager_window == None)
    return;

  window_manager=gdk_x11_screen_get_window_manager_name (gdk_screen_get_default());

  if ( !g_ascii_strcasecmp(window_manager, kde_window_manger) && kde_dock (icon)) {

    if ((background_pixbuf=kde_dock_background(icon)) !=NULL) {
    
      icon->box= gtk_fixed_new ();
      gtk_fixed_set_has_window(GTK_FIXED (icon->box),TRUE);
      gtk_container_add(GTK_CONTAINER(icon), icon->box);
      
      icon->image=gtk_image_new ();
      gtk_image_set_from_pixbuf(GTK_IMAGE(icon->image), icon->default_pix);
    
      icon->background =gtk_image_new ();
      gtk_image_set_from_pixbuf(GTK_IMAGE(icon->background), background_pixbuf);
      
      gtk_fixed_put (GTK_FIXED (icon->box), GTK_WIDGET(icon->background), 0, 0);
      gtk_fixed_put (GTK_FIXED (icon->box), GTK_WIDGET(icon->image), 0, 0);
      
      gtk_widget_show (icon->background);
      gtk_widget_show (icon->image);
      gtk_widget_show(icon->box);
      
      icon->ready=TRUE;
    }
  }  else {
  
      icon->box=gtk_event_box_new ();
      gtk_container_add(GTK_CONTAINER(icon), icon->box);
      
      icon->image=gtk_image_new ();
      gtk_image_set_from_pixbuf(GTK_IMAGE(icon->image),icon->default_pix);
      gtk_container_add(GTK_CONTAINER(icon->box), icon->image);
      
      gtk_widget_show (icon->image);
      gtk_widget_show(icon->box);
      
      GdkWindow *gdkwin;
      
      gdkwin = gdk_window_lookup (icon->manager_window);
      gdk_window_add_filter (gdkwin, egg_tray_icon_manager_filter, icon);
      
      /* Send a request that we'd like to dock */
      egg_tray_icon_send_dock_request (icon);
  }
}

EggTrayIcon *
egg_tray_icon_new (const char *name, GdkPixbuf *pix)
{
  EggTrayIcon *icon;
  char buffer[256];
  GdkWindow *root_window;
  Screen *xscreen=DefaultScreenOfDisplay (GDK_DISPLAY());

  g_return_val_if_fail  (pix!= NULL, NULL);
  
  icon = g_object_new (EGG_TYPE_TRAY_ICON, NULL);
  gtk_window_set_title (GTK_WINDOW (icon), name);

  gtk_plug_construct (GTK_PLUG (icon), 0);
  gtk_widget_realize (GTK_WIDGET (icon));
 
  icon->ready=FALSE;
  icon->default_pix=pix;

  /* Now see if there's a manager window around */
  g_snprintf (buffer, sizeof (buffer), "_NET_SYSTEM_TRAY_S%d",
          XScreenNumberOfScreen (xscreen));
  
  icon->selection_atom = XInternAtom (DisplayOfScreen (xscreen),
                      buffer, False);
  
  icon->manager_atom = XInternAtom (DisplayOfScreen (xscreen),
                    "MANAGER", False);
  
  icon->system_tray_opcode_atom = XInternAtom (DisplayOfScreen (xscreen),
                           "_NET_SYSTEM_TRAY_OPCODE", False);

   gtk_window_present (GTK_WINDOW (icon));
  egg_tray_icon_update_manager_window (icon);
  root_window = gdk_window_lookup (gdk_x11_get_default_root_xwindow ());
  
  /* Add a root window filter so that we get changes on MANAGER */
  gdk_window_add_filter (root_window, egg_tray_icon_manager_filter, icon);
              
  return icon;
}

guint
egg_tray_icon_send_message (EggTrayIcon *icon,
                gint         timeout,
                const gchar *message,
                gint         len)
{
  guint stamp;
  
  g_return_val_if_fail (EGG_IS_TRAY_ICON (icon), 0);
  g_return_val_if_fail (timeout >= 0, 0);
  g_return_val_if_fail (message != NULL, 0);
             
  if (icon->manager_window == None)
    return 0;

  if (len < 0)
    len = strlen (message);

  stamp = icon->stamp++;
  
  /* Get ready to send the message */
  egg_tray_icon_send_manager_message (icon, SYSTEM_TRAY_BEGIN_MESSAGE,
                      (Window)gtk_plug_get_id (GTK_PLUG (icon)),
                      timeout, len, stamp);

  /* Now to send the actual message */
  gdk_error_trap_push ();
  while (len > 0)
    {
      XClientMessageEvent ev;
      Display *xdisplay;

      xdisplay = GDK_DISPLAY();

      ev.type = ClientMessage;
      ev.window = (Window)gtk_plug_get_id (GTK_PLUG (icon));
      ev.format = 8;
      ev.message_type = XInternAtom (xdisplay,
                     "_NET_SYSTEM_TRAY_MESSAGE_DATA", False);
      if (len > 20)
    {
      memcpy (&ev.data, message, 20);
      len -= 20;
      message += 20;
    }
      else
    {
      memcpy (&ev.data, message, len);
      len = 0;
    }

      XSendEvent (GDK_DISPLAY(),
          icon->manager_window, False, StructureNotifyMask, (XEvent *)&ev);
      XSync (GDK_DISPLAY(), False);
    }
  gdk_error_trap_pop ();

  return stamp;
}

void
egg_tray_icon_cancel_message (EggTrayIcon *icon,
                  guint        id)
{
  g_return_if_fail (EGG_IS_TRAY_ICON (icon));
  g_return_if_fail (id > 0);
  
  egg_tray_icon_send_manager_message (icon, SYSTEM_TRAY_CANCEL_MESSAGE,
                      (Window)gtk_plug_get_id (GTK_PLUG (icon)),
                      id, 0, 0);
}

#endif /*GTK 2*/
