/* XQF - Quake server browser and launcher
 * Functions for plugin script handing
 * Copyright (C) 2005 Ludwig Nussel <l-n@users.sourceforge.net>
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

#include "gnuconfig.h"

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "game.h"
#include "server.h"
#include "xqf.h"
#include "utils.h"
#include "debug.h"
#include "scripts.h"
#include "config.h"
#include "i18n.h"

static unsigned MAX_SCRIPT_VERSION = 1;

static GSList* scriptdirs;
static GList* scripts = NULL;

GData* scriptdata = NULL;

typedef enum {
  ONSTART,
  ONQUIT,
  ONGAMESTART,
  ONGAMEQUIT,
  NUM_ACTIONS
} ScriptActionType;

static const char* action_key[] =
{
  "start",
  "quit",
  "gamestart",
  "gamequit",
  NULL
};

static GSList* action[NUM_ACTIONS];

enum script_option_type {
  SCRIPT_OPTION_TYPE_BOOL,
  SCRIPT_OPTION_TYPE_INT,
  SCRIPT_OPTION_TYPE_STRING,
  SCRIPT_OPTION_TYPE_LIST,
  NUM_SCRIPT_OPTION_TYPES,
  SCRIPT_OPTION_TYPE_INVALID = NUM_SCRIPT_OPTION_TYPES
};

typedef struct
{
  char* name;
  char* summary;
  char* author;
  char* license;
  GSList* options;
} Script;

typedef struct
{
  enum script_option_type type;
  char* name;
  char* section;
  GtkWidget* widget;
  char* defval; // valid for string, int and list
  int enable; // only valid for boolean type
  GPtrArray* list; // list of strings, only valid for list type
} ScriptOption;

static GtkWidget* notebook;

void scripts_add_dir(const char* dir)
{
  scriptdirs = g_slist_prepend(scriptdirs,g_strdup (dir));
}

static char* script_filter(const char* dir, const char* name)
{
    char buf[PATH_MAX];
    struct stat stb;

    snprintf(buf, sizeof(buf), "%s/%s", dir, name);

    if(!strcmp(buf, ".") || !strcmp(buf, ".."))
      return NULL;

    if(stat(buf, &stb) == -1)
      return NULL;

    if(S_ISREG(stb.st_mode) && (stb.st_mode & 0111))
	return strdup(name);

    return NULL;
}

static enum script_option_type scriptoption_get_type(const char* str)
{
  if(!strcmp(str, "string"))
    return SCRIPT_OPTION_TYPE_STRING;
  else if(!strcmp(str, "int"))
    return SCRIPT_OPTION_TYPE_INT;
  else if(!strcmp(str, "bool"))
    return SCRIPT_OPTION_TYPE_BOOL;
  else if(!strcmp(str, "list"))
    return SCRIPT_OPTION_TYPE_LIST;
  else
    return SCRIPT_OPTION_TYPE_INVALID;
}

static ScriptOption* scriptoption_new(const char* typestr)
{
  ScriptOption* opt;
  enum script_option_type type;

  if(!typestr)
    return NULL;
  
  type = scriptoption_get_type(typestr);

  if(type == SCRIPT_OPTION_TYPE_INVALID)
    return NULL;
  
  opt = g_new0(ScriptOption, 1);
  opt->type = type;

  return opt;
}

static void scriptoption_free(ScriptOption* opt)
{
  g_free(opt->name);
  g_free(opt->defval);
  g_free(opt->section);
  g_ptr_array_free(opt->list, TRUE);
  g_free(opt);
}

static Script* script_new()
{
    return g_new0(Script, 1);
}

static void script_free(Script* s)
{
  g_free(s->name);
  g_free(s->summary);
  g_free(s->author);
  g_free(s->license);

  g_slist_foreach(s->options, (GFunc)(scriptoption_free), NULL);

  g_free(s);
}

void scripts_load()
{
  GSList* dir;
  GList* s;
  unsigned i;
  char path[PATH_MAX];

  if(!scriptdata)
    g_datalist_init(&scriptdata);

  /*
  g_datalist_get_data(&scriptdata, "foo");
  g_datalist_set_data_full(&scriptdata,"foo",value,g_free);
  */

  for(dir = scriptdirs; dir; dir = g_slist_next(dir))
  {
    GList* s = dir_to_list(dir->data, script_filter);
    scripts = merge_sorted_string_lists(scripts, s);
  }

  for (s = scripts; s; s = g_list_next(s))
  {
    unsigned version;
    config_section_iterator* sit;
    Script* script;
    const char* filename = s->data;

    // already known?
    if(g_datalist_get_data(&scriptdata, filename))
      continue;

    script = script_new();

    snprintf(path, sizeof(path), "/scripts/%s/General", filename);
    config_push_prefix(path);

    version = config_get_int("xqf version");
    script->summary = config_get_string("summary");
    script->author = config_get_string("author");
    script->license = config_get_string("license");

    config_pop_prefix();

    if(version > MAX_SCRIPT_VERSION)
    {
      xqf_warning("Script %s has version %d, xqf only supports version %d. Script ignored",
	      filename, version, MAX_SCRIPT_VERSION);
      script_free(script);
      continue;
    }

    if(!script->summary)
    {
      xqf_warning("Script %s missing summary. Script ignored", filename, script->summary);
      script_free(script);
      continue;
    }
    if(!script->author)
    {
      xqf_warning("Script %s missing author. Script ignored", filename, script->author);
      script_free(script);
      continue;
    }
    if(!script->license)
    {
      xqf_warning("Script %s missing license. Script ignored", filename, script->license);
      script_free(script);
      continue;
    }

    script->name = g_strdup(filename);

    snprintf(path, sizeof(path), "/scripts/%s/Action", filename);
    config_push_prefix(path);

    for(i = 0; i < NUM_ACTIONS; ++i)
    {
      gboolean on = config_get_bool(action_key[i]);
      if(on)
	action[i] = g_slist_prepend(action[i], script);
    }

    config_pop_prefix();

    // treat script property 'enabled' as option as it has a widget
    // so it's easier to handle later
    {
      ScriptOption* opt;
      snprintf(path, sizeof(path), "/" CONFIG_FILE "/scripts/%s/enabled=false", filename);

      opt = scriptoption_new("bool");
      opt->enable = config_get_bool(path);
      // Translator: whether this plugin script is enabled
      opt->name = _("Enabled");
      opt->section = g_strdup("enabled");
      script->options = g_slist_prepend(script->options, opt);
    }

    snprintf(path, sizeof(path), "/scripts/%s", filename);
    sit = config_init_section_iterator(path);

    while(sit)
    {
      char* sname = NULL;

      sit = config_section_iterator_next(sit, &sname);

      if(strlen(sname) > 7 && !strncmp(sname, "option ", 7))
      {
	char* typestr;
	char* name;
	ScriptOption* opt;
	char settings_path[PATH_MAX];

	snprintf(settings_path, sizeof(settings_path), "/" CONFIG_FILE "/scripts/%s/%s", filename, sname);

	snprintf(path, sizeof(path), "/scripts/%s/%s", filename, sname);
	config_push_prefix(path);

	typestr = config_get_string("type");
	name = config_get_string("name");

	opt = scriptoption_new(typestr);

	g_free(typestr);

	if(!opt || !name)
	{
	  xqf_warning("script %s: invalid option %s", filename, sname+7);
	  goto next;
	}

	opt->name = name;
	opt->section = sname;

	switch(opt->type)
	{
	  case SCRIPT_OPTION_TYPE_LIST:
	    {
	      config_key_iterator* it;

	      it = config_init_iterator(path);

	      if(!opt->list)
		opt->list = g_ptr_array_new();

	      while(it)
	      {
		char* key = NULL;
		char* val = NULL;

		it = config_iterator_next(it, &key, &val);

		if(!strncmp(key, "value",5))
		{
		  g_ptr_array_add(opt->list, val);
		}
		else
		{
		  g_free(val);
		}

		g_free(key);
	      }
	    }
	    // fall through
	  case SCRIPT_OPTION_TYPE_STRING:
	  case SCRIPT_OPTION_TYPE_INT:
	    {
	      char* defval = NULL;
	      char* curval = NULL;

	      defval = config_get_string("default");
	      curval = config_get_string(settings_path);

	      if(curval)
		opt->defval = g_strdup(curval);
	      else if(defval)
		opt->defval = g_strdup(defval);

	      g_free(defval);
	      g_free(curval);
	    }
	    break;
	  case SCRIPT_OPTION_TYPE_BOOL:
	    {
	      gboolean defval;
	      gboolean curval;
	      int is_deflt = 0;

	      defval = config_get_bool("default=false");
	      curval = config_get_bool_with_default(settings_path, &is_deflt);

	      if(is_deflt)
		opt->enable = defval;
	      else
		opt->enable = curval;
	    }
	    break;

	  case SCRIPT_OPTION_TYPE_INVALID:
	    xqf_error("unreachable code");
	    break;
	}

	script->options = g_slist_prepend(script->options, opt);

next:
	config_pop_prefix();
      }
    }

    script->options = g_slist_reverse(script->options);

    g_datalist_set_data_full(&scriptdata, filename, script, (GDestroyNotify)script_free);

    snprintf(path, sizeof(path), "/scripts/%s", filename);
    config_drop_file(path);
  }
}

