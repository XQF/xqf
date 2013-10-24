/* XQF - Quake server browser and launcher
 * Splash screen
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

#ifndef SPLASH_H
#define SPLASH_H

#include <gtk/gtk.h>

#define MAPLOAD_PERCENT .8

void create_splashscreen (void);

void destroy_splashscreen(void);

/* set percentage on splash screen to absolute value, thanks to lopster for
 * this code */
void splash_set_progress(const char* message, guint per);

/* add per to current percentage */
void splash_increase_progress(const char* message, guint per);

#endif
