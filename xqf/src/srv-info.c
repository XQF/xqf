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

#include <sys/types.h>
#include <stdlib.h>	/* strtoul */
#include <string.h>	/* strcmp */

#include <gtk/gtk.h>

#include "xqf.h"
#include "sort.h"
#include "pixmaps.h"
#include "srv-info.h"


#define	QW_TEAMPLAY_CTF 	11

static char *qw_teamplay_ctf[QW_TEAMPLAY_CTF] = {
  "Health Protect",    		/*      1 */
  "Armor Protect",		/*      2 */
  "Damage to Attacker",		/*      4 */
  "Frag Penalty",		/*      8 */
				            
  "Death Penalty",		/*     16 */
  "Team Color Lock",		/*     32 */
  "Static Teams",		/*     64 */
  "Drop Items",			/*    128 */
				            
  "Capture the Flag",		/*    256 */
  "Custom Capture the Flag",	/*    512 */
  "Team Selection"		/*   1024 */
};


#define	QW_TEAMPLAY_TF 		19

static char *qw_teamplay_fortress[QW_TEAMPLAY_TF] = {
  "Teamplay",    				/*      1 */
  "1/2 damage from direct fire",		/*      2 */
  "No damage from direct fire",			/*      4 */
  "1/2 damage from area-affect weaponry",	/*      8 */
				            
  "No damage from area-affect weaponry",	/*     16 */
  "Advantage for team with less members",	/*     32 */
  "Advantage for team with lower score",	/*     64 */
  "1/2 armor damage from direct fire",		/*    128 */

  "No armor damage from direct fire",		/*    256 */
  "1/2 armor damage from area-affect weapon",	/*    512 */
  "No armor damage from area-affect weaponry",	/*   1024 */
  "1/2 mirror damage from direct fire",		/*   2048 */

  "Full mirror damage from direct fire",	/*   4096 */
  "1/2 mirror damage from area-affect weaponry",/*   8192 */
  "Full mirror damage from area-affect weaponry"/*  16384 */
};


#define Q2_DMFLAGS	20

static char *q2_dmflags[Q2_DMFLAGS] = {
  "No Health",                  /*      1 */
  "No Powerups",		/*      2 */
  "Weapons Stay",		/*      4 */
  "No Falling Damage",		/*      8 */
				            
  "Instant Powerups",		/*     16 */
  "Same Map",			/*     32 */
  "Teams by Skin",		/*     64 */
  "Teams by Model",		/*    128 */
				            
  "No Friendly Fire",		/*    256 */
  "Spawn Farthest",		/*    512 */
  "Force Respawn",		/*   1024 */
  "No Armor",			/*   2048 */
				            
  "Allow Exit",			/*   4096 */
  "Infinite Ammo",		/*   8192 */
  "Quad Drop",			/*  16384 */
  "Fixed FOV",		 	/*  32768 */

  NULL,				/*  65536 */
  "CTF Forced Join",	   	/* 131072 */
  "Armor Protect",	   	/* 262144 */
  "CTF No Tech Powerups"  	/* 524288 */
};


#define Q2_XATRIX_DMFLAGS	17

static char *q2_xatrix_dmflags[Q2_XATRIX_DMFLAGS] = {
  "No Health",                  /*      1 */
  "No Powerups",		/*      2 */
  "Weapons Stay",		/*      4 */
  "No Falling Damage",		/*      8 */
				            
  "Instant Powerups",		/*     16 */
  "Same Map",			/*     32 */
  "Teams by Skin",		/*     64 */
  "Teams by Model",		/*    128 */
				            
  "No Friendly Fire",		/*    256 */
  "Spawn Farthest",		/*    512 */
  "Force Respawn",		/*   1024 */
  "No Armor",			/*   2048 */
				            
  "Allow Exit",			/*   4096 */
  "Infinite Ammo",		/*   8192 */
  "Quad Drop",			/*  16384 */
  "Fixed FOV",		 	/*  32768 */

  "DualFire Drop"		/*  65536 */
};


#define Q2_ROGUE_DMFLAGS	21

