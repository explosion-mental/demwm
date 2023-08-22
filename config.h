/* See LICENSE file for copyright and license details. */

#define CI	static const int
#define CUI	static const unsigned int
#define CF	static const float

/* misc */
CUI systraypinning = 0; /* 0: sloppy systray follows selected monitor, >0: pin systray to monitor X */
CUI systraypinningfailfirst = 1; /* 1: if pinning fails, display systray on the first monitor, 0: display systray on the last monitor*/
CI  barh         = 9;      /* 1 or more/less means bar height */
CUI borderpx     = 5;      /* border pixel of tiled windows */
CUI fborderpx    = 2;      /* border pixel of floating windows */
CUI snap         = 10;     /* number of pixels until the window snaps to the screen border, while moving */
CUI gappih       = 15;     /* horizontal inner gap between windows */
CUI gappiv       = 20;     /* vertical inner gap between windows */
CUI gappoh       = 15;     /* horizontal outer gap between windows and screen edge */
CUI gappov       = 20;     /* vertical outer gap between windows and screen edge */
CUI hidevacant   = 1;      /* 1 means hide vacant tags */
CUI hovertofocus = 0;      /* 0 means click to focus */
CUI topbar       = 1;      /* 0 means bottom bar */
CUI gapspertag   = 1;      /* 0 means global gaps across all tags (default) */
CUI scalepreview = 4;      /* tag previews scaling (scalepreview / screensize) */
CUI swallowfloat = 0;      /* 1 means swallow floating windows by default */
CUI swallowffs   = 1;      /* 1 means swallow fullscreen windows by default */
CUI underlntitle = 1;      /* 1 means an underline on the title */
CF  mfact        = 0.55;   /* factor of master area size [0.05..0.95] */
CUI nmaster      = 1;      /* number of clients in master area */
CUI resizehints  = 0;      /* 1 means respect size hints in tiled resizals */
CUI floathints   = 0;      /* 1 means respect size hints if the window is floating */
CUI movefloat    = 22;     /* used in movfh_setmfact and movfv_pushstack */
static int smartgaps  = 0; /* 1 means no outer gap when there is only one window */
static int showbar    = 1; /* 0 means no bar */
static int showstatus = 1; /* 0 means no status text */
#define baralpha     120   /* bar opacity from 0 to 255 (Solid) */

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

/* colors */
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
	[SchemeTitle]  = { color0,	color1 }, /* window title */
	[SchemeStatus] = { fg_wal,	color0 }, /* status text */
	[SchemeDelim]  = { fg_wal,	color0 }, /* delimiter */
	[SchemeSys]    = { color2,	color2 }, /* system tray */
	[SchemeIndUrg] = { fg_wal,	color0 }, /* rectangle on urg tag */
	[SchemeIndOn]  = { color4,	color0 }, /* rectangle on sel tag */
	[SchemeIndOff] = { color2,	color0 }, /* rectangle on norm tag */
	[BorderNorm]   = { bg_wal },	/* normal window border */
	[BorderSel]    = { color2 },	/* selected window border */
	[BorderFloat]  = { color6 },	/* selected floating window border */
	[BorderUrg]    = { fg_wal },	/* urgent window border */
	/* custom blocks schemes */
	[SchemeLast+0] = { color7,	"#222222" },
	[SchemeLast+1] = { color2,	color0 },
	[SchemeLast+2] = { color0,	color2, },
	[SchemeLast+3] = { color0,	color6 },
	[SchemeLast+4] = { bg_wal,	color3 },
	[SchemeLast+5] = { color4,	color0 },
};
static const unsigned int alphas[][2] = { /* Schemes opacity */
			/* fg		bg	 */
	[SchemeNorm]   = { Solid,	baralpha },
	[SchemeSel]    = { Solid,	baralpha },
	[SchemeUrgent] = { Solid,	baralpha },
	[SchemeLt]     = { Solid,	baralpha },
	[SchemeTitle]  = { Solid,	baralpha + 40},
	[SchemeStatus] = { Solid,	baralpha + 40},
	[SchemeDelim]  = { Solid,	baralpha + 40},
	[SchemeSys]    = { Solid,	baralpha },
	[SchemeIndUrg] = { Solid,	baralpha },
	[SchemeIndOn]  = { Solid,	baralpha },
	[SchemeIndOff] = { Solid,	baralpha },
	[BorderNorm]   = { Solid },
	[BorderSel]    = { baralpha },
	[BorderFloat]  = { baralpha },
	[BorderUrg]    = { Solid },
	/* custom blocks schemes */
	[SchemeLast+0] = { Solid,	Solid },
	[SchemeLast+1] = { Solid,	baralpha + 40 },
	[SchemeLast+2] = { Solid,	baralpha + 40 },
	[SchemeLast+3] = { Solid,	baralpha + 40 },
	[SchemeLast+4] = { Solid,	baralpha + 40 },
	[SchemeLast+5] = { Solid,	baralpha + 40 },
};

