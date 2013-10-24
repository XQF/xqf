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

#ifndef __DNS_H__
#define __DNS_H__


#define DNS_MSG_PREFIX		"DNS:"
#define DNS_MSG_NOTFOUND	"NOTFOUND"
#define DNS_MSG_TIMEOUT		"TIMEOUT"
#define DNS_MSG_ERROR		"ERROR"

#define DNS_CMD_RESET		"/RESET"

enum dns_status {
  DNS_STATUS_OK = 0,
  DNS_STATUS_TIMEOUT,
  DNS_STATUS_NOTFOUND,
  DNS_STATUS_ERROR
};

extern	int str_is_ip_address (char *str);

extern	int dns_spawn_helper (void);
extern	void dns_gtk_init (void);
extern	void dns_helper_shutdown (void);
extern	void dns_cancel_requests (void);
extern	void dns_lookup (const char *str);
extern	void dns_set_callback (void (*cb) (char *item, struct host *h, 
                                           enum dns_status status, void *data),
                                                                   void *data);
extern	char *dns_lookup_by_addr (char *ip);
extern	char *dns_lookup_by_name (char *name);


#endif /* __DNS_H__ */
