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

#ifndef __SOURCE_H__
#define __SOURCE_H__

#include "xqf.h"


#define	FILENAME_FAVORITES	"favorites"
#define	FILENAME_LISTS		"lists"
#define	FILENAME_SRVINFO	"srvinfo"

//#define	PREFIX_MASTER		"master://"
//#define	PREFIX_GMASTER		"gmaster://"
//#define	PREFIX_URL_HTTP		"http://"

#define	ACTION_ADD		"ADD"
#define	ACTION_DELETE		"DELETE"

extern char* master_prefixes[MASTER_NUM_QUERY_TYPES];
extern char* master_designation[MASTER_NUM_QUERY_TYPES];

extern	struct master *favorites;
extern	GSList *master_groups;

extern	struct master *add_master (char *path,
				   char *name, 
				   enum server_type type, 
				   const char* qstat_query_arg,
				   int user, 
				   int lookup_only);

extern	void free_master (struct master *m);

extern	void save_favorites (void);

extern	void init_masters (int autoupdate);
extern	void update_master_list_builtin (void);
extern	void update_master_gslist_builtin (void);

extern	void free_masters (void);

extern	void collate_server_lists (GSList *masters, GSList **servers, 
                                                           GSList **uservers);
extern	void master_selection_to_lists (GSList *list, GSList **masters, 
                                         GSList **servers, GSList **uservers);
extern	int source_has_masters_to_update (GSList *source);
extern	int source_has_masters_to_delete (GSList *source);
extern	GSList *references_to_server (struct server *s);


extern enum master_query_type get_master_query_type_from_address(char* address);


/** \brief check whether any gslist master is configured */
extern gboolean have_gslist_masters();

/** \brief check whether the gslist program is in $PATH */
extern gboolean have_gslist_installed();

void master_remove_server(struct master* m, struct server* s);
void server_remove_from_all(struct server *s);

#endif /* __SOURCE_H__ */


