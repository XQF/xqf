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

#include <gtk/gtk.h>

#include "utils.h"
#include "debug.h"


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

  if (!dir || dir[0] == '\0')			/* dir "" is current dir */
    return (file)? g_strdup (file) : NULL;

  if (!file)
    return g_strdup (dir);

  if (dir[strlen (dir) - 1] != G_DIR_SEPARATOR)
    need_slash = 1;

  tmp = res = g_malloc (strlen (dir) + strlen (file) + need_slash + 1);

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
 * executables to search for. return name of first found file, must be freed
 * manually
 */
char* find_file_in_path(const char* files)
{
    char** binaries = NULL;
    char* path = NULL;
    int i = 0, j = 0;
    char** directories = NULL;
    char* found = NULL;

    binaries = g_strsplit(files,":",0);
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
		found = g_strdup(binaries[i]);
		debug(3,"found %s",binaries[i]);
	    }
	    g_free(file);
	}
    }

    g_strfreev(binaries);

    return found;
}

// find a directory inside another, prefer matching case otherwise search case
// insensitive
// returns name of found directory or duplicate of game, must be freed manually
char *find_game_dir (const char *basegamedir, const char *game, int *match_result)
{
  DIR *dp;
  struct dirent *ep;
  struct stat buf;
  char *path;
  char *temp;

  *match_result = 0; // No match, unless changed below
  
  if(!game)
    return g_strdup("");

  if(!basegamedir)
    return g_strdup(game);  

  debug( 1, "Looking for subdir/symlink %s in %s", game, basegamedir);
  
  // Look for exact match
  // Looks for file, not specifically a subdir or symlink - good enough
  path = file_in_dir (basegamedir, game);
  if (!stat (path, &buf)) {
    if ( (S_ISDIR(buf.st_mode) == 1) || (S_ISLNK(buf.st_mode)) ) {
      // directory found
      g_free (path);
      debug( 1, "Found exact match for subdir/symlink %s in %s", game, basegamedir);
      *match_result = 1; // Exact match
      return g_strdup (game);
    }
  }
  g_free (path);
  debug( 1, "Did not find exact match for subdir/symlink %s in %s", game, basegamedir);

  // Did not find exact match, perform search
  debug( 1, "Searching for subdir/symlink %s in %s ignoring case", game, basegamedir);
  dp = opendir (basegamedir);
  if (dp != NULL)
    {
      while ((ep = readdir (dp))) {
	stat(ep->d_name, &buf);
        if ( (S_ISDIR(buf.st_mode) == 1) || (S_ISLNK(buf.st_mode)) )
	  if (strcmp(ep->d_name,".") && strcmp(ep->d_name,".."))
	  {
	    if (!strcasecmp(ep->d_name,game)) {
              debug( 1, "Found subdir/symlink %s in %s that matches %s", ep->d_name, basegamedir,
                                                                    game);
              *match_result = 2; // Different case match
	      temp = g_strdup (ep->d_name);
	      closedir (dp);
	      return (temp);
	      break;
	    }
  	  }
      }
      closedir (dp);
      debug( 1, "Could not find any match for subdir/symlink %s in %s.  Returning %s",game, 
                                                                basegamedir, game);
    }
  else
    debug( 1, "Could not open base directory %s!!", basegamedir);

  return g_strdup (game);
}
