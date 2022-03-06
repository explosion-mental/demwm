/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static unsigned int borderpx  = 4;        /* border pixel of windows */
static unsigned int snap      = 10;       /* snap pixel */
static unsigned int gappih    = 15;       /* horiz inner gap between windows */
static unsigned int gappiv    = 20;       /* vert inner gap between windows */
static unsigned int gappoh    = 15;       /* horiz outer gap between windows and screen edge */
static unsigned int gappov    = 20;       /* vert outer gap between windows and screen edge */
static int hidevacant         = 1;        /* 1 means hide vacant tags */
static int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */
static int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static int showbar            = 1;        /* 0 means no bar */
static int showstatus         = 1;        /* 0 means no status text */
static const int topbar       = 1;        /* 0 means bottom bar */
static const int barh         = 8;        /* 1 or more means bar height */
static const int pertag       = 1;        /* 0 means global layout across all tags (default) */
static const int pertagbar    = 0;        /* 0 means using pertag, but with the same barpos */
static const int gapspertag   = 1;        /* 0 means global gaps across all tags (default) */
static const int scalepreview = 4;        /* tag previews scaling */
/* 1 means resize window by 1 pixel for each scroll event */
#define scrollsensetivity    30
/* bar opacity from 0 to 255, default is 185*/
#define baralpha    120
/* borders, default is 0xffU (OPAQUE) */
#define borderalpha OPAQUE

static const char *fonts[] = {
	"Monofur Nerd Font:size=10:antialias=true:autohint=true", /* Mono */
//	"ETBembo:size=14",
//	"JetBrains Mono Medium:pixelsize=12:antialias=true:autohint=true", /* Mono */
//	"Hack Nerd Font:pixelsize=12:antialias=true:autohint=true", /* Powerline */
//	"Hack Nerd Font:style=bold:pixelsize=12:antialias=true:autohint=true", /* Powerline */
//	"SauceCodePro Nerd Font:pixelsize=14:antialias=true:autohint=true", /* Code */
//	"Noto Color Emoji:pixelsize=16:antialias=true:autohint=true: style=Regular", /* Emojis */
	"JoyPixels:size=10:antialias=true:autohint=true" /* Emojis */
};
/* pywal */
static char color0[8], color1[8], color2[8], color3[8], color4[8], color5[8];
static char color6[8], color7[8], color8[8], bg_wal[8], fg_wal[8], cursor_wal[8];

static const char *colors[][2]	        = {
			/* fg		bg           description         */
	[SchemeNorm]   = { fg_wal,	color0 }, /* normal tags section */
	[SchemeSel]    = { color0,	color1 }, /* selected tag */
	[SchemeUrgent] = { fg_wal,	color0 }, /* urgent tag */
	[SchemeLt]     = { fg_wal,	color0 }, /* layout */
	[SchemeTitle]  = { color0,	color2 }, /* window title */
	[SchemeStatus] = { fg_wal,	color0 }, /* status bar */
	[SchemeDelim]  = { fg_wal,	color0 }, /* delimiter */
	[SchemeSys]    = { color0,	color0 }, /* system tray */
	[SchemeNotify] = { fg_wal,	color0 }, /* bar on urgent tag */
	[SchemeIndOn]  = { color4,	color0 }, /* rectangle on sel tag */
	[SchemeIndOff] = { color2,	color0 }, /* rectangle on norm tag */
	[BorderNorm]   = { color0 },	/* normal window border */
	[BorderSel]    = { color2 },	/* selected window border */
	[BorderUrg]    = { fg_wal },	/* urgent window border */
};
static const unsigned int alphas[][2]   = {
			/* fg		bg	 */
	[SchemeNorm]   = { OPAQUE,	baralpha },
	[SchemeSel]    = { OPAQUE,	baralpha },
	[SchemeUrgent] = { OPAQUE,	baralpha },
	[SchemeLt]     = { OPAQUE,	baralpha },
	[SchemeTitle]  = { OPAQUE,	OPAQUE,	 },
	[SchemeStatus] = { OPAQUE,	baralpha },
	[SchemeDelim]  = { OPAQUE,	baralpha },
	[SchemeSys]    = { OPAQUE,	baralpha },
	[SchemeNotify] = { OPAQUE,	baralpha },
	[SchemeIndOn]  = { OPAQUE,	baralpha },
	[SchemeIndOff] = { OPAQUE,	baralpha },
	[BorderNorm]   = { 0 },
	[BorderSel]    = { borderalpha },
	[BorderUrg]    = { borderalpha },
};