static char *q2_rogue_dmflags[Q2_ROGUE_DMFLAGS] = {
  "No Health",                  /*      1 */
  "No Powerups",		/*      2 */
  "Weapons Stay",		/*      4 */
  "No Falling Damage",		/*      8 */
				            
  "Instant Powerups",		/*     16 */
  "Same Map",			/*     32 */
  "Teams by Skin",		/*     64 */
  "Teams by Model",		/*    128 */
				            
  "No Friendly Fire",		/*    256 */
  "Spawn Farthest",		/*    512 */
  "Force Respawn",		/*   1024 */
  "No Armor",			/*   2048 */
				            
  "Allow Exit",			/*   4096 */
  "Infinite Ammo",		/*   8192 */
  "Quad Drop",			/*  16384 */
  "Fixed FOV",		 	/*  32768 */

  NULL,				/*  65536 */
  "No Mines",		   	/* 131072 */
  "No Stack Double",	   	/* 262144 */
  "No Nukes",  			/* 524288 */

  "No Spheres" 			/* 1048576 */
};


#define Q2_GXMOD_DMFLAGS	19

static char *q2_gxmod_dmflags[Q2_GXMOD_DMFLAGS] = {
  "No Health",                  /*      1 */
  "No Powerups",		/*      2 */
  "Weapons Stay",		/*      4 */
  "No Falling Damage",		/*      8 */

  "Instant Powerups",		/*     16 */
  "Same Level",			/*     32 */
  "Teams by Skin",		/*     64 */
  "Teams by Model",		/*    128 */

  "No Friendly Fire",		/*    256 */
  "Spawn Farthest",		/*    512 */
  "Force Respawn",		/*   1024 */
  "No Armor",			/*   2048 */

  "Allow Level Exit",		/*   4096 */
  "Infinite Ammo",		/*   8192 */
  "Quad Drop",			/*  16384 */
  "Fixed FOV",		 	/*  32768 */

  "Team DM",			/*  65536 */
  "Pratice Mode",	   	/* 131072 */
  "Armor Protect"	   	/* 262144 */
};


#define Q2_GXMOD_WEAPFLAGS	9

static char *q2_gxmod_weapflags[Q2_GXMOD_WEAPFLAGS] = {
  "Shotgun",     	/*      1 */
  "Super Shotgun",	/*      2 */
  "Machinegun",		/*      4 */
  "Chaingun",		/*      8 */

  "Grenade Launcher",	/*     16 */
  "Rocket Launcher",	/*     32 */
  "Hyperblaster",	/*     64 */
  "Railgun",		/*    128 */

  "BFG"			/*    256 */
};


#define Q2_GXMOD_POWERUPFLAGS	11

static char *q2_gxmod_powerupflags[Q2_GXMOD_POWERUPFLAGS] = {
  "Body Armor",         /*      1 */
  "Combat Armor",	/*      2 */
  "Jacket Armor",	/*      4 */
  "Armor Shard",	/*      8 */

  "Power Shield",	/*     16 */
  "Quad Damage",	/*     32 */
  "Mega Health",	/*     64 */
  "Large Health",	/*    128 */

  "Health",		/*    256 */
  "Small Health",	/*    512 */
  "Invincibility"	/*   1024 */
};


#define Q2_LMCTF_CTFFLAGS	15

static char *q2_lmctf_ctfflags[Q2_LMCTF_CTFFLAGS] = {
  "Weapon Balancing",		/*      1 */
  "Allow Invulnerability",	/*      2 */
  "Reset Teams Every Level",    /*      4 */
  "No Team Switch",		/*      8 */

  "Offhand Hook",		/*     16 */
  "No Voices",             	/*     32 */
  "No Damage From Grapple",     /*     64 */
  "No Teams (Deathmatch)",     	/*    128 */

  "No Flags (Deathmatch)",	/*    256 */
  "Score Balance",		/*    512 */
  "Armor Protection",		/*   1024 */
  "Power Armor Strength",	/*   2048 */

  "Random Map",			/*   4096 */
  "Random Quad Respawn Time",	/*   8192 */
  "Random Death Messages"	/*  16386 */
};


#define Q2_LMCTF_RUNES		4

static char *q2_lmctf_runes[Q2_LMCTF_RUNES] = {
  "Damage Powerup",		/*      1 */
  "Resist Powerup",	  	/*      2 */
  "Haste Powerup",    		/*      4 */
  "Regen Powerup"	  	/*      8 */
};