/* Status Text:
 * foreground and background colors of a block are defined in a scheme, see
 * colors[]. To add one you will need to use SchemeLast + X, where X is 0 and
 * it increments every time and make sure to also add the alphas[] value. */
static const Block blocks[] = {
	/*   scheme         command                  interval  signal */
	{ SchemeLast+0, "sb-clock",			20,	1},
	{ SchemeLast+1, "sb-disk",			9000,	2},
	{ SchemeLast+2, "sb-battery",			10,	3},
	{ SchemeLast+3, "sb-internet",			10,	4},
	{ SchemeStatus, "sb-mailbox",			0,	5},
	{ SchemeStatus, "sb-moonphase",			0,	6},
	{ SchemeStatus, "sb-forecast",			0,	7},
	{ SchemeStatus, "sb-volume",			0,	8},
	{ SchemeStatus, "sb-pacpackages",		0,	9},
	{ SchemeStatus, "sb-sync",			0,	10},
//	{ SchemeStatus, "sb-mpc",			0,	26},
	{ SchemeLast+4, "sb-music",			0,	11},
//	{ SchemeStatus, "sb-tasks",			10,	12},
	{ SchemeLast+5,  "sb-notes",			0,	13},
	{ SchemeStatus, "echo '';cat /tmp/recordingicon",0,	14},
//	{ SchemeStatus, "sb-count",			0,	21},
//	{ SchemeStatus, "sb-price btc Bitcoin💰",	9000,	21},
//	{ SchemeStatus, "sb-price eth Ethereum🍸",	9000,	23},
//	{ SchemeStatus, "sb-price xmr Monero🔒",	9000,	24},
//	{ SchemeStatus, "sb-price link Chainlink🔗",	300,	25},
//	{ SchemeStatus, "sb-price bat BAT🦁",		9000,	20},
//	{ SchemeStatus, "sb-price lbc LBRY📚",		9000,	22},
//	{ SchemeStatus, "sb-cpu",			10,	18},
//	{ SchemeStatus, "sb-kbselect",			0,	30},
//	{ SchemeStatus, "sb-memory",			10,	14},
//	{ SchemeStatus, "sb-torrent",			20,	7},
//	{ SchemeStatus, "sb-crypto",			0,	13},
//	{ SchemeStatus, "sb-help-icon",			0,	15},
//	{ SchemeStatus, "sb-nettraf",			1,	16},
//	{ SchemeStatus, "sb-news",			0,	6},
//	{ SchemeStatus, "sb-xbpsup",			18000,	8},
};

/* 0 means render blocks left to right, the default, 1 to start from right to left */
#define INVERSED		1
/* max number of characters bits that one block command output can contain */
#define CMDLENGTH		65
/* delimeter between block comand outputs */
static const char delimiter[] = " ";

/* properties:			  res_name  res_class */
static XClassHint systrayclass = { "demwm", "demwm" };
static XClassHint barclass     = { "demwm", "demwm" };
static XClassHint previewclass = { "demwm-preview", "demwm-preview" };