/* status bar */
static const Block blocks[] = {
	/* color  command				interval	signal */
	{ color7, "sb-clock",				20,		1},
	{ color2, "sb-disk",				9000,		2},
	{ color3, "sb-battery",				10,		3},
	{ color4, "sb-internet",			10,		4},
	{ color5, "sb-mailbox",				0,		5},
	{ color6, "sb-moonphase",			18000,		6},
	{ color1, "sb-forecast",			18000,		7},
	{ color8, "sb-volume",				0,		8},
//	{ color2, "sb-price btc Bitcoin 💰",		9000,		21},
//	{ color2, "sb-price eth Ethereum 🍸",		9000,		23},
//	{ color2, "sb-price xmr \"Monero\" 🔒",		9000,		24},
//	{ color2, "sb-price link \"Chainlink\" 🔗",	300,		25},
//	{ color2, "sb-price bat \"Basic Attention Token\" 🦁",9000,	20},
//	{ color2, "sb-price lbc \"LBRY Token\" 📚",	9000,		22},
//	{ color2, "sb-cpu",				10,		18},
//	{ color2, "sb-kbselect",			0,		30},
//	{ color2, "sb-memory",				10,		14},
//	{ color2, "sb-torrent",				20,		7},
//	{ color2, "sb-crypto",				0,		13},
//	{ color2, "sb-help-icon",			0,		15},
//	{ color2, "sb-nettraf",				1,		16},
//	{ color2, "sb-news",				0,		6},
//	{ color2, "sb-xbpsup",				18000,		8},
	{ color1, "sb-pacpackages",			0,		9},
	{ color2, "sb-sync",				0,		10},
//	{ color2, "sb-mpc",				0,		26},
	{ color3, "sb-music",				0,		11},
//	{ color3, "sb-tasks",				10,		12},
	{ color4, "sb-notes",				0,		13},
	{ color5, "cat /tmp/recordingicon 2>/dev/null",	0,		14},
//	{ color6, "sb-count",				0,		21},
};

/* if inversed, inverse the order of the blocks */
#define INVERSED
/* delimeter between blocks commands. NULL character ('\0') means no delimeter. */
static char delimiter[] = " ";
/* max number of character that one block command can output */
#define CMDLENGTH		50

/* tags */
static const char *tags[]     = { "📖", "", "💼", "", "🔬", "🎹", "📺", "💻", "🐧" };
static const int taglayouts[] = {    0,   1,    0,   0,    0,    0,    0,    0,    0 };
static const char *tagsalt[]  = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };

static const Rule rules[] = {
	/* flags: class, instance, title, wintype, role, tags, isfloating,
	 * isterminal, noswallow, isfakefullscreen, monitor
	 *
	 * order is actually important here (e.g. if .noswallow is before
	 * .isterminal then it would not take effect)
	 *
	 * xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 *	_NET_WM_WINDOW_TYPE(ATOM) = wintype
	 */
	RULE(.wintype = "_NET_WM_WINDOW_TYPE" "DIALOG",	.isfloating = 1)
	RULE(.wintype = "_NET_WM_WINDOW_TYPE" "UTILITY",.isfloating = 1)
	RULE(.wintype = "_NET_WM_WINDOW_TYPE" "TOOLBAR",.isfloating = 1)
	RULE(.wintype = "_NET_WM_WINDOW_TYPE" "SPLASH",	.isfloating = 1)
	RULE(.class = "Gimp",		.tags = 1 << 7)
	RULE(.class = "vlc",		.tags = 1 << 6)
	RULE(.class = "qutebrowser",	.tags = 1 << 5)
	RULE(.class = "Surf",		.tags = 1 << 7)
	RULE(.title = "Music",		.tags = 1 << 6)
	RULE(.title = "Sxiv - redyt",	.tags = 1 << 8)
	RULE(.title = "Sxiv - walld",	.tags = 1 << 8)
//	RULE(.class = "libreoffice",	.tags = 1 << 3)
//	RULE(.title = "LibreOffice",	.isfloating = 1, .noswallow = 1)
//	RULE(.class = "libreoffice",	.noswallow = 1)
//	RULE(.class = "Firefox",	.tags = 1 << 1, .isfakefullscreen = 1)
	RULE(.class = "firefox",	.tags = 1 << 1, .isfakefullscreen = 1)
	RULE(.class = "Brave-browser",	.tags = 1 << 4, .isfakefullscreen = 1)
	RULE(.class = "Pavucontrol",	.isfloating = 1)
	RULE(.class = "Pcmanfm",	.isfloating = 1)
//	RULE(.title = "pulsemixer",  .isfloating = 1)
	RULE(.title = "About Mozilla Firefox",	.isfloating = 1)
	RULE(.class = "St", .isterminal = 1)
	RULE(.title = "Event Tester",	.noswallow = 1) /* xev */
	RULE(.class = "QjackCtl",	.isfloating = 1)
	RULE(.title = "Firefox Update", .isfloating = 1)
	//customs
	RULE(.class = "Video",		.isfloating = 1)
	RULE(.title = "noswallow",	.noswallow = 1)
	RULE(.title = "mpvfloat",	.isfloating = 1)
	RULE(.instance = "mpvfloat",	.isfloating = 1)

	/* scratchpads */
	RULE(.instance = "term",	.tags = SPTAG(Sp1), .isfloating = 1)
	RULE(.instance = "notes",	.tags = SPTAG(Sp2), .isfloating = 1)
	RULE(.instance = "calc" ,	.tags = SPTAG(Sp3), .isfloating = 1)
	RULE(.instance = "pre"  ,	.tags = SPTAG(Sp4), .isfloating = 1)
	RULE(.instance = "music",	.tags = SPTAG(Sp5), .isfloating = 1)
	RULE(.instance = "pulsemixer",	.tags = SPTAG(Sp6), .isfloating = 1)
	RULE(.instance = "term",	.tags = SPTAG(Sp7), .isfloating = 1)
	RULE(.instance = "testi",	.tags = SPTAG(Sp8), .isfloating = 1)
	//RULE(.instance = "normal",	.tags = SPTAG(7))
	//RULE(.instance = "emacsfloat",	.tags = SPTAG(8), .isfloating = 1)
};

