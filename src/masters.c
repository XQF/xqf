/* XQF - Quake server browser and launcher Copyright (C) 1998-2000 Roman
 * Pozlevich <roma@botik.ru>
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

static char *builtin_masters_update_info[] = {
	/*
	 * known master servers that does not work anymore
	 *
	 */

	// does not work yet, not added 2019-02-17
	"DELETE Q2S:KP master://master.kingpin.info:27900 kingpin.info", // alias for gsm.qtracker.com on 2019-02-17
	"DELETE Q2S:KP master://master0.kingpin.info:27900 kingpin.info #2", // alias for hypo.hambloch.com on 2019-02-17
	"DELETE Q2S:KP master://master.hambloch.com:27900 hambloch.com", // alias for kp.servegame.com on 2019-02-17

	// use master subdomain instead, replaced 2018-08-13
	"DELETE UNVANQUISHEDS master://unvanquished.net:27950 unvanquished.net",

	// does no longer work, removed 2018-07-23
	"DELETE SAS http://masterserver.savage.s2games.com/gamelist_full.dat s2games.com",

	// uses http list instead, replaced 2018-03-24
	"DELETE AMS,-gsm,armygame gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE BF1942,-gsm,bfield1942 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE MHS,-gsm,mohaa gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE Q2S:KP,-gsm,kingpin gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE RUNESRV,-gsm,rune gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE SMS,-gsm,serioussam gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE SMSSE,-gsm,serioussamse gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE UNS,-gsm,ut gmaster://gsm.qtracker.com:28900 qtracker.com (ut99)",
	"DELETE UNS,-gsm,unreal gmaster://gsm.qtracker.com:28900 qtracker.com (unreal)",
	"DELETE UT2004S,-gsm,ut2004 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE UT2S,-gsm,ut2 gmaster://gsm.qtracker.com:28900 qtracker.com",
	"DELETE SNS,-gsm,sin gmaster://gsm.qtracker.com:28900 qtracker.com",

	// does no longer work, removed 2018-03-24
	"DELETE Q2S master://netdome.biz:27900 netdome.biz",
	"DELETE Q2S master://masterserver.exhale.de:27900 exhale.de", // down since 2011

	// does no longer work, removed 2018-03-21
	"DELETE UNS,-gsm,ut gmaster://master.noccer.de:28900 noccer.de",

	// does no longer work, removed 2018-01-07
	"DELETE WARSOWS master://eu.master.warsow.net:27950 warsow.net",
	"DELETE WARSOWS master://eu.master.warsow.gg:27950 warsow.gg",

	// does no longer work, removed 2015-06-27
	"DELETE NEXUIZS master://ghdigital.com ghdigital.com",
	"DELETE QWS master://192.246.40.37:27000 id Limbo",
	"DELETE QWS master://192.246.40.37:27002 id CTF",
	"DELETE QWS master://192.246.40.37:27003 id Team Fortress",
	"DELETE QWS master://192.246.40.37:27004 id Misc",
	"DELETE QWS master://192.246.40.37:27006 id Deathmatch",
	"DELETE T2S master://198.74.35.11:27999 Master 1",
	"DELETE T2S master://198.74.35.17:27999 Master 2",
	"DELETE T2S master://198.74.35.18:27999 Master 3",
	"DELETE WARSOWS master://ghdigital.com ghdigital.com",
	"DELETE WOS master://wolfmaster.idsoftware.com:27950 wolfmaster.idsoftware.com",
	"DELETE XONOTICS master://ghdigital.com ghdigital.com",

	// does no longer work, removed 2014-11-14
	"DELETE IOURTS master://master2.urbanterror.net:27900 master.urbanterror.net",
	"DELETE IOURTS master://master.urbanterror.net:27900 master.urbanterror.net",

	// does no longer work, removed 2014-11-03
	"DELETE A2S,-stma2s master://69.28.151.178:27011 Valve 1",
	"DELETE A2S,-stma2s master://steam1.steampowered.com:27011 Steam 1",
	"DELETE A2S,-stma2s master://steam2.steampowered.com:27011 Steam 2",
	"DELETE AMS http://simplembs.armygame.com/sparse.txt armygame.com",
	"DELETE EFS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=EF&hostport=1 multiplay.co.uk - Elite Force",
	"DELETE ETQWS http://etqw-ipgetter.demonware.net/ipgetter/ demonware",
	"DELETE HLA2S master://steam1.steampowered.com Steam 1",
	"DELETE HLS,-stm master://steam1.steampowered.com Steam 1",
	"DELETE HLS,-stm master://steam2.steampowered.com Steam 2",
	"DELETE MHS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=MOHAA&hostport=1 multiplay.co.uk - MOHAA",
	"DELETE NETP master://netpanzer.dyndns.org netpanzer.dyndns.org",
	"DELETE Q2S http://www.lithium.com/quake2/gamespy.txt Lithium",
	"DELETE Q2S:KP http://www.ogn.org:6666 OGN",
	"DELETE Q2S master://master.planetgloom.com gloom",
	"DELETE Q2S master://masterserver.exhale.de exhale.de",
	"DELETE SAS http://masterserver.savagedemo.s2games.com/gamelist.dat S2 Games",
	"DELETE SNS http://asp.planetquake.com/sinserverlist/servers.txt PlanetQuake",
	"DELETE UNS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT&hostport=1 multiplay.co.uk - UT",

	// does no longer work, removed 2013-10-27
	"DELETE Q3RALLYS master://master.q3alive.net master.q3alive.net",
	"DELETE Q3RALLYS master://master.q3rally.com master.q3rally.com",
	"DELETE SMOKINGUNSS master://master.q3alive.net master.q3alive.net"
	"DELETE SMOKINGUNSS master://master.q3alive.net master.q3alive.net",
	"DELETE SMOKINGUNSS master://soulserv.net:27950 soulserv.net",

	// not used anymore, removed 2013-10-27
	"DELETE Q3RALLYS master://master.ioquake3.org master.ioquake3.org",
	"DELETE REACTIONS master://master.ioquake3.org master.ioquake3.org",
	"DELETE SMOKINGUNSS master://master.ioquake3.org master.ioquake3.org",

	// does no longer work, removed 2005-09-07
	"DELETE AMS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=AA&hostport=1 multiplay.co.uk - Army Ops",
	"DELETE BF1942 http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=BF1942&hostport=1 multiplay.co.uk - BF1942",
	"DELETE CODS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=COD multiplay.co.uk - Call of Duty",
	"DELETE HLS http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=HLS&hostport=1 multiplay.co.uk - Half-Life",
	"DELETE QS http://ironman.planetquake.com/serversqspy.txt Ironman",
	"DELETE SOF2S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=SOF2&hostport=1 multiplay.co.uk - SOF2",
	"DELETE UT2004S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K4D&hostport=1 multiplay.co.uk - UT2004 Demo",
	"DELETE UT2004S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K4&hostport=1 multiplay.co.uk - UT2004",
	"DELETE UT2S http://tourneys.multiplay.co.uk/public/servers.pl?opt=ListGamespy&event=Online&type=UT2K3&hostport=1 multiplay.co.uk - UT2003",
	"DELETE UT2S http://ut2003master.epicgames.com/serverlist/demo-all.txt Epic Demo",
	"DELETE UT2S http://ut2003master.epicgames.com/serverlist/full-all.txt Epic",

	// does no longer work, removed 2005-02-04
	"DELETE WOS master://wolf.idsoftware.com:27950 id",

	// does no longer work, removed 2004-11-26
	"DELETE UNS gmaster://unreal.epicgames.com:28900 Epic",

	// does no longer work, removed 2004-11-01
	"DELETE HLS master://half-life.east.won.net WON East",
	"DELETE HLS master://half-life.west.won.net WON West",

	// does no longer work, removed 2004-10-09
	"DELETE D3P http://www.gameaholic.com/servers/qspy-descent3 gameaholic.com",
	"DELETE D3P master://gt.pxo.net:3445 PXO",

	// does no longer work, removed 2004-09-26
	"DELETE Q2S master://master.quake.inet.fi:27900 iNET (Finland)",
	"DELETE Q2S master://q2master.gxp.de:27900 gXp (Germany)",
	"DELETE Q2S master://q2master.mondial.net.au:27900 Australia",
	"DELETE Q2S master://q2master.planetquake.com:27900 PlanetQuake",
	"DELETE Q2S master://qwmaster.barrysworld.com:27900 BarrysWorld (UK)",
	"DELETE Q2S master://satan.idsoftware.com:27900 id",
	"DELETE Q2S master://telefragged.com:27900 TeleFragged",
	"DELETE Q3S master://q3master.barrysworld.com:27950 BarrysWorld",
	"DELETE QWS master://santa.quakeforge.net:27000 QuakeForge",
	"DELETE UNS gmaster://utmaster.barrysworld.com:28909 BarrysWorld",

	// does no longer work, removed 2004-05-16
	"DELETE T2S master://198.74.32.54:27999 Master 1",
	"DELETE T2S master://198.74.32.55:27999 Master 2",
	"DELETE T2S master://211.233.86.203:28002 Master 3",

	// does no longer work, removed 2003-10-18
	"DELETE HWS master://santa.quakeforge.net:26900 QuakeForge",

	// does no longer work, removed 2002-11-05
	"DELETE QWS master://194.251.249.32:27000 iNET (Finland)",
	"DELETE QWS master://200.255.244.2:27000 Brazil",
	"DELETE QWS master://203.55.240.100:27000 Australia",
	"DELETE QWS master://204.182.161.2:27000 PlanetQuake",
	"DELETE QWS master://207.88.6.18:27000 FortressFest TF",
	"DELETE QWS master://master.quakeworld.ru:27000 Russia",
	"DELETE QWS master://qwmaster.barrysworld.com:27000 BarrysWorld (UK)",
	"DELETE QWS master://qwmaster.mondial.net.au:27000 Mondial Aussie",

	// does no longer work, removed 2002-10-23
	"DELETE T2S master://211.233.32.77:28002 Tribes2 Master",

	// does no longer work, removed 2002-01-05
	"DELETE T2S master://198.74.33.29:28002 NA West1",
	"DELETE T2S master://209.67.28.144:28002 NA East",
	"DELETE T2S master://209.67.28..145:27999 NA East",
	"DELETE T2S master://64.94.105.141:27999 NA West",
	"DELETE T2S master://64.94.105.145:28000 NA West",

	// does no longer work, removed 2001-12-27
	"DELETE Q3S master://q3.golsyd.net.au Australia",
	"DELETE Q3S master://q3master.splatterworld.de Germany",

	/*
	 * known master servers that work
	 *
	 */

	// added 2019-02-17
	"ADD Q2S:KP http://kingpin.hambloch.com/gspylitefavors.txt hambloch.com",

	// added 2018-08-13
	"ADD UNVANQUISHEDS master://master.unvanquished.net:27950 unvanquished.net",
	"ADD UNVANQUISHEDS master://master2.unvanquished.net:27950 unvanquished.net #2",

	// added 2018-04-02
	"ADD CODWAWS http://www.qtracker.com/server_list_details.php?game=callofdutyworldatwar qtracker.com",

	// added 2018-03-31
	"ADD AQ2S http://q2servers.com/?g=action&raw=1 q2servers.com",

	// added 2018-03-27
	"ADD QUETOOS master://master.quetoo.org:1996 quetoo.org",

	// added 2018-03-24
	"ADD DDAYS http://q2servers.com/?g=dday&raw=1 q2servers.com",
	"ADD MHS,-gsm,mohaa gmaster://master.x-null.net:28900 x-null.net",
	"ADD Q2S master://master.rlogin.dk:27900 rlogin.dk",
	"ADD Q2S http://q2servers.com/?raw=1 q2servers.com",
	"ADD Q3S master://master.huxxer.de:27950 huxxer.de",
	"ADD AMS|GPS http://www.qtracker.com/server_list_details.php?game=armyoperations qtracker.com",
	"ADD BF1942|GPS http://www.qtracker.com/server_list_details.php?game=battlefield1942 qtracker.com",
	"ADD MHS|GPS http://www.qtracker.com/server_list_details.php?game=medalofhonoralliedassault qtracker.com",
	"ADD Q2S:KP|GPS http://www.qtracker.com/server_list_details.php?game=kingpin qtracker.com",
	"ADD RUNESRV|GPS http://www.qtracker.com/server_list_details.php?game=rune qtracker.com",
	"ADD SMS|GPS http://www.qtracker.com/server_list_details.php?game=serioussam qtracker.com",
	"ADD SMSSE|GPS http://www.qtracker.com/server_list_details.php?game=serioussamse qtracker.com",
	"ADD UNS|GPS http://www.qtracker.com/server_list_details.php?game=unrealtournament qtracker.com (ut99)",
	"ADD UNS|GPS http://www.qtracker.com/server_list_details.php?game=unreal qtracker.com (unreal)",
	"ADD UT2004S|GPS http://www.qtracker.com/server_list_details.php?game=unrealtournament2004 qtracker.com",
	"ADD UT2S|GPS http://www.qtracker.com/server_list_details.php?game=unrealtournament2003 qtracker.com",
	"ADD SNS|GPS http://www.qtracker.com/server_list_details.php?game=sin qtracker.com",

	// added 2018-03-21
	"ADD UNS,-gsm,unreal gmaster://master.hlkclan.net:28900 master.hlkclan.net (unreal)",
	"ADD UNS,-gsm,ut gmaster://master.hypercoop.tk:28900 hypercoop.tk (unreal)",
	"ADD UNS,-gsm,ut gmaster://master.oldunreal.com:28900 oldunreal.com",
	"ADD UNS,-gsm,ut gmaster://master2.oldunreal.com:28900 oldunreal.com #2",
	"ADD UNS,-gsm,ut gmaster://master.newbiesplayground.net:28900 newbiesplayground.net",

	// added 2018-03-18
	"ADD TEES master://master2.teeworlds.com teeworlds.com #2",
	"ADD TEES master://master3.teeworlds.com teeworlds.com #3",

	// added 2017-04-09
	"ADD CODUOS http://www.qtracker.com/server_list_details.php?game=callofdutyunitedoffensive qtracker.com",
	"ADD CODUOS master://codmaster.activision.com activision.com",
	"ADD COD2S http://www.qtracker.com/server_list_details.php?game=callofduty2 qtracker.com",
	"ADD COD2S master://cod2master.activision.com activision.com",
	"ADD COD4S http://www.qtracker.com/server_list_details.php?game=callofduty4 qtracker.com",
	"ADD COD4S master://cod4master.activision.com activision.com",

	// added 2017-03-03
	"ADD WOPS master://master.worldofpadman.com:27955 worldofpadman.com",
	"ADD WOPS master://master.worldofpadman.net:27955 worldofpadman.net",

	// added 2017-02-05
	"ADD JK3S master://master.jkhub.org:29060 jkhub.org",
	"ADD JK2S master://master.jkhub.org:28060 jkhub.org",

	// added 2015-08-21
	"ADD POSTAL2,-gsm,postal2 gmaster://master.333networks.com:28900 333networks.com",
	"ADD RUNESRV,-gsm,rune gmaster://master.333networks.com:28900 333networks.com",
	"ADD UNS,-gsm,ut gmaster://master.333networks.com:28900 333networks.com",
	"ADD UNS,-gsm,ut gmaster://master.errorist.tk:28900 errorist.tk",

	// added 2015-08-19
	"ADD ETLS master://master.etlegacy.com:27950 etlegacy.com",

	// added 2015-07-28
	"ADD EFS master://efmaster.tjps.eu:27953 tjps.eu",

	// added 2015-06-28
	"ADD ALIENARENAS http://www.qtracker.com/server_list_details.php?game=alienarena qtracker.com",
	"ADD AWS http://www.qtracker.com/server_list_details.php?game=quakeworld qtracker.com",
	"ADD CODS http://www.qtracker.com/server_list_details.php?game=callofduty qtracker.com",
	"ADD D3G http://www.qtracker.com/server_list_details.php?game=descent3 qtracker.com",
	"ADD DM3S http://www.qtracker.com/server_list_details.php?game=doom3 qtracker.com",
	"ADD EFS http://www.qtracker.com/server_list_details.php?game=eliteforce qtracker.com",
	"ADD ETQWS http://www.qtracker.com/server_list_details.php?game=enemyterritoryquakewars qtracker.com",
	"ADD ETLS http://www.qtracker.com/server_list_details.php?game=wolfensteinenemyterritory qtracker.com",
	"ADD H2S http://www.qtracker.com/server_list_details.php?game=hexen2 qtracker.com",
	"ADD HLS http://www.qtracker.com/server_list_details.php?game=halflife_won2 qtracker.com",
	"ADD HWS http://www.qtracker.com/server_list_details.php?game=hexenworld qtracker.com",
	"ADD IOURTS http://www.qtracker.com/server_list_details.php?game=urbanterror qtracker.com",
	"ADD JK2S http://www.qtracker.com/server_list_details.php?game=jediknight2 qtracker.com",
	"ADD JK3S http://www.qtracker.com/server_list_details.php?game=jediknightjediacademy qtracker.com",
	"ADD NEXUIZS http://www.qtracker.com/server_list_details.php?game=nexuiz qtracker.com",
	"ADD OPENARENAS http://www.qtracker.com/server_list_details.php?game=openarena qtracker.com",
	"ADD Q2S:HR http://www.qtracker.com/server_list_details.php?game=heretic2 qtracker.com",
	"ADD Q2S http://www.qtracker.com/server_list_details.php?game=quake2 qtracker.com",
	"ADD Q3S http://www.qtracker.com/server_list_details.php?game=quake3 qtracker.com",
	"ADD Q4S http://www.qtracker.com/server_list_details.php?game=quake4 qtracker.com",
	"ADD QS http://www.qtracker.com/server_list_details.php?game=quake qtracker.com",
	"ADD QWS http://www.qtracker.com/server_list_details.php?game=quakeworld qtracker.com",
	"ADD SFS http://www.qtracker.com/server_list_details.php?game=soldieroffortune qtracker.com",
	"ADD SOF2S http://www.qtracker.com/server_list_details.php?game=soldieroffortune2 qtracker.com",
	"ADD T2S http://www.qtracker.com/server_list_details.php?game=tribes2 qtracker.com",
	"ADD TREMFUSIONS http://www.qtracker.com/server_list_details.php?game=tremulous qtracker.com",
	"ADD TREMULOUSS http://www.qtracker.com/server_list_details.php?game=tremulous qtracker.com",
	"ADD WARSOWS http://www.qtracker.com/server_list_details.php?game=warsow qtracker.com",
	"ADD WOETS http://www.qtracker.com/server_list_details.php?game=wolfensteinenemyterritory qtracker.com",
	"ADD WOPS http://www.qtracker.com/server_list_details.php?game=worldofpadman qtracker.com",
	"ADD WOS http://www.qtracker.com/server_list_details.php?game=returntocastlewolfenstein qtracker.com",
	"ADD XONOTICS http://www.qtracker.com/server_list_details.php?game=xonotic qtracker.com",

	// added 2015-06-27
	"ADD H2S http://www.gameaholic.com/servers/qspy-hexen2 gameaholic.com",
	"ADD HWS http://www.gameaholic.com/servers/qspy-hexenworld gameaholic.com",
	"ADD Q2S master://master.quakeservers.net:27900 quakeservers.net",
	"ADD Q3S master://dctalk.no-ip.info:27950 dctalk.no-ip.info",
	"ADD Q3S master://master0.excessiveplus.net:27950 excessiveplus.net",
	"ADD QWS http://www.gameaholic.com/servers/qspy-quakeworld gameaholic.com",
	"ADD QWS master://master.quakeservers.net:27000 quakeservers.net",
	"ADD QWS master://qwmaster.fodquake.net:27000 fodquake.net",
	"ADD T2S http://t2.plugh.us/t2/serverlist.php t2.plugh.us",
	"ADD T2S http://master.tribesnext.com/list tribesnext.com",
	"ADD UNS http://www.gameaholic.com/servers/qspy-unreal gameaholic.com",
	"ADD UNS,-gsm,ut gmaster://unreal.epicgames.com:28900 epicgames.com",
	"ADD WARSOWS master://excalibur.nvg.ntnu.no:27950 nvg.ntnu.no",
	"ADD WOS master://dpmaster.deathmask.net:27950 deathmask.net",
	"ADD WOS master://wolfmaster.s4ndmod.com:27950 s4ndmod.com",

	// added 2015-06-22
	"ADD A2S,-stma2s master://hl2master.steampowered.com:27011 steampowered.com",

	// added 2015-06-22
	"ADD HLS master://master3.won2.steamlessproject.nl:27010 steamlessproject.nl",

	// added 2014-11-14
	"ADD IOURTS master://master2.urbanterror.info:27900 urbanterror.info #2",
	"ADD Q3S master://master3.idsoftware.com:27950 idsoftware.com", // alias for monster.idsoftware.com on 2014-11-14
	"ADD Q3S master://master.quake3arena.com:27950 quake3arena.com",

	// added 2014-11-06
	"ADD Q3S master://master.maverickservers.com:27950 maverickservers.com",

	// added 2014-10-30
	"ADD IOURTS master://master.urbanterror.info:27900 urbanterror.info",

	// added 2014-10-23
	"ADD JK2S master://masterjk2.ravensoft.com:28060 ravensoft.com",
	"ADD JK2S master://master.ouned.de:28060 ouned.de",

	// added 2014-10-14
	"ADD TURTLEARENAS master://dpmaster.deathmask.net:27950 deathmask.net",
	"ADD TURTLEARENAS master://master.ioquake3.org:27950 ioquake3.org",

	// added 2014-09-30
	"ADD ETLS master://etmaster.idsoftware.com:27950 idsoftware.com",

	// added 2014-09-28
	"ADD TREMFUSIONS master://master.tremulous.net:30710 tremulous.net",
	"ADD TREMULOUSGPPS master://master.tremulous.net:30700 tremulous.net",
	"ADD TREMULOUSS master://master.tremulous.net:30710 tremulous.net",

	// added 2013-11-10
	"ADD WOPS master://master.worldofpadman.com:27955 worldofpadman.com",

	// added 2013-10-31
	"ADD XONOTICS master://dpmaster.tchr.no tchr.no",

	// added 2013-10-27
	"ADD ALIENARENAS master://master2.corservers.com:27900 corservers.com #2",
	"ADD ALIENARENAS master://master.corservers.com:27900 corservers.com",
	"ADD OPENARENAS master://master.ioquake3.org:27950 ioquake3.org",
	"ADD Q3S master://master.ioquake3.org:27950 ioquake3.org",
	"ADD REACTIONS master://master.rq3.com:27950 rq3.com",

	// added 2013-10-26
	"ADD SMOKINGUNSS master://master.smokin-guns.org:27950 smokin-guns.org",
	"ADD SMOKINGUNSS master://parttimegeeks.net:27950 parttimegeeks.net",

	// added 2013-10-25
	"ADD XONOTICS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2013-10-10
	"ADD ZEQ2LITES master://master.ioquake3.org:27950 ioquake3.org",

	// added 2008-09-13
	"ADD OPENARENAS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2008-01-15
	"ADD OTTDS master://master.openttd.org openttd.org",

	// added 2007-01-15
	"ADD Q3S master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-11-01
	"ADD WARSOWS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-10-07
	"ADD Q4S master://q4master.idsoftware.com idsoftware.com",

	// added 2005-06-04
	"ADD NEXUIZS master://dpmaster.deathmask.net:27950 deathmask.net",

	// added 2005-03-24
	"ADD UT2004S master://ut2004master2.epicgames.com:28902 epicgames.com #2",

	// added 2004-10-17
	"ADD UT2004S master://ut2004master1.epicgames.com:28902 epicgames.com",

	// added 2004-10-14
	"ADD D3G http://d3.descent.cx/d3cxraw.d3?terse=y descent.cx",

	// added 2004-08-07
	"ADD DM3S master://idnet.ua-corp.com:27650 ua-corp.com",

	// added 2004-06-19
	"ADD JK3S master://masterjk3.ravensoft.com:29060 ravensoft.com",

	// added 2003-11-18
	"ADD CODS master://cod01.activision.com activision.com",

	// added 2003-04-24
	"ADD WOETS master://etmaster.idsoftware.com:27950 idsoftware.com",

	// added 2002-11-05
	"ADD QWS master://qwmaster.ocrana.de:27000 ocrana.de",

	// added 2002-09-29
	"ADD SOF2S master://master.sof2.ravensoft.com:20110 ravensoft.com",

	// added 2002-05-20
	"ADD EFS master://master.stef1.ravensoft.com:27953 ravensoft.com",

	// added 2001-12-27
	"ADD EFS http://www.gameaholic.com/servers/qspy-startrekeliteforce gameaholic.com",
	"ADD Q3S http://www.gameaholic.com/servers/qspy-quake3 gameaholic.com",

	// added 2001-10-12
	"ADD RUNESRV http://www.gameaholic.com/servers/qspy-rune gameaholic.com",

	// added 2001-09-28
	"ADD SFS http://www.gameaholic.com/servers/qspy-soldieroffortune gameaholic.com",

	// added 2000-11-05
	"ADD Q2S:HR http://www.gameaholic.com/servers/qspy-heretic2 gameaholic.com",
	"ADD Q2S http://www.gameaholic.com/servers/qspy-quake2 gameaholic.com",
	"ADD Q2S:KP http://www.gameaholic.com/servers/qspy-kingpin gameaholic.com",
	"ADD QS http://www.gameaholic.com/servers/qspy-quake gameaholic.com",
	"ADD SNS http://www.gameaholic.com/servers/qspy-sin gameaholic.com",

	// lan servers
	"ADD ALIENARENAS lan://255.255.255.255 LAN",
	"ADD AMS lan://255.255.255.255 LAN",
	"ADD AQ2S lan://255.255.255.255 LAN",
	"ADD CODS lan://255.255.255.255 LAN",
	"ADD CODUOS lan://255.255.255.255 LAN",
	"ADD COD2S lan://255.255.255.255 LAN",
	"ADD COD4S lan://255.255.255.255 LAN",
	"ADD CODWAWS lan://255.255.255.255 LAN",
	"ADD DDAYS lan://255.255.255.255 LAN",
	"ADD DM3S lan://255.255.255.255 LAN",
	"ADD EFS lan://255.255.255.255 LAN",
	"ADD ETLS lan://255.255.255.255 LAN",
	"ADD ETQWS lan://255.255.255.255 LAN",
	"ADD HLS lan://255.255.255.255 LAN",
	"ADD IOURTS lan://255.255.255.255:50724 LAN",
	"ADD JK2S lan://255.255.255.255 LAN",
	"ADD JK3S lan://255.255.255.255 LAN",
	"ADD MHS lan://255.255.255.255 LAN",
	"ADD NEXUIZS lan://255.255.255.255 LAN",
	"ADD OPENARENAS lan://255.255.255.255 LAN",
	"ADD OTTDS lan://255.255.255.255 LAN",
	"ADD POSTAL2 lan://255.255.255.255 LAN",
	"ADD Q2S lan://255.255.255.255 LAN",
	"ADD Q3RALLYS lan://255.255.255.255 LAN",
	"ADD Q3S lan://255.255.255.255 LAN",
	"ADD Q4S lan://255.255.255.255 LAN",
	"ADD QUETOOS lan://255.255.255.255 LAN",
	"ADD QS lan://255.255.255.255 LAN",
	"ADD QWS lan://255.255.255.255 LAN",
	"ADD REACTIONS lan://255.255.255.255 LAN",
	"ADD RUNESRV lan://255.255.255.255 LAN",
	"ADD SFS lan://255.255.255.255 LAN",
	"ADD SMOKINGUNSS lan://255.255.255.255 LAN",
	"ADD TEES lan://255.255.255.255 LAN",
	"ADD T2S lan://255.255.255.255 LAN",
	"ADD TREMFUSIONS lan://255.255.255.255 LAN",
	"ADD TREMULOUSGPPS lan://255.255.255.255 LAN",
	"ADD TREMULOUSS lan://255.255.255.255 LAN",
	"ADD TURTLEARENAS lan://255.255.255.255 LAN",
	"ADD UNS lan://255.255.255.255 LAN",
	"ADD UNVANQUISHEDS lan://255.255.255.255 LAN",
	"ADD UT2004S lan://255.255.255.255 LAN",
	"ADD UT2S lan://255.255.255.255 LAN",
	"ADD WARSOWS lan://255.255.255.255 LAN",
	"ADD WOETS lan://255.255.255.255 LAN",
	"ADD WOPS lan://255.255.255.255 LAN",
	"ADD WOS lan://255.255.255.255 LAN",
	"ADD XONOTICS lan://255.255.255.255 LAN",
	"ADD ZEQ2LITES lan://255.255.255.255 LAN",

	NULL
};