/* tags: Usual text for the tags
 * tagsalt: Used if hidevacants is enabled
 * taglayouts: Index that indicates which layouts[] use */
static const char *tags[]     = { "📖", "", "💼", "", "🔬", "🎹", "📺", "💻", "🐧" };
static const int taglayouts[] = {    1,   1,    0,   0,    1,    0,    0,    0,    1 };
static const char *tagsalt[]  = { "I", "2", "III", "4", "V", "6", "VII", "8", "IX" };
static const unsigned int alltagslayout = 0; /* the '~0' (all tags) tag */

/* attach methods: these make reference to the position in which a new window
 *                 will take place. */
static void (*attachmodes[])(Client *) = {
	attachabove,	/* new clients above the selected client (Xmonad) */
	attachdefault,	/* new clients become the new master (default dwm) */
};

/* layouts */
static const Layout layouts[] = {
      /* symbol  arrange  gaps */
 	{ "[]=", tile },               /* master on left, slaves on right */
//	{ "||=", tilewide },               /* master on left, slaves on right */
//	{ "🧐" , monocle },            /* all windows on top of eachother */
 	{ "🔍" , alphamonocle },       /* monocle but windows aren't stacked */
//	{ "TTT", bstack },             /* master on top, slaves on bottom */
//	{ "🐚" , spiral },             /* fibonacci spiral */
//	{ "[\\]",dwindle },            /* decreasing in size right and leftward */
	{ "[D]", deck },               /* master on left, slaves in monocle mode on right */
//	{ ">M>", centeredfloatmaster}, /* centermaster but master floats */
//	{ "|M|", centeredmaster },     /* master in middle, slaves on sides */
//	{ "===", bstackhoriz },        /* bstack but slaves stacked "monocle"-like */
//	{ "HHH", grid },               /* windows in a grid */
	{ "###", nrowgrid },           /* gaplessgrid with no gaps, but not equal size */
//	{ "#v#", vsplitnrowgrid },     /* nrowgrid: force two clients to always split vertically */
//	{ "---", horizgrid },          /* gaplessgrid but with horizontal order */
//	{ ":::", gaplessgrid },        /* another grid like layout variant */
	{ "🥏", NULL },                /* no layout function means floating behavior */
	{ "[ ]", clear },              /* hides all visible clients, enjoy your nice wallpaper */
};

