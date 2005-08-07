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

/*
 *  Simple 'ini'-file management.
 *  Function calls should be compatible with GNOME ones.
 */

#include <sys/types.h>
#include <stdio.h>	/* FILE */
#include <string.h>	/* strchr, strcmp, strlen */
#include <stdlib.h>	/* strtol, strtod */
#include <unistd.h>	/* unlink */

#include <glib.h>

#include "utils.h"
#include "debug.h"
#include "config.h"


static GSList* directories = NULL;
static GList *files = NULL;
static GList *prefix_stack = NULL;

struct config_file {
  char *filename;
  GList *sections;
  unsigned dirty : 1;
  unsigned read_only : 1;
};

struct config_section {
  char *name;
  GList *keys;
};

struct config_key {
  char *name;
  char *value;
};

#ifdef DEBUG
static void dump_base (void) {
  GList *fptr, *sptr, *kptr;
  struct config_file *file;
  struct config_section *section;
  struct config_key *key;

  for (fptr = files; fptr; fptr = fptr->next) {
    file = (struct config_file *) fptr->data;
    fprintf (stderr, "---- %s\n", file->filename);

    for (sptr = file->sections; sptr; sptr = sptr->next) {
      section = (struct config_section *) sptr->data;
      fprintf (stderr, "[%s]\n", section->name);

      for (kptr = section->keys; kptr; kptr = kptr->next) {
	key = (struct config_key *) kptr->data;
	fprintf (stderr, "%s=%s\n", key->name, key->value);
      }
    }
  }
  fprintf (stderr, "----\n");
}
#endif


/**
 * lookup keyname in secname in filename (secname and keyname can be NULL).
 * When create is set, appropriate entries will be created if they don't
 * already exist. When file1, section1 and key1 are not NULL (all three are
 * independent), pointers to the found/created structs are stored there.
**/
static void find_key (const char *filename,
		      const char *secname,
		      const char *keyname,
		      int create,
		      struct config_file **file1,
		      struct config_section **section1,
		      struct config_key **key1) {

  struct config_file *file = NULL;
  struct config_section *section = NULL;
  struct config_key *key = NULL;
  GList *list;

  if (file1) *file1 = NULL;
  if (section1) *section1 = NULL;
  if (key1) *key1 = NULL;

  // look if file is already known
  for (list = files; list; list = list->next) {
    file = (struct config_file *) list->data;
    if (strcmp (filename, file->filename) == 0)
      break;
  }

  // otherwise create one
  if (!list) {

    if (!create)
      return;

    file = g_malloc0 (sizeof (struct config_file));
    file->filename = g_strdup (filename);
    files = g_list_append (files, file);
  }

  if (file1)
    *file1 = file;

  if (secname) {
    // see if section is alreay known in file
    for (list = file->sections; list; list = list->next) {
      section = (struct config_section *) list->data;
      if (g_strcasecmp (secname, section->name) == 0)
	break;
    }

    // otherwise create one
    if (!list) {

      if (!create)
	return;

      section = g_malloc0 (sizeof (struct config_section));
      section->name = g_strdup (secname);
      file->sections = g_list_append (file->sections, section);
    }

    if (section1)
      *section1 = section;
  }

  if (secname && keyname) {
    // see if key is already in section
    for (list = section->keys; list; list = list->next) {
      key = (struct config_key *) list->data;
      if (g_strcasecmp (keyname, key->name) == 0)
	break;
    }

    // otherwise create one
    if (!list) {

      if (!create)
	return;

      key = g_malloc0 (sizeof (struct config_key));
      key->name = g_strdup (keyname);
      section->keys = g_list_append (section->keys, key);
    }

    if (key1)
      *key1 = key;
  }
}

/**
 * lookup keyname in section, if not found create a new config_key.
 * returns pointer to found or created key
 */
static struct config_key *key_in_section (struct config_section *section, 
      					                      char *keyname) {
  struct config_key *key;
  GList *list;

  for (list = section->keys; list; list = list->next) {
    key = (struct config_key *) list->data;
    if (g_strcasecmp (keyname, key->name) == 0)
      break;
  }

  if (!list) {
    key = g_malloc0 (sizeof (struct config_key));
    key->name = g_strdup (keyname);
    section->keys = g_list_append (section->keys, key);
  }

  return key;
}


#define BUF_SIZE	(1024*8)

struct state_s
{
  const char* filename;
  char *secname;
  struct config_section *section;
  void* data;
};

