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
#include <stdio.h>	/* fprintf, fflush */
#include <stdlib.h>	/* malloc, free */
#include <signal.h>	/* kill, ... */
#include <unistd.h>	/* pipe, close, dup2, _exit, fcntl, select */
                        /* alarm, pause */
#include <string.h>	/* memset, strdup, strlen, strcmp */
#include <sys/wait.h>   /* waitpid */
#include <errno.h>	/* errno */
#include <sys/time.h>	/* select */
#include <sys/socket.h>	/* struct in_addr, inet_aton, inet_ntoa */
#include <netinet/in.h>	/* struct in_addr, inet_aton, inet_ntoa */
#include <arpa/inet.h>	/* struct in_addr, inet_aton, inet_ntoa */
#include <netdb.h>	/* struct hostent, gethostbyaddr, gethostbyname */
#include <sys/param.h>	/* MAXHOSTNAMELEN */
#include <time.h>	/* time */

#ifndef h_errno
extern int h_errno;	/* h_errno */
#endif

#include <gtk/gtk.h>

#include "xqf.h"
#include "dns.h"
#include "host.h"
#include "utils.h"
#include "debug.h"

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN		64
#endif

#define DNS_BUF_SIZE		(MAXHOSTNAMELEN * 2 + 32)
#define RESOLVE_DELIM		'|'
#define RESOLVE_TIMEOUT		10
#define DNS_MAX_CHILDREN	8

#ifndef WAIT_ANY
# define WAIT_ANY       -1
#endif


struct dns_stream {
  int fd;

  char buf[DNS_BUF_SIZE];
  int pos;

  void (*parse) (char *str, void *data);
  void (*close) (int error, void *data);
  void *data;
};

struct dns_helper_descr {
  int pid;
  int tag;			/* GDK tag */
  int output;			/* fd */
  struct dns_stream *input;
  void (*resolved) (char *str, struct host *h, enum dns_status status, 
                                                             void *user_data);
  void *user_data;
};

struct dns_child_descr {
  struct dns_stream *input;
  pid_t pid;
};

struct dns_queue_link {
  char *str;
  struct dns_queue_link *next;
};

static struct dns_helper_descr dns_helper = { -1, -1, -1, NULL, NULL, NULL };

static struct dns_stream *dns_master_input = NULL;

static struct dns_child_descr dns_workers[DNS_MAX_CHILDREN];
static int dns_workers_num = 0;

static struct dns_queue_link *q_head = NULL, *q_tail = NULL;
static struct dns_queue_link *q_free = NULL;

static char *worker_arg = NULL;


static void dns_master_init (void);
static void dns_master_reset (void);


int str_is_ip_address (char *str) {
  struct in_addr dummy;

  return (str)? inet_aton (worker_arg, &dummy) : FALSE;
}


static int failed (char *name, char *arg) {
  fprintf (stderr, "%s(%s) failed: %s\n", name, (arg)? arg : "", 
                                                          g_strerror (errno));
  return TRUE;
}


static void master_sigchld_handler (int signum) {
  int pid;
  int status;

  debug(3,"master_sigchld_handler(%d)",signum);

  while ((pid = waitpid (WAIT_ANY, &status, WNOHANG)) > 0);
}


static void master_sigterm_handler (int sig) {
  if (sig != SIGTERM && sig != SIGINT) {
    fprintf (stderr, "DNS Helper: %s(%d) signal\n", g_strsignal (sig), sig);
  }
  dns_master_reset ();
  _exit (1);
}


static char *dns_queue_head (void) {
  struct dns_queue_link *link;
  char *str = NULL;

  if (q_head) {
    link = q_head;
    q_head = link->next;
    if (!q_head)
      q_tail = NULL;
    str = link->str;

    link->next = q_free;
    q_free = link;
  }

  return str;
}


static void dns_queue_add (char *str) {
  struct dns_queue_link *link;

  if (q_free) {
    link = q_free;
    q_free = link->next;
  }
  else {
    link = malloc (sizeof (struct dns_queue_link));
  }

  link->str = str;
  link->next = NULL;

  if (q_tail)
    q_tail->next = link;
  else
    q_head = link;

  q_tail = link;
}


