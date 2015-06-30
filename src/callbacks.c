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

void reset_main_status_bar (GtkBuilder *builder) {
	// Reset bottom left status bar to show number of servers
	print_status (GTK_WIDGET (gtk_builder_get_object (builder, "main-status-bar")), ngettext("%d server", "%d servers", server_clist->rows), server_clist->rows);
}

void set_widgets_sensitivity (GtkBuilder *builder) {
	GList *selected = server_clist->selection;
	int sens;
	int i;
	int source_is_favorites;
	int masters_to_update;
	int masters_to_delete;

	// Every button with callback that can modify its sensitivity
	// should be explicitly put to GTK_STATE_NORMAL state before
	// changing its insensitivity

	source_is_favorites = (cur_source != NULL && cur_source->next == NULL && (struct master *) cur_source->data == favorites);

	if (source_is_favorites) {
		masters_to_update = masters_to_delete = FALSE;
	}
	else {
		masters_to_update = source_has_masters_to_update (cur_source);
		masters_to_delete = source_has_masters_to_delete (cur_source);
	}

	sens = (!stat_process && cur_server);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_properties_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_properties_menu_item")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_CONNECT) != 0);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "connect_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_connect_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "connect-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "connect-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_SPECTATE) != 0 && (cur_server->flags & SERVER_SPECTATE) != 0);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "observe_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_observe_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "observe-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "observe-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_RECORD) != 0);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "record_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_record_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "record-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "record-button")), sens);

	sens = (!stat_process && cur_server && (games[cur_server->type].flags & GAME_RCON) != 0);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "rcon_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_rcon_menu_item")), sens);

	sens = (!stat_process && selected);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refrsel_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "view_refrsel_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "refrsel-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refrsel-button")), sens);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "resolve_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_resolve_menu_item")), sens);

	sens = (!stat_process);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "file_statistics_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "add_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_add_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_update_master_builtin_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_update_master_gslist_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_add_master_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_find_player_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_find_again_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "player_filter_menu_item")), sens);
	gtk_widget_set_sensitive (source_ctree, sens);

	sens = (!stat_process && selected && !source_is_favorites);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "favadd_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_favadd_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "server_favadd_menu_item")), sens);

	sens = (!stat_process && masters_to_delete);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_delete_master_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_clear_master_servers_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "source_delete_master_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "source_clear_master_servers_menu_item")), sens);

	// you can only edit one server a time, no groups and no favorites
	sens = (cur_source && cur_source->next == NULL && ! ((struct master *) cur_source->data)->isgroup && ! source_is_favorites);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "source_edit_master_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "edit_edit_master_menu_item")), sens);

	sens = (!stat_process && (server_clist->rows > 0));

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refresh_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "view_refresh_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "refresh-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "refresh-button")), sens);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "view_hostnames_menu_item")), sens);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "view_defport_menu_item")), sens);

	sens = (!stat_process && masters_to_update);

	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "view_update_menu_item")), sens);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "update-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "update-button")), sens);

	sens = (stat_process != NULL);

	gtk_widget_set_state     (GTK_WIDGET (gtk_builder_get_object (builder, "stop-button")), GTK_STATE_NORMAL);
	gtk_widget_set_sensitive (GTK_WIDGET (gtk_builder_get_object (builder, "stop-button")), sens);

	sens = (stat_process == NULL);

	for (i = 0; i < FILTERS_TOTAL; i++) {
		if (!filter_buttons[i]) {
			continue;
		}
		gtk_widget_set_state (filter_buttons[i], GTK_STATE_NORMAL);
		gtk_widget_set_sensitive (filter_buttons[i], sens);
		if (GTK_IS_TOGGLE_BUTTON (filter_buttons[i]) && GTK_TOGGLE_BUTTON (filter_buttons[i])->active) {
			gtk_widget_set_state (filter_buttons[i], GTK_STATE_ACTIVE);
		}
	}
}
