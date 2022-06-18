/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static unsigned int borderpx  = 5;        /* border pixel of windows */
static unsigned int fborderpx = 2;        /* border pixel of floating windows */
static unsigned int snap      = 10;       /* snap pixel */
static unsigned int gappih    = 15;       /* horiz inner gap between windows */
static unsigned int gappiv    = 20;       /* vert inner gap between windows */
static unsigned int gappoh    = 15;       /* horiz outer gap between windows and screen edge */
static unsigned int gappov    = 20;       /* vert outer gap between windows and screen edge */
static int hidevacant         = 1;        /* 1 means hide vacant tags */
static int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */
static int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static int swallowffs         = 1;        /* 1 means swallow fullscreen windows by default */
static int showbar            = 1;        /* 0 means no bar */
static int showstatus         = 1;        /* 0 means no status text */
static const int underlntitle = 1;        /* 1 means an underline on the title */
static const int topbar       = 1;        /* 0 means bottom bar */
static const int barh         = 9;        /* 1 or more means bar height */
static const int pertag       = 1;        /* 0 means global layout across all tags (default) */
static const int pertagbar    = 0;        /* 0 means using pertag, but with the same barpos */
static const int gapspertag   = 1;        /* 0 means global gaps across all tags (default) */
static const int scalepreview = 4;        /* tag previews scaling */
#define baralpha		120       /* bar opacity from 0 to 255/OPAQUE */

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
static char color6[8], color7[8], color8[8], bg_wal[8], fg_wal[8];
static const char fallbackcolor[8] = "#222222"; /* In case some variable couldn't be set */

static char *xrescolors[][2] = {
	/* variable	xresource */
	{ color0,	"color0" },
	{ color1,	"color1" },
	{ color2,	"color2" },
	{ color3,	"color3" },
	{ color4,	"color4" },
	{ color5,	"color5" },
	{ color6,	"color6" },
	{ color7,	"color7" },
	{ color8,	"color8" },
	{ bg_wal,	"background" },
	{ fg_wal,	"foreground" },
};

static const char *colors[][2] = {
			/* fg		bg           description         */
	[SchemeNorm]   = { fg_wal,	color0 }, /* normal tags section */
	[SchemeSel]    = { color0,	color1 }, /* selected tag */
	[SchemeUrgent] = { color0,	fg_wal }, /* urgent tag */
	[SchemeLt]     = { fg_wal,	color0 }, /* layout */
	[SchemeTitle]  = { color0,	color2 }, /* window title */
	[SchemeStatus] = { fg_wal,	color0 }, /* status text */
	[SchemeDelim]  = { fg_wal,	color0 }, /* delimiter */
	[SchemeSys]    = { color0,	color0 }, /* system tray */
	[SchemeIndUrg] = { fg_wal,	color0 }, /* rectangle on urg tag */
	[SchemeIndOn]  = { color4,	color0 }, /* rectangle on sel tag */
	[SchemeIndOff] = { color2,	color0 }, /* rectangle on norm tag */
	[BorderNorm]   = { bg_wal },	/* normal window border */
	[BorderSel]    = { color2 },	/* selected window border */
	[BorderFloat]  = { color6 },	/* selected floating window border */
	[BorderUrg]    = { fg_wal },	/* urgent window border */
};
static const unsigned int alphas[][2] = {
			/* fg		bg	 */
	[SchemeNorm]   = { OPAQUE,	baralpha },
	[SchemeSel]    = { OPAQUE,	baralpha },
	[SchemeUrgent] = { OPAQUE,	baralpha },
	[SchemeLt]     = { OPAQUE,	baralpha },
	[SchemeTitle]  = { OPAQUE,	baralpha + 40},
	[SchemeStatus] = { OPAQUE,	baralpha + 40},
	[SchemeDelim]  = { OPAQUE,	baralpha + 40},
	[SchemeSys]    = { OPAQUE,	baralpha },
	[SchemeIndUrg] = { OPAQUE,	baralpha },
	[SchemeIndOn]  = { OPAQUE,	baralpha },
	[SchemeIndOff] = { OPAQUE,	baralpha },
	[BorderNorm]   = { OPAQUE },
	[BorderSel]    = { baralpha },
	[BorderFloat]  = { baralpha },
	[BorderUrg]    = { OPAQUE },
};