void dns_helper_shutdown (void) {
  if (dns_helper.pid > 0) {
    kill (dns_helper.pid, SIGTERM);
    dns_helper.pid = -1;
  }

  if (dns_helper.output >= 0) {
    close (dns_helper.output);
    dns_helper.output = -1;
  }

  if (dns_helper.input) {
    if (dns_helper.input->fd >= 0)
      close (dns_helper.input->fd);
    g_free (dns_helper.input);
    dns_helper.input = NULL;
  }

  if (dns_helper.tag >= 0) {
    gdk_input_remove (dns_helper.tag);
    dns_helper.tag = -1;
  }

  dns_helper.resolved = NULL;
  dns_helper.user_data = NULL;
}


static void helper_parse_callback (char *str, void *data) {
  char *dnsmsg;
  struct host *h = NULL;
  enum dns_status status = DNS_STATUS_ERROR;
  char *token[3];
  int n;

  /* id|address|hostname */

  n = tokenize_bychar (str, token, 3, RESOLVE_DELIM);

  if (n < 2)
    return;

  if (n > 2) {
    status = DNS_STATUS_OK;

    if (!strncmp (token[2], DNS_MSG_PREFIX, sizeof (DNS_MSG_PREFIX) - 1)) {
      dnsmsg = token[2] + sizeof (DNS_MSG_PREFIX) - 1;

      if (!strcmp (dnsmsg, DNS_MSG_TIMEOUT))
	status = DNS_STATUS_TIMEOUT;
      else if (!strcmp (dnsmsg, DNS_MSG_NOTFOUND))
	status = DNS_STATUS_NOTFOUND;
      else
	status = DNS_STATUS_ERROR;
    }
  }

  if ((h = host_add (token[1])) != NULL) {

    switch (status) {

    case DNS_STATUS_OK:
      if (n > 2) {
	if (h->name)
	  g_free (h->name);
	h->name = g_strdup (token[2]);
      }
      h->refreshed = time (NULL);	/* Success */
      break;

    case DNS_STATUS_NOTFOUND:
      if (h->name) {
	g_free (h->name);
	h->name = NULL;
      }
      h->refreshed = time (NULL);	/* Success */
      break;

    default:
      h->refreshed = 0;			/* Try later */
      break;

    }

  }

  if (h)
    host_ref (h);

  if (dns_helper.resolved)
    (*dns_helper.resolved) (token[0], h, status, dns_helper.user_data);

  if (h)
    host_unref (h);
}


static void helper_close_callback (int error, void *data) {
  dns_helper_shutdown ();
}


static void worker_parse_callback (char *str, void *data) {
  int len;

#ifdef DEBUG
  int n = (int) data;
  fprintf (stderr, "DNS Master> got \"%s\" from worker %d\n", str, n);
#endif

  len = strlen (str);
  str[len] = '\n';
  write (1, str, len + 1);	/* stdout */
}


static void worker_close_callback (int error, void *data) {
  int n = (int) data;

  dns_workers[n].pid = -1;
  dns_workers_num--;

  close (dns_workers[n].input->fd);
  dns_workers[n].input->fd = -1;
}


static void print_resolved (char *str, char *addr, char *name) {
  printf ("%s%c%s%c%s\n", (str)? str : "", RESOLVE_DELIM,
                          (addr)? addr : "", RESOLVE_DELIM,
                     	  (name)? name : "");
  fflush (stdout);
}


static void worker_sigalrm_handler (int signum) {

  if (worker_arg) {
    print_resolved (worker_arg,
		    (str_is_ip_address (worker_arg))? worker_arg : NULL,
		    DNS_MSG_PREFIX DNS_MSG_TIMEOUT);
#ifdef DEBUG
    fprintf (stderr, "<DNS> timeout: %s\n", worker_arg);
#endif
    worker_arg = NULL;
  }

  _exit (0);
}


static void worker_sigterm_handler (int signum) {
#ifdef DEBUG
  if (worker_arg)
    fprintf (stderr, "<DNS> terminated: %s\n", worker_arg);
#endif
  _exit (0);
}


static char *herrno2msg (int err) {

  switch (err) {

  case HOST_NOT_FOUND:
  case NO_ADDRESS:
    return DNS_MSG_PREFIX DNS_MSG_NOTFOUND;

  case NO_RECOVERY:
    return DNS_MSG_PREFIX DNS_MSG_ERROR;

  case TRY_AGAIN:
    return DNS_MSG_PREFIX DNS_MSG_TIMEOUT;

  default:
    return NULL;
  }
}


