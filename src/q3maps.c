/* XQF - Quake server browser and launcher
 * Functions for finding installed q1-q3&clones maps
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <glib.h>
#include <unzip.h>

#include "debug.h"
#include "utils.h"

#include "q3maps.h"

struct q3mapinfo
{
	gchar* mapname;
	gchar* zipfile;                  // may be NULL
	gchar* levelshot;
};

static GSList* shotslist = NULL;    // temporary

/** pak file code ripped from q2 */

typedef unsigned char byte;

#define IDPAKHEADER (('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	int		ident;  // == IDPAKHEADER
	int		dirofs;
	int		dirlen;
} dpackheader_t;

static inline int    LittleLong (int l) {
#if BYTE_ORDER == BIG_ENDIAN
	byte    b1, b2, b3, b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
#else
	return l;
#endif
}

static void find__maps_pak(const char* packfile, GHashTable* maphash) {
	dpackheader_t header;
	// int numpackfiles;
	int fd;
	dpackfile_t info;

	fd = open(packfile, O_RDONLY);
	if (fd < 0) {
		perror(packfile);
		return;
	}

	if (read (fd, &header, sizeof(header))<0) {
		perror(packfile);
		return;
	}
	if (LittleLong(header.ident) != IDPAKHEADER) {
		debug(0, "%s is not a packfile\n", packfile);
		return;
	}
	header.dirofs = LittleLong (header.dirofs);
	header.dirlen = LittleLong (header.dirlen);

	// numpackfiles = header.dirlen / sizeof(dpackfile_t);

	// printf ("packfile %s (%i files)\n", packfile, numpackfiles);

	if (lseek (fd, header.dirofs, SEEK_SET) == -1) {
		perror(packfile);
		return;
	}

	while (read (fd, &info, sizeof(dpackfile_t))>0) {
		if (!strncmp(info.name, "maps/", 5) && !strcmp(info.name+strlen(info.name)-4, ".bsp")) {
			// s#maps/(.*)\.bsp#\1#
			gchar* mapname = g_ascii_strdown(info.name+5, strlen(info.name)-4-5);

			if (g_hash_table_lookup(maphash, mapname)) {
				g_free(mapname);
			}
			else {
				g_hash_table_insert(maphash, mapname, GUINT_TO_POINTER(-1));
			}
		}
	}

	close(fd);

	return;
}

static void free_q3mapinfo(struct q3mapinfo* mi) {
	g_free(mi->mapname);
	g_free(mi->levelshot);
	if (mi->zipfile)
		g_free(mi->zipfile);
	g_free(mi);
}

/** return pointer to last two path entries */
static inline const char* last_two_entries(const char* name) {
	const char *l = name;
	l = strrchr(l, '/');
	if (l && l != name) {
		const char *n = name;
		while (n != l) {
			++n;
			name = n;
			n = strchr(n, '/');
		}
	}
	else
		return NULL;

	return name;
}

static gchar* has_known_image_format(const gchar* name) {
	guint i, ext_num = 3;
	gchar *ext_list[3] = {
		".jpg",
		".png",
		".tga",
	};

	for (i = 0; i < ext_num; i++) {
		size_t name_len = strlen(name);
		size_t ext_len = strlen(ext_list[i]);

		if (name_len <= ext_len) {
			continue;
		}

		if (!g_ascii_strcasecmp(name + name_len - ext_len, ext_list[i])) {
			return g_strndup(name, name_len - ext_len);
		}
	}

	return NULL;
}

static char* is_q3_mapshot(const char* name) {
	gchar *mapname;
	debug(4, "check %s", name);
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "levelshots/", 11))
		return NULL;

	if ((mapname = has_known_image_format(name + 11))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

static char* is_unvanquished_mapshot(const char* name) {
	gchar *mapname;
	debug(4, "check %s", name);
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "meta/", 5))
		return NULL;

	if ((mapname = has_known_image_format(name + 5))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

static char* is_xonotic_mapshot(const char* name) {
	char* mapname;
	debug(4, "check %s", name);
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "maps/", 5))
		return NULL;

	if (g_strrstr(name, "/lm_") != NULL)
		return NULL;

	if ((mapname = has_known_image_format(name + 5))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

// must free
static gchar *strip_suffix(const gchar *name, const gchar *suffix) {
	if (g_str_has_suffix(name, suffix))
		return g_strndup(name, strlen(name) - strlen(suffix));
	return NULL;
}

// must free
static char* is_q3_map(const char* name) {
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "maps/", 5))
		return NULL;

	return strip_suffix(name + 5, ".bsp");
}

