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
    TAG_default_home,
    TAG_pd,
    TAG_end_basic = TAG_pd,
    
    TAG_start_multi,
    TAG_data = TAG_start_multi,
    TAG_main_mods,
    TAG_end_multi = TAG_main_mods,

    TAG_base,

    TAG_invalid,
} GameTag;

typedef struct
{
    unsigned inherit: 1;
    const xmlChar* name;

} Tag;

static const Tag tags[] =
{
    { 0, "type" },
    { 1, "flags" },
    { 0, "name" },
    { 1, "default_port" },
    { 1, "default_master_port" },
    { 0, "id" },
    { 1, "qstat_str" },
    { 1, "qstat_option" },
    { 1, "qstat_master_option" },
    { 1, "icon" },
    { 1, "parse_player" },
    { 1, "parse_server" },
    { 1, "analyze_serverinfo" },
    { 1, "config_is_valid" },
    { 1, "write_config" },
    { 1, "exec_client" },
    { 1, "custom_cfgs" },
    { 1, "save_info" },
    { 1, "init_maps" },
    { 1, "has_map" },
    { 1, "get_mapshot" },
    { 1, "arch_identifier" },
    { 1, "identify_cpu" },
    { 1, "identify_os" },
    { 0, "suggest_commands" },
    { 0, "default_home" },
    { 0, "pd" },
    { 0, "data" },
    { 0, "main_mods" },
    { 0, "base" },
    { 0, NULL },
};

const xmlChar* tagStr(GameTag tag)
{
    if(tag < TAG_start_basic || tag >= TAG_invalid)
	return NULL;

    return tags[tag].name;
}

unsigned tagInherit(GameTag tag)
{
    if(tag < TAG_start_basic || tag >= TAG_invalid)
	return 0;

    return tags[tag].inherit;
}

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
    struct MultiTag* multi[TAG_end_multi - TAG_start_multi + 1];
    unsigned num_multitags;
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

    for ( i = 0; tagStr(i); ++i)
    {
	if(!xmlStrcmp(str, tagStr(i)))
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
	    if(val)
	    {
		struct MultiTag* mt;
		MALLOC(mt, struct MultiTag);
		mt->val = val;
		mt->next = rawgame->multi[tag-TAG_start_multi];
		rawgame->multi[tag-TAG_start_multi] = mt;
	    }
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
	
	if(!val && rg->base && tagInherit(tag))
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
	    case TAG_default_home:
		fprintf(f, "    %-20s: \"%s\",\n", tagStr(tag), val);
		break;
	    case TAG_pd:
		fprintf(f, "    %-20s: &%s,\n", tagStr(tag), val);
		break;
	    default:
		fprintf(f, "    %-20s: %s,\n", tagStr(tag), val);
		break;
	}
    }

    for ( tag = TAG_start_multi; tag <= TAG_end_multi; ++tag)
    {
	struct MultiTag* m = rg->multi[tag - TAG_start_multi];
	
	if(!m && rg->base && tagInherit(tag))
	    m = rg->base->multi[tag - TAG_start_multi];
	
	if(!m)
	    m = template->multi[tag - TAG_start_multi];
	
	if(!m || !m->val || !xmlStrcmp(m->val, "NULL")) continue;

	fprintf(f, "    %-20s: stringlist%03u,\n", tagStr(tag), rg->num_multitags++ );
    }

    fputs("  },\n", f);
}

int main (int argc, char* argv[])
{
    GameList* list = NULL;
    GameList* ptr = NULL;
    GameList* next = NULL;
    RawGame* template = NULL;
    unsigned i = 0;

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

    for(ptr = list; ptr; ptr = ptr->next)
    {
	RawGame* rg = ptr->game;
	GameTag tag;

	for ( tag = TAG_start_multi; tag <= TAG_end_multi; ++tag)
	{
	    struct MultiTag* m = rg->multi[tag - TAG_start_multi];
	    
	    if(!m && rg->base)
		m = rg->base->multi[tag - TAG_start_multi];
	    
	    if(!m)
		m = template->multi[tag - TAG_start_multi];
	    
	    if(!m || !m->val || !xmlStrcmp(m->val, "NULL")) continue;

	    fprintf(f, "static char* stringlist%03u[] = {", i);
	    for(; m; m = m->next)
	    {
		fprintf(f, " \"%s\",", m->val);
	    }
	    fputs(" NULL };\n", f);
	    
	    ++i;
	}
    }

    fputs("struct game games[] = {\n", f);

    i = 0;
    for(ptr = list; ptr; ptr = ptr->next)
    {
	RawGame* rg = ptr->game;
	rg->num_multitags = i;
	printGame(f, rg, template);
	i = rg->num_multitags;
    }

    fputs("};\n", f);

    // TODO free
    
    return 0;
}

// vim: sw=4
