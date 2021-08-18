/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int systraypinning = 0;   /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
static const unsigned int systrayspacing = 2;   /* systray spacing */
static const int systraypinningfailfirst = 1;   /* 1: if pinning fails, display systray on the first monitor, False: display systray on the last monitor*/
static const int scalepreview = 4;        /* Tag previews scaling */
static unsigned int borderpx  = 3;        /* border pixel of windows */
static unsigned int snap      = 32;       /* snap pixel */
static unsigned int gappih    = 15;       /* horiz inner gap between windows */
static unsigned int gappiv    = 20;       /* vert inner gap between windows */
static unsigned int gappoh    = 15;       /* horiz outer gap between windows and screen edge */
static unsigned int gappov    = 20;       /* vert outer gap between windows and screen edge */
static int hidevacant         = 0;        /* 1 means hide vacant tags */
static int swallowfloating    = 0;        /* 1 means swallow floating windows by default */
static int smartgaps          = 0;        /* 1 means no outer gap when there is only one window */
static int showbar            = 1;        /* 0 means no bar */
static int topbar             = 1;        /* 0 means bottom bar */
static const int barh         = 4;        /* 1 or more, means bar height */
static const int pertag       = 1;        /* 0 means global layout across all tags (default) */
static const int pertagbar    = 0;        /* 0 means using pertag, but with the same barpos */
static const int gapspertag   = 1;        /* 0 means global gaps across all tags (default) */
//static const unsigned int baralpha    = 185;	/* Bar opacity (0-255) */
static const unsigned int baralpha    = 160;	/* Bar opacity (0-255) */
static const unsigned int borderalpha = OPAQUE;	/* Borders (0xffU) */
static char *fonts[] = {
	"Hack Nerd Font:pixelsize=12:antialias=true:autohint=true", /* Powerline */
//	"SauceCodePro Nerd Font:pixelsize=14:antialias=true:autohint=true",
//	"Noto Color Emoji:pixelsize=16:antialias=true:autohint=true: style=Regular", /* Emojis */
	"JoyPixels:pixelsize=14:antialias=true:autohint=true"
};
/* Pywal */
static char color0[8], color1[8], color2[8], color3[8], color4[8], color5[8], color6[8], color7[8], color8[8];
static char bg_wal[8], fg_wal[8], cursor_wal[8];
static char *colors[][3]	      = {
			/* fg		bg		border	    description		*/
	[SchemeNorm]   = { fg_wal,	color0,		color0 }, /* Normal tags section */
	[SchemeSel]    = { color0,	color1,		color2 }, /* Selected tag*/
	[SchemeLt]     = { color2,	color0,		NULL },   /* Layout*/
	[SchemeTitle]  = { color0,	color2,		NULL },   /* window title*/
	[SchemeStatus] = { color3,	color0,		color0 },   /* StatusBar*/
	[SchemeUrgent] = { fg_wal,	color0,		fg_wal }, /* background color for urgent tag*/
	[SchemeNotify] = { fg_wal,	color0,		NULL },   /* Little red bar on urgent tag*/
	[SchemeIndOn]  = { color4,	color0,		NULL },   /* rectangle on active tag*/
	[SchemeIndOff] = { color2,	color0,		NULL },   /* rectablge on def tag*/
};
static const unsigned int alphas[][3] = {
			/* fg		bg		border     */
	[SchemeNorm]   = { OPAQUE,	baralpha,	0 },
	[SchemeSel]    = { OPAQUE,	baralpha,	borderalpha },
	[SchemeLt]     = { OPAQUE,	baralpha,	borderalpha },
	[SchemeStatus] = { OPAQUE,	baralpha,	borderalpha },
	[SchemeTitle]  = { 255,		255,		255 },
};