static gboolean parse_line_file(char* buf, struct state_s* state)
{
  struct config_key *key = NULL;
  char *p;
  unsigned len;

  len = strlen(buf);

  if (len <= 1)
      return TRUE;

  if (buf[len - 1] == '\n') {
    buf[len - 1] = '\0';
    len--;
  }

  if (buf[len - 1] == '\r') {
    buf[len - 1] = '\0';
    len--;
  }

  if (*buf == '[') {
    p = strchr (buf + 1, ']');
    if (p) {
      *p = '\0';

      if (state->secname)
	g_free (state->secname);

      state->secname = g_strdup (buf + 1);
      state->section = NULL;
    }
  }
  else {
    p = strchr (buf, '=');
    if (p) {
      *p++ = '\0';

      if (!state->section && !state->secname)
	return TRUE;

      if (!state->section)
	find_key (state->filename, state->secname, buf, TRUE, NULL, &state->section, &key);
      else
	key = key_in_section (state->section, buf);

      if (key->value)
	g_free (key->value);

      key->value = g_strdup (p);
    }
  }

  return TRUE;
}

#define BEGIN_XQF_INFO  "### BEGIN XQF INFO"
#define END_XQF_INFO    "### END XQF INFO"
static gboolean parse_line_script(char* buf, struct state_s* state)
{
  switch(GPOINTER_TO_INT(state->data))
  {
    case 0:
      if(!strncmp(buf, BEGIN_XQF_INFO, strlen(BEGIN_XQF_INFO)))
	state->data = GINT_TO_POINTER(1);
      return TRUE;

    case 1:
      if(!strncmp(buf, "# ", 2))
	return parse_line_file(buf+2, state);
      else if(!strncmp(buf, END_XQF_INFO, strlen(END_XQF_INFO)))
	return FALSE;
  }
  return FALSE;
}
#undef BEGIN_XQF_INFO
#undef END_XQF_INFO

static void load_file (const char *filename) {
  FILE *f = NULL;
  char buf[BUF_SIZE];
  char *fn;
  struct state_s state = {0};
  GSList* l;
  gboolean (*parse_line)(char* buf, struct state_s* state);
  struct config_file* file = NULL;

  debug (0, "%s", filename);

  for(l = directories; l && !f; l = g_slist_next(l))
  {
    fn = file_in_dir (l->data, filename);
    f = fopen (fn, "r");
    if(f) debug(4, "loaded %s", fn);
    g_free (fn);
  }

  /* Create empty file record and thus cache lookups of non-existent files.
   * Don't mark it 'dirty' -- nothing was changed really.
   */

  find_key (filename, NULL, NULL, TRUE, &file, NULL, NULL);

  if (!f)
    return;

  state.filename = filename;

  // XXX
  if(!strncmp(filename, "scripts/", strlen("scripts/")))
  {
    parse_line = parse_line_script;
    file->read_only = 1;
  }
  else
    parse_line = parse_line_file;

  while (fgets (buf, BUF_SIZE, f))
    parse_line(buf, &state);

  g_free(state.secname);
  fclose (f);
}


static void unlink_file (const char *filename) {
  char *fn;

  fn = file_in_dir (directories->data, filename);
#ifdef DEBUG
  fprintf (stderr, "config.c: unlink_file (%s)\n", fn);
#endif
  unlink (fn);
  g_free (fn);
}


static char *path_concat (const char *str1, const char *str2) {
  if (str1[strlen (str1) - 1] != '/')
    return g_strconcat (str1, "/", str2, NULL);
  else
    return g_strconcat (str1, str2, NULL);
}


static struct config_key *parse_path (const char *path,
				      int create,
				      char **defval,
				      struct config_file **file1,
				      struct config_section **section1) {
  const char *filename;
  char *secname = NULL;
  char *keyname = NULL;
  char *def;
  char *buf = NULL;
  char *ptr = NULL;
  struct config_file *file;
  struct config_key *key = NULL;

  if (defval) {
    *defval = NULL;
    def = strchr (path, '=');
    if (def)
      *defval = def + 1;
  }

  if (*path != '/') {
    if (prefix_stack)
      buf = path_concat ((const char *) prefix_stack->data, path);
    else
      return NULL;

    if (*buf != '/') {
      g_free (buf);
      return NULL;
    }
  }
  else {
    buf = g_strdup (path);
  }

  def = strchr (buf, '=');
  if (def)
    *def = '\0';

  filename = ptr = &buf[1];

  // XXX
  if(!strncmp(buf, "/scripts", strlen("/scripts")))
  {
    ptr = strchr (ptr, '/');
    if(ptr)
      ++ptr;
  }

  secname = strchr (ptr, '/');
  if (secname) {
    *secname++ = '\0';

    keyname = strchr (secname, '/');
    if (keyname)
      *keyname++ = '\0';
  }

  /* Ensure that the file is loaded */

  find_key (filename, NULL, NULL, FALSE, &file, NULL, NULL);
  if (!file)
    load_file (filename);

  /* Actually look up the key */

  find_key (filename, secname, keyname, create, file1, section1, &key);

  if (buf)
    g_free (buf);

  return key;
}