static void worker_resolve (char *str) {
  struct hostent *h = NULL;
  struct in_addr ip;

  if (!str || !*str || strchr (str, '\n') || strchr (str, RESOLVE_DELIM)) {
    print_resolved ("BAD REQUEST", NULL, NULL);
    return;
  }

  worker_arg = str;

  if (!inet_aton (str, &ip)) {
    h = gethostbyname (str);
    if (!h) {
      worker_arg = NULL;
      alarm (0);
      print_resolved (str, NULL, herrno2msg (h_errno));
      return;
    }
    ip = *((struct in_addr *) h->h_addr_list[0]);
  }
  else {
    alarm (RESOLVE_TIMEOUT);
  }

  h = gethostbyaddr ((char *) &ip.s_addr, sizeof (ip.s_addr), AF_INET);
  worker_arg = NULL;

  alarm (0);

  print_resolved (str, inet_ntoa (ip), 
                            (h)? h->h_name : DNS_MSG_PREFIX DNS_MSG_NOTFOUND);
}


static int fork_worker (int n, char *str) {
  int fdset[2] = { -1, -1 };
  int pid;

  if ((pipe (fdset) < 0 && failed ("pipe", NULL)) ||
      ((pid = fork ()) < (pid_t) 0 && failed ("fork", NULL))) {

    if (fdset[0] > 0) close (fdset[0]);
    if (fdset[1] > 0) close (fdset[1]);

    return -1;
  }

  if (pid) {	/* parent */
    close (fdset[1]);

    dns_workers[n].pid = pid;

    if (!dns_workers[n].input)
      dns_workers[n].input = malloc (sizeof (struct dns_stream));

    dns_workers[n].input->fd = fdset[0];
    if(set_nonblock (dns_workers[n].input->fd) == -1)
	failed ("fcntl", NULL);
    dns_workers[n].input->pos = 0;

    dns_workers[n].input->parse = worker_parse_callback;
    dns_workers[n].input->close = worker_close_callback;
    dns_workers[n].input->data  = (void *) n;

    dns_workers_num++;

#ifdef DEBUG
    fprintf (stderr, "DNS Master> worker %d (str:%s) is forked\n", n, str);
#endif
  }
  else {	/* child */
    on_sig (SIGHUP,  _exit);
    on_sig (SIGINT,  _exit);
    on_sig (SIGQUIT, _exit);
    on_sig (SIGBUS,  _exit);
    on_sig (SIGSEGV, _exit);
    on_sig (SIGPIPE, _exit);
    on_sig (SIGCHLD, _exit);
    on_sig (SIGTERM, worker_sigterm_handler);
    on_sig (SIGALRM, worker_sigalrm_handler);

    dup2 (fdset[1], 1);		/* stdout */

    close (fdset[0]);
    close (fdset[1]);

#ifdef DEBUG
    fprintf (stderr, "DNS Worker [%s]> started\n", str);
#endif

    alarm (60 * 10);	/* suicide after 10 min */

    worker_resolve (str);

    _exit (0);
  }

  return 0;
}


static int dns_dispatch_to_worker (char *str) {
  int i;

  if (dns_workers_num < DNS_MAX_CHILDREN) {
    for (i = 0; i < DNS_MAX_CHILDREN; i++) {
      if (dns_workers[i].pid < 0) {
#ifdef DEBUG
	fprintf (stderr, "DNS Master> \"%s\" is dispatched to worker %d\n", 
                                                                      str, i);
#endif
	fork_worker (i, str);
	return TRUE;
      }
    }
  }
  return FALSE;
}


static int dns_move_queue (void) {
  int moved = FALSE;

  while (q_head && dns_dispatch_to_worker (q_head->str)) {
    free (dns_queue_head ());
    moved = TRUE;
  }
  return moved;
}


static void dns_resolve (char *str) {

  dns_move_queue ();

  if (dns_workers_num >= DNS_MAX_CHILDREN || !dns_dispatch_to_worker (str)) {
#ifdef DEBUG
    fprintf (stderr, "DNS Master> \"%s\" is put to the waiting queue\n",
                     str);
#endif
    dns_queue_add (strdup (str));
  }
}


static void dns_master_reset (void) {
  char *str;
  int i;

  for (i = 0; i < DNS_MAX_CHILDREN; i++) {
    if (dns_workers[i].pid > 0) {
      kill (dns_workers[i].pid, SIGTERM);
      dns_workers[i].pid = -1;
    }
    if (dns_workers[i].input) {
      if (dns_workers[i].input->fd >= 0)
	close (dns_workers[i].input->fd);
      free (dns_workers[i].input);
      dns_workers[i].input = NULL;
    }
  }
  dns_workers_num = 0;

  while ((str = dns_queue_head ()) != NULL) {
    free (str);
  }
}


