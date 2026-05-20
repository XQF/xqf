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
 * GKeyFile-backed config implementation.
 *
 * The public API in config.h is unchanged; only this file changes.
 *
 * Path format used by callers:
 *
 *   /filename/section/key
 *     filename — logical file ("config" for the main config,
 *                "scripts/<name>" for script metadata)
 *     section  — GKeyFile group  (may contain spaces, colons, etc.)
 *     key      — GKeyFile key    (may contain '/' and spaces for legacy
 *                                 paths like "myscript.sh/enabled")
 *
 *   relative/key
 *     Concatenated with prefix_stack->data (config_push/pop_prefix).
 *
 *   path=default
 *     '=' and everything after it is the default value hint; stripped here.
 *
 * All writes go to search_dirs->data (the user write directory — the last
 * directory passed to config_add_dir, which uses g_slist_prepend).
 * Reads search all directories in order until the file is found.
 *
 * Script metadata files embed INI content between
 * "### BEGIN XQF INFO" / "### END XQF INFO" markers, with each line
 * prefixed by "# ".  We strip those prefixes and feed the result to
 * g_key_file_load_from_data().
 *
 * String escaping: the old custom parser and GKeyFile both use \n, \r, \\
 * so existing config files are read and written without format changes.
 */

#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "utils.h"
#include "debug.h"
#include "config.h"


/* ------------------------------------------------------------------ */
/* State                                                                */
/* ------------------------------------------------------------------ */

static GKeyFile   *main_kf      = NULL;   /* main config, lazy-loaded  */
static gboolean    main_dirty   = FALSE;
static GSList     *search_dirs  = NULL;   /* head = user write dir     */
static GList      *prefix_stack = NULL;   /* config_push/pop_prefix    */
static GHashTable *script_cache = NULL;   /* "scripts/x" → GKeyFile*   */


/* ------------------------------------------------------------------ */
/* Iterator types (opaque in config.h)                                  */
/* ------------------------------------------------------------------ */

struct config_key_iterator {
	char    **keys;   /* from g_key_file_get_keys  */
	gsize     n;
	gsize     i;
	GKeyFile *kf;     /* borrowed — do not unref   */
	char     *group;
};

struct config_section_iterator {
	char  **groups;   /* from g_key_file_get_groups */
	gsize   n;
	gsize   i;
};


/* ------------------------------------------------------------------ */
/* Script metadata loading                                              */
/* ------------------------------------------------------------------ */

#define SCRIPT_BEGIN "### BEGIN XQF INFO"
#define SCRIPT_END   "### END XQF INFO"

static GKeyFile *
load_script_kf (const char *filename)
{
	FILE   *f = NULL;
	GSList *l;

	for (l = search_dirs; l && !f; l = g_slist_next (l)) {
		char *fn = file_in_dir ((char *) l->data, filename);
		f = g_fopen (fn, "r");
		g_free (fn);
	}

	if (!f)
		return NULL;

	GString  *buf      = g_string_new (NULL);
	char      line[4096];
	gboolean  in_block = FALSE;

	while (fgets (line, sizeof (line), f)) {
		if (!in_block) {
			if (strncmp (line, SCRIPT_BEGIN, strlen (SCRIPT_BEGIN)) == 0)
				in_block = TRUE;
		} else {
			if (strncmp (line, SCRIPT_END, strlen (SCRIPT_END)) == 0)
				break;
			if (strncmp (line, "# ", 2) == 0)
				g_string_append (buf, line + 2);
		}
	}
	fclose (f);

	GKeyFile *kf  = g_key_file_new ();
	GError   *err = NULL;

	if (buf->len > 0)
		g_key_file_load_from_data (kf, buf->str, buf->len, G_KEY_FILE_NONE, &err);

	g_string_free (buf, TRUE);

	if (err) {
		debug (2, "script config parse error %s: %s", filename, err->message);
		g_error_free (err);
	}

	return kf;
}

static GKeyFile *
get_script_kf (const char *filename)
{
	if (!script_cache)
		script_cache = g_hash_table_new_full (g_str_hash, g_str_equal,
		                                      g_free,
		                                      (GDestroyNotify) g_key_file_free);

	GKeyFile *kf = g_hash_table_lookup (script_cache, filename);
	if (!kf) {
		kf = load_script_kf (filename);
		if (kf)
			g_hash_table_insert (script_cache, g_strdup (filename), kf);
	}
	return kf;
}