void scripts_done()
{
  g_datalist_clear(&scriptdata);
}

static GtkWidget* create_script_option_widget(Script* script, ScriptOption* opt)
{
  GtkWidget* ret = NULL;

  opt->widget = NULL;

  switch(opt->type)
  {
    case SCRIPT_OPTION_TYPE_STRING:
    case SCRIPT_OPTION_TYPE_INT:
      {
	GtkWidget* hbox = ret = gtk_hbox_new(FALSE, 0);
	GtkWidget* label = gtk_label_new(opt->name);
	GtkWidget* entry = opt->widget = gtk_entry_new();

	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 4);

	if(opt->defval)
	{
	  gtk_entry_set_text(GTK_ENTRY(entry), opt->defval);
	}

	gtk_widget_show(hbox);
	gtk_widget_show(label);
	gtk_widget_show(entry);
      }
      break;
    case SCRIPT_OPTION_TYPE_BOOL:
      {
	GtkWidget* button = ret = opt->widget = gtk_check_button_new_with_label(opt->name);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(button), opt->enable);

	gtk_widget_show(button);
      }
      break;
    case SCRIPT_OPTION_TYPE_LIST:
      {
	GtkWidget* hbox = ret = gtk_hbox_new(FALSE, 0);
	GtkWidget* label = gtk_label_new(opt->name);
	GtkWidget* combo = gtk_combo_new ();
	GList* list = NULL;
	unsigned i;

	for(i = 0; i < opt->list->len; ++i)
	{
	  list = g_list_append(list, g_ptr_array_index(opt->list, i));
	}

	gtk_combo_set_popdown_strings(GTK_COMBO (combo), list);

	g_list_free(list);
	
	gtk_combo_set_use_arrows_always (GTK_COMBO (combo), TRUE);
	gtk_list_set_selection_mode (GTK_LIST (GTK_COMBO (combo)->list), GTK_SELECTION_BROWSE);

	if(opt->defval)
	{
	  gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (combo)->entry), opt->defval);
	}

	opt->widget = GTK_COMBO(combo)->entry;

	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 4);
	gtk_box_pack_start(GTK_BOX(hbox), combo, FALSE, FALSE, 4);

	gtk_widget_show(combo);
	gtk_widget_show(label);
	gtk_widget_show(hbox);
      }
      break;

    case SCRIPT_OPTION_TYPE_INVALID:
      xqf_error("unreachable code");
      break;
  }

  gtk_widget_ref(opt->widget);

  return ret;
}

