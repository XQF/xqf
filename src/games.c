// DO NOT EDIT THIS FILE, AUTOMATICALLY GENERATED
static char* stringlist000[] = { "id1", NULL };
static char* stringlist001[] = { "twilight-nq", "nq-sgl", "nq-glx", "nq-sdl", "nq-x11", NULL };
static char* stringlist002[] = { "id1", "qw", NULL };
static char* stringlist003[] = { "twilight-qw", "qw-client-sgl", "qw-client-glx", "qw-client-sdl", "qw-client-x11", NULL };
static char* stringlist004[] = { "baseq2", NULL };
static char* stringlist005[] = { "quake2", NULL };
static char* stringlist006[] = { "baseq3", "demoq3", NULL };
static char* stringlist007[] = { "q3", "quake3", NULL };
static char* stringlist008[] = { "wolfmp", "main", "demomain", NULL };
static char* stringlist009[] = { "wolf", NULL };
static char* stringlist010[] = { "etmain", NULL };
static char* stringlist011[] = { "et", NULL };
static char* stringlist012[] = { "base", NULL };
static char* stringlist013[] = { "doom3", NULL };
static char* stringlist014[] = { "BaseEF", NULL };
static char* stringlist015[] = { "sof", NULL };
static char* stringlist016[] = { "main", NULL };
static char* stringlist017[] = { "tribes2", NULL };
static char* stringlist018[] = { "ut", NULL };
static char* stringlist019[] = { "ut2003", "ut2003_demo", NULL };
static char* stringlist020[] = { "ut2004", "ut2004demo", NULL };
static char* stringlist021[] = { "rune", NULL };
static char* stringlist022[] = { "postal2mp", "postal2mpdemo", NULL };
static char* stringlist023[] = { "armyops", NULL };
static char* stringlist024[] = { "descent3", NULL };
static char* stringlist025[] = { "ssamtfe", NULL };
static char* stringlist026[] = { "ssamtse", NULL };
static char* stringlist027[] = { "main", NULL };
static char* stringlist028[] = { "mohaa", NULL };
static char* stringlist029[] = { "main", NULL };
static char* stringlist030[] = { "codmp", NULL };
static char* stringlist031[] = { "savage", NULL };
static char* stringlist032[] = { "base", NULL };
static char* stringlist033[] = { "jamp", NULL };
static char* stringlist034[] = { "netpanzer", NULL };
struct game games[] = {
  {
    type                : Q1_SERVER,
    flags               : GAME_CONNECT | GAME_RECORD | GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    name                : "Quake",
    default_port        : 26000,
    id                  : "QS",
    qstat_str           : "QS",
    qstat_option        : "-qs",
    icon                : "q1.xpm",
    parse_player        : poqs_parse_player,
    parse_server        : quake_parse_server,
    config_is_valid     : quake_config_is_valid,
    write_config        : write_q1_vars,
    exec_client         : q1_exec_generic,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : quake_init_maps,
    has_map             : quake_has_map,
    cmd_or_dir_changed  : q1_cmd_or_dir_changed,
    update_prefs        : q1_update_prefs,
    pd                  : &q1_private,
    main_mod            : stringlist000,
    command             : stringlist001,
  },
  {
    type                : QW_SERVER,
    flags               : GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_RCON
	    | GAME_QUAKE1_PLAYER_COLORS | GAME_QUAKE1_SKIN,
    name                : "QuakeWorld",
    default_port        : 27500,
    default_master_port : 27000,
    id                  : "QWS",
    qstat_str           : "QWS",
    qstat_option        : "-qws",
    qstat_master_option : "-qwm",
    icon                : "q.xpm",
    parse_player        : qw_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : qw_analyze_serverinfo,
    config_is_valid     : quake_config_is_valid,
    write_config        : write_qw_vars,
    exec_client         : qw_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : quake_init_maps,
    has_map             : quake_has_map,
    cmd_or_dir_changed  : qw_cmd_or_dir_changed,
    update_prefs        : qw_update_prefs,
    pd                  : &qw_private,
    main_mod            : stringlist002,
    command             : stringlist003,
  },
  {
    type                : Q2_SERVER,
    flags               : GAME_CONNECT | GAME_RECORD | GAME_SPECTATE | GAME_PASSWORD | GAME_RCON,
    name                : "Quake2",
    default_port        : 27910,
    default_master_port : 27900,
    id                  : "Q2S",
    qstat_str           : "Q2S",
    qstat_option        : "-q2s",
    qstat_master_option : "-q2m",
    icon                : "q2.xpm",
    parse_player        : q2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q2_analyze_serverinfo,
    config_is_valid     : quake_config_is_valid,
    write_config        : write_q2_vars,
    exec_client         : q2_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : quake_init_maps,
    has_map             : quake_has_map,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    cmd_or_dir_changed  : q2_cmd_or_dir_changed,
    update_prefs        : q2_update_prefs,
    pd                  : &q2_private,
    main_mod            : stringlist004,
    command             : stringlist005,
  },
  {
    type                : Q3_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Quake3: Arena",
    default_port        : 27960,
    default_master_port : 27950,
    id                  : "Q3S",
    qstat_str           : "Q3S",
    qstat_option        : "-q3s",
    qstat_master_option : "-q3m",
    icon                : "q3.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : quake3_config_is_valid,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : q3_update_prefs,
    default_home        : "~/.q3a",
    pd                  : &q3_private,
    main_mod            : stringlist006,
    command             : stringlist007,
  },
  {
    type                : WO_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Wolfenstein",
    default_port        : 27960,
    default_master_port : 27950,
    id                  : "WOS",
    qstat_str           : "RWS",
    qstat_option        : "-rws",
    qstat_master_option : "-rwm",
    icon                : "wo.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : wo_update_prefs,
    default_home        : "~/.wolf",
    pd                  : &wolf_private,
    main_mod            : stringlist008,
    command             : stringlist009,
  },
  {
    type                : WOET_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Enemy Territory",
    default_port        : 27960,
    default_master_port : 27950,
    id                  : "WOETS",
    qstat_str           : "WOETS",
    qstat_option        : "-woets",
    qstat_master_option : "-woetm",
    icon                : "et.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : et_update_prefs,
    default_home        : "~/.etwolf",
    pd                  : &wolfet_private,
    main_mod            : stringlist010,
    command             : stringlist011,
  },
  {
    type                : DOOM3_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Doom 3",
    default_port        : 27666,
    default_master_port : 27650,
    id                  : "DM3S",
    qstat_str           : "DM3S",
    qstat_option        : "-dm3s",
    qstat_master_option : "-dm3m",
    icon                : "doom3.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : doom3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : doom3_init_maps,
    has_map             : doom3_has_map,
    get_mapshot         : doom3_get_mapshot,
    arch_identifier     : "si_version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : doom3_update_prefs,
    default_home        : "~/.doom3",
    pd                  : &doom3_private,
    main_mod            : stringlist012,
    command             : stringlist013,
  },
  {
    type                : EF_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Voyager Elite Force",
    default_port        : 27960,
    default_master_port : 27950,
    id                  : "EFS",
    qstat_str           : "Q3S",
    qstat_option        : "-q3s",
    qstat_master_option : "-efm",
    icon                : "ef.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : ef_update_prefs,
    main_mod            : stringlist014,
  },
  {
    type                : H2_SERVER,
    flags               : GAME_CONNECT | GAME_QUAKE1_PLAYER_COLORS,
    name                : "Hexen2",
    default_port        : 26900,
    id                  : "H2S",
    qstat_str           : "H2S",
    qstat_option        : "-h2s",
    icon                : "hex.xpm",
    parse_player        : poqs_parse_player,
    parse_server        : quake_parse_server,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q1_exec_generic,
    save_info           : quake_save_info,
  },
  {
    type                : HW_SERVER,
    flags               : GAME_CONNECT | GAME_QUAKE1_PLAYER_COLORS | GAME_RCON,
    name                : "HexenWorld",
    default_port        : 26950,
    id                  : "HWS",
    qstat_str           : "HWS",
    qstat_option        : "-hws",
    icon                : "hw.xpm",
    parse_player        : qw_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : qw_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q1_exec_generic,
    save_info           : quake_save_info,
  },
  {
    type                : SN_SERVER,
    flags               : GAME_CONNECT | GAME_RCON,
    name                : "Sin",
    default_port        : 22450,
    id                  : "SNS",
    qstat_str           : "SNS",
    qstat_option        : "-sns",
    icon                : "sn.xpm",
    parse_player        : q2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q2_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q2_exec_generic,
    save_info           : quake_save_info,
  },
  {
    type                : HL_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON,
    name                : "Half-Life",
    default_port        : 27015,
    default_master_port : 27010,
    id                  : "HLS",
    qstat_str           : "HLS",
    qstat_option        : "-hls",
    qstat_master_option : "-hlm",
    icon                : "hl.xpm",
    parse_player        : hl_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : hl_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : hl_exec,
    save_info           : quake_save_info,
    init_maps           : quake_init_maps,
    has_map             : quake_has_map,
    arch_identifier     : "sv_os",
    identify_os         : identify_os,
    pd                  : &hl_private,
  },
  {
    type                : HL2_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD,
    name                : "Half-Life 2",
    default_port        : 27015,
    default_master_port : 27011,
    id                  : "HL2S",
    qstat_str           : "HL2S",
    qstat_option        : "-hl2s",
    qstat_master_option : "-stmhl2",
    icon                : "hl2.xpm",
    parse_player        : hl_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : hl_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : hl_exec,
    save_info           : quake_save_info,
    arch_identifier     : "sv_os",
    identify_os         : identify_os,
  },
  {
    type                : KP_SERVER,
    flags               : GAME_CONNECT | GAME_RCON,
    name                : "Kingpin",
    default_port        : 31510,
    id                  : "Q2S:KP",
    qstat_str           : "Q2S",
    qstat_option        : "-q2s",
    icon                : "kp.xpm",
    parse_player        : q2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q2_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q2_exec_generic,
    save_info           : quake_save_info,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
  },
  {
    type                : SFS_SERVER,
    flags               : GAME_CONNECT | GAME_RCON,
    name                : "Soldier of Fortune",
    default_port        : 28910,
    id                  : "SFS",
    qstat_str           : "SFS",
    qstat_option        : "-sfs",
    icon                : "sfs.xpm",
    parse_player        : q2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q2_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q2_exec_generic,
    save_info           : quake_save_info,
    command             : stringlist015,
  },
  {
    type                : SOF2S_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Soldier of Fortune 2",
    default_port        : 20100,
    default_master_port : 20110,
    id                  : "SOF2S",
    qstat_str           : "Q3S",
    qstat_option        : "-q3s",
    qstat_master_option : "-q3m",
    icon                : "sof2s.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    main_mod            : stringlist016,
  },
  {
    type                : T2_SERVER,
    flags               : GAME_CONNECT | GAME_RCON,
    name                : "Tribes 2",
    default_port        : 28000,
    default_master_port : 28000,
    id                  : "T2S",
    qstat_str           : "T2S",
    qstat_option        : "-t2s",
    qstat_master_option : "-t2m",
    icon                : "t2.xpm",
    parse_player        : t2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : t2_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : t2_exec,
    save_info           : quake_save_info,
    arch_identifier     : "linux",
    identify_os         : t2_identify_os,
    update_prefs        : tribes2_update_prefs,
    command             : stringlist017,
  },
  {
    type                : HR_SERVER,
    flags               : GAME_CONNECT | GAME_RCON,
    name                : "Heretic2",
    default_port        : 28910,
    id                  : "Q2S:HR",
    qstat_str           : "Q2S",
    qstat_option        : "-q2s",
    icon                : "hr2.xpm",
    parse_player        : q2_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q2_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q2_exec_generic,
    save_info           : quake_save_info,
  },
  {
    type                : UN_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD,
    name                : "Unreal / UT",
    default_port        : 7777,
    id                  : "UNS",
    qstat_str           : "UNS",
    qstat_option        : "-uns",
    icon                : "un.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    pd                  : &ut_private,
    command             : stringlist018,
  },
  {
    type                : UT2_SERVER,
    flags               : GAME_CONNECT | GAME_SPECTATE | GAME_PASSWORD | GAME_LAUNCH_HOSTPORT,
    name                : "UT 2003",
    default_port        : 7777,
    id                  : "UT2S",
    qstat_str           : "UT2S",
    qstat_option        : "-ut2s",
    icon                : "ut2.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    default_home        : "~/.ut2003",
    pd                  : &ut2_private,
    command             : stringlist019,
  },
  {
    type                : UT2004_SERVER,
    flags               : GAME_CONNECT | GAME_SPECTATE | GAME_PASSWORD | GAME_LAUNCH_HOSTPORT | GAME_MASTER_CDKEY,
    name                : "UT 2004",
    default_port        : 7777,
    default_master_port : 28902,
    id                  : "UT2004S",
    qstat_str           : "UT2S",
    qstat_option        : "-ut2s",
    qstat_master_option : "-ut2004m",
    icon                : "ut2004.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    update_prefs        : ut2004_update_prefs,
    default_home        : "~/.ut2004",
    pd                  : &ut2004_private,
    command             : stringlist020,
  },
  {
    type                : RUNE_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD,
    name                : "Rune",
    default_port        : 7777,
    id                  : "RUNESRV",
    qstat_str           : "UNS",
    qstat_option        : "-uns",
    icon                : "rune.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    pd                  : &rune_private,
    command             : stringlist021,
  },
  {
    type                : POSTAL2_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD,
    name                : "Postal 2",
    default_port        : 7777,
    id                  : "POSTAL2",
    qstat_str           : "UNS",
    qstat_option        : "-uns",
    icon                : "postal2.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    pd                  : &postal2_private,
    command             : stringlist022,
  },
  {
    type                : AAO_SERVER,
    flags               : GAME_CONNECT | GAME_SPECTATE | GAME_PASSWORD | GAME_LAUNCH_HOSTPORT,
    name                : "America's Army",
    default_port        : 1716,
    id                  : "AMS",
    qstat_str           : "AMS",
    qstat_option        : "-ams",
    icon                : "aao.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ut_exec,
    save_info           : quake_save_info,
    init_maps           : unreal_init_maps,
    has_map             : unreal_has_map,
    pd                  : &aao_private,
    command             : stringlist023,
  },
  {
    type                : DESCENT3_SERVER,
    flags               : GAME_CONNECT,
    name                : "Descent3",
    default_port        : 20142,
    id                  : "D3G",
    qstat_str           : "D3G",
    qstat_option        : "-d3g",
    icon                : "descent3.xpm",
    parse_player        : descent3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : descent3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : gamespy_exec,
    save_info           : quake_save_info,
    command             : stringlist024,
  },
  {
    type                : SSAM_SERVER,
    flags               : GAME_CONNECT,
    name                : "Serious Sam",
    default_port        : 25600,
    id                  : "SMS",
    qstat_str           : "SMS",
    qstat_option        : "-sms",
    icon                : "ssam.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ssam_exec,
    save_info           : quake_save_info,
    command             : stringlist025,
  },
  {
    type                : SSAMSE_SERVER,
    flags               : GAME_CONNECT,
    name                : "Serious Sam TSE",
    default_port        : 25600,
    id                  : "SMSSE",
    qstat_str           : "SMS",
    qstat_option        : "-sms",
    icon                : "ssam.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : ssam_exec,
    save_info           : quake_save_info,
    command             : stringlist026,
  },
  {
    type                : MOHAA_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Medal of Honor: Allied Assault",
    default_port        : 12204,
    default_master_port : 27950,
    id                  : "MHS",
    qstat_str           : "MHS",
    qstat_option        : "-mhs",
    qstat_master_option : "-q3m",
    icon                : "mohaa.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    arch_identifier     : "version",
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    default_home        : "~/.mohaa",
    pd                  : &mohaa_private,
    main_mod            : stringlist027,
    command             : stringlist028,
  },
  {
    type                : COD_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Call of Duty",
    default_port        : 28960,
    default_master_port : 20510,
    id                  : "CODS",
    qstat_str           : "CODS",
    qstat_option        : "-cods",
    qstat_master_option : "-codm",
    icon                : "cod.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : cod_update_prefs,
    pd                  : &cod_private,
    main_mod            : stringlist029,
    command             : stringlist030,
  },
  {
    type                : SAS_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_ADMIN,
    name                : "Savage",
    default_port        : 11235,
    id                  : "SAS",
    qstat_str           : "SAS",
    qstat_option        : "-sas",
    icon                : "savage.xpm",
    parse_player        : savage_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : savage_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : savage_exec,
    save_info           : quake_save_info,
    command             : stringlist031,
  },
  {
    type                : BF1942_SERVER,
    flags               : GAME_CONNECT,
    name                : "Battlefield 1942",
    default_port        : 14567,
    id                  : "BF1942",
    qstat_str           : "EYE",
    qstat_option        : "-eye",
    icon                : "bf1942.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : bf1942_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : bf1942_exec,
    save_info           : quake_save_info,
  },
  {
    type                : JK3_SERVER,
    flags               : GAME_CONNECT | GAME_PASSWORD | GAME_RCON | GAME_QUAKE3_MASTERPROTOCOL,
    name                : "Jedi Academy",
    default_port        : 29070,
    default_master_port : 29060,
    id                  : "JK3S",
    qstat_str           : "JK3S",
    qstat_option        : "-jk3s",
    qstat_master_option : "-jk3m",
    icon                : "jk3.xpm",
    parse_player        : q3_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : q3_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : q3_exec,
    custom_cfgs         : quake_custom_cfgs,
    save_info           : quake_save_info,
    init_maps           : q3_init_maps,
    has_map             : quake_has_map,
    get_mapshot         : q3_get_mapshot,
    identify_cpu        : identify_cpu,
    identify_os         : identify_os,
    update_prefs        : jk3_update_prefs,
    pd                  : &jk3_private,
    main_mod            : stringlist032,
    command             : stringlist033,
  },
  {
    type                : NETP_SERVER,
    flags               : GAME_CONNECT,
    name                : "NetPanzer",
    default_port        : 3030,
    default_master_port : 28900,
    id                  : "NETP",
    qstat_str           : "GPS",
    qstat_option        : "-netp",
    qstat_master_option : "-netpm,netpanzer",
    icon                : "netp.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : netpanzer_exec,
    save_info           : quake_save_info,
    command             : stringlist034,
  },
  {
    type                : GPS_SERVER,
    flags               : GAME_CONNECT,
    name                : "Generic Gamespy",
    default_port        : 27888,
    id                  : "GPS",
    qstat_str           : "GPS",
    qstat_option        : "-gps",
    icon                : "gamespy3d.xpm",
    parse_player        : un_parse_player,
    parse_server        : quake_parse_server,
    analyze_serverinfo  : un_analyze_serverinfo,
    config_is_valid     : config_is_valid_generic,
    exec_client         : descent3_exec,
    save_info           : quake_save_info,
  },
  {
    type                : UNKNOWN_SERVER,
    name                : "unknown",
    parse_server        : quake_parse_server,
    config_is_valid     : config_is_valid_generic,
    exec_client         : exec_generic,
    save_info           : quake_save_info,
  },
};