// must free
static char* is_doom3_mapshot(const char* name) {
	char* mapname;
	if (g_ascii_strncasecmp(name, "guis/assets/splash/", 19))
		return NULL;

	if ((mapname = has_known_image_format(name + 19))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

static char* is_doom3_map(const char* name) {
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "maps/game/", 10))
		return NULL;

	return strip_suffix(name + 10, ".map");
}

// must free
static char* is_quake4_mapshot(const char* name) {
	char* mapname;
	if (g_ascii_strncasecmp(name, "gfx/guis/loadscreens/", 21))
		return NULL;

	if ((mapname = has_known_image_format(name + 21))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

static char* is_quake4_map(const char* name) {
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "maps/", 5))
		return NULL;

	return strip_suffix(name + 5, ".map");
}

// must free
static char* is_etqw_mapshot(const char* name) {
	char* mapname;
	if (g_ascii_strncasecmp(name, "levelshots/thumbs/", 18))
		return NULL;

	if ((mapname = has_known_image_format(name + 18))) {
		debug(3, "found: %s", name);
		return mapname;
	}

	return NULL;
}

static char* is_etqw_map(const char* name) {
	if (*name == '/')
		name = last_two_entries(name);

	if (g_ascii_strncasecmp(name, "maps/", 5))
		return NULL;

	return strip_suffix(name + 5, ".stm");
}

static gboolean if_map_insert(const char* path, GHashTable* maphash, char* (*is_map_func)(const char* name)) {
	gchar* mapname = NULL;
	gchar* tmp;
	mapname = is_map_func(path);
	if (mapname) {
		// s#maps/(.*)\.bsp#\1#
		tmp = g_ascii_strdown(mapname, -1);
		strcpy(mapname, tmp);
		g_free(tmp);

		if (g_hash_table_lookup(maphash, mapname)) {
			g_free(mapname);
		}
		else {
			g_hash_table_insert(maphash, mapname, GUINT_TO_POINTER(-1));
		}
		return TRUE;
	}
	return FALSE;
}

static gboolean if_shot_insert(const char* path,
		char* (*is_mapshot_func)(const char* name),
		const char* zipfile) {
	gchar* mapname;
	//    debug(0, "check (%d) %s", (int)(zipfile!=NULL), path);
	mapname = is_mapshot_func(path);
	if (mapname) {
		struct q3mapinfo* mi = g_malloc0(sizeof(struct q3mapinfo));

		mi->mapname = g_ascii_strdown(mapname, -1);
		mi->levelshot = g_strdup(path);
		if (zipfile)
			mi->zipfile = g_strdup(zipfile);

		shotslist = g_slist_prepend(shotslist, mi);
		g_free(mapname);
		//		debug(0, "found shot %s", mi->levelshot);
		return TRUE;
	}
	return FALSE;
}


/** open zip file and insert all contained .bsp into maphash */
static void find_q3_maps_zip(const char* zipfile, GHashTable* maphash,
		char* (*is_map_func)(const char* name),
		char* (*is_mapshot_func)(const char* name)) {
	unzFile f;
	int ret;

	// some minizip releases do not allow NULL pointers for some unzGetCurrentFileInfo parameters
	// so we need to use real pointers even if we never read them:
	unz_file_info pfile_info = { 0 };
	void *extraField = { 0 };
	char buf[256] = "";

	if (!zipfile || !maphash || !is_map_func || !is_mapshot_func)
		return;

	f=unzOpen(zipfile);
	if (!f) {
		xqf_warning("could not open zip file %s", zipfile);
		return;
	}

	for (ret = unzGoToFirstFile(f); ret == UNZ_OK; ret = unzGoToNextFile(f)) {
		if (unzGetCurrentFileInfo(f, &pfile_info, buf, sizeof(buf), extraField, 0, NULL, 0) == UNZ_OK) {
			if (if_map_insert(buf, maphash, is_map_func)) {
			}
			if (if_shot_insert(buf, is_mapshot_func, zipfile)) {
			}
		}
	}

	if (unzClose(f) != UNZ_OK) {
		debug(0, "couldn't close file %s", zipfile);
	}
}