void unref_option_widgets(Script* script)
{
  GSList* optlist;

  for(optlist = script->options; optlist; optlist = g_slist_next(optlist))
  {
    ScriptOption* opt = optlist->data;
    gtk_widget_unref(opt->widget);
  }
}

static GtkWidget *generic_script_frame(const char* filename, Script* script) {
  GtkWidget *frame;
  GtkWidget *page_vbox;
  GtkWidget *label;
  GtkWidget *vbox;
  ScriptOption* opt;
  GSList* optlist;

  page_vbox = gtk_vbox_new (FALSE, 4);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);

  {
    GString* s = g_string_new(script->summary);

    g_string_sprintfa(s, "\nAuthor: %s", script->author);
    g_string_sprintfa(s, "\nLicense: %s", script->license);

    label = gtk_label_new (s->str);
    gtk_container_add (GTK_CONTAINER (frame), label);
    gtk_widget_show (label);

    g_string_free(s, TRUE);
  }

  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);


  optlist = script->options;

  // first option is whether script is enabled
  {
    GtkWidget* widget;

    opt=optlist->data;

    widget = create_script_option_widget(script, opt);

    gtk_box_pack_start(GTK_BOX(page_vbox), widget, FALSE, FALSE, 4);

    optlist = g_slist_next(optlist);
  }

  frame = gtk_frame_new (_("Options"));

  vbox = gtk_vbox_new (FALSE, 4);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  for(; optlist; optlist = g_slist_next(optlist))
  {
    GtkWidget* widget;

    opt = optlist->data;

    widget = create_script_option_widget(script, opt);

    gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 4);
  }

  gtk_widget_show (vbox);

  gtk_box_pack_start (GTK_BOX (page_vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_widget_show (page_vbox);

  return page_vbox;
}

static void scripts_page_select_callback(GtkItem *item, gpointer d)
{
  unsigned i = GPOINTER_TO_INT(d);
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), i);
}

