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

#include <sys/types.h>	/* getpwnam, readdir, dirent */
#include <stdio.h>	/* FILE, putc */
#include <string.h>	/* strlen, strncpy, strcmp, strspn, strcspn, strchr */
#include <ctype.h>	/* isspace */
#include <pwd.h>	/* getpwnam */
#include <stdlib.h>	/* strtol */
#include <dirent.h>	/* readdir, dirent */
#include <signal.h>	/* sigaction, sigemptyset */
#include <ctype.h>	/* tolower */
#include <sys/stat.h>	/* stat */
#include <unistd.h>	/* access */
#include <fcntl.h>	/* fcntl */
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>


#include <gtk/gtk.h>

#include "utils.h"
#include "debug.h"
#include "i18n.h"

static char* _find_file_in_path(const char* files, gboolean relative);
static char* _find_file_in_path_list(char** binaries, gboolean relative);

short strtosh (const char *str) {
  long tmp;

  tmp = strtol (str, NULL, 10);
  return CLAMP (tmp, G_MINSHORT, G_MAXSHORT);
}


unsigned short strtoush (const char *str) {
  long tmp;

  tmp = strtol (str, NULL, 10);
  return CLAMP (tmp + G_MINSHORT, G_MINSHORT, G_MAXSHORT) - G_MINSHORT;
}


char *strdup_strip (const char *str) {
  const char *start;
  const char *end;
  char *res;

  if (!str)
    return NULL;

  for (start = str; *start && isspace (*start); start++);

  if (!strlen(str))
    return NULL;

  for (end = str + strlen (str) - 1; end >= start && isspace (*end); end--);

  if (start > end)
    return NULL;

  res = g_malloc (end - start + 1 + 1);
  strncpy (res, start, end - start + 1);
  res[end - start + 1] = '\0';

  return res;
}


// concatenate dir and file, insert slash if necessary
// returned string must be freed manually
char *file_in_dir (const char *dir, const char *file) {
  char *res, *tmp;
  int need_slash = 0;

  if (!dir || dir[0] == '\0') /* dir "" is current dir */
    return (file)? g_strdup (file) : NULL;

  if (!file)
    return g_strdup (dir);

  if (dir[strlen (dir) - 1] != G_DIR_SEPARATOR)
    need_slash = 1;

  tmp = res = g_malloc0 (strlen (dir) + strlen (file) + need_slash + 1);

  strcpy (tmp, dir);
  tmp += strlen (dir);
  if (need_slash)
    *tmp++ = G_DIR_SEPARATOR;
  strcpy (tmp, file);

  return res;
}


int str_isempty (const char *str) {
  if (!str)
    return TRUE;

  for (; *str; str++) 
    if (!isspace (*str))
      return FALSE;

  return TRUE;
}


#define MAXNAMELEN	256

char *expand_tilde (const char *path) {
  char *res = NULL;
  char *slash;
  char name[MAXNAMELEN];
  int namelen;
  struct passwd *pwd;

  if (path) {
    if (path[0] == '~') {
      if (path[1] == '\0') {
	res = g_strdup (g_get_home_dir ());
      }
      else if (path[1] == G_DIR_SEPARATOR) {
	res = file_in_dir (g_get_home_dir (), path + 2);
      }
      else {
	slash = strchr (&path[1], G_DIR_SEPARATOR);
	if (slash)
	  namelen = slash - &path[1];
	else
	  namelen = strlen (&path[1]);

	if (namelen > MAXNAMELEN)
	  namelen = MAXNAMELEN - 1;

	strncpy (name, &path[1], namelen);
	name[namelen] = '\0';

	pwd = getpwnam (name);
	if (pwd)
	  res = file_in_dir (pwd->pw_dir, path + 2 + namelen);
	else
	  res = g_strdup (path);
      }
    }
    else {
      res = g_strdup (path);
    }
  }

#ifdef DEBUG
  fprintf (stderr, "Tilde expansion: %s -> %s\n", path, res);
#endif

  return res;
}


/*
 *  Directory Scaning
 */


GList *dir_to_list (const char *dirname, 
                              char * (*filter) (const char *, const char *)) {
  DIR *directory;
  struct dirent *dirent_ptr;
  GList *list = NULL;
  char *str;

  if(!dirname)
    return NULL;

  directory = opendir (dirname);
  if (directory == NULL)
    return NULL;

  while ((dirent_ptr = readdir (directory)) != NULL) {
    if (filter)
      str = filter (dirname, dirent_ptr->d_name);
    else
      str = g_strdup (dirent_ptr->d_name);

    if (str)
      list = g_list_prepend (list, str);
  }

  closedir (directory);

  return g_list_sort (list, (GCompareFunc) strcmp);
}