/* status bar */
static const Block blocks[] = {
	/* color  command				interval	signal */
	{ color7, "sb-clock",				20,		1},
	{ color2, "sb-disk",				9000,		2},
	{ color3, "sb-battery",				10,		3},
	{ color4, "sb-internet",			10,		4},
	{ color5, "sb-mailbox",				0,		5},
	{ color6, "sb-moonphase",			0,		6},
	{ color1, "sb-forecast",			0,		7},
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
	{ color5, "echo '';cat /tmp/recordingicon",	0,		14},
//	{ color6, "sb-count",				0,		21},
};

/* 1 means inverse the order of the blocks, 0 means normal order */
#define INVERSED		1
/* max number of character that one block command can output */
#define CMDLENGTH		65
/* delimeter between blocks commands. NULL character ('\0') means no delimeter. */
static const char delimiter[] = " ";

/* properties:			res_name  res_class */
static char *systrayclass[2] = { "demwm", "demwm" };
static char *barclass[2]     = { "demwm", "demwm" };
static char *previewclass[2] = { "demwm-preview", "demwm-preview" };

/* tags */
//static const char *tags[] = { "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉" };
static const char *tags[]     = { "📖", "", "💼", "", "🔬", "🎹", "📺", "💻", "🐧" };
static const int taglayouts[] = {    0,   1,    0,   0,    1,    0,    0,    0,    0 };
static const char *tagsalt[]  = { "I", "2", "III", "4", "V", "6", "VII", "8", "IX" };

static const Rule rules[] = {
	/*
	 * Select a window you wish to assign a rule to with xprop.
	 *
	 * xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 *
	 * Assign a flag to the window, by default none are assign.
	 *
	 * Avaliable flags:
	 *  monitor, in which monitor the window will spawn
	 *  tags, in which tags the window will spawn
	 *  isfloating, enable floating
	 *  isterminal, will make the window swallow child ones
	 *  isfakefullscreen, enable fakefullscreen
	 *  noswallow, don't swallow this window
	 *  uncursor, puts the cursor in the bottom right whenever the window
	 * is focused (useful when there are gaps)
	 */
//	RULE(.class = "Gimp",		.tags = 1 << 7)
	RULE(.class = "vlc",		.tags = 1 << 6)
	RULE(.class = "qutebrowser",	.tags = 1 << 5)
	RULE(.class = "Surf",		.tags = 1 << 7)
	RULE(.title = "Music",		.tags = 1 << 6)
	RULE(.title = "Sxiv - redyt",	.tags = 1 << 8)
	RULE(.title = "Sxiv - walld",	.tags = 1 << 8)
//	RULE(.class = "libreoffice",	.tags = 1 << 3)
//	RULE(.title = "LibreOffice",	.isfloating = 1, .noswallow = 1)
//	RULE(.class = "libreoffice",	.noswallow = 1)
	RULE(.class = "firefox",	.tags = 1 << 1, .isfakefullscreen = 1)
	RULE(.class = "Brave-browser",	.tags = 1 << 4, .isfakefullscreen = 1)
	RULE(.class = "St",		.isterminal = 1)
	RULE(.title = "Event Tester",	.noswallow = 1) /* xev */
	/* floating */
	RULE(.class = "Pavucontrol",	.isfloating = 1)
	RULE(.class = "Pcmanfm",	.isfloating = 1)
	RULE(.title = "About Mozilla Firefox",	.isfloating = 1)
	RULE(.class = "QjackCtl",	.isfloating = 1)
	RULE(.title = "Firefox Update", .isfloating = 1)
	RULE(.title = "Krita - Edit Text", .isfloating = 1)
	RULE(.class = "Blueman-manager",.isfloating = 1)
	//customs
	RULE(.class = "Video",		.isfloating = 1)
	RULE(.class = "dialect",	.isfloating = 1)
	RULE(.title = "mpvfloat",	.isfloating = 1)
	RULE(.instance = "mpvfloat",	.tags = SPTAG(Sp9), .isfloating = 1)
	RULE(.title = "noswallow",	.noswallow = 1)


	/* scratchpads */
	RULE(.instance = "term",	.tags = SPTAG(Sp1), .isfloating = 1, .isterminal = 1)
	RULE(.instance = "notes",	.tags = SPTAG(Sp2), .isfloating = 1)
	RULE(.instance = "calc" ,	.tags = SPTAG(Sp3), .isfloating = 1)
	RULE(.instance = "pre"  ,	.tags = SPTAG(Sp4), .isfloating = 1)
	RULE(.instance = "music",	.tags = SPTAG(Sp5), .isfloating = 1)
	RULE(.instance = "pulsemixer",	.tags = SPTAG(Sp6), .isfloating = 1)
	RULE(.instance = "samedir",	.tags = SPTAG(Sp7), .isfloating = 1, .isterminal = 1)
	RULE(.instance = "testi",	.tags = SPTAG(Sp8), .isfloating = 1)
	//RULE(.instance = "normal",	.tags = SPTAG(7))
	//RULE(.instance = "emacsfloat",	.tags = SPTAG(8), .isfloating = 1)
};