#define Q2_EXPERT_EXPFLAGS	18

static char *q2_expert_expflags[Q2_EXPERT_EXPFLAGS] = {
  "Expert Weapons",		/*      1 */
  "Balanced Items",		/*      2 */
  "Free Gear",			/*      4 */
  "Expert Powerups",		/*      8 */

  "No Powerups",		/*      16 */
  "Expert Hook",		/*      32 */
  "No Hacks",			/*      64 */
  "Player ID",			/*      128 */

  "Enforced Teams",		/*      256 */
  "Fair Teams",			/*      512 */
  "No Team Switch",		/*     1024 */
  "Pogo",			/*     2048 */

  "Slow Hook",			/*     4096 */
  "Sky Solid",			/*     8192 */
  "No Plats",			/*    16384 */
  "Team Distribution",		/*    32768 */

  "Alternate Restore"		/*    65536 */
  "Ammo Regen"			/*   131072 */
};


#define Q2_WF_WFFLAGS	15

static char *q2_wf_wfflags[Q2_WF_WFFLAGS] = {
  "Friendly Fire",		/*      1 */
  "Std logging",		/*      2 */
  "No Fort Respawn",		/*      4 */
  "No Homing Rockets",		/*      8 */

  "No Flying",			/*      16 */
  "Decoy Pursue",		/*      32 */
  "No Railgun Effect (unused)",	/*      64 */
  "No Turrets",			/*      128 */

  "No Earthquake Grenades",	/*      256 */
  NULL,				/*      512 */
  "Next Map Voting",		/*     1024 */
  NULL,				/*     2048 */

  "CTF Mode",			/*     4096 */
  "No Player Classes",		/*     8192 */
  "ZBot Detection"		/*    16384 */
};


#define Q3_DMFLAGS	16

static char *q3_dmflags[Q3_DMFLAGS] = {
  "No Health",                  /*      1 */
  "No Powerups",		/*      2 */
  "Weapons Stay",		/*      4 */
  "No Falling Damage",		/*      8 */
				            
  "Instant Powerups",		/*     16 */
  "Same Map",			/*     32 */
  "Teams by Skin",		/*     64 */
  "Teams by Model",		/*    128 */
				            
  "No Friendly Fire",		/*    256 */
  "Spawn Farthest",		/*    512 */
  "Force Respawn",		/*   1024 */
  "No Armor",			/*   2048 */
				            
  "Allow Exit",			/*   4096 */
  "Infinite Ammo",		/*   8192 */
  "Quad Drop",			/*  16384 */
  "Fixed FOV"		 	/*  32768 */
};


static void show_extended_flags (const char *str, char *names[], int size,
                                          int showall, GtkCTreeNode *parent) {
  unsigned long flags;
  unsigned long mask;
  char *text[2];
  char buf[32];
  int i;

  if (!str || !*str)
    return;

  flags = strtoul (str, NULL, 10);
  if (flags == 0 || flags == ULONG_MAX)
    return;

  mask = 1;

  for (i = 0; flags || (showall && i < size); i++) {
    if ((showall && names[i]) || (flags & mask) != 0) {
      g_snprintf (buf, 32, "%lu", mask);

      text[0] = buf;
      text[1] = (i < size && names[i])? names[i] : "?????";

      gtk_ctree_insert_node (srvinf_ctree, parent, NULL, text, 4,
                      ((flags & mask) != 0)? gplus_pix.pix : rminus_pix.pix,
                      ((flags & mask) != 0)? gplus_pix.mask : rminus_pix.mask,
                      ((flags & mask) != 0)? gplus_pix.pix : rminus_pix.pix,
                      ((flags & mask) != 0)? gplus_pix.mask : rminus_pix.mask,
		      TRUE, FALSE);
    }

    flags &= ~mask;
    mask <<= 1;
  }
}


