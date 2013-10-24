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

#ifndef __ADDSERVER_H__
#define __ADDSERVER_H__

#include "xqf.h"


/** dialog to prompt user for server type and address
 * @param type pointer where to store the selected type
 * @param addr preset string value for address field, NULL for nothing
 * @returns address string or NULL if user pressed cancel
 */
extern	char *add_server_dialog (enum server_type *type, const char* addr);

extern	void add_server_init (void);
extern	void add_server_done (void);


#endif /* __ADDSERVER_H__ */