static const Rule rules[] = {
	/*
	 * Select a window you wish to assign a rule to with xprop.
	 *
	 * xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 *
	 * monitor: in which monitor the window will spawn
	 * tags: in which tags the window will spawn
	 *
	 * Assign a flag to the window, by default none are assign.
	 *
	 * Avaliable flags:
	 *
	 ** AlwOnTop: be 'on top' even while unfocused
	 ** Float: enable floating
	 ** Terminal: will make the window swallow child ones
	 ** FakeFS: enable fakefullscreen
	 ** NoSwallow: don't swallow this window
	 ** Sticky: show this window across all tags
	 ** UnCursor: puts the cursor in the bottom right whenever the window
	 * is focused (useful when there are gaps)
	 */
//	RULE(.class = "Gimp",		.tags = 1 << 7)
	RULE(.class = "vlc",		.tags = 1 << 6)
	RULE(.class = "qutebrowser",	.tags = 1 << 5)
	RULE(.class = "Surf",		.tags = 1 << 7)
	RULE(.class = "Element",	.tags = 1 << 7)
	RULE(.title = "Music",		.tags = 1 << 6)
	RULE(.title = "Sxiv - redyt",	.tags = 1 << 8)
	RULE(.title = "Sxiv - walld",	.tags = 1 << 8)
//	RULE(.class = "libreoffice",	.tags = 1 << 3)
//	RULE(.title = "LibreOffice",	.isfloating = 1, .noswallow = 1)
//	RULE(.class = "libreoffice",	.noswallow = 1)
	RULE(.class = "firefox",	.tags = 1 << 1, .flags = FakeFS)
	RULE(.class = "LibreWolf",	.tags = 1 << 1, .flags = FakeFS)
	RULE(.class = "Brave-browser",	.tags = 1 << 4, .flags = FakeFS)
	RULE(.class = "discord",	.tags = 1 << 7, .flags = FakeFS)
	RULE(.class = "St",		.flags = Terminal)
	RULE(.class = "Alacritty",	.flags = Terminal)
	RULE(.title = "Event Tester",	.flags = NoSwallow | Float) /* xev */
	/* floating */
	RULE(.class = "Pavucontrol",	.flags = Float)
	RULE(.class = "Pcmanfm",	.flags = Float)
	RULE(.title = "About Mozilla Firefox",	.flags = Float)
	RULE(.class = "QjackCtl",	.flags = Float)
	RULE(.title = "Firefox Update", .flags = Float)
	RULE(.title = "Krita - Edit Text", .flags = Float)
	RULE(.class = "Blueman-manager", .flags = Float)
	//customs
	RULE(.class = "Video",		.flags = Float)
	RULE(.class = "dialect",	.flags = Float)
	RULE(.title = "mpvfloat",	.flags = Float)
	RULE(.instance = "mpvfloat",	.tags = SPTAG(Sp9), .flags = Float)
	RULE(.title = "noswallow",	.flags = NoSwallow)

	/* scratchpads */
	RULE(.instance = "term",	.tags = SPTAG(Sp1), .flags = Float | Terminal)
	RULE(.instance = "notes",	.tags = SPTAG(Sp2), .flags = Float)
	RULE(.instance = "calc" ,	.tags = SPTAG(Sp3), .flags = Float)
	RULE(.instance = "pre"  ,	.tags = SPTAG(Sp4), .flags = Float)
	RULE(.instance = "music",	.tags = SPTAG(Sp5), .flags = Float)
	RULE(.instance = "pulsemixer",	.tags = SPTAG(Sp6), .flags = Float)
	RULE(.instance = "samedir",	.tags = SPTAG(Sp7), .flags = Float | Terminal)
	RULE(.instance = "testi",	.tags = SPTAG(Sp8), .flags = Float)
	//RULE(.instance = "normal",	.tags = SPTAG(7))
	//RULE(.instance = "emacsfloat",	.tags = SPTAG(8), .isfloating = 1)
};

/* key definitions, some lines below are avaliable modifiers */
#define MOD Super

#define TAGKEYS(KEY,TAG) /* keys from 1 to 9 */ \
	{ MOD,			KEY,	comboview,      { .ui = 1 << TAG } },	\
	{ MOD|Shift,		KEY,	combotag,       { .ui = 1 << TAG } },	\
	{ MOD|Ctrl,		KEY,	toggleview,     { .ui = 1 << TAG } },	\
	/*{ MOD|Ctrl,		KEY,	previewtag,	{ .ui = TAG }	   },*/	\
	/* { MOD|Ctrl|Shift, KEY,	toggletag,      { .ui = 1 << TAG } },*/	\
	{ MOD|Shift|Ctrl,	KEY,	swaptags,	{ .ui = 1 << TAG } },

#define SPKEYS(MKEYS,KEY,NUM) \
	{ MKEYS,		KEY,	togglescratch,	{ .ui = NUM } },

/* helper for spawning shell commands */
#define SHCMD(cmd)	spawn, { .v = (const char *[]){ "/bin/sh", "-c", cmd, NULL } }

/* helper for spawning shell commands directly (without calling /bin/sh) */
#define CMD(...)	spawn, { .v = (const char *[]){ __VA_ARGS__, NULL } }

/* helper for spawning shell commands in scratchpads */
#define SH(cmd)		{ "/bin/sh", "-c", cmd, NULL }