/* ------------------------------------------------------------------ */
/* Main config loading                                                   */
/* ------------------------------------------------------------------ */

static void
ensure_main_kf (void)
{
	if (main_kf)
		return;

	main_kf = g_key_file_new ();

	for (GSList *l = search_dirs; l; l = g_slist_next (l)) {
		char     *fn  = file_in_dir ((char *) l->data, "config");
		GError   *err = NULL;
		gboolean  ok  = g_key_file_load_from_file (main_kf, fn,
		                                            G_KEY_FILE_KEEP_COMMENTS,
		                                            &err);
		g_free (fn);
		if (ok)
			break;
		if (err) {
			if (!g_error_matches (err, G_FILE_ERROR, G_FILE_ERROR_NOENT))
				debug (2, "config load: %s", err->message);
			g_error_free (err);
		}
	}
}


/* ------------------------------------------------------------------ */
/* Path resolution                                                       */
/* ------------------------------------------------------------------ */

/*
 * Resolve a config path to (kf, group, key).
 *
 * Returns FALSE on malformed input or if kf cannot be found.
 * *kf_out  — borrowed, do not unref.
 * *group_out, *key_out — caller must g_free(); key is NULL for
 *   section-level paths (the caller must tolerate this).
 */
static gboolean
cfg_parse_path (const char  *path,
              GKeyFile   **kf_out,
              char       **group_out,
              char       **key_out)
{
	if (!path || !*path)
		return FALSE;

	char *buf;

	if (*path != '/') {
		if (!prefix_stack)
			return FALSE;

		const char *pfx  = (const char *) prefix_stack->data;
		gsize       plen = strlen (pfx);

		if (plen > 0 && pfx[plen - 1] != '/')
			buf = g_strconcat (pfx, "/", path, NULL);
		else
			buf = g_strconcat (pfx, path, NULL);

		if (*buf != '/') {
			g_free (buf);
			return FALSE;
		}
	} else {
		buf = g_strdup (path);
	}

	/* Strip optional =default suffix */
	char *eq = strchr (buf, '=');
	if (eq) *eq = '\0';

	char     *p = buf + 1;   /* skip leading '/' */
	GKeyFile *kf;
	char     *group, *key = NULL;

	if (strncmp (buf, "/scripts/", 9) == 0) {
		/* /scripts/<name>/section[/key] — script metadata cache */
		char *slash1 = strchr (p, '/');            /* '/' after "scripts"    */
		if (!slash1) { g_free (buf); return FALSE; }
		char *slash2 = strchr (slash1 + 1, '/');   /* '/' after script name  */
		if (!slash2) { g_free (buf); return FALSE; }

		*slash1 = '\0';
		char *scriptname = slash1 + 1;
		*slash2 = '\0';
		char *rest = slash2 + 1;

		char *filename = g_strconcat ("scripts/", scriptname, NULL);
		kf = get_script_kf (filename);
		g_free (filename);

		char *ksep = strchr (rest, '/');
		if (ksep) {
			*ksep = '\0';
			group = g_strdup (rest);
			key   = g_strdup (ksep + 1);
		} else {
			group = g_strdup (rest);
		}
	} else {
		/* /filename/section[/key] — main config */
		char *slash1 = strchr (p, '/');
		if (!slash1) { g_free (buf); return FALSE; }
		*slash1 = '\0';
		/* p = filename (always "config"; ignored — one main_kf) */
		char *rest = slash1 + 1;

		ensure_main_kf ();
		kf = main_kf;

		char *ksep = strchr (rest, '/');
		if (ksep) {
			*ksep = '\0';
			group = g_strdup (rest);
			key   = g_strdup (ksep + 1);
		} else {
			group = g_strdup (rest);
		}
	}

	g_free (buf);

	if (!kf) { g_free (group); g_free (key); return FALSE; }

	if (kf_out)    *kf_out    = kf;
	if (group_out) *group_out = group; else g_free (group);
	if (key_out)   *key_out   = key;   else g_free (key);

	return TRUE;
}


/* ------------------------------------------------------------------ */
/* Getters                                                               */
/* ------------------------------------------------------------------ */

int config_get_int_with_default (const char *path, int *def)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key) {
		if (def) *def = TRUE;
		return 0;
	}

	GError *err = NULL;
	int     val = g_key_file_get_integer (kf, group, key, &err);

	if (err) {
		if (def) *def = TRUE;
		val = 0;
		g_error_free (err);
	} else {
		if (def) *def = FALSE;
	}
	g_free (group);
	g_free (key);
	return val;
}