/*
 *  Lists
 */


GList *merge_sorted_string_lists (GList *list1, GList *list2) {
  GList *res = NULL;
  GList *ptr1;
  GList *ptr2;
  int cmpres;

  ptr1 = list1;
  ptr2 = list2;

  while (ptr1 && ptr2) {
    cmpres = strcmp ((char *) ptr1->data, (char *) ptr2->data);

    if (cmpres == 0) {
      res = g_list_prepend (res, ptr1->data);
      g_free (ptr2->data);
      ptr1 = ptr1->next;
      ptr2 = ptr2->next;
    } else {
      if (cmpres < 0) {
	res = g_list_prepend (res, ptr1->data);
	ptr1 = ptr1->next;
      }
      else {
	res = g_list_prepend (res, ptr2->data);
	ptr2 = ptr2->next;
      }
    }
  }

  while (ptr1) {
    res = g_list_prepend (res, ptr1->data);
    ptr1 = ptr1->next;
  }

  while (ptr2) {
    res = g_list_prepend (res, ptr2->data);
    ptr2 = ptr2->next;
  }

  res = g_list_reverse (res);
  g_list_free (list1);
  g_list_free (list2);
  return res;
}


GSList *unique_strings (GSList *strings) {
  GSList *result = NULL;
  GSList *tmp;

  while (strings) {
    for (tmp = result; tmp; tmp = tmp->next) {
      if (strcmp ((char *) tmp->data, (char *) strings->data))
	result = g_slist_prepend (result, strings->data);
    }
    strings = strings->next;
  }

  return result;
}

// build GList from array of char*
GList* createGListfromchar(char* strings[])
{
  GList *list = NULL;
  char** ptr = NULL;
  for(ptr=strings;ptr&&*ptr;ptr++)
  {
    list = g_list_append (list, *ptr);
  }

  return list;
}


/*
 *  Signals 
 */


void on_sig (int signum, void (*func) (int signum)) {
  struct sigaction action;

  action.sa_handler = func;
  sigemptyset (&action.sa_mask);
  action.sa_flags = (signum == SIGCHLD)? 
                                       SA_RESTART | SA_NOCLDSTOP : SA_RESTART;
  sigaction (signum, &action, NULL);
}


void ignore_sigpipe (void) {
  on_sig (SIGPIPE, SIG_IGN);
}


/*
 *  String Output
 */

void print_dq_string (FILE *f, const unsigned char *ptr) {

  if (!f)
    return;

  putc ('\"', f);

  while (ptr && *ptr) {
    switch (*ptr) {

    case '\n':
      putc ('\\', f); putc ('n', f);
      break;

    case '\t':
      putc ('\\', f); putc ('t', f);
      break;

    case '\r':
      putc ('\\', f); putc ('r', f);
      break;

    case '\b':
      putc ('\\', f); putc ('b', f);
      break;

    case '\f':
      putc ('\\', f); putc ('f', f);
      break;

    case '\\':
    case '\'':
    case '\"':
      putc ('\\', f); putc (*ptr, f);
      break;

    default:
      if (*ptr >= 0x20 && *ptr != 0x7F)
	putc (*ptr, f);
      else
	fprintf (f, "\\%03o", *ptr);
      break;

    }

    ptr++;
  }

  putc ('\"', f);
}


char *lowcasestrstr (const char *str, const char *substr) {
  int slen;
  int sublen;
  const char *end;
  int i;

  if (!str || !substr)
    return NULL;

  slen = strlen (str);
  sublen = strlen (substr);

  if (slen < sublen)
    return NULL;

  end = &str[slen - sublen + 1];

  while (str < end) {
    for (i = 0; i < sublen; i++) {
      if (tolower (substr[i]) != tolower (str[i]))
	goto loop;
    }
    return (char *) str;

  loop:
    str++;
  }

  return NULL;
}


/*
 *  Parse string to tokens
 */

int tokenize (char *str, char *token[], int max, const char *dlm) {
  int num = 0;

  while (str && num < max) {
    str += strspn (str, dlm);

    if (*str == '\0')
      break;

    token[num++] = str;

    if (num == max)
      break;

    str += strcspn (str, dlm);

    if (*str == '\0')
      break;

    *str++ = '\0';
  }

  return num;
}