static char *string_escape (const char *s) {
  const char *c;
  char *p;
  char *res;
  int n = 0;

  if (!s)
    return NULL;

  for (c = s; *c; c++) {
    if (*c == '\n' || *c == '\r' || *c == '\\') {
      n++;
    }
    n++;
  }

  p = res = g_malloc (n + 1);

  do {
    switch (*s) {

    case '\n':
      *p++ = '\\';
      *p++ = 'n';
      break;

    case '\r':
      *p++ = '\\';
      *p++ = 'r';
      break;

    case '\\':
      *p++ = '\\';
      *p++ = '\\';
      break;

    default:
      *p++ = *s;
      break;

    }
  } while (*s++);

  return res;
}


static char *string_unescape (const char *s) {
  const char *c;
  char *p;
  char *res;
  int n = 0;

  if (!s)
    return NULL;

  for (c = s; *c; c++) {
    if (*c == '\\' && (c[1] == 'n' || c[1] == 'r' || c[1] == '\\'))
      c++;
    n++;
  }

  p = res = g_malloc (n + 1);

  do {

    if (*s != '\\') {
      *p++ = *s;
    }
    else {
      s++;

      switch (*s) {

      case 'n':
	*p++ = '\n';
	break;

      case 'r':
	*p++ = '\r';
	break;

      case '\\':
	*p++ = '\\';
	break;

      default:
	*p++ = '\\';
	*p++ = *s;
	break;

      }
    }

  } while (*s++);

  return res;
}


static char *config_get_raw_with_default (const char *path, int *def) {
  struct config_file *file;
  struct config_key *key;
  char *val;

  key = parse_path (path, FALSE, &val, &file, NULL);
  if (key) {
    if (def) *def = FALSE;
    val = key->value;
  }
  else {
    if (def) *def = TRUE;
  }
  return val;
}


int config_get_int_with_default (const char *path, int *def) {
  char *val;

  val = config_get_raw_with_default (path, def);
  return (val)? strtol (val, NULL, 10) : 0;
}


double config_get_float_with_default (const char *path, int *def) {
  char *val;

  val = config_get_raw_with_default (path, def);
  return (val)? strtod (val, NULL) : 0.0;
}


int config_get_bool_with_default (const char *path, int *def) {
  char *val;

  val = config_get_raw_with_default (path, def);

  /* Gnome returns FALSE as default value */

  if (val && !g_strcasecmp (val, "true"))
    return TRUE;
  else
    return FALSE;
}


char *config_get_string_with_default (const char *path, int *def) {
  char *val;

  val = config_get_raw_with_default (path, def);
  return (val)? string_unescape (val) : NULL;
}


static void config_set_raw (const char *path, char *raw_value) {
  struct config_key *key;
  struct config_file *file;

  key = parse_path (path, TRUE, NULL, &file, NULL);
  if (key) {
    if (key->value)
      g_free (key->value);
    key->value = raw_value;
    if (file)
      file->dirty = TRUE;
  }
}


void config_set_int (const char *path, int i) {
  config_set_raw (path, g_strdup_printf ("%d", i));
}


void config_set_float (const char *path, double f) {
  config_set_raw (path, g_strdup_printf ("%g", f));
}


void config_set_bool (const char *path, gboolean b) {
  config_set_raw (path, g_strdup ((b)? "true" : "false"));
}


void config_set_string (const char *path, const char *str) {
  config_set_raw (path, string_escape (str));
}


config_key_iterator* config_init_iterator (const char *path) {
  struct config_section *section;

  parse_path (path, FALSE, NULL, NULL, &section);
  return (section)? (config_key_iterator*)section->keys : NULL;
}


config_key_iterator* config_iterator_next (config_key_iterator* iterator, char **key, char **val) {
  struct config_key *cfg_key;

  if (!iterator)
    return NULL;

  cfg_key = ((GList *) iterator)->data;

  if (key) *key = g_strdup (cfg_key->name);
  if (val) *val = g_strdup (cfg_key->value);

  return (config_key_iterator*)((GList *) iterator)->next;
}

config_section_iterator* config_init_section_iterator (const char *path) {
  struct config_file *file;

  parse_path (path, FALSE, NULL, &file, NULL);
  return (file)? (config_section_iterator*)file->sections : NULL;
}