double config_get_float_with_default (const char *path, int *def)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key) {
		if (def) *def = TRUE;
		return 0.0;
	}

	GError *err = NULL;
	double  val = g_key_file_get_double (kf, group, key, &err);

	if (err) {
		if (def) *def = TRUE;
		val = 0.0;
		g_error_free (err);
	} else {
		if (def) *def = FALSE;
	}
	g_free (group);
	g_free (key);
	return val;
}


int config_get_bool_with_default (const char *path, int *def)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key) {
		if (def) *def = TRUE;
		return FALSE;
	}

	GError   *err = NULL;
	gboolean  val = g_key_file_get_boolean (kf, group, key, &err);

	if (err) {
		if (def) *def = TRUE;
		val = FALSE;
		g_error_free (err);
	} else {
		if (def) *def = FALSE;
	}
	g_free (group);
	g_free (key);
	return (int) val;
}


char *config_get_string_with_default (const char *path, int *def)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key) {
		if (def) *def = TRUE;
		return NULL;
	}

	GError *err = NULL;
	char   *val = g_key_file_get_string (kf, group, key, &err);

	if (err) {
		if (def) *def = TRUE;
		g_error_free (err);
	} else {
		if (def) *def = FALSE;
	}
	g_free (group);
	g_free (key);
	return val;
}


/* ------------------------------------------------------------------ */
/* Setters (write to main_kf only)                                      */
/* ------------------------------------------------------------------ */

void config_set_int (const char *path, int i)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key || kf != main_kf)
		return;
	g_key_file_set_integer (kf, group, key, i);
	main_dirty = TRUE;
	g_free (group);
	g_free (key);
}


void config_set_float (const char *path, double f)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key || kf != main_kf)
		return;
	g_key_file_set_double (kf, group, key, f);
	main_dirty = TRUE;
	g_free (group);
	g_free (key);
}


void config_set_bool (const char *path, int b)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key || kf != main_kf)
		return;
	g_key_file_set_boolean (kf, group, key, b ? TRUE : FALSE);
	main_dirty = TRUE;
	g_free (group);
	g_free (key);
}


void config_set_string (const char *path, const char *s)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key || kf != main_kf)
		return;
	g_key_file_set_string (kf, group, key, s ? s : "");
	main_dirty = TRUE;
	g_free (group);
	g_free (key);
}


/* ------------------------------------------------------------------ */
/* Key iterator                                                          */
/* ------------------------------------------------------------------ */

config_key_iterator *config_init_iterator (const char *path)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key))
		return NULL;
	g_free (key);   /* section-level: key component not needed */

	gsize   n   = 0;
	GError *err = NULL;
	char  **keys = g_key_file_get_keys (kf, group, &n, &err);

	if (err) {
		g_error_free (err);
		g_free (group);
		return NULL;
	}
	if (!keys || n == 0) {
		g_strfreev (keys);
		g_free (group);
		return NULL;
	}

	config_key_iterator *it = g_new0 (config_key_iterator, 1);
	it->keys  = keys;
	it->n     = n;
	it->i     = 0;
	it->kf    = kf;
	it->group = group;
	return it;
}


config_key_iterator *config_iterator_next (config_key_iterator *it,
                                            char **key_out,
                                            char **val_out)
{
	if (!it)
		return NULL;

	if (key_out) *key_out = g_strdup (it->keys[it->i]);
	if (val_out) *val_out = g_key_file_get_value (it->kf, it->group,
	                                               it->keys[it->i], NULL);
	it->i++;

	if (it->i >= it->n) {
		g_strfreev (it->keys);
		g_free (it->group);
		g_free (it);
		return NULL;
	}
	return it;
}


/* ------------------------------------------------------------------ */
/* Section iterator                                                      */
/* ------------------------------------------------------------------ */

config_section_iterator *config_init_section_iterator (const char *path)
{
	GKeyFile *kf = NULL;

	if (!path || *path != '/')
		return NULL;

	/* /scripts/<name>  — may not have a section component */
	if (strncmp (path, "/scripts/", 9) == 0) {
		const char *name = path + 1;   /* "scripts/<name>" */
		kf = get_script_kf (name);
	} else {
		/* resolve_path handles the general case */
		char *group;
		if (!cfg_parse_path (path, &kf, &group, NULL))
			return NULL;
		g_free (group);
	}

	if (!kf)
		return NULL;

	gsize   n      = 0;
	char  **groups = g_key_file_get_groups (kf, &n);

	if (!groups || n == 0) {
		g_strfreev (groups);
		return NULL;
	}

	config_section_iterator *it = g_new0 (config_section_iterator, 1);
	it->groups = groups;
	it->n      = n;
	it->i      = 0;
	return it;
}