int safe_tokenize (const char *str, char *token[], int max, const char *dlm) {
  int num = 0;

  while (str && num < max) {
    str += strspn (str, dlm);

    if (*str == '\0')
      break;

    token[num++] = (char *) str;
    str += strcspn (str, dlm);
  }

  return num;
}


int tokenize_bychar (char *str, char *token[], int max, char dlm) {
  int num = 0;

  if (!str || str[0] == '\0')
    return 0;

  while (str && num < max) {
    token[num++] = str;
    str = strchr (str, dlm);
    if (str)
      *str++ = '\0';
  }

  return num;
}


int hostname_is_valid (const char *hostname) {
  const char *ptr;

  if (!hostname)
    return FALSE;

  for (ptr = hostname; *ptr; ptr++) {
    if (*ptr != '.' && *ptr != '-' &&
	(*ptr < '0' || *ptr > '9') &&
	(*ptr < 'a' || *ptr > 'z') &&
	(*ptr < 'A' || *ptr > 'Z')) {
#ifdef DEBUG
      fprintf (stderr, "Host name is not valid: %s\n", hostname);
#endif
      return FALSE;
    }
  }
  return TRUE;
}


char* find_server_setting_for_key (char *key, char **info_ptr){
  char **ptr;
  
  if (key == NULL || info_ptr == NULL) return (NULL);
  
  for (ptr = info_ptr; ptr && *ptr; ptr += 2) {
    if( strcasecmp (*ptr, key) == 0){
      debug( 3, "find_server_setting_for_key() -- Found key '%s' with value '%s'", key, *(ptr+1)  );
      return (*(ptr+1));
    } else {
      debug( 8, "Key '%s' w/val '%s' did not match '%s'", *ptr, *(ptr+1), key);
    }
  }
  return (NULL);
}

// returns true if str is "true", false otherwise
int str2bool(const char* str)
{
  if(!str) return FALSE;

  if(!strcasecmp(str,"true"))
  {
    return TRUE;
  }
  return FALSE;
}

// return "false" if i == 0, "true" otherwise
const char* bool2str(int i)
{
  if(!i)
  {
    return "false";
  }
  return "true";
}

/** find executable file in $PATH. file is a colon seperated list of
 * executables to search for. return name of first found file, NULL otherwise
 * must be freed manually
 */
char* find_file_in_path(const char* files)
{
  return _find_file_in_path(files, FALSE);
}

char* find_file_in_path_relative(const char* files)
{
  return _find_file_in_path(files, TRUE);
}

char* find_file_in_path_list(char** files)
{
  return _find_file_in_path_list(files, FALSE);
}

char* find_file_in_path_list_relative(char** files)
{
  return _find_file_in_path_list(files, TRUE);
}

/**
  @param binaries NULL terminated list of strings
 */
static char* _find_file_in_path_list(char** binaries, gboolean relative)
{
    char* path = NULL;
    int i = 0, j = 0;
    char** directories = NULL;
    char* found = NULL;

    path = getenv("PATH");

    if(!binaries) return NULL;
    if(!path) return NULL;

    directories = g_strsplit(path,":",0);

    for(i=0; binaries[i] && !found; i++)
    {
//	debug(0,"search for %s",binaries[i]);
	for(j=0; directories[j] && !found; j++)
	{
//	    debug(0,"search in %s",directories[j]);
	    char *file = file_in_dir (directories[j], binaries[i]);
	    if(!file) continue;
	   
	    if(!access(file,X_OK))
	    {
		// If directory name is blank, don't add a / - happens if
		// a complete path was passed
		if (!relative && strlen(directories[j]))
  		  found = g_strconcat(directories[j],"/",binaries[i],NULL);
		else
		  found = g_strdup(binaries[i]);
		debug(3,"found %s in %s",binaries[i],directories[j]);
	    }
	    g_free(file);
	}
    }

    g_strfreev(directories);

    return found;
}

static char* _find_file_in_path(const char* files, gboolean relative)
{
    char** binaries = NULL;
    char* found = NULL;

    binaries = g_strsplit(files,":",0);

    found = _find_file_in_path_list(binaries, relative);

    g_strfreev(binaries);

    return found;
}

/** find a directory inside another, prefer matching case otherwise search case insensitive
 * @param basegamedir where to search
 * @param game directory to search for
 * @param match_result output: 0 = not found, 1 = exact match, 2 = differnet case match
 * @returns name of found directory or NULL, must be freed manually
 */