config_section_iterator* config_section_iterator_next (config_section_iterator* iterator, char **section) {
  struct config_section *cfg_sect;

  if (!iterator)
    return NULL;

  cfg_sect = ((GList *) iterator)->data;

  if (section) *section = g_strdup (cfg_sect->name);

  return (config_section_iterator*)((GList *) iterator)->next;
}



static void dump_file (struct config_file *file) {
  GList *sptr, *kptr;
  struct config_section *section;
  struct config_key *key;
  FILE *f;
  char *fn;

  if(!file->dirty)
    return;

  if(file->read_only)
    return;

  if (!file->sections) {
    unlink_file (file->filename);
    return;
  }

  fn = file_in_dir (directories->data, file->filename);
  f = fopen (fn, "w");
  g_free (fn);

  if (f) {
    for (sptr = file->sections; sptr; sptr = sptr->next) {
      section = (struct config_section *) sptr->data;
      fprintf (f, "[%s]\n", section->name);

      for (kptr = section->keys; kptr; kptr = kptr->next) {
	key = (struct config_key *) kptr->data;
	fprintf (f, "%s=%s\n", key->name, key->value);
      }

      fprintf (f, "\n");
    }
    fclose (f);
  }
}


void config_sync (void) {
  GList *list;
  struct config_file *file;

  for (list = files; list; list = list->next) {
    file = (struct config_file *) list->data;
    dump_file (file);
  }
  /* config_drop_all (); */
}


static void drop_file (struct config_file *file) {
  GList *sptr, *kptr;
  struct config_section *section;
  struct config_key *key;

#ifdef DEBUG
  fprintf (stderr, "config.c: drop_file (%s)\n", file->filename);
#endif

  for (sptr = file->sections; sptr; sptr = sptr->next) {
    section = (struct config_section *) sptr->data;

    for (kptr = section->keys; kptr; kptr = kptr->next) {
      key = (struct config_key *) kptr->data;
      g_free (key->name);
      g_free (key->value);
      g_free(key);
      key=NULL;
    }

    g_list_free (section->keys);
    g_free (section->name);
    g_free(section);
    section=NULL;
  }
 
  g_list_free (file->sections);
  g_free (file->filename);

  files = g_list_remove (files, file);

  g_free(file);
}


static void drop_section (struct config_file *file, 
			  struct config_section *section) {
  GList *kptr;
  struct config_key *key;

  for (kptr = section->keys; kptr; kptr = kptr->next) {
    key = (struct config_key *) kptr->data;
    g_free (key->name);
    g_free (key->value);
    g_free (key);
  }
  g_list_free (section->keys);
  g_free (section->name);
  file->sections = g_list_remove (file->sections, section);

  g_free(section);

  file->dirty = TRUE;
}


static void drop_key (struct config_file *file, 
		      struct config_section *section,
		      struct config_key *key) {
  g_free (key->name);
  g_free (key->value);
  section->keys = g_list_remove (section->keys, key);
  g_free(key);

  file->dirty = TRUE;

  if (section->keys == NULL)
    drop_section (file, section);
}


void config_drop_all (void) {
  while (files) {
    drop_file ((struct config_file *) files->data);
  }
}


void config_drop_file (const char *path) {
  struct config_file *file = NULL;

  parse_path (path, FALSE, NULL, &file, NULL);
  if (file)
    drop_file (file);
}


void config_clean_key (const char *path) {
  struct config_file *file;
  struct config_section *section;
  struct config_key *key;

  key = parse_path (path, FALSE, NULL, &file, &section);
  if (key)
    drop_key (file, section, key);
}


void config_clean_section (const char *path) { 
  struct config_file *file;
  struct config_section *section = NULL;

  parse_path (path, FALSE, NULL, &file, &section);
  if (section)
    drop_section (file, section);
}


void config_clean_file (const char *path) {
  struct config_file *file = NULL;
  char *filename;

  parse_path (path, FALSE, NULL, &file, NULL);
  if (file) {
    filename = g_strdup (file->filename);

    drop_file (file);

    /* Re-create file record and mark it as 'dirty' */

    find_key (filename, NULL, NULL, TRUE, &file, NULL, NULL);
    file->dirty = TRUE;

    g_free (filename);
  }
}


void config_push_prefix (const char *prefix) {
  if (prefix && *prefix)
    prefix_stack = g_list_prepend (prefix_stack, g_strdup (prefix));
}


void config_pop_prefix (void) {
  void *data;

  if (prefix_stack) {
    data = prefix_stack->data;
    prefix_stack = g_list_remove (prefix_stack, data);
    g_free (data);
  }
}


void config_add_dir (const char *dir) {
  directories = g_slist_prepend(directories,g_strdup (dir));
}