config_section_iterator *config_section_iterator_next (config_section_iterator *it,
                                                        char **section_out)
{
	if (!it)
		return NULL;

	if (section_out) *section_out = g_strdup (it->groups[it->i]);
	it->i++;

	if (it->i >= it->n) {
		g_strfreev (it->groups);
		g_free (it);
		return NULL;
	}
	return it;
}


/* ------------------------------------------------------------------ */
/* Clean / drop                                                          */
/* ------------------------------------------------------------------ */

void config_clean_key (const char *path)
{
	GKeyFile *kf;
	char     *group, *key;

	if (!cfg_parse_path (path, &kf, &group, &key) || !key || kf != main_kf)
		return;

	g_key_file_remove_key (kf, group, key, NULL);

	/* Drop the group if it is now empty */
	gsize   n    = 0;
	char  **keys = g_key_file_get_keys (kf, group, &n, NULL);
	g_strfreev (keys);
	if (n == 0)
		g_key_file_remove_group (kf, group, NULL);

	main_dirty = TRUE;
	g_free (group);
	g_free (key);
}


void config_clean_section (const char *path)
{
	GKeyFile *kf;
	char     *group;

	if (!cfg_parse_path (path, &kf, &group, NULL) || kf != main_kf)
		return;

	g_key_file_remove_group (kf, group, NULL);
	main_dirty = TRUE;
	g_free (group);
}


void config_clean_file (const char *path)
{
	GKeyFile *kf;
	char     *group;

	if (!cfg_parse_path (path, &kf, &group, NULL) || kf != main_kf)
		return;
	g_free (group);

	gsize   n      = 0;
	char  **groups = g_key_file_get_groups (kf, &n);
	for (gsize i = 0; i < n; i++)
		g_key_file_remove_group (kf, groups[i], NULL);
	g_strfreev (groups);
	main_dirty = TRUE;
}


/* ------------------------------------------------------------------ */
/* Sync / drop                                                           */
/* ------------------------------------------------------------------ */

void config_sync (void)
{
	if (!main_kf || !main_dirty || !search_dirs)
		return;

	const char *write_dir = (const char *) search_dirs->data;
	char       *fn        = file_in_dir (write_dir, "config");

	gsize   n      = 0;
	char  **groups = g_key_file_get_groups (main_kf, &n);
	g_strfreev (groups);

	if (n == 0) {
		g_unlink (fn);
	} else {
		GError *err = NULL;
		if (!g_key_file_save_to_file (main_kf, fn, &err)) {
			debug (1, "config_sync: %s", err ? err->message : "unknown error");
			if (err) g_error_free (err);
		}
	}
	g_free (fn);
	main_dirty = FALSE;
}


void config_drop_all (void)
{
	if (main_kf) {
		g_key_file_free (main_kf);
		main_kf    = NULL;
		main_dirty = FALSE;
	}
	if (script_cache) {
		g_hash_table_destroy (script_cache);
		script_cache = NULL;
	}
	while (prefix_stack) {
		g_free (prefix_stack->data);
		prefix_stack = g_list_delete_link (prefix_stack, prefix_stack);
	}
}


void config_drop_file (const char *path)
{
	if (!script_cache || !path)
		return;

	/* path is "/scripts/<name>" — strip the leading '/' */
	const char *name = (*path == '/') ? path + 1 : path;
	g_hash_table_remove (script_cache, name);
}


/* ------------------------------------------------------------------ */
/* Prefix stack                                                          */
/* ------------------------------------------------------------------ */

void config_push_prefix (const char *prefix)
{
	if (prefix && *prefix)
		prefix_stack = g_list_prepend (prefix_stack, g_strdup (prefix));
}


void config_pop_prefix (void)
{
	if (prefix_stack) {
		g_free (prefix_stack->data);
		prefix_stack = g_list_delete_link (prefix_stack, prefix_stack);
	}
}


/* ------------------------------------------------------------------ */
/* Search directories                                                    */
/* ------------------------------------------------------------------ */

void config_add_dir (const char *dir)
{
	search_dirs = g_slist_prepend (search_dirs, g_strdup (dir));
}