char *find_game_dir (const char *basegamedir, const char *game, int *match_result)
{
  DIR *dp = NULL;
  struct stat buf = {0};
  char *path = NULL;
  char* ret = NULL;
  int result = 0;

  if(!game || !basegamedir)
    return NULL;

  debug( 3, "Looking for subdir %s in %s", game, basegamedir);
  
  // Look for exact match
  path = file_in_dir (basegamedir, game);
  if (!stat (path, &buf) && S_ISDIR(buf.st_mode))
  {
    debug( 3, "Found exact match for subdir %s in %s", game, basegamedir);
    result = 1; // Exact match
    ret = g_strdup(game);
  }
  g_free (path);

  // Did not find exact match, perform search
  if(!ret)
  {
    debug( 3, "Did not find exact match for subdir %s in %s", game, basegamedir);
    debug( 3, "Searching for subdir %s in %s ignoring case", game, basegamedir);

    dp = opendir (basegamedir);
    if(!dp)
    {
      debug( 3, "Could not open base directory %s!", basegamedir);
    }
    else
    {
      struct dirent *ep;
      while ((ep = readdir (dp)))
      {
	char* name = ep->d_name;

	if (!strcmp(name, ".") || !strcmp(name,".."))
	  continue;

	path = file_in_dir(basegamedir, name);
	
	if(stat(path, &buf) == -1)
	{
	  g_free(path);
	  continue;
	}
	
	g_free(path);
	
	if (!S_ISDIR(buf.st_mode))
	  continue;

	if (!g_strcasecmp(name, game))
	{
	    debug( 3, "Found subdir %s in %s that matches %s", ep->d_name, basegamedir, game);
	    result = 2; // Different case match
	    ret = g_strdup (name);
	    break;
	}
      }
      closedir (dp);
    }
  }

  if(!ret)
    debug( 3, "Could not find any match for subdir %s in %s.",game, basegamedir);

  if(match_result)
    *match_result = result;

  return ret;
}

/** sort list and remove duplicates
 * @param list list to sort
 * @compare_func function to use for comparing
 * @unref_func function to call for each deleted entry
 * @return sorted list without duplicates
 */
GSList* slist_sort_remove_dups(GSList* list, GCompareFunc compare_func, void (*unref_func)(void*))
{
  int i;
  GSList* serverlist = NULL;
  GSList* serverlistnext = NULL;

  if(!list)
    return NULL;

  list = serverlist = g_slist_sort (list, compare_func);

  i = 0;
  while(serverlist)
  {
    serverlistnext=serverlist->next;
    if(!serverlistnext)
    {
      // last element, quit loop
      serverlist = serverlistnext;
    }
    else if(!compare_func(serverlist->data,serverlistnext->data))
    {
      GSList* dup = serverlistnext;
      serverlistnext = g_slist_remove_link(serverlistnext, dup);
      serverlist->next = serverlistnext;
      if(unref_func) unref_func(dup->data);
      g_slist_free_1(dup);
    }
    else
    {
      serverlist= serverlistnext;
    }
    i++;
  }
  debug(2,"number of servers %d",i);

  return list;
}

/** \brief determine directory for game binary
 *
 * Extracts the path from path using the following rules:
 * - If path is a symlink:
 *   - If pointed to file contains '..', just stop, otherwise strip filename and
 *     store as directory
 *   - If there is no /'s in the pointed to file, use the original path instead and
 *     strip filename and store as directory
 * 
 * - If path is not a symlink:
 *   - strip filename and store as directory
 *
 * Path can be either a file or a directory
 *
 * Examples:
 *
 * path:	/usr/bin/quake2 symlink to /games/quake2/quake2
 * result:	/games/quake2/
 *
 * path:	/usr/bin/quake symlink to ../../games/quake2/quake2
 * result:	(stops - leaves as-is)
 *
 * path:	/games/quake2/quake2
 * result:	/games/quake2/
 *
 * path:	quake2
 * result:	search $PATH for binary then use above rules
 *
 * path:	~/bin/quake2
 * result:	expand tilde then use above rules
 *
 * @returns game direcory or NULL. must be freed
 */