/* tags */
static const char *tags[]     = { "📖", "", "💼", "", "🔬", "🎹", "📺", "💻", "🐧" };
static const int taglayouts[] = {    0,   1,    0,   0,    0,    0,    0,    0,    0 };

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 *	_NET_WM_WINDOW_TYPE(ATOM) = wintype
	 * tags, isfloating, isterminal, noswallow, isfakefullscreen
	 * Order is actually important here (e.g. if .noswallow is before
	 * .isterminal then it would not take effect)
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

	RULE(.instance = "notes",	.tags = SPTAG(0), .isfloating = 1)
	RULE(.instance = "calc" ,	.tags = SPTAG(1), .isfloating = 1)
	RULE(.instance = "pre"  ,	.tags = SPTAG(2), .isfloating = 1)
	RULE(.instance = "term",	.tags = SPTAG(3), .isfloating = 1)
	RULE(.instance = "music",	.tags = SPTAG(4), .isfloating = 1)
	RULE(.instance = "pulsemixer",	.tags = SPTAG(5), .isfloating = 1)
	RULE(.instance = "term",	.tags = SPTAG(6), .isfloating = 1)
	RULE(.instance = "normal",	.tags = SPTAG(7))
	RULE(.instance = "emacsfloat",	.tags = SPTAG(8), .isfloating = 1)
};

/* layout(s) */
static float mfact     = 0.55;	/* factor of master area size [0.05..0.95] */
static int nmaster     = 1;	/* number of clients in master area */
static int resizehints = 0;	/* 1 means respect size hints in tiled resizals */

/* Uncomment layouts you want */
#define TILE
#define MONOCLE
#define ALPHAMONOCLE
#define CENTEREDFLOATMASTER
//#define BSTACK
#define SPIRAL
//#define DWINDLE
#define DECK
//#define CENTEREDMASTER
//#define BSTACKHORIZ
#define GRID
//#define NROWGRID
//#define FORCE_VSPLIT 1	/* nrowgrid: force two clients to always split vertically */
//#define HORIZGRID
//#define GAPLESSGRID
//#define PIDGIN
//#define EGO
#include "layouts.c"

static const Layout layouts[] = {
	/* symbol	arrange function			Description			*/
 	{ "[]=",	tile },			/* Master on left, slaves on right */
 	{ "🧐",		monocle },		/* All windows on top of eachother */
 	{ "{}",		alphamonocle },		/* monocle but windows aren't stacked */
	//{ "TTT",	bstack },		/* Master on top, slaves on bottom */
	{ "🐚",		spiral },		/* Fibonacci spiral */
	//{ "[\\]",	dwindle },		/* Decreasing in size right and leftward */
	{ "[D]",	deck },			/* Master on left, slaves in monocle mode on right */
	{ ">M>",	centeredfloatmaster},	/* Centermaster but master floats */
	//{ "|M|",	centeredmaster },	/* Master in middle, slaves on sides */
	//{ "===",      bstackhoriz },		/* Bstack but slaves stacked "monocle"-like */
	{ "HHH",      grid },			/* windows in a grid */
	//{ "###",      nrowgrid },		/* Gaplessgrid with no gaps, but not equal size */
	//{ "---",      horizgrid },		/* Gaplessgrid but with horizontal order */
	//{ ":::",      gaplessgrid },		/* grid ajusted in such that there are no gaps */
	//{ "🐷", 	pidgin },		/* Basically grid? */
	//{ ")M(",	ego },
	//{ "🥏",	NULL },			/* no layout function means floating behavior */
	{ NULL,		NULL },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,	view,           { .ui = 1 << TAG } }, \
	{ MODKEY|ShiftMask,             KEY,	tag,            { .ui = 1 << TAG } }, \
	{ MODKEY|ControlMask,           KEY,	toggleview,     { .ui = 1 << TAG } }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,	toggletag,      { .ui = 1 << TAG } }, \
	{ MODKEY|Mod1Mask,		KEY,	swaptags,	{ .ui = 1 << TAG } },
#define SCRATCHKEYS(MOD,KEY,NUM) \
	{ MOD,			KEY,	togglescratch,	{ .ui = NUM } },
/* helper for spawning shell commands in the pre dwm-5.0 fashion, maybe use shkd? */
#define SHCMD(cmd)	spawn, { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }
/* helper macro for the custom command of mine 'spawncmd' */
#define CMDCMD(cmd) 	spawncmd,{ .v = cmd }
/* macro for any dmenu commands, colorize them */
#define DMENUARGS "-m", dmenumon, "-nb", color0, "-nf", color8, "-sb", color2, "-sf", color0
/* Helper for defining commands */
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
static const char *vifm[]      = { "sh",  "-c", "st -t 'FileManager🗄️' -e vifmrun", NULL };
static const char *samevifm[]  = { "samedirvifm", NULL };
//EXEC(samedmenu, "samedirmenu"