/* resizemousescroll direction argument list */
static const int scrollargs[4][2] = {
	/* width change         height change */
	{ -scrollsensetivity,	0 },
	{ +scrollsensetivity,	0 },
	{ 0, 			-scrollsensetivity },
	{ 0, 			+scrollsensetivity },
};

/* layout(s) */
static float mfact     = 0.55;	/* factor of master area size [0.05..0.95] */
static int nmaster     = 1;	/* number of clients in master area */
static int resizehints = 0;	/* 1 means respect size hints in tiled resizals */

static const Layout layouts[] = {
	/* symbol	arrange function   gaps (enabled if omitted)			*/
 	{ "[]=",	tile },			/* master on left, slaves on right */
 	{ "🧐",		monocle },		/* all windows on top of eachother */
 	{ "{}",		alphamonocle, 1 },	/* monocle but windows aren't stacked */
	//{ "TTT",	bstack },		/* master on top, slaves on bottom */
	//{ "🐚",		spiral },		/* fibonacci spiral */
	//{ "[\\]",	dwindle },		/* decreasing in size right and leftward */
	{ "[D]",	deck },			/* master on left, slaves in monocle mode on right */
	//{ ">M>",	centeredfloatmaster},	/* centermaster but master floats */
	//{ "|M|",	centeredmaster },	/* master in middle, slaves on sides */
	//{ "===",      bstackhoriz },		/* bstack but slaves stacked "monocle"-like */
	//{ "HHH",      grid },			/* windows in a grid */
	{ "###",	nrowgrid },		/* gaplessgrid with no gaps, but not equal size */
	//{ "#v#",	vsplitnrowgrid },	/* nrowgrid: force two clients to always split vertically */
	//{ "---",      horizgrid },		/* gaplessgrid but with horizontal order */
	//{ ":::",      gaplessgrid },		/* grid ajusted in such that there are no gaps */
	//{ "🐷", 	pidgin },		/* basically grid? */
	//{ ")M(",	ego },
	{ "🥏",		NULL },			/* no layout function means floating behavior */
	{ "[ ]",	clear },		/* hides all visible clients, enjoy your nice wallpaper */
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,	comboview,      { .ui = 1 << TAG } }, \
	{ MODKEY|ShiftMask,             KEY,	combotag,       { .ui = 1 << TAG } }, \
	/*{ MODKEY|ControlMask,           KEY,	toggleview,     { .ui = 1 << TAG } },*/\
	{ MODKEY|ControlMask|ShiftMask, KEY,	toggletag,      { .ui = 1 << TAG } }, \
	{ MODKEY|ControlMask,		KEY,	swaptags,	{ .ui = 1 << TAG } },

#define SPKEYS(MOD,KEY,NUM) \
	{ MOD,			KEY,	togglescratch,	{ .ui = NUM } },