static void master_parse_callback (char *str, void *data) {
#ifdef DEBUG
  fprintf (stderr, "DNS Master> got \"%s\"\n", str);
#endif
  if (strcmp (str, DNS_CMD_RESET) == 0) {
#ifdef DEBUG
    fprintf (stderr, "DNS Master> RESET\n");
#endif
    dns_master_reset ();
  }
  else {
    dns_resolve (str);
  }
}


static void master_close_callback (int error, void *data) {
#ifdef DEBUG
  fprintf (stderr, "DNS Master> pipe closed, errors: %s\n", 
                                                       (error)? "yes" : "no");
#endif
  dns_master_reset ();
  _exit (0);
}


static void dns_master_init (void) {
  int i;

  for (i = 0; i < DNS_MAX_CHILDREN; i++) {
    dns_workers[i].pid = -1;
    dns_workers[i].input = NULL;
  }

  dns_workers_num = 0;

  dns_master_input = malloc (sizeof (struct dns_stream));

  dns_master_input->fd = 0;			/* stdin */
  if(set_nonblock (dns_master_input->fd) == -1)
    failed ("fcntl", NULL);

  dns_master_input->pos = 0;
  dns_master_input->parse = master_parse_callback;
  dns_master_input->close = master_close_callback;
  dns_master_input->data = NULL;

  q_head = q_tail = q_free = NULL;

  on_sig (SIGHUP,  master_sigterm_handler);
  on_sig (SIGINT,  master_sigterm_handler);
  on_sig (SIGQUIT, master_sigterm_handler);
  on_sig (SIGBUS,  master_sigterm_handler);
  on_sig (SIGSEGV, master_sigterm_handler);
  on_sig (SIGALRM, master_sigterm_handler);
  on_sig (SIGTERM, master_sigterm_handler);
}


static void dns_input_callback (struct dns_stream *stream, int fd,
                                                GdkInputCondition condition) {
  char *tmp;
  int first_used = 0;
  int res;

  res = read (fd, stream->buf + stream->pos, DNS_BUF_SIZE - stream->pos);

  if (res < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      return;
    failed ("read", NULL);
    (*stream->close) (TRUE, stream->data);
    return;
  }

  if (res == 0) {		/* EOF */
    (*stream->close) (FALSE, stream->data);
    return;
  }

  tmp = stream->buf + stream->pos;
  stream->pos += res;

  while (res && (tmp = memchr (tmp, '\n', res)) != NULL) {
    *tmp++ = '\0';

    (*stream->parse) (stream->buf + first_used, stream->data);

    first_used = tmp - stream->buf;
    res = stream->buf + stream->pos - tmp;
  }

  if (first_used > 0) {
    if (first_used != stream->pos) {
      g_memmove (stream->buf, stream->buf + first_used, 
                                                    stream->pos - first_used);
    }
    stream->pos -= first_used;
  }
}


static void dns_master_mainloop (void) {
  fd_set readfds;
  int n;
  int i;

  on_sig (SIGCHLD, master_sigchld_handler);
  dns_master_init ();

  while (1) {
    if (dns_workers_num < DNS_MAX_CHILDREN && q_head) {
	dns_move_queue ();
    }

#ifdef DEBUG
    fprintf (stderr, "DNS Master> %d workers, queue is %s\n", 
                            dns_workers_num,
			    (q_head)? "not empty" : "empty");
#endif

    FD_ZERO (&readfds);
    FD_SET (0, &readfds);

    for (i = 0; i < DNS_MAX_CHILDREN; i++) {
      if (dns_workers[i].pid > 0)
	FD_SET (dns_workers[i].input->fd, &readfds);
    }

    n = select (FD_SETSIZE, &readfds, NULL, NULL, NULL);
    if (n == -1 && errno == EINTR)
      continue;

    if (n < 0) {
      failed ("select", NULL);
      dns_master_reset ();
      return;
    }

    for (i = 0; i < DNS_MAX_CHILDREN; i++) {
      if (dns_workers[i].pid > 0 && 
                              FD_ISSET (dns_workers[i].input->fd, &readfds)) {
	dns_input_callback (dns_workers[i].input, 
                                    dns_workers[i].input->fd, GDK_INPUT_READ);
      }
    }

    if (FD_ISSET (0, &readfds))
      dns_input_callback (dns_master_input, 0, GDK_INPUT_READ);
  }

  /* NOT REACHED */

  dns_master_reset ();
}


