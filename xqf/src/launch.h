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

#ifndef __LAUNCH_H__
#define __LAUNCH_H__

#include "xqf.h"


struct condef {
  struct server *s;
  char *server;
  char *gamedir;
  char *password;
  char *spectator_password;
  char *rcon_password;
  char *custom_cfg;
  char *demo;
  int spectate;
};

extern	void client_init (void);
extern	void client_detach_all (void);

extern	int client_launch_exec (int forkit, char *dir, char *argv[], 
                                                            struct server *s);

extern 	int client_launch (const struct condef *con, int forkit);

extern	struct condef *condef_new (struct server *s);
extern	void condef_free (struct condef *con);


#endif /* __LAUNCH_H__ */