char* resolve_path(const char* path)
{
  
  struct stat statbuf;
  int length = 0;
  char buf[256];
  char *ptr = NULL;
  char *dir = NULL;
  char* tmp = NULL;
  
  if(!path || !*path)
    return NULL;

  if(path[0] == '~')
  {
    tmp = expand_tilde(path);
    path = tmp;
  }
  else if(path[0] != '/')
  {
    tmp = find_file_in_path(path);
    if(!tmp)
    {
      debug(3, "%s not found in $PATH", path);
      return NULL;
    }
    path = tmp;
  }

  if(lstat(path, &statbuf) == -1)
  {
    debug(3, "lstat on %s failed", path);
    g_free(tmp);
    return NULL;
  }
  
  if ( S_ISLNK(statbuf.st_mode) == 1)
  {
    // Grab directory from sym link of cmd_entry
    
    debug(3, "path is a sym link");    

    length = readlink (path, buf, sizeof(buf) - 1);

    if (length > 0)
    {
      buf[length]='\0';

      if(buf[length-1] == '/')
	buf[length-1] = '\0';
      
      ptr = strrchr(buf, '/');
      
      if (ptr) {	    			// contains a / so pull from symlink
	if (!strstr(buf,"..")) {		// don't bother if it's got any ..'s in it
	  dir = g_strndup(buf, ptr-buf+1);
	}
      }
      else {        				// no / so pull from path instead
	ptr = strrchr(path, '/');
	if (ptr) {      			// contains a /
	  dir = g_strndup(path, ptr-path+1);
	}
      }        
    }
  }
  else
  {
    // Grab directory from cmd_entry
    
    debug(3,"path is NOT a sym link");
  
    ptr = strrchr(path, '/');

    if (ptr) // contains a /
    {
      gboolean dir_is_path = FALSE;
      char* PATH = getenv("PATH");

      // ignore direcory if it is in $PATH. It doesn't make sense to suggest
      // /usr/bin as game direcory
      if(PATH)
      {
	int i;
	char** directories = g_strsplit(PATH,":",0);
	char* basedir = g_strndup(path, ptr-path);

	for(i=0; directories[i]; ++i)
	{
	  if(!strcmp(directories[i], basedir))
	  {
	    dir_is_path = TRUE;
	    break;
	  }
	}

	g_strfreev(directories);
	g_free(basedir);
      }

      if(!dir_is_path)
      {
	dir = tmp;
	tmp = NULL;
      }
      else
	debug(3, "found directory %s in $PATH, ignoring", tmp);
    }
  }  

  g_free(tmp);

  return dir;
}

/** workaround to prevent gcc from complaining about %c as suggested by "man strftime" */
static inline size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm) {
  return strftime(s, max, fmt, tm);
}

// return locale's string representation of t. must be freed manually
char* timet2string(const time_t* t)
{
    enum { timebuf_len = 128 };
    char timebuf[timebuf_len] = {0};
    struct tm tm_s;
    char* str;

    gmtime_r(t,&tm_s);
    if(!my_strftime(timebuf,timebuf_len,"%c",&tm_s))
    {
	// error converting time to string representation, shouldn't happen
	str=_("<error>");
    }
    else
	str = timebuf;

    return strdup(str);
}

int set_nonblock (int fd)
{
    int flags;

    flags = fcntl (fd, F_GETFL, 0);
    if (flags < 0 || fcntl (fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
	return -1;
    }
    return 0;
}

// TODO: code is generic enough to move into separate file

int start_prog_and_return_fd(char *const argv[], pid_t *pid)
{
    int pipefds[2];

    *pid = -1;

    if (pipe (pipefds) < 0)
    {
	xqf_error("error creating pipe: %s",strerror(errno));
	return -1;
    }
    *pid = fork();
    if (*pid < (pid_t) 0)
    {
	xqf_error("fork failed: %s",strerror(errno));
	return -1;
    }

    if (*pid == 0)	// child
    {
	close(1);
	//    close(2);
	close (pipefds[0]);
	dup2 (pipefds[1], 1);
	//    dup2 (pipefds[1], 2);
	close (pipefds[1]);

	debug(3,"child about to exec %s", argv[0]);

	execvp (argv[0], argv);

	xqf_error("failed to exec %s: %s",argv[0],strerror(errno));

	_exit (1);
    }

    close (pipefds[1]);

    if (set_nonblock(pipefds[0]) == -1)
    {
	close (pipefds[1]);
	if(*pid > 0)
	    kill(*pid,SIGTERM);
	xqf_error("fcntl failed: %s", strerror(errno));
	return -1;
    }

    return pipefds[0];
}

void external_program_close_input(struct external_program_connection* conn)
{
    if(!conn) return;
    gdk_input_remove(conn->tag);
    close(conn->fd);
    if(conn->pid > 0) kill(conn->pid,SIGTERM);
    if(conn->do_quit) gtk_main_quit();
}

void external_program_input_callback(struct external_program_connection* conn, int fd, GIOCondition condition)
{
    int bytes;
    char* sol; // start of line pointer
    char* eol; // end of line pointer

    if(!conn) return;

    if(conn->pos >= conn->bufsize )
    {
	xqf_error("line %d too long",conn->linenr+1);
	external_program_close_input(conn);
	return;
    }

    bytes = read (fd, conn->buf + conn->pos, conn->bufsize - conn->pos);

    if (bytes < 0)
    {
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
	    return;
	}

	xqf_error("Error reading from child: %s",g_strerror(errno));
	external_program_close_input(conn);
	return;
    }
    if (bytes == 0) {	/* EOF */
	external_program_close_input(conn);
	return;
    }


    sol = conn->buf;
    eol = conn->buf + conn->pos;
    conn->pos += bytes;
    bytes = conn->pos;

    // buffer can contain multiple lines
    for(;(eol = memchr(eol,'\n',bytes-(eol-sol))) != NULL;bytes-=eol-sol+1,sol= ++eol)
    {
	*eol = '\0';
	//	debug(0,"%4d, line(%4d,%4d-%4d)>%s<",bytes, eol-sol,sol-conn->buf,eol-conn->buf,sol);
	++conn->linenr;
	conn->current_line = sol;
	if(conn->linefunc) (conn->linefunc)(conn);
    }
    // sol now points to begin of next line, if any

    if(sol-conn->buf)
    {
	if(bytes)
	{
	    memmove(conn->buf,sol,bytes);
	}
	conn->pos = bytes;
    }
}