gboolean quake_contains_dir(const char* name, int level, GHashTable* maphash) {
	size_t len;
	// printf("directory %s at level %d\n", name, level);
	if (level == 0)
		return TRUE;

	len = strlen(name);

	if (level == 1 && len > 4 && !g_ascii_strcasecmp(name+len-4, "maps"))
		return TRUE;
	if (level == 1 && len > 10 && !g_ascii_strcasecmp(name+len-10, "levelshots"))
		return TRUE;

	return FALSE;
}

void quake_contains_file(const char* name, int level, GHashTable* maphash) {
	// printf("%s at level %d\n", name, level);
	if (strlen(name)>4 && !g_ascii_strcasecmp(name+strlen(name)-4, ".pak") && level == 1) {
		find__maps_pak(name, maphash);
	}
	if (strlen(name)>4 && !g_ascii_strcasecmp(name+strlen(name)-4, ".bsp") && level == 2) {
		gchar* basename=g_path_get_basename(name);
		gchar* mapname=g_ascii_strdown(basename, strlen(basename)-4);    /* g_ascii_strdown does implicit strndup */
		if (g_hash_table_lookup(maphash, mapname)) {
			g_free(mapname);
		}
		else {
			g_hash_table_insert(maphash, mapname, GUINT_TO_POINTER(-1));
		}
		g_free(basename);
	}
}

static void _q3_contains_file(const char* name, int level, GHashTable* maphash,
		char* (*is_map_func)(const char* name),
		char* (*is_mapshot_func)(const char* name)) {
	// printf("%s at level %d\n", name, level);
	if (level == 1 && !g_ascii_strcasecmp(name+strlen(name)-4, ".pk3") && strlen(name) > 4) {
		find_q3_maps_zip(name, maphash, is_map_func, is_mapshot_func);
	}
	else if (level == 2 && if_map_insert(name, maphash, is_map_func)) {
	}
	else if (level == 2 && if_shot_insert(name, is_mapshot_func, NULL)) {
	}
}

void q3_contains_file(const char* name, int level, GHashTable* maphash) {
	_q3_contains_file(name, level, maphash, is_q3_map, is_q3_mapshot);
}

void xonotic_contains_file(const char* name, int level, GHashTable* maphash) {
	_q3_contains_file(name, level, maphash, is_q3_map, is_xonotic_mapshot);
}

static void _daemon_contains_file(const char* name, int level, GHashTable* maphash,
		char* (*is_map_func)(const char* name),
		char* (*is_mapshot_func)(const char* name)) {
	// printf("%s at level %d\n", name, level);
	if (level == 1 && !g_ascii_strcasecmp(name+strlen(name)-4, ".dpk") && strlen(name) > 4) {
		find_q3_maps_zip(name, maphash, is_map_func, is_mapshot_func);
	}
	else if (level == 1 && !g_ascii_strcasecmp(name+strlen(name)-4, ".pk3") && strlen(name) > 4) {
		find_q3_maps_zip(name, maphash, is_map_func, is_mapshot_func);
	}
	else if (level == 2 && if_map_insert(name, maphash, is_map_func)) {
	}
	else if (level == 2 && if_shot_insert(name, is_mapshot_func, NULL)) {
	}
}

void unvanquished_contains_file(const char* name, int level, GHashTable* maphash) {
	_daemon_contains_file(name, level, maphash, is_q3_map, is_unvanquished_mapshot);
}

static void _doom3_contains_file(const char* name, int level, GHashTable* maphash,
		char* (*is_map_func)(const char* name),
		char* (*is_mapshot_func)(const char* name)) {
	// printf("%s at level %d\n", name, level);
	if (level == 1 && !g_ascii_strcasecmp(name+strlen(name)-4, ".pk4") && strlen(name) > 4) {
		find_q3_maps_zip(name, maphash, is_map_func, is_mapshot_func);
	}
}

void doom3_contains_file(const char* name, int level, GHashTable* maphash) {
	_doom3_contains_file(name, level, maphash, is_doom3_map, is_doom3_mapshot);
}

void quake4_contains_file(const char* name, int level, GHashTable* maphash) {
	_doom3_contains_file(name, level, maphash, is_quake4_map, is_quake4_mapshot);
}

