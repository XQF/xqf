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

#ifndef __FILTER_H__
#define __FILTER_H__

#include <gtk/gtk.h>

#include "xqf.h"

#define FILTERS_TOTAL		2
#define FILTERS_MASK		0x03

#define	FILTER_SERVER_MASK	0x01
#define	FILTER_PLAYER_MASK	0x02

enum filter_num {
  FILTER_SERVER = 0,
  FILTER_PLAYER = 1
};

enum filter_status {
  FILTER_NOT_CHANGED = 0,
  FILTER_CHANGED,
  FILTER_DATA_CHANGED	/* data changed, but no need to re-apply filter */
}; 


struct server_filter_vars {
  int	  filter_retries;
  int	  filter_ping;
  int 	  filter_not_full;
  int 	  filter_not_empty;
  int	  filter_no_cheats;
  int	  filter_no_password;
  char    *filter_name;
  char    *game_contains;
  char    *version_contains;
};


struct filter {
  char *name;
  char *short_name;
  char *short_cfg_name;

  int (*func) (struct server *s, struct server_filter_vars *vars);

  void (*filter_init) (void);
  void (*filter_done) (void);

  unsigned last_changed;
  enum filter_status changed;
};

extern  struct filter filters[];
extern	unsigned char cur_filter;

struct server_filter_vars server_filters[MAX_SERVER_FILTERS];
unsigned int  current_server_filter;
extern unsigned int current_server_filter;


extern	void apply_filters (unsigned mask, struct server *s);
extern	GSList *build_filtered_list (unsigned mask, GSList *servers);

extern	int	filter_retries;
extern	int	filter_ping;
extern	int 	filter_not_full;
extern  int 	filter_not_empty;
extern  int	filter_no_cheats;
extern  int	filter_no_password;

extern	unsigned filter_current_time;

extern	int	filters_cfg_dialog (int page_num);

extern	void	filters_init (void);
extern	void	filters_done (void);



#endif /* __FILTER_H__ */