void srvinf_ctree_set_server (struct server *s) {
  char *text[2];
  char **info;
  GtkCTreeNode *node;

  if (!s || s->info == NULL) {
    gtk_clist_clear (GTK_CLIST (srvinf_ctree));
    return;
  }

  info = s->info;

  gtk_clist_freeze (GTK_CLIST (srvinf_ctree));
  gtk_clist_clear (GTK_CLIST (srvinf_ctree));

  while (info[0]) {
    text[0] = (info[0])? info[0] : NULL;
    text[1] = (info[1])? info[1] : NULL;

    node = gtk_ctree_insert_node (srvinf_ctree, NULL, NULL, text, 4,
                                         NULL, NULL, NULL, NULL, FALSE, TRUE);
    gtk_ctree_node_set_row_data (srvinf_ctree, node, info);

    switch (s->type) {

    case QW_SERVER:
      if (info[0] && !strcmp (info[0], "teamplay")) {
	if (s->game && (!g_strcasecmp (s->game, "ctf") ||
                                       !g_strcasecmp (s->game, "starwars"))) {
	  show_extended_flags (info[1], qw_teamplay_ctf, QW_TEAMPLAY_CTF, 
                                                                 FALSE, node);
	}
	else if (s->game && !g_strcasecmp (s->game, "fortress")) {
	  show_extended_flags (info[1], qw_teamplay_fortress, QW_TEAMPLAY_TF, 
                                                                 FALSE, node);
	}
      }
      break;

    case Q2_SERVER:
      if (info[0] && !strcmp (info[0], "dmflags")) {
	if (s->game && !g_strncasecmp (s->game, "gxmod", 5)) {
	  show_extended_flags (info[1], q2_gxmod_dmflags, Q2_GXMOD_DMFLAGS, 
                                                                 FALSE, node);
	}
	else if (s->game && !g_strncasecmp (s->game, "xatrix", 6)) {
	  show_extended_flags (info[1], q2_xatrix_dmflags, Q2_XATRIX_DMFLAGS, 
                                                                 FALSE, node);
	}
	else if (s->game && !g_strncasecmp (s->game, "rogue", 5)) {
	  show_extended_flags (info[1], q2_rogue_dmflags, Q2_ROGUE_DMFLAGS, 
                                                                 FALSE, node);
	}
	else {
	  show_extended_flags (info[1], q2_dmflags, Q2_DMFLAGS, FALSE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "ctfflags")) {
	if (s->game && !g_strncasecmp (s->game, "lmctf", 5)) {
	  show_extended_flags (info[1], q2_lmctf_ctfflags, Q2_LMCTF_CTFFLAGS, 
                                                                 FALSE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "runes")) {
	if (s->game && !g_strncasecmp (s->game, "lmctf", 5)) {
	  show_extended_flags (info[1], q2_lmctf_runes, Q2_LMCTF_RUNES, 
                                                                  TRUE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "expflags")) {
	if (s->game && !g_strncasecmp (s->game, "expert", 6)) {
	  show_extended_flags (info[1], q2_expert_expflags,
                                             Q2_EXPERT_EXPFLAGS, FALSE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "weapflags")) {
	if (s->game && (!g_strncasecmp (s->game, "gxmod", 5) || 
                                             !g_strcasecmp (s->game, "gx"))) {
	  show_extended_flags (info[1], q2_gxmod_weapflags, 
                                              Q2_GXMOD_WEAPFLAGS, TRUE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "powerupflags")) {
	if (s->game && !g_strncasecmp (s->game, "gxmod", 5)) {
	  show_extended_flags (info[1], q2_gxmod_powerupflags, 
                                           Q2_GXMOD_POWERUPFLAGS, TRUE, node);
	}
      }
      else if (info[0] && !strcmp (info[0], "wfflags")) {
	if (s->game && !g_strcasecmp (s->game, "wf")) {
	  show_extended_flags (info[1], q2_wf_wfflags, Q2_WF_WFFLAGS, 
                                                                 FALSE, node);
	}
      }
      break;

#ifdef QSTAT23
    case Q3_SERVER:
      if (info[0] && !strcmp (info[0], "dmflags")) {
	show_extended_flags (info[1], q3_dmflags, Q3_DMFLAGS, FALSE, node);
      }
      break;
#endif

    default:
      break;
    }

    info += 2;
  }

  gtk_ctree_sort_node (srvinf_ctree, NULL);
  gtk_clist_thaw (GTK_CLIST (srvinf_ctree));
}