/* personal macros to not repeat myself */
#define DMENUARGS	"-nb", color0, "-nf", color8, "-sb", color2, "-sf", color0
#define NOTES		"-e", "nvim", "+$", "+startinsert!" /* last line in insert mode */
#define FURSIZE		"90x25"
#define FURFONT		"Monofur Nerd Font:pixelsize=20:antialias=true:autohint=true"
#define VOL(Q)		SHCMD("wpctl set-volume @DEFAULT_AUDIO_SINK@ " Q /*5%-+*/ "; demwm updateblock 8")
#define MUTE /*toggle mute*/ SHCMD("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle; demwm updateblock 8")
#define SHOT(FLAGS)	SHCMD("scrot " FLAGS " -e 'mv $f ~/Downloads && notify-send --icon=~/Downloads/$f ScreenShot_Taken $f -t 1000'")
#define MPCVOL(Q)	CMD("mpc", "volume", Q)
#define MPCNEXT		SHCMD("mpc next && mpdnoti 900; demwm updateblock 11")
#define MPCPREV		SHCMD("mpc prev && mpdnoti 900; demwm updateblock 11")
#define LIGHT(Q)	CMD("brightnessctl", "-q", "set", Q)
#define V(var)		spawn, { .v = var }

/* commands */
static const char *dmenucmd[]  = { "dmenu_run_i", DMENUARGS, NULL };
static const char *dmenuipc[]  = { "dmenu_dwmc", DMENUARGS, NULL };
static const char *samedmenu[] = { "samedirmenu", DMENUARGS, NULL };
static const char *clip[]      = { "clipmenu", "-i", "-l", "25", "-mon", "-1", DMENUARGS, NULL };
static const char *random_wall[] = { "demwm_random_wall", NULL };
static const char *rofcalc[]   = { "rofi", "-show", "calc", "-modi", "calc", "-no-show-match", "-no-sort", NULL };

/* scratchpads */
static const char *scratchpads[][32] = {
[Sp1] = { "st", "-n", "term", "-g", "115x30" , NULL }, /* terminal */
//[Sp2] = { "st", "-n", "notes", "-g", FURSIZE, "-f", FURFONT, NOTES, "/home/faber/Docs/testi/notes.md", NULL }, /* notes */
[Sp2] = { "neovide", "--x11-wm-class-instance", "notes", "--geometry", "70x25", "/home/faber/Docs/testi/notes.md", NULL }, /* notes */
[Sp3] = { "st", "-n", "calc", "-f", "monospace:size=16", "-g", "50x20", "-e", "bc", "-lq", NULL }, /* calculator */
[Sp4] = { "st", "-n", "pre", "-g", "70x25", NOTES, "/home/faber/Docs/testi/testi-is", NULL }, /* uni */
[Sp5] = { "st", "-n", "music", "-g", "105x27", "-f", FURFONT, "-e", "ncmpcpp", "-q", NULL }, /* music */
[Sp6] = SH("st -n pulsemixer -g 100x25 -f 'SauceCodePro Nerd Font: style=Mono Regular:size=12' -e pulsemixer"), /* pulsepad */
[Sp7] = { "samedir", "-n", "samedir", "-g", "115x30", NULL }, /* samedir */
//[Sp8] = { "st", "-n", "testi", "-g", FURSIZE, "-f", FURFONT, NOTES, "/home/faber/Docs/testi/testi.md", NULL }, /* notes */
[Sp8] = { "neovide", "--x11-wm-class-instance", "testi", "--geometry", "70x25", "/home/faber/Docs/testi/testi.md", NULL }, /* notes */
[Sp9] = { "st", "-n", "mpvfloat", "-g", "115x30" , NULL }, /* terminal */
};
/* modifier(s) can be: Alt, AltGr, Ctrl, Shift, ShiftGr, Super, or MOD
 * See the man page for a description of the functions and arguments */
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
	{ MOD,			XK_x,	V(rofcalc) },
	//SPKEYS(MOD|Ctrl,	XK_s,	/* uni		*/	Sp4)
	SPKEYS(MOD|Ctrl,	XK_s,	/* samedir	*/	Sp7)
	SPKEYS(MOD,		XK_n,	/* music	*/	Sp5)
	{ MOD|Shift,		XK_p,	updateblock,	{ .ui = 8 }	},
	SPKEYS(MOD|Shift,	XK_p,	/* pulsemixer	*/	Sp6)
	SPKEYS(MOD|Ctrl,	XK_e,	/* notes - is	*/	Sp8)
	SPKEYS(MOD|Shift,	XK_n,	/* notes - is	*/	Sp9)
			/* dynamic scratchpads */
	{ MOD|Ctrl,		XK_v,	scratchpad_remove,	{0} },
	{ MOD,                  XK_v,	scratchpad_show,	{0} },
	{ MOD|Shift,            XK_v,	scratchpad_hide,	{0} },