/* 1 means resize window by 1 pixel for each scroll event on resizemousescroll */
#define scrollsensetivity	18
static const int scrollargs[4][2] = {
	/* width change         height change */
	{ -scrollsensetivity,	0 },
	{ +scrollsensetivity,	0 },
	{ 0, 			-scrollsensetivity },
	{ 0, 			+scrollsensetivity },
};

/* layout(s) */
static const float mfact     = 0.55;	/* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;	/* number of clients in master area */
static const int resizehints = 0;	/* 1 means respect size hints in tiled resizals */
static const int floathints  = 0;	/* 1 means respect size hints if the window is floating */
static const int movefloat   = 22;

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
#define MOD Super
#define MODKEY MOD /* compatibility with dwm configs */

#define TAGKEYS(KEY,TAG) \
	{ MOD,			KEY,	comboview,      { .ui = 1 << TAG } },	\
	{ MOD|Shift,		KEY,	combotag,       { .ui = 1 << TAG } },	\
	/*{ MOD|Ctrl,           KEY,	toggleview,     { .ui = 1 << TAG } },*/	\
	{ MOD|Ctrl,		KEY,	previewtag,	{ .ui = TAG }	   }, \
	/* { MOD|Ctrl|Shift, KEY,	toggletag,      { .ui = 1 << TAG } },*/ \
	{ MOD|Shift|Ctrl,	KEY,	swaptags,	{ .ui = 1 << TAG } },

#define SPKEYS(MKEY,KEY,NUM) \
	{ MKEY,			KEY,	togglescratch,	{ .ui = NUM } },

/* helper for spawning shell commands */
#define SHCMD(cmd)	spawn, { .v = (const char *[]){ "/bin/sh", "-c", cmd, NULL } }

/* helper for spawning shell commands in scratchpads */
#define SH(cmd)		{ "/bin/sh", "-c", cmd, NULL }

/* macros of common comand line arguments */
#define DMENUARGS	"-m", dmenumon, "-nb", color0, "-nf", color8, "-sb", color2, "-sf", color0
#define NOTES		"-e", "nvim", "+$", "+startinsert!"
#define FURSIZE		"90x25"

