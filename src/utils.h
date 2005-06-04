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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <sys/types.h>
#include <stdio.h>	/* FILE */
#include <time.h>

#include <glib.h>


extern	short strtosh (const char *str);
extern	unsigned short strtoush (const char *str);

extern	char *strdup_strip (const char *);
extern  char *file_in_dir (const char *, const char *);
extern  int str_isempty (const char *);
extern	char *expand_tilde (const char *path);

extern	GList *dir_to_list (const char *dirname, 
                               char * (*filter) (const char *, const char *));

extern	GList *merge_sorted_string_lists (GList *list1, GList *list2);

extern	GSList *unique_strings (GSList *strings);

// build GList from array of char*
extern GList* createGListfromchar(char* strings[]);
  
extern	void on_sig (int signum, void (*func) (int signum));
extern 	void ignore_sigpipe (void);

extern	void print_dq_string (FILE *f, const unsigned char *str);

extern	char *lowcasestrstr (const char *str, const char *substr);

extern	int tokenize (char *str, char *token[], int max, const char *dlm);
extern	int safe_tokenize (const char *str, char *token[], 
                                                    int max, const char *dlm);
extern	int tokenize_bychar (char *str, char *token[], int max, char dlm);

extern	int hostname_is_valid (const char *hostname);

extern	char *find_game_dir (const char *basegamedir, const char *game,
						int *match_result);

extern	char* resolve_path(const char* path);

/* 
   Find a server setting from the info list in 
   the server struct.  The key passed will be converted to lower case.
*/
extern char* find_server_setting_for_key (char*, char**);


// return "false" if i == 0, "true" otherwise
const char* bool2str(int i);

// returns true if str is "true", false otherwise
int str2bool(const char* str);


/** \brief find executable file in $PATH.
 *
 * @param files colon seperated list of executables to search for
 * @returns absolute path to first found file, must be freed manually
 */
char* find_file_in_path(const char* files);

/** \brief find executable file in $PATH.
 *
 * @param files colon seperated list of executables to search for
 * @returns first found file, must be freed manually
 */
char* find_file_in_path_relative(const char* files);

/** \brief find executable file in $PATH.
 *
 * like find_file_in_path but take a NULL terminated list of files
 */
char* find_file_in_path_list(char** files);

/** \brief find executable file in $PATH.
 *
 * like find_file_in_path_relative but take a NULL terminated list of files
 */
char* find_file_in_path_list_relative(char** files);

/** sort list and remove duplicates
 * @param list list to sort
 * @compare_func function to use for comparing
 * @unref_func function to call for each deleted entry
 * @return sorted list without duplicates
 */
GSList* slist_sort_remove_dups(GSList* list, GCompareFunc compare_func, void (*unref_func)(void*));

/**
 * return locale's string representation of t. must be freed manually
 */
char* timet2string(const time_t* t);

/** set fd non blocking
  @param fd the file descriptor
  @return zero on success, -1 on failure
 */
int set_nonblock (int fd);

struct external_program_connection
{
    pid_t pid;
    int fd;
    gint tag; // for gdkinput
    char* buf;
    size_t bufsize;
    size_t pos;
    unsigned linenr;

    // contains the \0 terminated line without \n when linefunc is called
    const char* current_line;

    // function to be called when a complete line was received
    void (*linefunc)(struct external_program_connection* conn);

    // call gtk_main_quit
    gboolean do_quit;

    gpointer data;

    int result;
};

int start_prog_and_return_fd(char *const argv[], pid_t *pid);
void external_program_input_callback(struct external_program_connection* conn,
		int fd, GIOCondition condition);

/** enters gtk main loop */
int external_program_foreach_line(char* argv[], void (*linefunc)(struct external_program_connection* conn), gpointer data);

int run_program_sync(const char* argv[]);

#endif /* __UTILS_H__ */