//	{ MOD|Ctrl,          	XK_v,	scratchpad_remove,	{0} },

				/* Navigation */
	{ MOD,		        XK_c,	swapfocus, 	{0} },
	//{ MOD,		        XK_Tab,	swapfocus, 	{0} },
	{ MOD,			XK_Tab,	focusstack,	{ .i = -1 }	},
	{ MOD,			XK_j,	focusstack,	{ .i = -1 }	},
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
	{ MOD,                  XK_k,	focusstack,	{ .i = 1 }	},

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
	//{ MOD,			XK_Tab,	view,			{0}	},
	{ MOD,			XK_space,view,			{0}	},
	{ MOD,			XK_w,	zoomswap,		{0}	},
	{ MOD|Shift,		XK_w,	focusmaster,		{0}	},
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
	{ MOD|Ctrl,	    XK_grave,	toggletagbar,		{0}	},
//	{ MOD|Ctrl,	    XK_grave,	togglebar,		{0}	},
	{ MOD,			XK_F1,	togglefullscreen,	{0}	},
	{ MOD|Ctrl,        	XK_F1,	togglefakefullscreen,	{0}	},
	{ MOD|Shift,		XK_F5,	quit,           	{0}	},
	{ MOD,			XK_F5,	restart,		{0}	},
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

			/* Custom bindings (maybe use shkd) */
					/* Media */
	{ MOD|Shift,		XK_minus,	MPCVOL("-3")	},
	{ MOD|Shift,		XK_equal,	MPCVOL("+3")	},
	{ MOD|Shift,		XK_bracketleft,	CMD("mpc", "seek", "-10")	},
	{ MOD|Shift,		XK_bracketright,CMD("mpc", "seek", "+10")	},
	{ MOD,			XK_minus,	VOL("3%-")	},
	{ MOD,			XK_equal,	VOL("3%+")	},
	{ MOD,			XK_BackSpace,	MUTE },
	{ 0,	XF86XK_AudioPrev,		MPCPREV	},
	{ 0,	XF86XK_AudioNext,		MPCNEXT	},
	{ MOD,			XK_p,	SHCMD("[ $(mpc status '%state%') = 'paused' ] && \
			{ mpc play && mpdnoti 2000 ;} || mpc pause; demwm updateblock 11")	},
	{ MOD,			XK_bracketleft,	 MPCPREV	},
	{ MOD,			XK_bracketright, MPCNEXT	},
	{ MOD|Ctrl,		XK_p,            CMD("mpdnoti")	},
	{ 0, XF86XK_AudioLowerVolume,	VOL("2%-")	},
	{ 0, XF86XK_AudioRaiseVolume,	VOL("2%+")	},
	{ 0, XF86XK_AudioMute,		MUTE },
