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

#ifndef __CONFIG_H__
#define __CONFIG_H__


#include <glib.h>

struct config_file {
  char *filename;
  GList *sections;
  int dirty;
};

struct config_section {
  char *name;
  GList *keys;
};

struct config_key {
  char *name;
  char *value;
};

extern	int	config_get_int_with_default (const char *path, int *def);
extern	double	config_get_float_with_default (const char *path, int *def);
extern	int	config_get_bool_with_default (const char *path, int *def);
extern	char *	config_get_string_with_default (const char *path, int *def);

#define	config_get_int(Path)	config_get_int_with_default ((Path), NULL)
#define	config_get_float(Path)	config_get_float_with_default ((Path), NULL)
#define	config_get_bool(Path)	config_get_bool_with_default ((Path), NULL)
#define	config_get_string(Path)	config_get_string_with_default ((Path), NULL)

extern	void	config_set_int (const char *path, int i);
extern	void	config_set_float (const char *path, double f);
extern	void	config_set_bool (const char *path, int b);
extern	void	config_set_string (const char *path, const char *s);

extern	void	*config_init_iterator (const char *path);
extern	void	*config_iterator_next (void *iterator, char **key, char **val);

extern	void 	config_clean_key (const char *path);
extern	void 	config_clean_section (const char *path);
extern	void 	config_clean_file (const char *path);

extern	void	config_sync (void);
extern	void	config_drop_all (void);
extern	void	config_drop_file (const char *path);

extern	void 	config_push_prefix (const char *prefix);
extern	void 	config_pop_prefix (void);

extern	void	config_set_base_dir (const char *dir);


#endif /* __CONFIG_H__ */

