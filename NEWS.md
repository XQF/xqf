XQF 1.0.6-current
-----------------

Changes since 1.0.6:

* Add gettext template (online translation tool now hosted by Transifex)
* Add some Wolfenstein: Enemy Territory and Enemy Territory: Legacy mods (like TrueCombat mods)
* Add PO template generation script
* Recognize some OpenArena games hosted with Q3Arena and Q3Arena games hosted by OpenArena
* Update Catalan, French, German and Russian translation
* Update master server list
* Install icons in the correct hicolor per-size paths
* Generate games.c from games.xml at build time
* Move game launch option to appear before exit option
* Filter escape codes in Call Of Duty gametypes
* Fix Q3Rally and Warsow support (status2 packet)
* Fix Wolfenstein: Enemy Territory and Enemy Territory: Legacy empty server query
* Fix Unvanquished, Turtle Arena, Xonotic and Urban Terror LAN server browsing
* Fix gametype strings for a few games
* Fix ioquake3 and iostvef basegame gametype strings
* Fix Quake 3 memory settings
* Fix OpenArena protocol list
* Fix World of Padman gametype names
* Fix gamesxml2c compilation warnings
* Fix automake warnings
* Port some UI part to GtkBuilder, use GtkAboutDialog
* Port build system from autotools to cmake
* Allow out-of-tree compilation
* Clean up massively the source tree
* Remove GLib/GTK individual includes
* Remove i18n.h and use GLib's gi18n.h instead
* Remove GTK+1 remains
* Remove broken GTK+ classes
* Remove XMMS support
* Remove splash screen
* Remove trayicon
* Remove bzip2 support
* Use external minizip library

XQF 1.0.6 -- October 26, 2014
-----------------------------

Changes since 1.0.5:

