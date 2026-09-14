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

#ifndef __RCON_MSG_H__
#define __RCON_MSG_H__

/*
 * msg_terminate() used to be static in rcon.c. It doesn't touch sockets,
 * huffman, or GTK, so it's split out here to be testable on its own
 * (tests/test_rcon_msg.c) without linking the rest of rcon.c, which pulls
 * in readline (BUILD_RCON) or the GTK rcon console widget (BUILD_XQF).
 */

/** ensure msg is terminated by \n\0. return string that fulfills this
 * criteria. returned string must be freed afterwards */
extern char *msg_terminate (char *msg, int size);

#endif /* __RCON_MSG_H__ */