void etqw_contains_file(const char* name, int level, GHashTable* maphash) {
	_doom3_contains_file(name, level, maphash, is_etqw_map, is_etqw_mapshot);
}


/**
 * traverse directory tree starting at startdir. Calls found_file for each file
 * and found_dir for each directory (non-recoursive). Note: There is no loop
 * detection so make sure found_dir returns false at some level.
 **/
void traverse_dir(const char* startdir, FoundFileFunction found_file, FoundDirFunction found_dir, gpointer data) {
	DIR* dir = NULL;
	struct dirent* dire = NULL;
	char* curdir = NULL;
	GSList* dirstack = NULL;
	typedef struct
	{
		char* name;
		int level;
	} DirStackEntry;

	DirStackEntry* dse;

	if (!startdir || !found_dir || !found_file)
		return;

	dse = g_new0(DirStackEntry, 1);
	dse->name=g_strdup(startdir);

	dirstack = g_slist_prepend(dirstack, dse);

	while (dirstack) {
		dse = dirstack->data;
		curdir = dse->name;

		dirstack = g_slist_delete_link(dirstack, dirstack);

		dir = opendir(curdir);
		if (!dir) {
			perror(curdir);
			g_free(curdir);
			g_free(dse);
			continue;
		}

		while ((dire = readdir(dir))) {
			char* name = dire->d_name;
			struct stat statbuf;

			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			name = g_strconcat(curdir, "/", name, NULL);
			if (lstat(name, &statbuf)) {
				perror(name);
				g_free(name);
				continue;
			}
			if (S_ISDIR(statbuf.st_mode)) {
				if (found_dir(name, dse->level, data)) {
					DirStackEntry* tmpdse = g_new0(DirStackEntry, 1);
					tmpdse->name=name;
					tmpdse->level=dse->level+1;
					dirstack = g_slist_prepend(dirstack, tmpdse);
				}
				else {
					g_free(name);
				}
			}
			else {
				found_file(name, dse->level, data);
				g_free(name);
			}
		}

		closedir(dir);
		g_free(curdir);
		g_free(dse);
	}
}

static gboolean maphashforeachremovefunc(gpointer key, gpointer value, gpointer user_data) {
	g_free(key);
	if (value && value!=GINT_TO_POINTER(-1)) {
		free_q3mapinfo((struct q3mapinfo*)value);
	}
	return TRUE;
}

/** free all keys and destroy maphash */
void q3_clear_maps(GHashTable* maphash) {
	if (!maphash)
		return;
	g_hash_table_foreach_remove(maphash, maphashforeachremovefunc, NULL);
	g_hash_table_destroy(maphash);
}

/** create map hash */
GHashTable* q3_init_maphash() {
	return g_hash_table_new(g_str_hash, g_str_equal);
}

/** return true if mapname is contained in maphash, false otherwise */
gboolean q3_lookup_map(GHashTable* maphash, const char* mapname) {
	if (g_hash_table_lookup(maphash, mapname))
		return TRUE;
	return FALSE;
}

/** return true if mapname is contained in maphash, false otherwise */
gboolean doom3_lookup_map(GHashTable* maphash, const char* mapname) {
	char* basename = g_path_get_basename(mapname);
	gpointer p = g_hash_table_lookup(maphash, basename);
	g_free(basename);
	if (p)
		return TRUE;
	return FALSE;
}