/* scratchpads */
#define NOTES		"-e", "nvim", "+$", "+startinsert!"
const char *spname[] = { "notes", "calc", "pre", "term", "music", "pulsemixer", "term" };
static Sp scratchpads[9];
const char *spcmd0[] = { "st", "-n", "notes", "-g", "100x25", NOTES, "/home/faber/Docs/testi/testi", NULL };
const char *spcmd1[] = { "st", "-n", "calc", "-f", "monospace:size=16", "-g", "50x20", "-e", "bc", "-lq", NULL };
const char *spcmd2[] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/pre-Uni.txt", NULL };
const char *spcmd3[] = { "st", "-n", "term", "-g", "115x30" , NULL };
const char *spcmd4[] = { "st", "-n", "music", "-g", "105x27",  "-f", "SauceCodePro Nerd Font: style=Mono Regular:size=12", "-e", "ncmpcpp", "-q", NULL };
const char *spcmd5[] = { "st", "-n", "pulsemixer", "-g", "110x28", "-e", "pulsemixer", NULL };
const char *spcmd6[] = { "samedir", "-n", "term", "-g", "115x30", NULL };
const char *spcmd7[] = { "st", "-n", "normal", NULL };
const char *spcmd8[] = { "emacs", "--name", "emacsfloat", "-g", "115x30", NULL };

static Key keys[] = {
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
	//SCRATCHKEYS(MODKEY,		XK_e,	/* notes	*/	0)
	SCRATCHKEYS(MODKEY,		XK_x,	/* calculator	*/	1)
	SCRATCHKEYS(MODKEY|ControlMask,	XK_s,	/* uni		*/	2)
	SCRATCHKEYS(MODKEY,		XK_s,	/* terminal	*/	3)
	SCRATCHKEYS(MODKEY,		XK_n,	/* music	*/	4)
	SCRATCHKEYS(MODKEY|ShiftMask,	XK_p,	/* pulsemixer	*/	5)
	SCRATCHKEYS(MODKEY|ShiftMask,	XK_s,	/* samedir	*/	6)
	SCRATCHKEYS(MODKEY|ControlMask,	XK_e,	/* stnormal	*/	7)
	SCRATCHKEYS(MODKEY,		XK_v,	/* emacs	*/	8)
//	{ MODKEY,		   XK_Num_Lock,	togglescratch,	{.ui = 1 } },/* bc */

				/* Navigation */
	{ MODKEY,			XK_j,	focusstack,	{ .i = INC(1) }		},
	{ MODKEY|ShiftMask,		XK_j,	pushstack,	{ .i = INC(1) }		},

	{ MODKEY|ControlMask,		XK_j,	shifttag,	{ .i = -1 }		},
	{ MODKEY|ControlMask|ShiftMask,	XK_j,  shifttagclients,	{ .i = -1 }		},
	{ MODKEY|ControlMask|ShiftMask,	XK_k,  shifttagclients,	{ .i = +1 }		},
	{ MODKEY|ControlMask,		XK_k,	shifttag,	{ .i = +1 }		},

	{ MODKEY|ShiftMask,		XK_k,	pushstack,	{ .i = INC(-1) }	},
	{ MODKEY,                       XK_k,	focusstack,	{ .i = INC(-1) }	},

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
	{ MODKEY,                       XK_w,	zoom,			{0}		},
	{ MODKEY,	              XK_Tab,	view,			{0}		},
	{ MODKEY,	            XK_space,	view,			{0}		},
	{ MODKEY|ControlMask,	    XK_grave,	togglebar,		{0}		},
//	{ MODKEY|ShiftMask,	    XK_grave,	toggletopbar,		{0}		},
//	{ MODKEY|ShiftMask,             XK_c,	quit,           	{0}		},
	{ MODKEY,			XK_F5,	/*restart*/	quit,	{1}		},
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
	{ MODKEY|ControlMask,		XK_b,	nostatus,	{ .i = 1  }		},
	{ MODKEY|ControlMask,		XK_n,	nostatus,	{ .i = -1 }		},

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
{ 0, XF86XK_AudioLowerVolume,	SHCMD("pamixer --allow-boost -d 3; kill -44 $(pidof dwmblocks)") },
{ 0, XF86XK_AudioRaiseVolume,	SHCMD("pamixer --allow-boost -i 3; kill -44 $(pidof dwmblocks)") },
{ 0, XF86XK_AudioMute,		SHCMD("pamixer -t; kill -44 $(pidof dwmblocks)") },
{ MODKEY,	XK_minus,	SHCMD("pamixer --allow-boost -d 3; kill -44 $(pidof dwmblocks)") },
{ MODKEY,	XK_equal,	SHCMD("pamixer --allow-boost -i 3; kill -44 $(pidof dwmblocks)") },
{ MODKEY,	 XK_BackSpace,	SHCMD("pamixer -t; kill -44 $(pidof dwmblocks)") },
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
	{ MODKEY,		XK_p,		SHCMD("mpc toggle")			},
	{ MODKEY,	XK_bracketleft,		SHCMD("mpc prev")			},
	{ MODKEY,	XK_bracketright,	SHCMD("mpc next")			},
	{ MODKEY|ControlMask,	XK_p,		SHCMD("mpdnoti")			},

			/* Custom bindings (may be better using shkd) */
	{ MODKEY,			XK_b,	CMDCMD("Books001")		},
	{ MODKEY|ShiftMask,		XK_u,	SHCMD("bookmenu")		},
	{ MODKEY|ShiftMask,		XK_b,	SHCMD("Boletin001")		},
	{ MODKEY,		        XK_c,	SHCMD("st -e calcurse")		},
	{ MODKEY,	         	XK_z,	SHCMD("redyt -r")		},
	{ MODKEY|ShiftMask,	      	XK_z,	SHCMD("walldown")		},
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
	{ 0, XK_Print,	SHCMD("scrot -u -se 'mv $f ~/Downloads && \
		magick mogrify -fuzz 4% -define trim:percent-background=0% -trim +repage -format png ~/Downloads/$f'") },
	{ MODKEY,		    XK_Print,	SHCMD("dmenurecord")		},
	{ ShiftMask,		    XK_Print,	SHCMD("scrot")			},
};