/* helper for spawning shell commands in the pre dwm-5.0 fashion, maybe use shkd? */
#define SHCMD(cmd)	spawn, { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
/* helper macro for the custom command of mine 'spawncmd' */
#define CMDCMD(cmd) 	spawncmd,{ .v = cmd }
/* macro for any dmenu commands, colorize them */
#define DMENUARGS "-m", dmenumon, "-nb", color0, "-nf", color8, "-sb", color2, "-sf", color0
/* macro defining commands */
#define EXEC(name,cmd)	static const char *name[] = { "/bin/sh", "-c", cmd, NULL };
/* commands */
static const char *dmenucmd[]  = { "dmenu_run_i", DMENUARGS, NULL };
static const char *samedmenu[] = { "samedirmenu", DMENUARGS, NULL };
static const char *clip[]      = { "clipmenu", "-i", "-l", "25", DMENUARGS, NULL };
//static const char *passmenu[]  = { "passmenu", "-i", "-l", "25", "-p", "Passmenu:", DMENUARGS, NULL };
//static const char *termcmd[]   = { terminalcmd, NULL };
EXEC(termcmd, "st")
static const char *syncthing[] = { "surf", "127.0.0.1:1210", NULL };
static const char *web[]       = { "surf", "start.duckduckgo.com", NULL };
static const char *vifm[]      = { "st", "-e", "vifmrun", NULL };
static const char *samevifm[]  = { "samedirvifm", NULL };
//EXEC(samedmenu, "samedirmenu"
static const char pulsepad[] = "st -n pulsemixer -g 100x25 -f 'SauceCodePro Nerd Font: style=Mono Regular:size=12' -e pulsemixer";

/* macro for nvim to start on insertmode on the last line */
#define NOTES		"-e", "nvim", "+$", "+startinsert!"
#define FURSIZE		"90x25"
/* scratchpads */
static const char *scratchpads[][256] = {
	[Sp1] = { "st", "-n", "term", "-g", "115x30" , NULL }, /* terminal */
	[Sp2] = { "st", "-n", "notes", "-g", FURSIZE, "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", NOTES, "/home/faber/Docs/testi/notes", NULL }, /* notes */
	[Sp3] = { "st", "-n", "calc", "-f", "monospace:size=16", "-g", "50x20", "-e", "bc", "-lq", NULL }, /* calculator */
	//[Sp4] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/pre-Uni.txt", NULL }, /* uni */
	[Sp4] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/testi-is", NULL }, /* uni */
	[Sp5] = { "st", "-n", "music", "-g", "105x27", "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", "-e", "ncmpcpp", "-q", NULL }, /* music */
	//[Sp6] = { "st", "-n", "pulsemixer", "-g", "100x25", "-f", "SauceCodePro Nerd Font: style=Mono Regular:size=12", "-e", "pulsemixer", NULL }, /* pulsemixer */
	[Sp6] = { "/bin/sh", "-c", pulsepad, NULL },
	[Sp7] = { "samedir", "-n", "term", "-g", "115x30", NULL }, /* samedir */
	[Sp8] = { "st", "-n", "testi", "-g", FURSIZE, "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", NOTES, "/home/faber/Docs/testi/testi", NULL }, /* notes */
};