/***************/
static size_t readimagefromzip(guchar** buf, const char* zipfile, const char* filename) {
	unzFile zf;
	int ret;
	int error = 0;
	size_t buflen = 0;
	unz_file_info info = { 0 };

	// some minizip releases do not allow NULL pointers for some unzGetCurrentFileInfo parameters
	// so we need to use real pointers even if we never read them:
	void *extraField = { 0 };
	char szFileName[256] = "";

	g_return_val_if_fail(zipfile!=NULL, 0);
	g_return_val_if_fail(filename!=NULL, 0);

	zf = unzOpen(zipfile);
	g_return_val_if_fail(zf!=NULL, 0);

	do
	{
		ret = unzLocateFile(zf, filename, 2);
		if (ret!=UNZ_OK) {
			g_warning("unable to locate %s inside zip archive %s\n", filename, zipfile);
			error = 1;
			break;
		}

		ret = unzOpenCurrentFile(zf);
		if (ret!=UNZ_OK) {
			g_warning("unable to open %s inside zip archive %s\n", filename, zipfile);
			error = 1;
			break;
		}

		ret = unzGetCurrentFileInfo(zf, &info, szFileName, sizeof(szFileName), extraField, 0, NULL, 0);
		if (ret!=UNZ_OK || info.uncompressed_size <= 0) {
			g_warning("unable to retrieve info on %s inside zip archive %s\n", filename, zipfile);
			error = 1;
			break;
		}

		buflen = info.uncompressed_size;
		*buf = g_malloc0(buflen);
		if (!*buf) {
			g_error("out of memory");
			error = 1;
			break;
		}

		ret = unzReadCurrentFile(zf, *buf, buflen);
		if (ret<=0) {
			g_warning("unable read %s inside zip archive %s\n", filename, zipfile);
			error = 1;
			break;
		}

		ret = unzCloseCurrentFile(zf);
		if (ret == UNZ_CRCERROR) {
			g_warning("CRC Error: %s inside zip archive %s\n", filename, zipfile);
			error = 1;
			break;
		}

	} while(0);

	unzClose(zf);

	if (!error)
		return buflen;

	g_free(*buf);
	*buf=NULL;
	return 0;
}
/***************/

/** return true if mapname is contained in maphash, false otherwise */
size_t q3_lookup_mapshot(GHashTable* maphash, const char* mapname, guchar** buf) {
	struct q3mapinfo* mi = g_hash_table_lookup(maphash, mapname);
	if (mi && mi != GINT_TO_POINTER(-1)) {
		if (!mi->zipfile) {
			size_t size;
			*buf = (guchar*)load_file_mem(mi->levelshot, &size);
			return size;
		}
		else
			return readimagefromzip(buf, mi->zipfile, mi->levelshot);
	}
	return 0;
}

/** return true if mapname is contained in maphash, false otherwise
 * same as q3_lookup_mapshot except that the basename of the map is used
 */
size_t doom3_lookup_mapshot(GHashTable* maphash, const char* mapname, guchar** buf) {
	char* basename = g_path_get_basename(mapname);
	struct q3mapinfo* mi = g_hash_table_lookup(maphash, basename);
	g_free(basename);
	if (mi && mi != GINT_TO_POINTER(-1) && mi->zipfile) {
		return readimagefromzip(buf, mi->zipfile, mi->levelshot);
	}
	return 0;
}


static void process_levelshots(GHashTable* maphash) {
	GSList* ptr;
	for (ptr=shotslist;ptr;ptr=g_slist_next(ptr)) {
		struct q3mapinfo* mi = ptr->data;
		struct q3mapinfo* mih = NULL;
		char* origkey = NULL;
		gboolean found = FALSE;

		found = g_hash_table_lookup_extended(maphash, mi->mapname, (gpointer)&origkey, (gpointer)&mih);

		if (found != TRUE || mih != GINT_TO_POINTER(-1)) // not in hash or mapinfo alread defined
		{
			// debug(0, "drop shot %s %p", mapname, mih);
			free_q3mapinfo(mi);
		}
		else {
			// debug(0, "insert shot %s", mapname);
			g_hash_table_insert(maphash, origkey, mi);
		}

	}
	g_slist_free(shotslist);
	shotslist=NULL;
}

void find_q3_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)q3_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	process_levelshots(maphash);
}

void find_unvanquished_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)unvanquished_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	process_levelshots(maphash);
}

void find_xonotic_maps(GHashTable* maphash, const char* startdir) {
	// this code walks .xonotic to walk .xonotic/data
	traverse_dir(startdir, (FoundFileFunction)xonotic_contains_file, (FoundDirFunction)quake_contains_dir, maphash);

	// HACK: this code walks .xonotic/data to walk .xonotic/data/dlcache
	gchar *datadir;
	datadir = g_strconcat(startdir, "/data", NULL);
	traverse_dir(datadir, (FoundFileFunction)xonotic_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	g_free(datadir);

	process_levelshots(maphash);
}

void find_quake_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)quake_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
}

void find_doom3_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)doom3_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	process_levelshots(maphash);
}

void find_quake4_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)quake4_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	process_levelshots(maphash);
}

void find_etqw_maps(GHashTable* maphash, const char* startdir) {
	traverse_dir(startdir, (FoundFileFunction)etqw_contains_file, (FoundDirFunction)quake_contains_dir, maphash);
	process_levelshots(maphash);
}