/* button definitions
 * click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button3,        cyclelayout,    {.i = -1 } },
	{ ClkLtSymbol,          0,              Button4,        cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button5,        cyclelayout,    {.i = -1 } },
//	{ ClkLtSymbol,          0,              Button4,        focusstack,	{.i = INC(1)} },
//	{ ClkLtSymbol,          0,              Button5,        focusstack,	{.i = INC(-1)} },

//	{ ClkWinTitle,          0,              Button1,	SHCMD("sleep 0.2 ; scrot -se 'mv $f ~/Downloads'") },
	{ ClkWinTitle,          0,              Button1,	SHCMD("maim -sDq ~/Downloads/$(date +'%d-%m_%H_%M_%S').png") },
	{ ClkWinTitle,          0,              Button2,	zoom,           {0} },
	{ ClkWinTitle,          0,              Button2,	killclient,	{0} },
	{ ClkWinTitle,		0,		Button3,	SHCMD("scrot -us -e 'mv $f ~/Downloads'") },
	{ ClkWinTitle,          0,              Button4,        focusstack,	{.i = INC(1)} },
	{ ClkWinTitle,          0,              Button5,        focusstack,	{.i = INC(-1)} },

	{ ClkStatusText,        0,              Button1,        sigdwmblocks,   {.i = 1 } },
	{ ClkStatusText,        0,              Button2,        sigdwmblocks,   {.i = 2 } },
	{ ClkStatusText,        0,              Button3,        sigdwmblocks,   {.i = 3 } },
	{ ClkStatusText,        0,              Button4,        sigdwmblocks,   {.i = 4 } },
	{ ClkStatusText,        0,              Button5,        sigdwmblocks,   {.i = 5 } },
	{ ClkStatusText,        ShiftMask,      Button1,        sigdwmblocks,   {.i = 6 } },

	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },

	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
	{ ClkTagBar,            0,              Button4,	shiftview,	{ .i = +1 } },
	{ ClkTagBar,            0,              Button5,	shiftview,	{ .i = -1 } },
};