static char *builtin_gslist_masters_update_info[] = {
	// GameSpy was shutdown, removed 2015-06-22
	"DELETE QS gslist://master.gamespy.com;gsmtype=quake1 Gslist",
	"DELETE QWS gslist://master.gamespy.com;gsmtype=quakeworld Gslist",
	"DELETE Q2S gslist://master.gamespy.com;gsmtype=quake2 Gslist",
	"DELETE Q3S gslist://master.gamespy.com;gsmtype=quake3 Gslist",
	"DELETE Q4S gslist://master.gamespy.com;gsmtype=quake4 Gslist",
	"DELETE WOS gslist://master.gamespy.com;gsmtype=rtcw Gslist",
	"DELETE WOETS gslist://master.gamespy.com;gsmtype=rtcwet Gslist",
	"DELETE DM3S gslist://master.gamespy.com;gsmtype=doom3 Gslist",
	"DELETE RUNESRV gslist://master.gamespy.com;portadjust=-1;gsmtype=rune Gslist",
	"DELETE UNS gslist://master.gamespy.com;portadjust=-1;gsmtype=ut Gslist",
	"DELETE UT2004S gslist://master.gamespy.com;portadjust=-10;gsmtype=ut2004 Gslist",
	"DELETE UT2004S gslist://master.gamespy.com;portadjust=-10;gsmtype=ut2004d Gslist (Demo)",
	"DELETE POSTAL2 gslist://master.gamespy.com;portadjust=-1;gsmtype=postal2 Gslist",
	"DELETE POSTAL2 gslist://master.gamespy.com;portadjust=-1;gsmtype=postal2d Gslist (Demo)",
	"DELETE AMS gslist://master.gamespy.com;portadjust=-1;gsmtype=armygame Gslist",
	"DELETE D3G gslist://master.gamespy.com;gsmtype=descent3 Gslist",
	"DELETE SOF2S gslist://master.gamespy.com;gsmtype=sof2 Gslist",

	// no fixed port offset
	// "ADD GPS gslist://master.gamespy.com;gsmtype=mohaa Gslist",

	// not compatible with linux version
	// "ADD SMS gslist://master.gamespy.com;portadjust=-1;gsmtype=serioussam Gslist",
	// "ADD SMSSE gslist://master.gamespy.com;portadjust=-1;gsmtype=serioussamse Gslist",

	NULL
};