//	{ 0, XF86XK_Calculator,		SHCMD("sleep 0.2 ; scrot -se 'mv $f ~/Downloads'") },
	{ 0, XF86XK_ScreenSaver,	CMD("xset", "dpms", "force", "off")	},
	{ 0, XF86XK_MonBrightnessUp,	LIGHT("+1%")	},
	{ 0, XF86XK_MonBrightnessDown,	LIGHT("1%-")	},
	{ 0, XF86XK_AudioPlay,		SHCMD("[ $(mpc status '%state%') = 'paused' ] && \
			{ mpc play && mpdnoti 2000 ;} || mpc pause; demwm updateblock 11")	},
	//{ 0, XF86RFKill,	LIGHT("1%-")	},


		 			/* one shooters */
	{ MOD,		   XK_Return,	CMD("st")			},
	{ MOD|Shift,	   XK_Return,	CMD("samedir")			},
	{ MOD,			XK_m,	CMD("st", "-e", "vifmrun")	},
	{ MOD|Shift,		XK_m,	CMD("samedirvifm")		},
	{ MOD|Ctrl,		XK_m,	SHCMD("maim -usDq ${HOME}/Downloads/$(date +'%d-%m_%H_%M_%S').png") },
	{ MOD,			XK_b,	CMD("Books001")		},
	{ MOD|Shift,		XK_u,	CMD("bookmenu")		},
	{ MOD,		        XK_r,	CMD("st", "-f", FURFONT, "-e", "calcurse")},
	{ MOD,	         	XK_z,	CMD("redyt", "-r")		},
	{ MOD|Shift,	      	XK_z,	CMD("waldl")		},
	{ MOD,		    XK_grave,	CMD("dmenuunicode")		},
//	{ MOD,	        XK_semicolon,	SHCMD("dmenu_mpc")		},
//	{ MOD|Shift,	    XK_slash,	SHCMD("tuxi -q")		},
	{ MOD,			XK_u,	CMD("clipmagick")		},
	{ MOD|Shift,		XK_y,	SHCMD("termyt -r")		},
	{ MOD,		    XK_slash,	CMD("dmenu_browser")		},
	{ MOD|Ctrl,	        XK_n,	CMD("xdotool", "click", "1")	},
	{ MOD,			XK_t,	CMD("testi")			},
	{ MOD,		   XK_Escape,	CMD("sysfunctions")	},
//	{ MOD,			XK_e,	SHCMD("st -t New-e newsboat -q; pkill -RTMIN+6 dwmblocks") },
//	{ MOD,			XK_r,	SHCMD("st -t NewsBoat -e newsboat -q") },
//	{ MOD,			XK_F2,	SHCMD("dmenu_man")	},
//	{ MOD,	         	XK_F3,	SHCMD("dmenumount")	},
	{ MOD,	         	XK_F3,	CMD("dmenumount")	},
	{ MOD,		 	XK_F4,	CMD("dmenuumount")	},
//	{ MOD,		 	XK_F4,	SHCMD("syncthing & kill -55 $(pidof dwmblocks)") },
//	{ MOD,	         	XK_F6,	SHCMD("dmenumount")	},
//	{ MOD,		 	XK_F7,	SHCMD("dmenumountq")	},
//	{ MOD,		 	XK_F8,	SHCMD("sleep 0.2 ; xdotool key Caps_Lock") },
	/* remove black bars on the screenshot (90% percent accuracy) */
	{ MOD,			XK_Print,CMD("dmenurecord")	},
				/* miscelaneous */
	{ MOD|Shift,    XK_apostrophe,	SHCMD("clipctl disable && passmenu -i \
	-l 25 -p 'Passmenu:' && notify-send 'Password will be deleted on 45 seconds❌' ; clipctl enable")},
	{ Shift,	XK_Print,	SHOT("--focused --select") },
	/*{ Shift,	XK_Print,	SHCMD("scrot -u -se 'mv $f ~/Downloads &&
	magick mogrify -fuzz 4% -define trim:percent-background=0% -trim +repage -format png ~/Downloads/$f'") }, */
	{ 0,			XK_Print,	SHOT("--focused") },
/* panic key */ { MOD, XK_Delete, SHCMD("mpv '/home/faber/Media/Videos/Fight the Power!.mkv' --loop-file=inf --fs") },
};

