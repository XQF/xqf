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

#include <sys/types.h>	/* chmod, waitpid, kill */
#include <string.h>	/* memchr, strlen */
#include <unistd.h>	/* execvp, fork, _exit, chdir, read, close */
                        /* write, fcntl, sleep */
#include <fcntl.h>	/* fcntl */
#include <sys/wait.h>	/* waitpid */
#include <errno.h>	/* errno */
#include <signal.h>     /* kill, signal... */
#include <stdlib.h>     /* setenv */

#include <gtk/gtk.h>

#include "i18n.h"
#include "xqf.h"
#include "game.h"
#include "pref.h"
#include "utils.h"
#include "dialogs.h"
#include "server.h"
#include "launch.h"
#include "scripts.h"

#include "debug.h"

#ifndef WAIT_ANY
# define WAIT_ANY	-1
#endif

#define CLIENT_ERROR_BUFFER	256
#define	CLIENT_ERROR_MSG_HEAD	"<XQF ERROR> "


struct running_client {
  pid_t	pid;

  int fd;
  int tag;

  char *buffer;
  int pos;

  struct server *server;
};


static GSList *clients = NULL;


static void dialog_failed (char *func, char *arg) {
  dialog_ok (_("XQF: ERROR!"), _("ERROR!\n\n%s(%s) failed: %s"), 
                                   func, (arg)? arg : "", g_strerror (errno));
}


static void client_free (struct running_client *cl) {

  debug(3, "client detached (pid:%d, fd:%d)\n", cl->pid, cl->fd);

  if (cl->fd >= 0) {
    gdk_input_remove (cl->tag);
    close (cl->fd);
  }

  if (cl->buffer)
    g_free (cl->buffer);

  if (cl->server)
    server_unref (cl->server);

  g_free (cl);
}


static void client_detach (struct running_client *cl) {
  script_action_gamequit(NULL, cl->server);
  client_free (cl);
  clients = g_slist_remove (clients, cl);
}


void client_detach_all (void) {
  g_slist_foreach (clients, (GFunc) client_free, NULL);
  g_slist_free (clients);
  clients = NULL;
}


static void detach_unused (void) {
  GSList *tmp = clients;
  struct running_client *cl;

  while (tmp) {
    cl = (struct running_client *) tmp->data;
    if (cl->pid == 0) {
      client_detach (cl);
      tmp = clients;
      continue;
    }
    tmp = tmp->next;
  }
}


static void client_sigchild_handler (int signum) {
  int pid;
  int status;
  GSList *list;
  struct running_client *cl;

  debug(3,"client_sigchild_handler(%d)",signum);

  while ((pid = waitpid (WAIT_ANY, &status, WNOHANG)) > 0) {
    debug(4,"client_sigchild_handler() -- pid %d, status %d",pid,WEXITSTATUS(status));
    if(WIFSIGNALED(status)&& WTERMSIG(status)==SIGSEGV)
    {
      debug(0,"*** SEGFAULT pid %d ***",pid);
    }
    for (list = clients; list; list = list->next) {
      cl = (struct running_client *) list->data;
      if (cl->pid == pid) {

	/*  Mark it as dead and get out.  All the resource freeing 
         *  should be done outside signal handler.
	 */

	cl->pid = 0;
	break;
      }
    }

  }
}


void client_init (void) {
  on_sig (SIGCHLD, client_sigchild_handler);
}


static void client_input_callback (struct running_client *cl, int fd, 
                                                GdkInputCondition condition) {
  int res;
  int pid;
  char *tmp;

  if (!cl->buffer)
    cl->buffer = g_malloc (CLIENT_ERROR_BUFFER);

  res = read (fd, cl->buffer + cl->pos, CLIENT_ERROR_BUFFER - 1 - cl->pos);

  if (res <= 0) {	/* read error or EOF */
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;

    client_detach (cl);
    return;
  }

  if (cl->pos + res == CLIENT_ERROR_BUFFER - 1) {
    tmp = &cl->buffer[CLIENT_ERROR_BUFFER - 1];
    *tmp = '\0';
  }
  else {
    tmp = memchr (cl->buffer + cl->pos, '\0', res);
    cl->pos += res;
  }

  if (tmp) {
    gdk_input_remove (cl->tag);
    close (cl->fd);
    cl->fd = -1;

    if (!strncmp (cl->buffer, 
                     CLIENT_ERROR_MSG_HEAD, strlen (CLIENT_ERROR_MSG_HEAD))) {
      dialog_ok (_("XQF: ERROR!"), _("ERROR!\n\n%s"), 
	                         cl->buffer + strlen (CLIENT_ERROR_MSG_HEAD));

      pid = cl->pid;  /* save PID value */
      client_detach (cl);
      if(pid) kill (pid, SIGTERM); // kill(0) would kill this process too!
    }
    else {
      client_detach (cl);
    }
  }
}