static const Key keys[] = {
	/* modifier(s)			key	function	argument */

				/* Commands */
	{ MODKEY,		   XK_Return,	spawn,		{ .v = termcmd }	},
	{ MODKEY,			XK_d,   spawn,		{ .v = dmenucmd }	},
	{ MODKEY|ShiftMask,		XK_d,   spawn,		{ .v = samedmenu }	},
	{ MODKEY,			XK_m,	spawn,		{ .v = vifm }		},
	{ MODKEY|ShiftMask,		XK_m,	spawn,		{ .v = samevifm }	},
	{ MODKEY,	       XK_apostrophe,	spawn,		{ .v = clip }		},
	{ MODKEY|ShiftMask,		XK_w,	spawn,		{ .v = web }		},
	{ MODKEY|ControlMask,		XK_w,	spawn,		{ .v = syncthing }	},
//	{ MODKEY|ShiftMask,    XK_apostrophe,	spawn,		{ .v = passmenu }	},
	{ MODKEY|ShiftMask,    XK_apostrophe,	SHCMD("clipctl disable && passmenu -i \
	-l 25 -p 'Passmenu:' && notify-send 'Password will be deleted on 45 seconds❌' ; clipctl enable")},
	SPKEYS(MODKEY,			XK_s,	/* terminal	*/	Sp1)
	SPKEYS(MODKEY,			XK_e,	/* notes	*/	Sp2)
	SPKEYS(MODKEY,			XK_x,	/* calculator	*/	Sp3)
	SPKEYS(MODKEY|ControlMask,	XK_s,	/* uni		*/	Sp4)
	SPKEYS(MODKEY,			XK_n,	/* music	*/	Sp5)
	SPKEYS(MODKEY|ShiftMask,	XK_p,	/* pulsemixer	*/	Sp6)
	{ MODKEY|ShiftMask,		XK_p,	updateblock,		{ .ui = 8 }	},
	SPKEYS(MODKEY|ShiftMask,	XK_s,	/* samedir	*/	Sp7)
	SPKEYS(MODKEY|ControlMask,	XK_e,	/* notes - is	*/	Sp8)
	//SCRATCHKEYS(MODKEY|ControlMask,	XK_e,	/* stnormal	*/	7)
	{ MODKEY|ControlMask,		XK_v,	scratchpad_remove,	{0} },
	{ MODKEY,                       XK_v,	scratchpad_show,	{0} },
	{ MODKEY|ShiftMask,             XK_v,	scratchpad_hide,	{0} },
	//{ MODKEY|ControlMask,          	XK_v,	scratchpad_remove,	{0} },
	//{ MODKEY,			XK_s,	togglescratch,	{.ui = Sp1 } },/* bc */

				/* Navigation */
	{ MODKEY,			XK_j,	focusstack,	{ .i = -1 }		},
	{ MODKEY|ShiftMask,		XK_j,	pushstack,	{ .i = 1 }		},

	{ MODKEY|ControlMask,		XK_j,	shifttag,	{ .i = -1 }		},
	{ MODKEY|ControlMask|ShiftMask,	XK_j,  shifttagclients,	{ .i = -1 }		},
	{ MODKEY|ControlMask|ShiftMask,	XK_k,  shifttagclients,	{ .i = +1 }		},
	{ MODKEY|ControlMask,		XK_k,	shifttag,	{ .i = +1 }		},

	{ MODKEY|ShiftMask,		XK_k,	pushstack,	{ .i = -1 }		},
	{ MODKEY,                       XK_k,	focusstack,	{ .i = +1 }		},

	{ MODKEY,                       XK_h,	setmfact,	{ .f = -0.02 }		},
	{ MODKEY|ShiftMask,		XK_h,	shiftboth,	{ .i = -1 }		},
	{ MODKEY|ControlMask,		XK_h,	shiftswaptags,	{ .i = -1 }		},
//	{ MODKEY|ShiftMask,             XK_h,	setcfact,	{ .f = +0.05 }		},
//	{ MODKEY|ShiftMask,             XK_l,	setcfact,	{ .f = -0.05 }		},
	{ MODKEY|ControlMask,		XK_l,	shiftswaptags,	{ .i = +1 }		},
	{ MODKEY|ShiftMask,             XK_l,	shiftboth,	{ .i = +1 }		},
	{ MODKEY,                       XK_l,	setmfact,	{ .f = +0.02 }		},

	{ MODKEY,                       XK_o, shiftviewclients,	{ .i = +1 }		},
	{ MODKEY|ShiftMask,             XK_o,	shiftview,	{ .i = +1 }		},
	{ MODKEY|ShiftMask,             XK_i,	shiftview,	{ .i = -1 }		},
	{ MODKEY,	                XK_i, shiftviewclients,	{ .i = -1 }		},

	{ MODKEY,		XK_semicolon,	incnmaster,	{ .i = +1 }		},
	{ MODKEY|ShiftMask,	XK_semicolon,	incnmaster,	{ .i = -1 }		},
	{ MODKEY,	                XK_q,	killclient,		{0}		},
	{ MODKEY|ShiftMask,         XK_space,	togglefloating,		{0}		},
	{ MODKEY,                       XK_F1,	fullscreen,		{0}		},
	{ MODKEY|ControlMask,        	XK_F1,	fakefullscreen,		{0}		},
	{ MODKEY,                       XK_w,	zoomswap,		{0}		},
	{ MODKEY,	              XK_Tab,	view,			{0}		},
	{ MODKEY,	            XK_space,	view,			{0}		},
	{ MODKEY|ControlMask,	    XK_grave,	togglebar,		{0}		},
//	{ MODKEY|ShiftMask,	    XK_grave,	toggletopbar,		{0}		},
	{ MODKEY|ShiftMask,             XK_F5,	quit,           	{0}		},
	{ MODKEY,			XK_F5,	refresh,		{0}		},
	{ MODKEY,                   XK_comma,	focusmon,	{ .i = -1 }		},
	{ MODKEY|ShiftMask,         XK_comma,	tagmon,		{ .i = -1 }		},
	{ MODKEY|ShiftMask,        XK_period,	tagmon,		{ .i = +1 }		},
	{ MODKEY,                  XK_period,	focusmon,	{ .i = +1 }		},
	{ MODKEY,                       XK_0,	view,		{.ui = ~SPTAGMASK }	},
	{ MODKEY|ShiftMask,             XK_0,	tag,		{.ui = ~SPTAGMASK }	},
	{ MODKEY|ControlMask,		XK_0,	view,		{.ui = ~0 }		},
	//{ MODKEY|ControlMask,		XK_0,	tag,		{.ui = ~0 }		},
	  TAGKEYS(			XK_1,				0)
	  TAGKEYS(			XK_2,				1)
	  TAGKEYS(			XK_3,				2)
	  TAGKEYS(			XK_4,				3)
	  TAGKEYS(			XK_5,				4)
	  TAGKEYS(			XK_6,				5)
	  TAGKEYS(			XK_7,				6)
	  TAGKEYS(			XK_8,				7)
	  TAGKEYS(			XK_9,				8)
	{ MODKEY|ControlMask,		XK_b,	togglestatus,		{0}		},

				/* LAYOUTS */
	{ MODKEY,		XK_backslash,   cyclelayout,	{.i = +1 }		},
	{ MODKEY|ShiftMask,	XK_backslash,   cyclelayout,	{.i = -1 }		},
//	{ MODKEY,			XK_t,	setlayout,	{.v = &layouts[0]} }, //monocle
//	{ MODKEY|ShiftMask,		XK_t,	setlayout,	{.v = &layouts[1]} }, //tile
//	{ MODKEY,			XK_n,	setlayout,	{.v = &layouts[2]} }, //bstack
//	{ MODKEY|ShiftMask,		XK_i,	setlayout,	{.v = &layouts[3]} }, //spiral
//	{ MODKEY,			XK_i,	setlayout,	{.v = &layouts[4]} }, //dwindle
//	{ MODKEY|ShiftMask,		XK_f,	setlayout,	{.v = &layouts[5]} }, //deck
//	{ MODKEY,			XK_f,	setlayout,	{.v = &layouts[6]} }, //centeredmaster

				/* GAPS */
	{ MODKEY,			XK_f,	incrgaps,	{.i = +3 }		},
	{ MODKEY,			XK_g,	incrgaps,	{.i = -3 }		},
	{ MODKEY|ShiftMask,		XK_a,	togglegaps,		{0}		},
	{ MODKEY|ControlMask,   	XK_a,	defaultgaps,		{0}		},
	{ MODKEY|ControlMask|ShiftMask,	XK_a,	togglesmartgaps,	{0}		},
	{ MODKEY,			XK_a,	togglevacant,		{0}		},
	//{ MODKEY|ControlMask|ShiftMask,	XK_a,	toggleborder,	{0}		},
//	{ MODKEY|ControlMask,           XK_o,	setcfact,	{.f =  0.00}		},

				/* Media */
	{ MODKEY|ShiftMask,		XK_minus,		SHCMD("mpc volume -3")	},
	{ MODKEY|ShiftMask,		XK_equal,		SHCMD("mpc volume +3")	},
	{ MODKEY|ShiftMask,		XK_bracketleft,		SHCMD("mpc seek -10")	},
	{ MODKEY|ShiftMask,		XK_bracketright,	SHCMD("mpc seek +10")	},
	{ MODKEY,			XK_minus,	SHCMD("pamixer --allow-boost -d 3")},
	{ MODKEY,			XK_minus, updateblock,		{ .ui = 8 }	},
	{ MODKEY,			XK_equal,	SHCMD("pamixer --allow-boost -i 3")},
	{ MODKEY,			XK_equal, updateblock,		{ .ui = 8 }	},
	{ MODKEY,			XK_BackSpace,	SHCMD("pamixer -t; xsetroot -name 8")},
	{ 0,	XF86XK_AudioLowerVolume,	SHCMD("pamixer --allow-boost -d 2")	},
	{ 0,	XF86XK_AudioLowerVolume,	updateblock,		{ .ui = 8 } 	},
	{ 0,	XF86XK_AudioRaiseVolume,	SHCMD("pamixer --allow-boost -i 2")	},
	{ 0,	XF86XK_AudioRaiseVolume,	updateblock,		{ .ui = 8 }	},
	{ 0,	XF86XK_AudioMute,		SHCMD("pamixer -t; xsetroot -name 8")	},
//{ 0,	XF86XK_Calculator,		SHCMD("sleep 0.2 ; scrot -se 'mv $f ~/Downloads'") },
//{ 0, XF86XK_ScreenSaver,		SHCMD("slock & xset dpms force off; mpc pause; pauseallmpv") },
//	{ 0,	XF86XK_AudioStop,		SHCMD("mpc toggle)		},
//	{ 0,	XF86XK_Sleep,			SHCMD("sudo zzz")		},
	{ 0,	XF86XK_ScreenSaver,		SHCMD("xset dpms force off")		},
	{ 0,	XF86XK_MonBrightnessUp,		SHCMD("sudo brightnessctl -q set +1%")	},
	{ 0,	XF86XK_MonBrightnessDown,	SHCMD("sudo brightnessctl -q set 1%-")	},
	{ 0,	XF86XK_AudioPlay,		SHCMD("mpc toggle")			},
	{ 0,	XF86XK_AudioPrev,		SHCMD("mpc prev")			},
	{ 0,	XF86XK_AudioNext,		SHCMD("mpc next")			},
	//{ 0,	XF86XK_RFKill,			random_wall, {0}		},
	{ MODKEY,		XK_p,		SHCMD("mpc toggle")},
	{ MODKEY,		XK_p,		updateblock,		{ .ui = 11 }	},
	{ MODKEY,	XK_bracketleft,		SHCMD("mpc prev")	},
	{ MODKEY,	XK_bracketleft,		updateblock,		{ .ui = 11 }	},
	{ MODKEY,	XK_bracketright,	SHCMD("mpc next")	},
	{ MODKEY,	XK_bracketright,	updateblock,		{ .ui = 11 }	},
	{ MODKEY|ControlMask,	XK_p,		SHCMD("mpdnoti")			},

			/* Custom bindings (may be better using shkd) */
	{ MODKEY,			XK_b,	CMDCMD("Books001")		},
	{ MODKEY|ShiftMask,		XK_u,	SHCMD("bookmenu")		},
	{ MODKEY|ShiftMask,		XK_b,	SHCMD("Boletin001")		},
	{ MODKEY,		        XK_c,	SHCMD("st -f 'Monofur Nerd Font:pixelsize=22:antialias=true:autohint=true' -e calcurse")},
	{ MODKEY,	         	XK_z,	SHCMD("redyt -r")		},
	{ MODKEY|ShiftMask,	      	XK_z,	SHCMD("waldl")		},
	{ MODKEY,		    XK_grave,	SHCMD("dmenuunicode")		},
	{ MODKEY|ShiftMask,	   XK_Return,	SHCMD("samedir &")		},
//	{ MODKEY,	        XK_semicolon,	SHCMD("dmenu_mpc")		},
//	{ MODKEY|ShiftMask,	    XK_slash,	SHCMD("tuxi -q")		},
	{ MODKEY,			XK_u,	SHCMD("clipmagick")		},
	{ MODKEY,			XK_y,	SHCMD("termyt -r")		},
	{ MODKEY|ShiftMask,		XK_y,	SHCMD("dmenuhandler")		},
	{ MODKEY,		    XK_slash,	SHCMD("dmenu_browser")		},
	{ MODKEY|ShiftMask,	        XK_n,	SHCMD("xdotool click 1")	},
//	{ MODKEY,			XK_v,	SHCMD("killall xcompmgr || setsid xcompmgr &")	},
	{ MODKEY,			XK_t,	SHCMD("testi")			},
	{ MODKEY,		   XK_Escape,	SHCMD("sysfunctions")	},
//{ MODKEY,	XK_e,	spawn,	SHCMD("st -t New-e newsboat -q; pkill -RTMIN+6 dwmblocks") },
	{ MODKEY,			XK_r,	SHCMD("st -t NewsBoat -e newsboat -q") },
//					XK_F1, Feen
//	{ MODKEY,			XK_F2,	SHCMD("dmenu_man")			},
//	{ MODKEY,	         	XK_F3,	SHCMD("dmenumount")			},
	{ MODKEY,	         	XK_F3,	SHCMD("dmenumount")			},
	{ MODKEY,		 	XK_F4,	SHCMD("dmenuumount")			},
//	{ MODKEY,		 	XK_F4,	SHCMD("syncthing & kill -55 $(pidof dwmblocks)") },
//					XK_F5,
//	{ MODKEY,	         	XK_F6,	SHCMD("dmenumount")			},
//	{ MODKEY,		 	XK_F7,	SHCMD("dmenumountq")			},
//	{ MODKEY,	 		XK_F7,	SHCMD("st -e nvim -c VimwikiIndex") },
	{ MODKEY,		 	XK_F8,	SHCMD("sleep 0.2 ; xdotool key Caps_Lock") },
	{ MODKEY,			XK_F9,	SHCMD("setkeys & notify-send -t 2400 \
		'Keyboard remapping⌨️ ' 'WAIT!\nRerunning <b>customs</b> shorcuts'")	},
	{ MODKEY,			XK_F10,	SHCMD("setxkbmap -layout us -variant altgr-intl -option nodeadkeys & notify-send 'Keyboard⌨️ ' 'Keyboard remapping...\nRunning keyboard defaults, US altgr-intl variant with nodeadkeys...'") },
	//{ MODKEY,			XK_F11,	spawn,	SHCMD("setbg $HOME/Media/Pictures/Wallpapers &") },
	{ MODKEY,                       XK_F11,	random_wall,		{0}	},
	{ MODKEY,                       XK_F12,	xrdb,			{0}	},
	//remove black bar on the screenshot %90 percent accuracy
	{ ShiftMask, XK_Print,	SHCMD("scrot -u -se 'mv $f ~/Downloads && \
		magick mogrify -fuzz 4% -define trim:percent-background=0% -trim +repage -format png ~/Downloads/$f'") },
	{ MODKEY,		    XK_Print,	SHCMD("dmenurecord")		},
	{ 0,		    XK_Print,	SHCMD("scrot")			},
/* panic key */ { MODKEY, XK_Delete, SHCMD("mpv '/home/faber/Media/Videos/Fight the Power!.mkv' --loop-file=inf --fs") },
};