GtkWidget *scripts_config_page ()
{
  GtkWidget *page_vbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *gtklist=NULL;
  GtkWidget *scrollwin=NULL;
  GtkWidget *games_hbox;
  unsigned i = 0;
  GList* s;

  page_vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (page_vbox), 8);

  games_hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (games_hbox), 0);
  gtk_box_pack_start (GTK_BOX (page_vbox), games_hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  scrollwin = gtk_scrolled_window_new (NULL, NULL);

  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin),
		  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

  gtklist = gtk_list_new ();

  gtk_widget_set_usize (gtklist, 136, -1);
  
//  gtk_container_add (GTK_CONTAINER (scrollwin), gtklist);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollwin), gtklist);

  gtk_widget_show(gtklist);
  gtk_container_add (GTK_CONTAINER (frame), scrollwin);
  gtk_widget_show(scrollwin);
  gtk_box_pack_start (GTK_BOX (games_hbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  notebook = gtk_notebook_new ();
  // the tabs are hidden, so nobody will notice its a notebook
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_box_pack_start (GTK_BOX (games_hbox), notebook, FALSE, FALSE, 15);

  for (s = scripts; s; s = g_list_next(s), ++i)
  {
    const char* filename = s->data;
    GtkWidget *page;
    GtkWidget* item;
    Script* script;

    script = g_datalist_get_data(&scriptdata, filename);

    if(!script)
    {
      xqf_error("no data for script %s", filename);
      continue;
    }

    item = gtk_list_item_new();
    if(s == scripts)
      gtk_list_item_select(GTK_LIST_ITEM(item));
    
    gtk_container_add (GTK_CONTAINER (item), gtk_label_new(filename));

    gtk_signal_connect (GTK_OBJECT (item), "select",
	   GTK_SIGNAL_FUNC(scripts_page_select_callback), GINT_TO_POINTER(i));

    gtk_widget_show_all(item);
    gtk_container_add (GTK_CONTAINER (gtklist), item);

    label = gtk_label_new (filename);
    gtk_widget_show (label);

    page = generic_script_frame(filename, script);

    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), page, label);
    gtk_object_set_data_full (GTK_OBJECT (notebook), filename, script,
                            (GtkDestroyNotify) unref_option_widgets);
  }

  gtk_widget_show (notebook);

  gtk_widget_show (page_vbox);

  gtk_widget_show (games_hbox);

  return page_vbox;
}

gboolean check_script_prefs()
{
  return TRUE;
}