* New games: Unvanquished, Tremulous GPP, TremFusion, Xonotic, Smokin' Guns, Urban Terror, Alien Arena, Reaction, Q3 Rally, World of Padman, OpenArena, OpenTTD, Enemy Territory: Quake Wars, Enemy Territory: Legacy, ZEQ2 Lite, Jedi Outcast, Turtle Arena
* Add LAN broadcast for many games
* Add or substitute new master servers (like ioquake3 master substitute id master)
* Add Warsow protocol numbers
* Add Hexen World master server support
* Add scalable SVG icon
* Filter Unvanquished, Wolfenstein: Enemy Territory and Savage extended color codes
* Reuse game descriptions for similar games (like Western Q3 and Smokin' Guns, or Wolfenstein: Enemy Territory and Enemy Territory: Legacy).
* Drop disabled master servers (like id master server for Quake 3)
* Drop GTK+1 support
* Enhance Steam applaunch support
* Follow XDG Base Directory Specification for user configuration directory and migrate previously created directory
* Workaround non ASCII characters in servers strings
* Rewrite the master and servers input callbacks with GIOChannel
* Rewrite many deprecated calls (like old strings functions)
* Fix build with newer linker versions
* Fix autotools, intltool
* Fix x11 build dependency
* Fix readline check
* Fix crash with 24 bit images
* Fix the q3_unescape routine, no longer segfaults when a string is terminated by an escape code
* Fix a string table of Quake 2 game properties
* Fix gamesxml2c compilation
* Fix compilation using clang
* Use a new Savage hack to query Savage master server
* Determine number of GeoIP countries at runtime
* Complete French translation

XQF 1.0.5 -- November 04, 2006
------------------------------

Changes since 1.0.4:

* New games: Warsow, Tremulous
* Fix Quake 4 RCON
* Add "Show only configured games" button again
* Fix SOF2 query
* Add new America's Army master server
* Support copying server info values to clipboard
* Fix build with newer GTK2 versions
* Don't pass -steam option to hl2 anymore
* Fix cursor navigation in server list
* Allow to also delete servers that are not in the Favorites list
* Split Half-Life support into old and new version
* New Polish translation

XQF 1.0.4 -- October 20, 2005
-----------------------------

Changes since 1.0.3:

* New games: Nexuiz, Quake 4
* Add Epic's second UT2004 master
* New Finish translation
* Add q3a Excessive Plus gametypes
* Honor setting to ignore bots when copying server info to clipboard
* Add support for plugin scripts
* Support multiple sort modes per column

XQF 1.0.3 -- April 03, 2005
---------------------------

Changes since 1.0.2:

* New games: Half-Life 2 (wine)
* Make all icons themeable
* Use GTK2 colors and Raleigh theme by default in GTK1 version
* Add --nomapscan command line parameter
* Fix ut2k4 and doom3 master query not working after starting XQF
* Fix saving of quake2 passwords
* Fix doom3 protocol detection
* Fix q1/qw/q2 skin list update
* Fix crashes in GTK2 version

XQF 1.0.2 -- December 22, 2004
------------------------------

Changes since 1.0:

* New games: Netpanzer
* Support for Gslist (http://aluigi.altervista.org/papers.htm#gslist)
* Automatic detection of the Doom3 network protocol version
* Check osmask of Doom3 servers and warn if the server has no Linux support
* Configurable qstat source ip and port range for people with broken NAT
* Country statistics for game servers
* Additional Descent3 server listing
* UT2004 Master support
* xqf-rcon uses $XQF_RCON_PASSWORD for the rcon password if set
* Requires qstat 2.7

XQF 1.0 -- August 15, 2004
--------------------------

Changes since 0.9.14:

* New games: Doom3, Jedi Academy (wine), America's Army 2.1
* Redial understands free private slots and won't connect to password protected servers if there is no password defined by the user
* Support for the Half-Life steam master
* Support for America's Army 2.1
* Export the variables XQF_SERVER_NAME, XQF_SERVER_MAP, XQF_SERVER_HOSTNAME and XQF_SERVER_GAME when launching a game
* Custom arguments for RedOrchestra, Troopers, AlienSwarm UT2004 mods
* Animated tray icon for the GTK2 version
* Quake 3 gametypes for World of Padman
* Copy/Copy+ also copies to the CLIPBOARD instead of only PRIMARY to allow paste via CTRL-v
* Quick Filter on main screen to search in every server's name, hostname, map, game, gametype and rule value.
* Requires qstat 2.6

XQF 0.9.14 -- March 21, 2004
----------------------------

Changes since 0.9.13:

* New games: UT2004, Postal2, BF1942(wine). All without official master.
* Improved --launch parameter, now only requires IP address and asks for type if needed. It's possible now to use this together with e.g. XChat.
* Add --add parameter to just add a server to favorites
* Fix LAN browsing
* Add LAN masters by default
* Support password on Savage servers
* New server properties: Comment and "this server sucks"
* Add exec function for hexenworld (anyone ever played that game?)
* Switch to intltool for i18n
* Install desktop files

XQF 0.9.13 -- November 24, 2003
-------------------------------

Changes since 0.9.12:

* New games: America's Army, Savage, Medal of Honor, Call of Duty(wine)
* New splash screen and desktop icons
* GeoIP support allows filtering by country
* Server side filtering for Half-Life
* Updated Enemy Territory default protocol number
* Quake III gametypes for TrueCombat 1.0 and Urban Terror 3
* Additional gametypes for some RTCW and ET mods
* Added custom arguments for Death Ball and FragOps UT2003 mods
* Detection of cheating-death on HalfLife servers
* Added twilight to q1 and qw command suggestion
* Display team of player for RTCW, ET and Q3 mods that provide the necessary information such as OSP and TrueCombat
* Command line option --launch to automatically add a server to favorites, ping it and then launch the game
* The environment variable XQF_SERVER_ANTICHEAT is now set before launching a game when the server requires some anti-cheat software.
* Option to stop current song in XMMS when launching a game
* Experimental GTK2 compilation support
* New French translation

XQF 0.9.12 -- June 10, 2003
---------------------------

Changes since 0.9.11:

* Added Enemy Territory Support support
* Added Serious Sam: The Second Encounter support
* Map scan function for q1, qw, q2 and hl to verify that you have the map installed before connecting to the server
* Ability to see level screenshot when clicking on the map column for jpg shots inside of PK3 files for Quake3 and Wolfenstein
* XQF startup splash screen support using gdk-pixbuf
* Allow filtering for map and server name
* Fix high cpu load when dialogs are shown during launch phase
* If more than 100 servers are to be updated, the screen is not immediately refreshed.  This helps eliminate long delays with Half Life updates
* Pass RCON password on command line when launching Half-Life
* Minor memory leaks fixed
* Added QuakeForge's HexenWorld master
* Added Quake3 Western Q3 game type
* Default custom args for Rocket Arena removed as 1.6 does not need them anymore
* Now displays player team for Wolfenstein and Enemy Territory (skin column)
* With Half Life, private clients now set based on reserve_slots variable
* Display number of private clients in player column
* Option to not count bots as players
* Move server filter submenu to top level and remove rarely used buttons from toolbar
* Added docs/PreLaunch.example
* Hostname resolving now off by default
* Automatically creates qstat config if required
* gdk-pixbuf now required
* New Danish translation
* New French translation

XQF 0.9.11 -- December 19, 2002
-------------------------------

Changes since 0.9.10:

* Serious Sam support (requires SMS gametype via ~/.qstatrc). Does not currently support any masters.
* Fixed -game parameter for Half-Life
* RTCW voteflags decoded in properties pane
* Visual marker in the map column to show if you have the listed map installed on your computer. (Q3, RTCW, UT, Rune, UT2)

XQF 0.9.10 -- November 16, 2002
-------------------------------

Changes since 0.9.9:

* File dialog boxes for adding game command line and directory
* Greatly speed up the startup of XQF when loading large lists
* Greatly speed up response time when applying filters to large lists
* Ability to automatically set cl_punkbuster when connecting to a server in Q3A
* Added Punkbuster icon to Priv column
* Ability to define custom command-line arguments for a game based on the 'game' type
* Q3A now searches for a matching mod directory.  Should correctly launch even if mod directory is incorrect by case
* Added sound disable support for Unreal based games
* Can now hide games that are not configured
* Changed default Quake3 protocol to 68
* Sound support for XQF events using external sound player
* Busy server redial with reserved slots support
* Soldier of Fortune 2 support (requires qstat sof2s gametype via ~/.qstatrc or qstat >2.5b)
* Use correct parameters (-game,+connect,+password) when launching Half-Life
* New master type of "file" to read IP addresses from a file
* Unreal Tournament 2003 support
* Fixed Half-Life rcon support
* Player search visible improvements
* Fixed Tribes2 master support and added additional masters
* Updated QuakeWorld master list
* Unlimited number of server filters instead of ten
* Changed default Wolf protocol to 60
* Standalone rcon program that doesn't need X (xqf-rcon)
* Quake3 launching now uses 'game' instead of 'gamename' to help prevent launch problems due to case
* Various segfault fixes
* XQF now requires qstat 2.5c


XQF 0.9.9 -- July 3, 2002
-------------------------

Changes since 0.9.8:

* Added Voyager Elite Force support
* Changed default Quake3 protocol to 67
* You can now select Quake3 and Wolfenstein's protocol
* Added 20sec timeout for wget (nice if Gameaholic is down)
* Added Tribes2 server statistics
* Fixed Tribes2 and Quake3 masters
* Added support for LAN broadcast queries
* Improved master support handling
* Added preferences tab for Quake3 memory settings
* New Catalan translation


XQF 0.9.8 -- December 17, 2001
------------------------------
Changes since 0.9.7:

* Repackaged with libtool 1.4, so it builds on all Linux architectures
* Fixed trasparency of Gamespy's pixmap
* Don't distribute debian stuff


XQF 0.9.7 -- December 16, 2001
------------------------------

Changes since 0.9.6g:

* Support for games using the GameSpy protocol
* Support for Descent3 with qstat 2.4e (please note Descent can't be launched from within XQF at the moment)
* Support for Rune
* Reorganization of settings dialogs
* New gametypes for Quake3 mods Threewave and TribalCTF
* Support for Wolfenstein retail (protocol 57)
* Server statistics for Wolfenstein, Kingpin and Half-Life
* Works on PowerPC again


XQF 0.9.6g -- September 25, 2001
--------------------------------

Changes since 0.9.6f:

* Internationalization (gettext) support, Spanish and German translations
* Initial Return to Castle Wolfenstein support
* Tribes2 support
* Support for Q3A protocol v66
* Added "Quake3" preferences page which allows the protocol version and other options to be changed
* Added "General" preferences page, which hosts many of the options which were in "Appearance" previously
* Added "game type" filter
* New man page
* New documentation in html format (docs/xqfdocs.html)
* XQF now requires qstat 2.4c


XQF 0.9.6f -- March 23, 2001
----------------------------

Changes since 0.9.5:

* Multiple server filters; Filter name configurable and appears in the status bar
* Lock Icon to show if server is private or not; icon next to number of players turns yellow if all of the public client spaces are full
* Pressing "Insert" brings up the add server dialog; pressing SHIFT+Insert adds the currently selected server to ones favorites
* Added support for new Team Arena Game types
* Protocol 48 (1.27) Q3A servers get queried with protocol 48 in qstat
* Execute "PreLaunch" script when launching game (for use with ICQ scripts, etc.)
* Improved support for Half-Life servers
* Improved support for Unreal Tournament
* Improved support for Quake2
* Hack for supporting multiple Q3A protocols i.e. xqf can run different Q3A's depending on if it is a 1.17 or 1.27 server: see the README file
* Q3A hack for connecting to arena servers so that all of the vm_* settings are correct on the command line
* Resolved one major source of core dumps. It should be much more stable now
