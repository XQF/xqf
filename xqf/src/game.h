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

#ifndef __GAME_H__
#define __GAME_H__

#include <sys/types.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include "xqf.h"
#include "launch.h"
#include "pixmaps.h"

#define	GAME_CONNECT		0x0001
#define	GAME_RECORD		0x0002
#define	GAME_SPECTATE		0x0004
#define	GAME_PASSWORD		0x0008
#define	GAME_RCON		0x0010

#define	GAME_QUAKE1_PLAYER_COLORS	0x0100
#define	GAME_QUAKE1_SKIN		0x0200

struct game {
  enum server_type type;
  int flags;

  char *name;
  unsigned short default_port;
  unsigned short default_master_port;

  char *id;
  char *qstat_str;
  char *qstat_option;
  char *qstat_master_option;
  struct pixmap *pix;

  struct player * (*parse_player) (char *tokens[], int num);
  void (*parse_server) (char *tokens[], int num, struct server *s);
  void (*analyze_serverinfo) (struct server *s);
  int (*config_is_valid) (struct server *s);
  int (*write_config) (const struct condef *con);
  int (*exec_client) (const struct condef *con, int forkit);
  GList * (*custom_cfgs) (char *dir, char *game);
  void (*save_info) (FILE *f, struct server *s);

  char *arch_identifier;
  enum CPU (*identify_cpu) (struct server *s, const char *versionstr);
  enum OS (*identify_os) (struct server *s, char *versionstr);

  char *cmd;
  char *dir;
  char *real_dir;
  char *game_cfg;
};

extern struct game games[];

extern	enum server_type id2type (const char *id);
extern	const char *type2id (enum server_type type);
extern	GtkWidget *game_pixmap_with_label (enum server_type type);

#endif /* __GAME_H__ */