void save_script_prefs()
{
  GList* s;
  char path[PATH_MAX];
  unsigned i;

  for (s = scripts; s; s = g_list_next(s), ++i)
  {
    Script* script;
    GSList* optlist;

    script = g_datalist_get_data(&scriptdata, s->data);

    if(!script)
    {
      xqf_error("no data for script %s", s->data);
      continue;
    }

    snprintf(path, sizeof(path), "/" CONFIG_FILE "/scripts/%s", (char*)s->data);
    config_push_prefix(path);

    for(optlist = script->options; optlist; optlist = g_slist_next(optlist))
    {
      ScriptOption* opt = optlist->data;
      const char* val = NULL;
      int enable = 0;

      switch(opt->type)
      {
	case SCRIPT_OPTION_TYPE_STRING:
	case SCRIPT_OPTION_TYPE_INT:
	case SCRIPT_OPTION_TYPE_LIST:
	  val = gtk_entry_get_text(GTK_ENTRY(opt->widget));

	  if(!strlen(val))
	    val = NULL;

	  if((!val && opt->defval) || (val && !opt->defval) || (val && opt->defval && strcmp(opt->defval, val)))
	  {
	    g_free(opt->defval);
	    debug(0, "set %s/%s=%s (before: %s)", s->data, opt->section, val, opt->defval);
	    if(opt->type == SCRIPT_OPTION_TYPE_INT)
	    {
	      int tmp = val?atoi(val):0;
	      config_set_int(opt->section, tmp);
	      opt->defval = g_strdup_printf("%d", tmp);
	    }
	    else
	    {
	      config_set_string(opt->section, val?val:"");
	      opt->defval = val?strdup(val):NULL;
	    }
	  }
	  break;
	case SCRIPT_OPTION_TYPE_BOOL:
	  enable = GTK_TOGGLE_BUTTON(opt->widget)->active;
	  if(enable != opt->enable)
	  {
	    config_set_bool(opt->section, enable);
	    debug(0, "set %s/%s=%d", s->data, opt->section, enable);
	    opt->enable = enable;
	  }
	  break;
	case SCRIPT_OPTION_TYPE_INVALID:
	  break;
      }
    }

    config_pop_prefix();
  }
}

struct script_env_callback_data
{
  struct game* g;
  struct server* s;
  Script* script;
};

void script_set_env(struct script_env_callback_data* data)
{
  char buf[256];
  GSList* l;

  server_set_env(data->s);

  for (l = g_slist_next(data->script->options); l; l = g_slist_next(l))
  {
    ScriptOption* opt = l->data;

    debug(0, "%s %s", opt->section, opt->defval);

    snprintf(buf, sizeof(buf), "XQF_SCRIPT_OPTION_%s", opt->section+7);

    switch(opt->type)
    {
	case SCRIPT_OPTION_TYPE_STRING:
	case SCRIPT_OPTION_TYPE_INT:
	case SCRIPT_OPTION_TYPE_LIST:
	  if(opt->defval)
	    setenv(buf, opt->defval, 1);
	  break;
	case SCRIPT_OPTION_TYPE_BOOL:
	  setenv(buf, opt->enable?"true":"false", 1);
	  break;
	case SCRIPT_OPTION_TYPE_INVALID:
	  xqf_error("unreachable code");
	  break;
    }
  }
}

static void run_scripts(ScriptActionType type, struct game* g, struct server* s)
{
  GSList* l;

  debug(0, "%s", action_key[type]);

  for (l = action[type]; l; l = g_slist_next(l))
  {
    char path[PATH_MAX] = "";
    const char* cmd[3];
    Script* script = l->data;
    GSList* dir;

    if(!((ScriptOption*)(script->options->data))->enable)
      continue;

    for(dir = scriptdirs; dir; dir = g_slist_next(dir))
    {
      snprintf(path, sizeof(path), "%s/%s", (char*)dir->data, script->name);
      debug(0, "check %s", path);
      if(access(path, X_OK))
      {
	if(access(path, R_OK) == 0)
	  xqf_warning("%s not executable, not running it", path);
	path[0] = '\0';
      }
      else
	break;
    }

    if(path[0])
    {
      struct script_env_callback_data data;

      cmd[0] = path;
      cmd[1] = action_key[type];
      cmd[2] = NULL;

      data.s = s;
      data.g = g;
      data.script = script;

      debug(0, "running script %s %s", cmd[0], cmd[1]);
      run_program_sync_callback(cmd, script_set_env, &data);
    }
  }
}

void script_action_start()
{
  run_scripts(ONSTART, NULL, NULL);
}

void script_action_quit()
{
  run_scripts(ONQUIT, NULL, NULL);
}

void script_action_gamestart(struct game* g, struct server* s)
{
  run_scripts(ONGAMESTART, g, s);
}

void script_action_gamequit(struct game* g, struct server* s)
{
  run_scripts(ONGAMEQUIT, g, s);
}