int dns_spawn_helper (void) {
  int fdset1[2] = { -1, -1 };
  int fdset2[2] = { -1, -1 };
  int pid;

  dns_helper_shutdown ();

  if ((pipe (fdset1) < 0 && failed ("pipe", NULL)) ||
      (pipe (fdset2) < 0 && failed ("pipe", NULL)) ||
      ((pid = fork ()) < (pid_t) 0 && failed ("fork", NULL))) {

    if (fdset1[0] > 0) close (fdset1[0]);
    if (fdset1[1] > 0) close (fdset1[1]);
    if (fdset2[0] > 0) close (fdset2[0]);
    if (fdset2[1] > 0) close (fdset2[1]);

    return -1;
  }

  if (pid) {	/* parent */
    close (fdset1[1]);
    close (fdset2[0]);

    dns_helper.output = fdset2[1];

    dns_helper.input = malloc (sizeof (struct dns_stream));
    dns_helper.input->fd  = fdset1[0];
    if(set_nonblock (dns_helper.input->fd) == -1)
	failed("fcntl", NULL);

    dns_helper.pid = pid;

    dns_helper.input->pos = 0;
    dns_helper.input->parse = helper_parse_callback;
    dns_helper.input->close = helper_close_callback;
    dns_helper.input->data = NULL;
  }
  else {	/* child */
    on_sig (SIGHUP,  _exit);
    on_sig (SIGINT,  _exit);
    on_sig (SIGQUIT, _exit);
    on_sig (SIGBUS,  _exit);
    on_sig (SIGSEGV, _exit);
    on_sig (SIGALRM, _exit);
    on_sig (SIGTERM, _exit);

    on_sig (SIGPIPE, SIG_IGN);
    on_sig (SIGCHLD, SIG_IGN);

    dup2 (fdset1[1], 1);	/* stdout */
    dup2 (fdset2[0], 0);	/* stdin */

    close (fdset1[0]);
    close (fdset1[1]);
    close (fdset2[0]);
    close (fdset2[1]);

    dns_master_mainloop ();
    _exit (0);
  }

  return 0;
}


void dns_gtk_init (void) {
  if (dns_helper.input && dns_helper.input->fd > 0) {
    dns_helper.tag = gdk_input_add (dns_helper.input->fd, 
				    GDK_INPUT_READ | GDK_INPUT_EXCEPTION, 
				    (GdkInputFunction) dns_input_callback,
				    dns_helper.input);
  }
}


void dns_cancel_requests (void) {
  const char cmd[] = DNS_CMD_RESET "\n";

  dns_set_callback (NULL, NULL);

  if (dns_helper.output >= 0) {
    write (dns_helper.output, cmd, sizeof (cmd) - 1);
  }
}


void dns_lookup (const char *str) {
  char host[MAXHOSTNAMELEN + 1];
  int len;

  debug(8,"dns_lookup(%s)",str);

  len = strlen (str);
  if (len <= MAXHOSTNAMELEN) {
    strcpy (host, str);
    host[len] = '\n';
    host[len + 1] = '\0';
    write (dns_helper.output, host, len + 1);
  }
}


void dns_set_callback (
        void (*cb) (char *item, struct host *h, enum dns_status, void *data),
                                                                 void *data) {
  dns_helper.resolved = cb;
  dns_helper.user_data = data;
}


char *dns_lookup_by_addr (char *ip) {
  struct hostent *h;
  struct in_addr addr;

  if (inet_aton (ip, &addr)) {
    h = gethostbyaddr ((char *) &addr.s_addr, sizeof (addr.s_addr), AF_INET);
    if (h)
      return g_strdup (h->h_name);
  }
  return NULL;
}


char *dns_lookup_by_name (char *name) {
  struct hostent *h;
  char *ip;

  h = gethostbyname (name);
  if (h) {
    ip = inet_ntoa ( *((struct in_addr *) h->h_addr_list[0]) );
    if (ip)
      return g_strdup (ip);
  }
  return NULL;
}


#ifdef DNS_STANDALONE

int main (int argc, char *argv[]) {
  dns_master_mainloop ();
  return 0;
}

#endif