static void client_attach (pid_t pid, int fd, struct server *s) {
  struct running_client *cl;

  cl = g_malloc0 (sizeof (struct running_client));

  cl->pid = pid;

  cl->fd = fd;
  cl->tag = gdk_input_add (cl->fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, 
                                (GdkInputFunction) client_input_callback, cl);
  cl->server = s;
  server_ref (s);

  debug (3, "client attached (pid:%d)", cl->pid);

  clients = g_slist_append (clients, cl);
}


int client_launch_exec (int forkit, char *dir, char* argv[], 
                                                           struct server *s) {
  int pid;
  int pipefds[2];
  int flags;
  char msg[CLIENT_ERROR_BUFFER];

  if (get_debug_level() || dontlaunch)
  {
    char* cmdline = g_strjoinv(" # ",argv);
    debug(0,"%s",cmdline);
    g_free(cmdline);
  }

  if (dontlaunch)
    return -1;

  script_action_gamestart(NULL, s);

  if (forkit) {

    if (pipe (pipefds) < 0) {
      dialog_failed ("pipe", NULL);
      return -1;
    }

    pid = fork ();

    if (pid == -1) {
      dialog_failed ("fork", NULL);
      return -1;
    }

    if (pid) {	/* parent */
      close (pipefds[1]);

      flags = fcntl (pipefds[0], F_GETFL, 0);
      if (flags < 0 || fcntl (pipefds[0], F_SETFL, flags | O_NONBLOCK) < 0) {
	dialog_failed ("fcntl", NULL);
	return -1;
      }

      client_attach (pid, pipefds[0], s);
    }
    else {	/* child */
      close (pipefds[0]);

      if (dir && dir[0] != '\0') {
	if (chdir (dir) != 0) {
	  g_snprintf (msg, CLIENT_ERROR_BUFFER, "%schdir failed: %s", 
                          CLIENT_ERROR_MSG_HEAD, g_strerror (errno));
	  goto error_out;
	}
      }


      server_set_env(s);

      execvp (argv[0], argv);
  
      g_snprintf (msg, CLIENT_ERROR_BUFFER, "%sexec(%s) failed: %s", 
                          CLIENT_ERROR_MSG_HEAD, argv[0], g_strerror (errno));

error_out:
      write (pipefds[1], msg, strlen (msg) + 1);
      close (pipefds[1]);

      on_sig (SIGHUP,  _exit);
      on_sig (SIGINT,  _exit);
      on_sig (SIGQUIT, _exit);
      on_sig (SIGBUS,  _exit);
      on_sig (SIGSEGV, _exit);
      on_sig (SIGPIPE, _exit);
      on_sig (SIGTERM, _exit);
      on_sig (SIGALRM, _exit);
      on_sig (SIGCHLD, SIG_DFL);

      _exit (1);
    }
    return pid;
  }

  execvp (argv[0], argv);

  dialog_failed ("exec", argv[0]);
  return -1;
}


static int already_running (enum server_type type) {
  int another = FALSE;
  struct server *s = NULL;
  struct running_client *cl;
  GSList *tmp;
  int res;

  detach_unused ();

  if (!clients)
    return FALSE;

  for (tmp = clients; tmp; tmp = tmp->next) {
    cl = (struct running_client *) tmp->data;
    if (cl->server->type == type) {
      another = TRUE;
      break;
    }
  }

  s = (struct server *) ((struct running_client *) clients->data) -> server;

  res = dialog_yesno (NULL, 1, _("Launch"), _("Cancel"), 
		      _("There is %s client running.\n\nLaunch %s client?"),
		      (another)? games[type].name : games[s->type].name,
		      (another)? "another" : games[type].name);

  return TRUE - res;
}


int client_launch (const struct condef *con, int forkit) {

  if (!con || !con->server || !con->s)
    return FALSE;

  if (already_running (con->s->type))
    return FALSE;

  if (games[con->s->type].write_config) {
    if (!(*games[con->s->type].write_config) (con))
      return FALSE;
  }

  if (games[con->s->type].exec_client) {
    if ( (*games[con->s->type].exec_client) (con, forkit) < 0)
      return FALSE;
  }

  return TRUE;
}


struct condef *condef_new (struct server *s) {
  struct condef *con;

  con = g_malloc0 (sizeof (struct condef));

  con->s = s;
  server_ref (s);

  return con;
}


void condef_free (struct condef *con) {

  g_return_if_fail(con!=NULL);

  if (con->s)
    server_unref (con->s);

  if (con->server)
    g_free (con->server);

  if (con->gamedir)
    g_free (con->gamedir);

  if (con->password) 
    g_free (con->password);

  if (con->spectator_password)
    g_free (con->spectator_password);

  if (con->rcon_password)
    g_free (con->rcon_password);

  if (con->custom_cfg)
    g_free (con->custom_cfg);

  if (con->demo)
    g_free (con->demo);

  g_free (con);
}



