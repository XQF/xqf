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

#ifndef __FILTER_EVAL_H__
#define __FILTER_EVAL_H__

/*
 * Pure server-filter evaluation logic, split out of filter.c so it can be
 * linked into a test binary (tests/test_filter.c) without also pulling in
 * filter.c's GTK dialog code.
 */

#include "filter.h"

extern struct server_filter_vars *server_filter_vars_new (void);
extern void                       server_filter_vars_free (struct server_filter_vars *v);
extern struct server_filter_vars *server_filter_vars_copy (struct server_filter_vars *v);

extern int server_pass_filter (struct server *s);
extern int quick_filter (struct server *s);

#endif /* __FILTER_EVAL_H__ */