/* commands */
static const char *dmenucmd[]  = { "dmenu_run_i", DMENUARGS, NULL };
static const char *dmenuipc[]  = { "dmenu_dwmc", DMENUARGS, NULL };
static const char *samedmenu[] = { "samedirmenu", DMENUARGS, NULL };
static const char *clip[]      = { "clipmenu", "-i", "-l", "25", "-mon", "-1", "-nb", color0, "-nf", color8, "-sb", color2, "-sf", color0, NULL };
static const char *random_wall[] = { "demwm_random_wall", NULL };

/* scratchpads */
static const char *scratchpads[][32] = {
	[Sp1] = { "st", "-n", "term", "-g", "115x30" , NULL }, /* terminal */
	[Sp2] = { "st", "-n", "notes", "-g", FURSIZE, "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", NOTES, "/home/faber/Docs/testi/notes", NULL }, /* notes */
	[Sp3] = { "st", "-n", "calc", "-f", "monospace:size=16", "-g", "50x20", "-e", "bc", "-lq", NULL }, /* calculator */
	//[Sp4] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/pre-Uni.txt", NULL }, /* uni */
	[Sp4] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/testi-is", NULL }, /* uni */
	[Sp5] = { "st", "-n", "music", "-g", "105x27", "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", "-e", "ncmpcpp", "-q", NULL }, /* music */
	[Sp6] = SH("st -n pulsemixer -g 100x25 -f 'SauceCodePro Nerd Font: style=Mono Regular:size=12' -e pulsemixer"), /* pulsepad */
	[Sp7] = { "samedir", "-n", "samedir", "-g", "115x30", NULL }, /* samedir */
	[Sp8] = { "st", "-n", "testi", "-g", FURSIZE, "-f", "Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true", NOTES, "/home/faber/Docs/testi/testi", NULL }, /* notes */
	[Sp9] = { "st", "-n", "mpvfloat", "-g", "115x30" , NULL }, /* terminal */
};