/* button definitions
 * click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkTagBar,            0,              Button4,	shiftview,	{.i = +1 } },
	{ ClkTagBar,            0,              Button5,	shiftview,	{.i = -1 } },

	{ ClkLtSymbol,          0,              Button1,        togglegaps,     {0} },
	{ ClkLtSymbol,          0,              Button2,        togglevacant,   {0} },
	//{ ClkLtSymbol,          0,              Button3,        togglevacant,   {0} },
//	{ ClkLtSymbol,          0,              Button1,        cyclelayout,    {.i = +1 } },
//	{ ClkLtSymbol,          0,              Button3,        cyclelayout,    {.i = -1 } },
	{ ClkLtSymbol,          0,              Button4,        cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button5,        cyclelayout,    {.i = -1 } },
//	{ ClkLtSymbol,          0,              Button4,        focusstack,	{.i = +1} },
//	{ ClkLtSymbol,          0,              Button5,        focusstack,	{.i = -1} },

	{ ClkWinTitle,          0,              Button1,	SHCMD("maim -sDq ~/Downloads/$(date +'%d-%m_%H_%M_%S').png") },
	{ ClkWinTitle,          0,              Button2,	zoomswap,       {0} },
	{ ClkWinTitle,          0,              Button2,	killclient,	{0} },
	{ ClkWinTitle,		0,		Button3,	SHCMD("scrot -us -e 'mv $f ~/Downloads'") },
	{ ClkWinTitle,          0,              Button4,        focusstack,	{.i = +1 } },
	{ ClkWinTitle,          0,              Button5,        focusstack,	{.i = -1 } },

	{ ClkStatusText,        0,              Button1,        sendstatusbar,   {.i = 1 } },
	{ ClkStatusText,        0,              Button2,        sendstatusbar,   {.i = 2 } },
	{ ClkStatusText,        0,              Button3,        sendstatusbar,   {.i = 3 } },
	{ ClkStatusText,        0,              Button4,        sendstatusbar,   {.i = 4 } },
	{ ClkStatusText,        0,              Button5,        sendstatusbar,   {.i = 5 } },
	{ ClkStatusText,        ShiftMask,      Button1,        sendstatusbar,   {.i = 6 } },

	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         MODKEY,         Button4,        resizemousescroll, {.v = &scrollargs[0]} },
	{ ClkClientWin,         MODKEY,         Button5,        resizemousescroll, {.v = &scrollargs[1]} },
	{ ClkClientWin,         MODKEY,         Button6,        resizemousescroll, {.v = &scrollargs[2]} },
	{ ClkClientWin,         MODKEY,         Button7,        resizemousescroll, {.v = &scrollargs[3]} },
//	{ ClkClientWin,   MODKEY|ShiftMask,     Button4,        focusstack,	{.i = 1 } },
//	{ ClkClientWin,   MODKEY|ShiftMask,     Button5,        focusstack,	{.i = -1 } },
};
