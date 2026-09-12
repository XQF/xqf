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

#include "debug.h"
#include "filter.h"
#include "pref.h"
#include "source.h"
#include "server.h"
#include "srv-list.h"
#include "stat.h"
#include "xqf-ui.h"
#include "xqf-lists.h"

void reset_main_status_bar (GtkBuilder *builder) {
	// Reset bottom left status bar to show number of servers
	int nrows = server_store ? (int) g_list_model_get_n_items (G_LIST_MODEL (server_store)) : 0;
	print_status (GTK_WIDGET (gtk_builder_get_object (builder, "main-status-bar")), ngettext("%d server", "%d servers", nrows), nrows);
}

extern GActionGroup *win_action_group (void);

static void set_action_enabled (const char *name, gboolean enabled) {
	GActionGroup *grp = win_action_group ();
	if (!grp) return;
	GAction *action = g_action_map_lookup_action (G_ACTION_MAP (grp), name);
	if (action)
		g_simple_action_set_enabled (G_SIMPLE_ACTION (action), enabled);
}

void set_widgets_sensitivity (GtkBuilder *builder) {
	gboolean selected = (cur_server != NULL);
	int sens;
	int i;
	int source_is_favorites;
	int masters_to_update;
	int masters_to_delete;

	source_is_favorites = (cur_source != NULL && cur_source->next == NULL && (struct master *) cur_source->data == favorites);

	if (source_is_favorites) {
		masters_to_update = masters_to_delete = FALSE;
	}
	else {
		masters_to_update = source_has_masters_to_update (cur_source);
		masters_to_delete = source_has_masters_to_delete (cur_source);
	}

	sens = (!stat_process && cur_server);

	set_action_enabled ("properties", sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_CONNECT) != 0);

	set_action_enabled ("connect", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "connect-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_SPECTATE) != 0 && (cur_server->flags & SERVER_SPECTATE) != 0);

	set_action_enabled ("observe", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "observe-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_RECORD) != 0);

	set_action_enabled ("record-demo", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "record-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_RCON) != 0);

	set_action_enabled ("rcon", sens);

	sens = (!stat_process && selected);

	set_action_enabled ("refresh-selected", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refrsel-button")), sens);
	set_action_enabled ("dns-lookup", sens);

	sens = (!stat_process);

	set_action_enabled ("statistics", sens);
	set_action_enabled ("add-server", sens);
	set_action_enabled ("add-default-masters", sens);
	set_action_enabled ("add-gslist-masters", sens);
	set_action_enabled ("add-master", sens);
	set_action_enabled ("find-player", sens);
	set_action_enabled ("find-again", sens);
	gtk_widget_set_sensitive (source_treeview, sens);

	sens = (!stat_process && selected && !source_is_favorites);

	set_action_enabled ("add-to-favorites", sens);

	sens = (!stat_process && masters_to_delete);

	set_action_enabled ("delete-master", sens);
	set_action_enabled ("clear-servers", sens);

	// you can only edit one server a time, no groups and no favorites
	sens = (cur_source && cur_source->next == NULL && ! ((struct master *) cur_source->data)->isgroup && ! source_is_favorites);

	set_action_enabled ("rename-master", sens);

	sens = (!stat_process && server_store && g_list_model_get_n_items (G_LIST_MODEL (server_store)) > 0);

	set_action_enabled ("refresh", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refresh-button")), sens);
	set_action_enabled ("show-hostnames", sens);
	set_action_enabled ("show-default-port", sens);

	sens = (!stat_process && masters_to_update);

	set_action_enabled ("update-from-master", sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "update-button")), sens);

	sens = (stat_process != NULL);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "stop-button")), sens);

	sens = (stat_process == NULL);

	for (i = 0; i < FILTERS_TOTAL; i++) {
		if (!filter_buttons[i]) {
			continue;
		}
		gtk_widget_set_sensitive (filter_buttons[i], sens);
	}
}