/* modifier can be: Alt, AltGr, Ctrl, Shift, ShiftGr, Super */
static const Key keys[] = {
	/* modifier(s)		key	function	argument */

				/* commands */
	{ MOD,			XK_d,   spawn,	{ .v = dmenucmd }	},
	{ MOD|Ctrl,		XK_d,   spawn,	{ .v = dmenuipc }	},
	{ MOD|Shift,		XK_d,   spawn,	{ .v = samedmenu }	},
	{ MOD,	       XK_apostrophe,	spawn,	{ .v = clip }		},
	{ MOD,			XK_F11,	spawn,	{ .v = random_wall }	},

			/* scratchpads */
	SPKEYS(MOD,		XK_s,	/* terminal	*/	Sp1)
	SPKEYS(MOD,		XK_e,	/* notes	*/	Sp2)
	SPKEYS(MOD|Ctrl,	XK_x,	/* calculator	*/	Sp3)
	{ MOD,			XK_x,	SHCMD("rofi -show calc -modi calc -no-show-match -no-sort") },
	SPKEYS(MOD|Ctrl,	XK_s,	/* uni		*/	Sp4)
	SPKEYS(MOD,		XK_n,	/* music	*/	Sp5)
	{ MOD|Shift,		XK_p,	updateblock,	{ .ui = 8 }	},
	SPKEYS(MOD|Shift,	XK_p,	/* pulsemixer	*/	Sp6)
	SPKEYS(MOD|Shift,	XK_s,	/* samedir	*/	Sp7)
	SPKEYS(MOD|Ctrl,	XK_e,	/* notes - is	*/	Sp8)
	SPKEYS(MOD|Shift,	XK_n,	/* notes - is	*/	Sp9)
			/* dynamic scratchpads */
	{ MOD|Ctrl,		XK_v,	scratchpad_remove,	{0} },
	{ MOD,                  XK_v,	scratchpad_show,	{0} },
	{ MOD|Shift,            XK_v,	scratchpad_hide,	{0} },
//	{ MOD|Ctrl,          	XK_v,	scratchpad_remove,	{0} },

				/* Navigation */
	{ MOD,			XK_j,	focusstack,	{ .i = 1 }	},
//	{ MOD|Shift,		XK_j,	movefloatvert,	{ .i = 1 }	},
//	{ MOD|Shift,		XK_j,	pushstack,	{ .i = 1 }	},
	{ MOD|Shift,		XK_j,	movfv_pushstack,{ .i = 1 }	},

	{ MOD|Ctrl|Shift,	XK_j,	shifttag,	{ .i = -1 }	},
	{ MOD|Ctrl,		XK_j,  shifttagclients,	{ .i = -1 }	},
	{ MOD|Ctrl,		XK_k,  shifttagclients,	{ .i = +1 }	},
	{ MOD|Ctrl|Shift,	XK_k,	shifttag,	{ .i = +1 }	},

	{ MOD|Shift,		XK_k,	movfv_pushstack,{ .i = -1 }	},
//	{ MOD|Shift,		XK_k,	movefloatvert,	{ .i = -1 }	},
//	{ MOD|Shift,		XK_k,	pushstack,	{ .i = -1 }	},
	{ MOD,                  XK_k,	focusstack,	{ .i = -1 }	},

//	{ MOD,			XK_h,	setmfact,	{ .f = -0.02 }	},
//	{ MOD,			XK_h,	movefloathorz,	{ .i = -1 }	},
	{ MOD,			XK_h,	movfh_setmfact,	{ .f = -0.02 }	},

	{ MOD|Shift,		XK_h,	shiftboth,	{ .i = -1 }	},
	{ MOD|Ctrl,		XK_h,	shiftswaptags,	{ .i = -1 }	},
//	{ MOD|Shift,            XK_h,	setcfact,	{ .f = +0.05 }	},
//	{ MOD|Shift,            XK_l,	setcfact,	{ .f = -0.05 }	},
	{ MOD|Ctrl,		XK_l,	shiftswaptags,	{ .i = +1 }	},
	{ MOD|Shift,            XK_l,	shiftboth,	{ .i = +1 }	},

	{ MOD,			XK_l,	movfh_setmfact,	{ .f = +0.02 }	},
//	{ MOD,			XK_l,	movefloathorz,	{ .i = +1 }	},
//	{ MOD,			XK_l,	setmfact,	{ .f = +0.02 }	},

	{ MOD,			XK_o, shiftviewclients,	{ .i = +1 }	},
	{ MOD|Shift,		XK_o,	shiftview,	{ .i = +1 }	},
//	{ MOD|Shift,		XK_o,	shiftpreview,	{ .i = +1 }	},
//	{ MOD|Shift,		XK_i,	shiftpreview,	{ .i = -1 }	},
	{ MOD|Shift,		XK_i,	shiftview,	{ .i = -1 }	},
	{ MOD,			XK_i, shiftviewclients,	{ .i = -1 }	},
	{ MOD,			XK_0,	view,		{.ui = ~SPTAGMASK } },
	{ MOD|Shift,		XK_0,	tag,		{.ui = ~SPTAGMASK } },
	{ MOD|Ctrl,		XK_0,	view,		{.ui = ~0 }	},
//	{ MOD|Ctrl,		XK_0,	tag,		{.ui = ~0 }	},
	{ MOD,			XK_Tab,	view,			{0}	},
	{ MOD,			XK_space,view,			{0}	},
	{ MOD,			XK_w,	zoomswap,		{0}	},
	  TAGKEYS(		XK_1,				0)
	  TAGKEYS(		XK_2,				1)
	  TAGKEYS(		XK_3,				2)
	  TAGKEYS(		XK_4,				3)
	  TAGKEYS(		XK_5,				4)
	  TAGKEYS(		XK_6,				5)
	  TAGKEYS(		XK_7,				6)
	  TAGKEYS(		XK_8,				7)
	  TAGKEYS(		XK_9,				8)


				/* miscellaneous */
	{ MOD,			XK_a,	togglesticky,		{0}	},
//	{ MOD,			XK_a,	togglevacant,		{0}	},
	{ MOD|Ctrl,		XK_b,	togglestatus,		{0}	},
	{ MOD,	                XK_q,	killclient,		{0}	},
	{ MOD,			XK_y,	togglealwaysontop,	{0}	},
//	{ MOD|Ctrl,           XK_o,	setcfact,	{.f =  0.00}	},
//	{ MOD|Shift,	    XK_grave,	toggletopbar,		{0}	},
	{ MOD|Shift,         XK_space,	togglefloating,		{0}	},
	{ MOD,		XK_semicolon,	incnmaster,	{ .i = +1 }	},
	{ MOD|Shift,	XK_semicolon,	incnmaster,	{ .i = -1 }	},
	{ MOD|Ctrl,	    XK_grave,	togglebar,		{0}	},
	{ MOD,			XK_F1,	togglefullscreen,	{0}	},
	{ MOD|Ctrl,        	XK_F1,	togglefakefullscreen,	{0}	},
	{ MOD|Shift,		XK_F5,	quit,           	{0}	},
	{ MOD,			XK_F5,	refresh,		{0}	},
	{ MOD,			XK_F12,	xrdb,			{0}	},

				/* monitor */
	{ MOD|Shift,         XK_comma,	tagmon,		{ .i = -1 }	},
	{ MOD|Shift,         XK_comma,	focusmon,	{ .i = -1 }	},
	{ MOD,               XK_comma,	focusmon,	{ .i = -1 }	},
	{ MOD|Shift,        XK_period,	tagmon,		{ .i = +1 }	},
	{ MOD|Shift,        XK_period,	focusmon,	{ .i = +1 }	},
	{ MOD,              XK_period,	focusmon,	{ .i = +1 }	},

				/* layouts */
	{ MOD,		XK_backslash,   cyclelayout,	{ .i = +1 }	},
	{ MOD|Shift,	XK_backslash,   cyclelayout,	{ .i = -1 }	},
//	{ MOD,			XK_t,	setlayout,	{.v = &layouts[0]} },

				/* gaps */
	{ MOD,			XK_f,	incrgaps,	{.i = +3 }	},
	{ MOD,			XK_g,	incrgaps,	{.i = -3 }	},
	{ MOD|Shift,		XK_a,	togglegaps,		{0}	},
	{ MOD|Ctrl,		XK_a,	defaultgaps,		{0}	},
	{ MOD|Ctrl|Shift,	XK_a,	togglesmartgaps,	{0}	},

			/* Custom bindings (may be better using shkd) */
					/* Media */
	{ MOD|Shift,		XK_minus,		SHCMD("mpc volume -3")	},
	{ MOD|Shift,		XK_equal,		SHCMD("mpc volume +3")	},
	{ MOD|Shift,		XK_bracketleft,		SHCMD("mpc seek -10")	},
	{ MOD|Shift,		XK_bracketright,	SHCMD("mpc seek +10")	},
	{ MOD,			XK_minus,	SHCMD("pamixer -d 3; xsetroot -name 8")},
	{ MOD,			XK_equal,	SHCMD("pamixer --allow-boost -i 3; xsetroot -name 8")},
	{ MOD,			XK_BackSpace,	SHCMD("pamixer -t; xsetroot -name 8")},
	{ 0,	XF86XK_AudioPrev,		SHCMD("mpc prev && mpdnoti 900; xsetroot -name 11")	},
	{ 0,	XF86XK_AudioNext,		SHCMD("mpc next && mpdnoti 900; xsetroot -name 11")	},
	{ MOD,		XK_p,		SHCMD("[ $(mpc status '%state%') = 'paused' ] && \
			{ mpc play && mpdnoti 2000 ;} || mpc pause; xsetroot -name 11")	},
	{ MOD,	XK_bracketleft,		SHCMD("mpc prev && mpdnoti 900; xsetroot -name 11")	},
	{ MOD,	XK_bracketright,	SHCMD("mpc next && mpdnoti 900; xsetroot -name 11")	},
	{ MOD|Ctrl,	XK_p,		SHCMD("mpdnoti")		},
	{ 0,	XF86XK_AudioLowerVolume,	SHCMD("pamixer -d 2; xsetroot -name 8")},
	{ 0,	XF86XK_AudioRaiseVolume,	SHCMD("pamixer --allow-boost -i 2; xsetroot -name 8")},
	{ 0,	XF86XK_AudioMute,		SHCMD("pamixer -t; xsetroot -name 8")	},
//{ 0,	XF86XK_Calculator,		SHCMD("sleep 0.2 ; scrot -se 'mv $f ~/Downloads'") },
//{ 0, XF86XK_ScreenSaver,		SHCMD("slock & xset dpms force off; mpc pause; pauseallmpv") },
//	{ 0,	XF86XK_AudioStop,		SHCMD("mpc toggle)		},
//	{ 0,	XF86XK_Sleep,			SHCMD("sudo zzz")		},
	{ 0,	XF86XK_ScreenSaver,		SHCMD("xset dpms force off")		},
	{ 0,	XF86XK_MonBrightnessUp,		SHCMD("sudo brightnessctl -q set +1%")	},
	{ 0,	XF86XK_MonBrightnessDown,	SHCMD("sudo brightnessctl -q set 1%-")	},
	{ 0,	XF86XK_AudioPlay,		SHCMD("[ $(mpc status '%state%') = 'paused' ] && \
			{ mpc play && mpdnoti 2000 ;} || mpc pause; xsetroot -name 11")	},
			/* Custom bindings (may be better using shkd) */
	{ MOD,		   XK_Return,	SHCMD("st")			},
	{ MOD,			XK_m,	SHCMD("st -e vifmrun")		},
	{ MOD|Shift,		XK_m,	SHCMD("samedirvifm")		},
	{ MOD|Shift,    XK_apostrophe,	SHCMD("clipctl disable && passmenu -i \
	-l 25 -p 'Passmenu:' && notify-send 'Password will be deleted on 45 seconds❌' ; clipctl enable")},
	{ MOD,			XK_b,	SHCMD("Books001")		},
	{ MOD|Shift,		XK_u,	SHCMD("bookmenu")		},
	{ MOD|Shift,		XK_b,	SHCMD("Boletin001")		},
	{ MOD,		        XK_c,	SHCMD("st -f 'Monofur Nerd Font:pixelsize=22:antialias=true:autohint=true' -e calcurse")},
	{ MOD,	         	XK_z,	SHCMD("redyt -r")		},
	{ MOD|Shift,	      	XK_z,	SHCMD("waldl")		},
	{ MOD,		    XK_grave,	SHCMD("dmenuunicode")		},
	{ MOD|Shift,	   XK_Return,	SHCMD("samedir &")		},
//	{ MOD,	        XK_semicolon,	SHCMD("dmenu_mpc")		},
//	{ MOD|Shift,	    XK_slash,	SHCMD("tuxi -q")		},
	{ MOD,			XK_u,	SHCMD("clipmagick")		},
	{ MOD|Shift,		XK_y,	SHCMD("termyt -r")		},
	{ MOD|Shift,		XK_y,	SHCMD("dmenuhandler")		},
	{ MOD,		    XK_slash,	SHCMD("dmenu_browser")		},
	{ MOD|Shift,	        XK_n,	SHCMD("xdotool click 1")	},
	{ MOD,			XK_t,	SHCMD("testi")			},
	{ MOD,		   XK_Escape,	SHCMD("sysfunctions")	},
//	{ MOD,			XK_e,	SHCMD("st -t New-e newsboat -q; pkill -RTMIN+6 dwmblocks") },
	{ MOD,			XK_r,	SHCMD("st -t NewsBoat -e newsboat -q") },
//	{ MOD,			XK_F2,	SHCMD("dmenu_man")	},
//	{ MOD,	         	XK_F3,	SHCMD("dmenumount")	},
	{ MOD,	         	XK_F3,	SHCMD("dmenumount")	},
	{ MOD,		 	XK_F4,	SHCMD("dmenuumount")	},
//	{ MOD,		 	XK_F4,	SHCMD("syncthing & kill -55 $(pidof dwmblocks)") },
//	{ MOD,	         	XK_F6,	SHCMD("dmenumount")	},
//	{ MOD,		 	XK_F7,	SHCMD("dmenumountq")	},
	{ MOD,		 	XK_F8,	SHCMD("sleep 0.2 ; xdotool key Caps_Lock") },
	/* remove black bars on the screenshot, 90% percent accuracy */
	{ Shift,	XK_Print,	SHCMD("scrot -u -se 'mv $f ~/Downloads && \
	magick mogrify -fuzz 4% -define trim:percent-background=0% -trim +repage -format png ~/Downloads/$f'") },
	{ MOD,	XK_Print,	SHCMD("dmenurecord")	},
	{ 0,	XK_Print,	SHCMD("maim ~/Downloads/$(date +'%d-%m_%H_%M_%S').png")	},
/* panic key */ { MOD, XK_Delete, SHCMD("mpv '/home/faber/Media/Videos/Fight the Power!.mkv' --loop-file=inf --fs") },
};

