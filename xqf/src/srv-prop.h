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

#ifndef __SRV_PROP_H__
#define __SRV_PROP_H__

#include "xqf.h"


struct server_props {
  struct host *host;
  int port;

  char *custom_cfg;

  char *server_password;
  char *spectator_password;
  char *rcon_password;
  int  reserved_slots; /*pulp*/

  int sucks;
  char *comment;
};

extern	struct server_props *properties (struct server *s);
extern	struct server_props *properties_new (struct host *host, 
                                                         unsigned short port);
extern	void props_free_all (void);
extern	void props_save (void);
extern	void props_load (void);

extern 	void properties_dialog (struct server *s);

extern	void combo_set_vals (GtkWidget *combo, GList *strlist, const char *str);


#endif /* __SRV_PROP_H__ */