int external_program_foreach_line(char* argv[], void (*linefunc)(struct external_program_connection* conn), gpointer data)
{
  struct external_program_connection conn = {0};

  conn.fd = start_prog_and_return_fd(argv,&conn.pid);

  if (conn.fd<0||conn.pid<=0)
    return FALSE;

  conn.bufsize = 512;
  conn.buf = g_new0(char,conn.bufsize);
  conn.result = FALSE;
  conn.do_quit = TRUE;
  conn.linenr = 0;
  conn.linefunc = linefunc;
  conn.data = data;

  conn.tag = gdk_input_add (conn.fd, GDK_INPUT_READ | GDK_INPUT_EXCEPTION, 
                                (GdkInputFunction) external_program_input_callback, &conn);

  gtk_main();

  g_free(conn.buf);

  return conn.result;
}

int _run_program_sync(const char* argv[], void(*child_callback)(void*), gpointer data)
{
    int status = -1;
    pid_t pid;

    pid = fork();
    if ( pid == 0) {
	if(child_callback)
	    child_callback(data);
	execvp(argv[0],argv);
	_exit(EXIT_FAILURE);
    }     
    else if(pid > 0)
    {
	waitpid(pid,&status,0);

	if(WIFEXITED(status))
	{
	    debug(3,"%s exited normally", argv[0]);
	}
	else
	{
	    debug(3,"%s exited with status %d", argv[0], WEXITSTATUS(status));
	}

	if(WIFSIGNALED(status))
	{
	    debug(3,"%s was killed by signal %d", argv[0], WTERMSIG(status));
	}
    }

    return status;
}

int run_program_sync(const char* argv[])
{
    return _run_program_sync(argv, NULL, NULL);
}

int run_program_sync_callback(const char* argv[], void(*child_callback)(void*), gpointer data)
{
    return _run_program_sync(argv, child_callback, data);
}

const char* copy_file(const char* src, const char* dest)
{
  char buf[4*4096];
  const char* msg = NULL;
  int fdin = -1, fdout = -1;
  int ret;

  fdin = open(src, O_RDONLY);
  if(fdin == -1)
  {
    msg = _("Can't open source file for reading");
    goto error_out;
  }

  fdout = open(dest, O_CREAT|O_WRONLY|O_TRUNC, 0777);
  if(fdout == -1)
  {
    msg = _("Can't open destination file for writing");
    goto error_out;
  }

  while((ret = read(fdin, buf, sizeof(buf))) > 0)
  {
    if(write(fdout, buf, ret) == -1)
    {
      msg = _("write error on destination file");
      goto error_out;
    }
  }

  if(ret == -1)
  {
    msg = _("read error on source file");
    goto error_out;
  }

error_out:
  if(fdin != -1)
    close(fdin);
  if(fdout != -1)
    close(fdout);

  if(msg)
    unlink(dest);

  return msg;
}