/* button definitions
 * click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkTagBar,            0,		Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MOD,		Button1,        tag,            {0} },
	{ ClkTagBar,            MOD,		Button3,        toggletag,      {0} },
	{ ClkTagBar,            0,              Button4,	shiftview,	{.i = +1 } },
	{ ClkTagBar,            0,              Button5,	shiftview,	{.i = -1 } },

	{ ClkLtSymbol,          0,              Button1,        togglegaps,	{0} },
	{ ClkLtSymbol,          0,              Button2,        togglevacant,	{0} },
	{ ClkLtSymbol,          0,              Button3,        togglebar,	{0} },
//	{ ClkLtSymbol,          0,              Button1,        cyclelayout,    {.i = +1 } },
//	{ ClkLtSymbol,          0,              Button3,        cyclelayout,    {.i = -1 } },
	{ ClkLtSymbol,          0,              Button4,        cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button5,        cyclelayout,    {.i = -1 } },
//	{ ClkLtSymbol,          0,              Button4,        focusstack,	{.i = +1} },
//	{ ClkLtSymbol,          0,              Button5,        focusstack,	{.i = -1} },

	{ ClkWinTitle,          0,    Button1,	SHCMD("maim -usDq ~/Downloads/$(date +'%d-%m_%H_%M_%S').png") },
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
	{ ClkStatusText,        Shift,      Button1,        sendstatusbar,   {.i = 6 } },

	{ ClkClientWin,         MOD,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MOD,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MOD,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         MOD,         Button4,        resizemousescroll, {.v = &scrollargs[0]} },
	{ ClkClientWin,         MOD,         Button5,        resizemousescroll, {.v = &scrollargs[1]} },
	{ ClkClientWin,         MOD,         Button6,        resizemousescroll, {.v = &scrollargs[2]} },
	{ ClkClientWin,         MOD,         Button7,        resizemousescroll, {.v = &scrollargs[3]} },
//	{ ClkClientWin,   MOD|Shift,     Button4,        focusstack,	{.i = 1 } },
//	{ ClkClientWin,   MOD|Shift,     Button5,        focusstack,	{.i = -1 } },
};
