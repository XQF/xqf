// XQF Game Server Browser
// Copyright (C) 1998-2015 XQF Team - https://xqf.github.io
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

#include <stdio.h>

#include <libintl.h>
#include <locale.h>

#include <glib.h>
#include <gtk/gtk.h>

void reset_main_status_bar (GtkBuilder *builder);
void set_widgets_sensitivity (GtkBuilder *builder);
