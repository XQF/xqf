/* XQF - Quake server browser and launcher
 * generate C code from games.xml
 * Copyright (C) 2003 Ludwig Nussel <l-n@users.sourceforge.net>
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

typedef enum
{
    TAG_start_basic,
    TAG_type = TAG_start_basic,
    TAG_flags,
    TAG_name,
    TAG_default_port,
    TAG_default_master_port,
    TAG_id,
    TAG_qstat_str,
    TAG_qstat_option,
    TAG_qstat_master_option,
    TAG_pix,
    TAG_parse_player,
    TAG_parse_server,
    TAG_analyze_serverinfo,
    TAG_config_is_valid,
    TAG_write_config,
    TAG_exec_client,
    TAG_custom_cfgs,
    TAG_save_info,
    TAG_init_maps,
    TAG_has_map,
    TAG_get_mapshot,
    TAG_arch_identifier,
    TAG_identify_cpu,
    TAG_identify_os,
    TAG_suggest_commands,
    TAG_pd,
    TAG_end_basic = TAG_pd,
    
    TAG_start_multi,
    TAG_data = TAG_start_multi,
    TAG_end_multi = TAG_data,

    TAG_base,

    TAG_invalid,
} GameTag;

static const xmlChar* tagstr[] =
{
    "type",
    "flags",
    "name",
    "default_port",
    "default_master_port",
    "id",
    "qstat_str",
    "qstat_option",
    "qstat_master_option",
    "icon",
    "parse_player",
    "parse_server",
    "analyze_serverinfo",
    "config_is_valid",
    "write_config",
    "exec_client",
    "custom_cfgs",
    "save_info",
    "init_maps",
    "has_map",
    "get_mapshot",
    "arch_identifier",
    "identify_cpu",
    "identify_os",
    "suggest_commands",
    "pd",
    "data",
    "base",
    NULL,
};

struct MultiTag
{
    xmlChar* val;
    struct MultiTag* next;
};

typedef struct _RawGame
{
    struct _RawGame* base;
    xmlChar* basestr;
    xmlChar* basic[TAG_end_basic - TAG_start_basic + 1];
    struct MultiTag* multi[TAG_end_multi - TAG_start_multi];
} RawGame;

typedef struct _GameList
{
    RawGame* game;
    struct _GameList* next;
} GameList;

GameTag getGameTag(const xmlChar* str)
{
    unsigned i;
    GameTag tag = TAG_invalid;

    for ( i = 0; tagstr[i]; ++i)
    {
	if(!xmlStrcmp(str, tagstr[i]))
	{
	    tag = i;
	    break;
	}
    }

    return tag;
}

#define MALLOC(var, type) var = malloc(sizeof(type)); if(!var) abort(); memset(var, 0, sizeof(type));

RawGame* parseGame(xmlDocPtr doc, xmlNodePtr node)
{
    GameTag tag;
    xmlChar* val = NULL;
    RawGame* rawgame;

    MALLOC(rawgame, RawGame);

    for (node = node->xmlChildrenNode; node; node = node->next)
    {
	if(node->type != XML_ELEMENT_NODE) continue;
	
	tag = getGameTag(node->name);

	if(tag == TAG_invalid)
	{
	    fprintf(stderr,"invalid tag: %s\n", node->name); 
	    continue;
	}

	val = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);

	if(tag <= TAG_end_basic )
	{
	    rawgame->basic[tag] = val;
	}
	else if(tag >= TAG_start_multi && tag <= TAG_end_multi )
	{
	    struct MultiTag* mt;
	    MALLOC(mt, struct MultiTag);
	    mt->val = val;
	    mt->next = rawgame->multi[tag];
	    rawgame->multi[tag] = mt;
	}
	else if (tag == TAG_base)
	{
	    rawgame->basestr = val;
	}
    }

    return rawgame;
}

GameList* parseGames(const char* filename)
{
    xmlDocPtr doc;
    xmlNodePtr node;
    GameList* list = NULL;

    doc = xmlParseFile(filename);
    if (doc == NULL) return(NULL);

    node = xmlDocGetRootElement(doc);
    if (node == NULL) {
        fprintf(stderr,"empty document\n");
	xmlFreeDoc(doc);
	return(NULL);
    }

    if (xmlStrcmp(node->name, (const xmlChar *) "games")) {
        fprintf(stderr,"document of the wrong type, root node != games");
	xmlFreeDoc(doc);
	return(NULL);
    }
    
    for (node = node->xmlChildrenNode; node; node = node->next)
    {
	if ((!xmlStrcmp(node->name, (const xmlChar *) "game")))
	{
	    RawGame* rawgame;
	    rawgame = parseGame(doc, node);
	    if(rawgame)
	    {
		GameList* rg = NULL;
		MALLOC(rg, GameList);
		rg->game = rawgame;
		rg->next = list;
		list = rg;
	    }
	}
    }

    return list;
}

void printGame(FILE* f, RawGame* rg, RawGame* template)
{
    GameTag tag;

    fputs("  {\n", f);
    for ( tag = TAG_start_basic; tag <= TAG_end_basic; ++tag)
    {
	xmlChar* val = rg->basic[tag];
	
	if(!val && rg->base)
	    val = rg->base->basic[tag];
	
	if(!val)
	    val = template->basic[tag];
	
	if(!val || !xmlStrcmp(val, "NULL")) continue;

	switch(tag)
	{
	    case TAG_id:
	    case TAG_name:
	    case TAG_qstat_str:
	    case TAG_qstat_option:
	    case TAG_qstat_master_option:
	    case TAG_arch_identifier:
	    case TAG_pix:
	    case TAG_suggest_commands:
		fprintf(f, "    %-20s: \"%s\",\n", tagstr[tag], val);
		break;
	    case TAG_pd:
		fprintf(f, "    %-20s: &%s,\n", tagstr[tag], val);
		break;
	    default:
		fprintf(f, "    %-20s: %s,\n", tagstr[tag], val);
		break;
	}
    }

    fputs("  },\n", f);
}

int main (int argc, char* argv[])
{
    GameList* list = NULL;
    GameList* ptr = NULL;
    GameList* next = NULL;
    RawGame* template = NULL;

    FILE* f = stdout;

    if(argc < 2) return 1;

    list = parseGames(argv[1]);

    if(!list) return 2;

    // reverse list and find template
    ptr = list;
    list = NULL;
    for(; ptr; ptr = next)
    {
	next = ptr->next;
	if(!template && !xmlStrcmp(ptr->game->basic[TAG_type], "DEFAULT"))
	{
	    template = ptr->game;
	    // free(ptr);
	    continue;
	}
	ptr->next = list;
	list = ptr;
    }

    // FIXME: this is O(n^2), use hash table
    // fill in base links for derived servers
    for(ptr = list; ptr; ptr = ptr->next)
    {
	GameList* l = NULL;
	if(!ptr->game->basestr) continue;
	for(l = list; l; l = l->next)
	{
	    if(!xmlStrcmp(l->game->basic[TAG_type], ptr->game->basestr))
	    {
		if(l->game->base)
		    fprintf(stderr,"%s: base server must not base on another one\n", ptr->game->basic[TAG_type]);
		ptr->game->base = l->game;
		xmlFree(ptr->game->basestr);
		break;
	    }
	}
    }

    fputs("// DO NOT EDIT THIS FILE, AUTOMATICALLY GENERATED\n", f);
    fputs("struct game games[] = {\n", f);

    for(ptr = list; ptr; ptr = ptr->next)
    {
	printGame(f, ptr->game, template);
    }

    fputs("};\n", f);

    // TODO free
    
    return 0;
}

// vim: sw=4