/* mouse button definitions */

/* 1 means resize window by 1 pixel for each scroll event on resizemousescroll */
#define scrollsensetivity	18
static const int scrollargs[4][2] = {
	/* width change         height change */
	{ -scrollsensetivity,	0 },
	{ +scrollsensetivity,	0 },
	{ 0, 			-scrollsensetivity },
	{ 0, 			+scrollsensetivity },
};

/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                modifier(s)      button          function        argument */
	{ ClkTagBar,            0,		Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MOD,		Button1,        tag,            {0} },
	{ ClkTagBar,            MOD,		Button3,        toggletag,      {0} },
	{ ClkTagBar,            0,              Button4,	shiftview,	{.i = +1 } },
	{ ClkTagBar,            0,              Button5,	shiftview,	{.i = -1 } },

	{ ClkLtSymbol,          0,              Button1,        togglegaps,	{0} },
	{ ClkLtSymbol,          0,              Button2,        togglebar,	{0} },
	{ ClkLtSymbol,          0,              Button3,        togglevacant,	{0} },
//	{ ClkLtSymbol,          0,              Button1,        cyclelayout,    {.i = +1 } },
//	{ ClkLtSymbol,          0,              Button3,        cyclelayout,    {.i = -1 } },
	{ ClkLtSymbol,          0,              Button4,        cyclelayout,    {.i = +1 } },
	{ ClkLtSymbol,          0,              Button5,        cyclelayout,    {.i = -1 } },
//	{ ClkLtSymbol,          0,              Button4,        focusstack,	{.i = +1} },
//	{ ClkLtSymbol,          0,              Button5,        focusstack,	{.i = -1} },

	{ ClkWinTitle,          0,    Button1,	SHCMD("maim -usDq ~/Downloads/$(date +'%d-%m_%H:%M:%S_%g').png") },
//	{ ClkWinTitle,          0,              Button2,	zoomswap,       {0} },
//	{ ClkWinTitle,          0,              Button2,	killclient,	{0} },
	{ ClkWinTitle,		0,		Button3,	SHOT("--focused --select") },
	{ ClkWinTitle,          0,              Button4,        focusstack,	{.i = +1 } },
	{ ClkWinTitle,          0,              Button5,        focusstack,	{.i = -1 } },

	{ ClkStatusText,        0,              Button1,        sendstatusbar,   {.i = 1 } },
	{ ClkStatusText,        0,              Button2,        sendstatusbar,   {.i = 2 } },
	{ ClkStatusText,        0,              Button3,        sendstatusbar,   {.i = 3 } },
	{ ClkStatusText,        0,              Button4,        sendstatusbar,   {.i = 4 } },
	{ ClkStatusText,        0,              Button5,        sendstatusbar,   {.i = 5 } },
	{ ClkStatusText,        Shift,		Button1,        sendstatusbar,   {.i = 6 } },

	{ ClkClientWin,         MOD,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MOD|Shift,   Button1,        resizemouse,    {0} },
	{ ClkClientWin,         MOD,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MOD,         Button3,        resizemouse,    {0} },
	{ ClkClientWin,         MOD,         Button4,        resizemousescroll, {.v = &scrollargs[0]} },
	{ ClkClientWin,         MOD,         Button5,        resizemousescroll, {.v = &scrollargs[1]} },
	{ ClkClientWin,         MOD,         Button6,        resizemousescroll, {.v = &scrollargs[2]} },
	{ ClkClientWin,         MOD,         Button7,        resizemousescroll, {.v = &scrollargs[3]} },
//	{ ClkClientWin,         MOD|Shift,   Button1, SHCMD("maim -usDq ~/Downloads/$(date +'%d-%m_%H_%M_%S').png") },
//	{ ClkClientWin,   MOD|Shift,     Button4,        focusstack,	{.i = 1 } },
//	{ ClkClientWin,   MOD|Shift,     Button5,        focusstack,	{.i = -1 } },
};
