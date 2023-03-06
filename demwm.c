/* See LICENSE file for copyright and license details.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys, Buttons and Rules are organized as arrays and defined in config.h.
 *
 * Most of dwm/demwm codebase is around handling X events, thus being a window
 * manager. We access the X server by the good old Xlib library (demwm uses the
 * xcb lib to get the pid, swallow patch). Whenever we need to handle an event,
 * we call handler, array that contains the calls and function to handle them
 * (how to handle them).
 *
 * Most of that X stuff you won't need to touch, unless you plan on
 * refactoring, go to golf, logic hunting and all for the precious (most likely
 * micro) optimizations. The functions that take the struct type Arg as an
 * argument is the 'user functions', you can take a look how they do things.
 *
 * Every window that is spawned goes through by manage(), so it is another good
 * start.
 *
 * To understand everything else, start reading main(), commit messages,
 * www.x.org/docs/X11/xlib.pdf and the comments I left, happy hacking.
 */
#include <errno.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>
#include <X11/Xlib-xcb.h>
#include <xcb/res.h>
#ifdef __OpenBSD__
#include <sys/sysctl.h>
#include <kvm.h>
#endif /* __OpenBSD__ */
#ifdef TAG_PREVIEW
#include <Imlib2.h>
#endif /* TAG_PREVIEW */
#ifdef ICONS
#include <limits.h>
#include <stdint.h>
#endif /* ICONS */

#include "drw.h"
#include "util.h"

/* macros */
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->seltags) || C->f & Sticky)
#define LENGTH(X)               (sizeof(X) / sizeof(X[0]))
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define NUMTAGS			(LENGTH(tags) + LENGTH(scratchpads) + 1)
//#define TAGMASK		((1 << LENGTH(tags)) - 1)
#define SCRATCHPAD_MASK		(1 << (NUMTAGS - 1)) /* dynamic scratchpads */
#define TAGMASK     		((1 << NUMTAGS) - 1)
#define SPTAG(i) 		((1 << LENGTH(tags)) << (i))
#define SPTAGMASK		(((1 << LENGTH(scratchpads)) - 1) << LENGTH(tags))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define TTEXTW(X)               (drw_fontset_getwidth(drw, (X)))
#define RULE(...)		{ .monitor = -1, __VA_ARGS__ },
#define SETVAL(X, flag, val)	X->f = ((val) ? X->f | flag : X->f & ~flag)
#define LOG(...)		do { fprintf(stderr, "demwm: " __VA_ARGS__); fputc('\n', stderr); } while (0)

#ifdef DEBUG
#define debug(...)		do { fprintf(stderr, "demwm(debug): %s:\n", __func__); fprintf(stderr, "\t" __VA_ARGS__); } while (0)
#else
#define debug(...)
#endif /* DEBUG */

/* enums */
#ifdef SYSTRAY
enum { VERSION_MAJOR = 0, VERSION_MINOR = 0, XEMBED_MAPPED = (1 << 0),
       XEMBED_EMBEDDED_VERSION = (VERSION_MAJOR << 16) | VERSION_MINOR, }; /* Xembed messages */
enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
#endif /* SYSTRAY */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel, SchemeUrgent, SchemeLt, SchemeTitle,
       SchemeStatus, SchemeDelim, SchemeSys, SchemeIndUrg, SchemeIndOff,
       SchemeIndOn, BorderNorm, BorderSel, BorderFloat, BorderUrg, SchemeLast }; /* color schemes */
enum { Sp1, Sp2, Sp3, Sp4, Sp5, Sp6, Sp7, Sp8, Sp9, Sp10 }; /* scratchpads */
enum { NetWMName, NetClientList, NetWMState, NetWMFullscreen, NetActiveWindow,
       NetWMWindowTypeDesktop, NetWMWindowType, NetWMStateAbove, NetWMPid,
#ifdef ICONS
       NetWMIcon,
#endif /* ICONS */
#ifdef SYSTRAY
       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation,
       NetSystemTrayVisual, NetWMWindowTypeDock, NetSystemTrayOrientationHorz,
#endif /* SYSTRAY */
       NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { EMMons, EMFlags, EMTags, EMPosx, EMPosy, EMLast }; /* Explo atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
enum {	AlwOnTop   = 1 << 0,  /* AlwaysOnTop */
	Float      = 1 << 1,
	Fixed      = 1 << 2,  /* same height and width */
	FS         = 1 << 3,  /* FullScreen */
	FSLock     = 1 << 4,  /* FullScreen bit lock (prev on oldstate) */
	FakeFS     = 1 << 5,  /* FakeFullScreen */
	HintsValid = 1 << 6,  /* reduces calls to updatesizehints() */
	NeverFocus = 1 << 7,  /* XWMhints InputHint */
	NoSwallow  = 1 << 8,  /* don't 'swallow' this child */
	Sticky     = 1 << 9,  /* window shown in all tags */
	Terminal   = 1 << 10, /* 'swallow' child processes */
	UnCursor   = 1 << 11,
	Urg        = 1 << 12, /* urgent */
	WasFloat   = 1 << 13, /* oldstate: used in fullscreen operations */
	LastFlag   = 1 << 14, /* placeholder for the last flag */
}; /* client flags/state */
enum { HideVacant = 1 << 0, ShowBar = 1 << 1, TopBar = 1 << 2 }; /* mon flags */
enum {	Alt     = Mod1Mask,
	AltGr   = Mod3Mask,
	Button6 = 6,
	Button7 = 7,
	Ctrl    = ControlMask,
	Shift   = ShiftMask,
	ShiftGr = Mod5Mask,
	Super   = Mod4Mask,
}; /* modifiers */
enum {	BUTTONMASK = (ButtonPressMask|ButtonReleaseMask),
	MOUSEMASK  = (ButtonPressMask|ButtonReleaseMask|PointerMotionMask),
	WINMASK    = (CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask),
}; /* masks */
enum { INTa, UNIa, FLTa, NOOa }; /* function table args */
enum { IPCSIZE = 64, Solid = 0xffU }; /* magick numbers */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	const unsigned int scheme;
	const char *command;
	const unsigned int interval;
	const unsigned int signal;
} Block;

typedef struct {
	const unsigned int click;
	const unsigned int mask;
	const unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Client Client;
typedef struct Monitor Monitor;
typedef struct Pertag Pertag;

struct Client {
	char name[256];
	float mina, maxa;
	float cfact;
	int sfx, sfy, sfw, sfh; /* old float geometry */
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int bw, oldbw;
	unsigned int tags;
	unsigned int f; /* flags */
	int fakefullscreen;
	pid_t pid;
	#ifdef ICONS
	unsigned int icw, ich;
	Picture icon;
	#endif /* ICONS */
	Client *next;
	Client *snext;
	Client *swallowing;
	Monitor *mon;
	Window win;
};

typedef struct {
	const unsigned int mod;
	const KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	const int gaps;
} Layout;

struct Monitor {
	char ltsymbol[16];
	float mfact;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	int gappih;           /* horizontal gap between windows */
	int gappiv;           /* vertical gap between windows */
	int gappoh;           /* horizontal outer gaps */
	int gappov;           /* vertical outer gaps */
	unsigned int nmaster;
	unsigned int f;       /* flags */
	unsigned int seltags, oldtags;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
#ifdef TAG_PREVIEW
	Window tagwin;
	int previewshow;
	Pixmap *tagmap;
#endif /* TAG_PREVIEW */
	const Layout *lt;
	Pertag *pertag;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	const unsigned int tags;
	const int monitor;
	const unsigned int flags;
} Rule;

#ifdef SYSTRAY
typedef struct {
	Window win;
	Client *icons;
} Systray;
#endif /* SYSTRAY */

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attachdefault(Client *c);
static void attachabove(Client *c);
static void attachstack(Client *c);
static void attachcycle(const Arg *arg);
static void attachwhere(const Arg *arg);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static int drawstatus(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static Client *findbefore(Client *c);
static void focus(Client *c);
static void focusin(XEvent *e);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
#ifdef ICONS
static uint32_t prealpha(uint32_t p);
static void geticonprop(Client *c);
static void freeicon(Client *c);
static void updateicon(Client *c);
#endif /* ICONS */
static void getcmd(int i, char *button);
static void getcmds(int time);
static void getsigcmds(unsigned int signal);
static int gcd(int a, int b);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void losefullscreen(Client *next);
static void readxresources(void);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static Client *nexttiled(Client *c);
static void pop(Client *c);
static Client *prevtiled(Client *c);
static void propertynotify(XEvent *e);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static void resizebarwin(Monitor *m);
#ifdef SYSTRAY
static Atom getatomprop(Client *c, Atom prop);
static unsigned long getatom(Window w, Atom prop, long size, Atom req, unsigned char **p);
static void removesystrayicon(Client *i);
static void resizerequest(XEvent *e);
static Monitor *systraytomon(Monitor *m);
static void updatesystray(void);
static void sendsystrayev(Window w, long code);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i);
static Client *wintosystrayicon(Window w);
#endif /* SYSTRAY */
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void saveclientprop(Client *c);
static void setclientprop(Client *c);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setup(void);
static void setsignal(int sig, void (*sahandler)(int sig));
static void seturgent(Client *c, int urg);
static void shift(unsigned int *tag, int i);
static void showhide(Client *c);
#ifdef TAG_PREVIEW
static void showtagpreview(unsigned int i);
static void getpreview(void);
#endif /* TAG_PREVIEW */
static void sigalrm(int unused);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewmhints(Client *c);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);

static void scratchpad_show_client(Client *c);
static int scratchpad_last_showed_is_killed (void);
static void scratchpad_show_first(void);

/* vanitygaps */
static void getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc);
static void getfacts(Monitor *m, int msize, int ssize, float *mf, float *sf, int *mr, int *sr);
static void setgaps(int oh, int ov, int ih, int iv);

static pid_t getparentprocess(pid_t p);
static int isdescprocess(pid_t p, pid_t c);
static Client *swallowingclient(Window w);
static Client *termforwin(const Client *c);
static void swallow(Client *p, Client *c);
static void unswallow(Client *c);
static pid_t winpid(Window w);

/* user functions */
static void combotag(const Arg *arg);
static void comboview(const Arg *arg);
static void cyclelayout(const Arg *arg);
static void focusmaster(const Arg *arg);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void incnmaster(const Arg *arg);
static void killclient(const Arg *arg);
static void movemouse(const Arg *arg);
static void pushstack(const Arg *arg);
static void quit(const Arg *arg);
static void restart(const Arg *arg);
static void resizemouse(const Arg *arg);
static void resizemousescroll(const Arg *arg);
static void sendstatusbar(const Arg *arg);
static void setlayout(const Arg *arg);
static void setcfact(const Arg *arg);
static void setmfact(const Arg *arg);
static void shiftpreview(const Arg *arg);
static void shifttag(const Arg *arg);
static void shifttagclients(const Arg *arg);
static void shiftview(const Arg *arg);
static void shiftviewclients(const Arg *arg);
static void shiftboth(const Arg *arg);
static void shiftswaptags(const Arg *arg);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void toggletagbar(const Arg *arg);
static void togglefakefullscreen(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void togglescratch(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void togglesticky(const Arg *arg);
static void previewtag(const Arg *arg);
static void updateblock(const Arg *arg);
static void updateallblocks(const Arg *arg);
static void view(const Arg *arg);
static void xrdb(const Arg *arg);
static void xinitvisual(int screen);
static void zoom(const Arg *arg);
static void zoomswap(const Arg *arg);
static void scratchpad_hide(const Arg *arg);
static void scratchpad_remove(const Arg *arg);
static void scratchpad_show(const Arg *arg);
static void togglesmartgaps(const Arg *arg);
static void togglegaps(const Arg *arg);
static void defaultgaps(const Arg *arg);
static void incrgaps(const Arg *arg);	/* inner + outter gaps */
static void incrigaps(const Arg *arg);	/* inner gaps (horiz + vert) */
static void incrivgaps(const Arg *arg);	/* vertical inner gaps */
static void incrihgaps(const Arg *arg);	/* horizontal inner gaps */
static void incrogaps(const Arg *arg);	/* outter gaps (horiz + vert) */
static void incrohgaps(const Arg *arg);	/* vertcal outter gaps */
static void incrovgaps(const Arg *arg);	/* horizontal outter gaps */
static void swapfocus(const Arg *arg);
/* Customs */
static void togglealwaysontop(const Arg *arg);
static void movefloathorz(const Arg *arg);
static void movefloatvert(const Arg *arg);
static void movfh_setmfact(const Arg *arg);
static void movfv_pushstack(const Arg *arg);
static void swaptags(const Arg *arg);
static void toggletopbar(const Arg *arg);
//static void toggleborder(const Arg *arg);
static void togglevacant(const Arg *arg);
static void togglestatus(const Arg *arg);

static const struct { const unsigned int type;
 void (*func)(const Arg *arg); const char *name; } parsetable[] = {
	{ UNIa, updateblock, "updateblock" },
	{ NOOa, updateallblocks, "updateallblocks" },
	{ INTa, cyclelayout, "cyclelayout" },
	{ INTa, focusmon, "focusmon" },
	{ INTa, focusstack, "focusstack" },
	{ INTa, incnmaster, "incnmaster" },
	{ INTa, incrgaps, "incrgaps" },
	{ INTa, incrogaps, "incrogaps" },
	{ INTa, incrohgaps, "incrohgaps" },
	{ INTa, incrovgaps, "incrovgaps" },
	{ INTa, incrigaps, "incrigaps" },
	{ INTa, incrihgaps, "incrihgaps" },
	{ INTa, incrivgaps, "incrivgaps" },
	{ NOOa, defaultgaps, "defaultgaps" },
	{ INTa, setlayout, "setlayout" },
	{ INTa, movefloathorz, "movefloathorz" },
	{ INTa, movefloatvert, "movefloatvert" },
	{ INTa, pushstack, "pushstack" },
	{ NOOa, scratchpad_hide, "scratchpad_hide" },
	{ NOOa, scratchpad_remove, "scratchpad_remove" },
	{ NOOa, scratchpad_show, "scratchpad_show" },
	{ FLTa, setmfact, "setmfact" },
	{ INTa, shiftboth, "shiftboth" },
	{ INTa, shifttag, "shifttag" },
	{ INTa, shifttagclients, "shifttagclients" },
	{ INTa, shiftview, "shiftview" },
	{ INTa, shiftviewclients, "shiftviewclients" },
	{ UNIa, swaptags, "swaptags" },
	{ UNIa, tag, "tag" },
	{ INTa, tagmon, "tagmon" },
	{ NOOa, togglealwaysontop, "togglealwaysontop" },
	{ NOOa, togglebar, "togglebar" },
	{ NOOa, toggletagbar, "toggletagbar" },
	{ NOOa, togglefloating, "togglefloating" },
	{ NOOa, togglefullscreen, "togglefullscreen" },
	{ NOOa, togglefakefullscreen, "togglefakefullscreen" },
	{ NOOa, togglegaps, "togglegaps" },
	{ NOOa, togglesmartgaps, "togglesmartgaps" },
	{ NOOa, togglevacant, "togglevacant" },
	{ NOOa, togglestatus, "togglestatus" },
	{ NOOa, toggletopbar, "toggletopbar" },
	{ UNIa, toggletag, "toggletag" },
	{ UNIa, toggleview, "toggleview" },
	{ NOOa, togglesticky, "togglesticky" },
	{ UNIa, view, "view" },
	{ NOOa, xrdb, "xrdb" },
	{ NOOa, zoom, "zoom" },
	{ NOOa, zoomswap, "zoomswap" },
	{ NOOa, killclient, "killclient" },
	{ NOOa, restart, "restart" },
	{ NOOa, quit, "quit" },
};

static void (*handler[LASTEvent])(XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyRelease] = keyrelease,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	#ifdef SYSTRAY
	[ResizeRequest] = resizerequest,
	#endif /* SYSTRAY */
	[UnmapNotify] = unmapnotify
};

/* variables */
#ifdef SYSTRAY
static Atom xatom[XLast];
static Systray *systray = NULL;
static unsigned int sysw = 1; /* systray width */
#endif /* SYSTRAY */
static Atom wmatom[WMLast], netatom[NetLast];
static Cur *cursor[CurLast];
static Client *scratchpad_last_showed = NULL;
static Client *prevclient = NULL;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
static const char broken[] = "broken";
static unsigned int stsw = 0; /* status width */
static unsigned int blocknum; /* blocks idx in mouse click */
static int combo = 0;         /* combo flag */
static int bh;                /* bar height */
static int lrpad;             /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *); /* x11 error func */
static unsigned int numlockmask = 0;
static unsigned int sleepinterval = 0, maxinterval = 0;
static int running = 0; /* -1 restart, 0 quit, 1 running */
static void (*attach)(Client *);

/* various layouts to use on the config */
#include "layouts.c"

/* configuration, allows nested code to access above variables */
#include "config.h"

static Clr *scheme[LENGTH(colors)] = {0};
static char blockoutput[LENGTH(blocks)][CMDLENGTH + 1] = {0}; /* +1 for '\0' */
static int pipes[LENGTH(blocks)][2] = {0};
static unsigned int execlock = 0; /* ensure only one child process exists per block at an instance */
struct Pertag {
	unsigned int curtag, prevtag;		/* current and previous tag */
	unsigned int showbars;			/* display bar for the current tag */
	unsigned int enablegaps;		/* display gaps for the current tag */
	int nmasters[LENGTH(tags) + 1];		/* number of windows in master area */
	float mfacts[LENGTH(tags) + 1];		/* mfacts per tag */
	const Layout *ltidxs[LENGTH(tags) + 1];	/* matrix of tags and layouts indexes */
	Client *prevzooms[LENGTH(tags) + 1];	/* store zoom information */
	unsigned int gaps[LENGTH(tags) + 1];	/* gaps per tag */
};

#include "cmds.c"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[NUMTAGS > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->tags = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (r = &rules[0]; r <= &rules[LENGTH(rules) - 1]; r++) {
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			if (r->flags & FakeFS)
				c->fakefullscreen = 1;
			c->f |= (r->flags & ~FakeFS);
			c->tags |= r->tags;

			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : (c->mon->seltags & ~SPTAGMASK);
	debug("client title: '%s'\n\tX resource class: '%s'\n\tX resource name: '%s'\n\tflags: '%u'\n\tmonitor: '%d'\n\ttags: '%u'\n", c->name, ch.res_class, ch.res_name, c->f, c->mon->num, c->tags);

	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > drw->w)
			*x = drw->w - WIDTH(c);
		if (*y > drw->h)
			*y = drw->h - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || ((c->f & Float) && floathints) || !c->mon->lt->arrange) {
		if (!(c->f & HintsValid))
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
	strncpy(m->ltsymbol, m->lt->symbol, sizeof m->ltsymbol);
	if (m->lt->arrange)
		m->lt->arrange(m);
}

//void
//attach(Client *c)
//{
//	c->next = c->mon->clients;
//	c->mon->clients = c;
//}

void
attachdefault(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachabove(Client *c)
{
	Client *at;
	if (!(c->mon->sel == NULL || c->mon->sel == c->mon->clients || (c->f & Float))) {
		for (at = c->mon->clients; at->next != c->mon->sel; at = at->next);
		c->next = at->next;
		at->next = c;
		return;
	}
	attachdefault(c);
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
swallow(Client *p, Client *c)
{
	XWindowChanges wc;
	Window tmp;

	if ((c->f & (Terminal | NoSwallow))
	|| ((!swallowfloat && c->f & Float) && !(c->f & FS))
	|| (!swallowffs && c->f & FS))
		return;

	detach(c);
	detachstack(c);

	setclientstate(c, WithdrawnState);
	XUnmapWindow(dpy, p->win);

	p->swallowing = c;
	c->mon = p->mon;

	tmp = p->win;
	p->win = c->win;
	c->win = tmp;
#ifdef ICONS
	updateicon(p);
#endif /* ICONS */
	updatetitle(p);

	wc.border_width = p->bw;
	XConfigureWindow(dpy, p->win, CWBorderWidth, &wc);
	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
	XSetWindowBorder(dpy, p->win, scheme[p->f & Float ? BorderFloat : BorderSel][ColFg].pixel);
	configure(p);

	updateclientlist();
}

void
unswallow(Client *c)
{
	XWindowChanges wc;

	c->win = c->swallowing->win;

	free(c->swallowing);
	c->swallowing = NULL;

	/* unfullscreen the client */
	setfullscreen(c, 0);
#ifdef ICONS
	freeicon(c);
#endif /* ICONS */
	updatetitle(c);
	XMapWindow(dpy, c->win);

	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	XSetWindowBorder(dpy, c->win, scheme[c->f & Float ? BorderFloat : BorderSel][ColFg].pixel);
	configure(c);

	setclientstate(c, NormalState);
	focus(NULL);
	arrange(c->mon);
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click, len, occ = 0;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if (ev->window == selmon->barwin) {
		i = x = 0;
		for (c = m->clients; c; c = c->next)
			occ |= c->tags == 255 ? 0 : c->tags;
		do {
			/* do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->seltags & 1 << i) && m->f & HideVacant)
				continue;
			x += TEXTW(m->f & HideVacant ? tagsalt[i] : tags[i]);
		} while (ev->x >= x && ++i < LENGTH(tags));
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
#ifdef TAG_PREVIEW
			/* hide preview if we click the bar */
			if (selmon->previewshow) {
				selmon->previewshow = 0;
				XUnmapWindow(dpy, selmon->tagwin);
			}
#endif /* TAG_PREVIEW */
		} else if (ev->x < x + TEXTW(selmon->ltsymbol))
			click = ClkLtSymbol;
		else if (ev->x > (x = selmon->ww - stsw
			#ifdef SYSTRAY
			- sysw
			#endif /* SYSTRAY */
			)) {
			click = ClkStatusText;

			#if INVERSED
			for (i = LENGTH(blocks) - 1; i >= 0; i--)
			#else
			for (i = 0; i < LENGTH(blocks); i++)
			#endif /* INVERSED */
			{
				if (*blockoutput[i] == '\0') /* empty string, ignore */
					continue;
				len = TTEXTW(blockoutput[i]) + TTEXTW(delimiter);
				x += len;
				if (ev->x <= x && ev->x >= x - len) { /* if the mouse is between the block area */
					blocknum = i; /* store what block the mouse is clicking */
					break;
				}
			}
		} else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		/* scrolling doesn't trigger focus */
		if (ev->button != Button4 && ev->button != Button5)
			focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Monitor *m;
	size_t i;

	for (i = 0; i < LENGTH(blocks); i++) { /* close the pipes */
		close(pipes[i][0]);
		close(pipes[i][1]);
	}

	for (m = mons; m; m = m->next)
		while (m->stack) {
			if (running == -1)
				saveclientprop(m->stack);
			unmanage(m->stack, 0);
		}

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);

#ifdef SYSTRAY
	if (systray) {
		while (systray->icons)
			removesystrayicon(systray->icons);
		if (systray->win) {
			XUnmapWindow(dpy, systray->win);
			XDestroyWindow(dpy, systray->win);
		}
		free(systray);
	}
#endif /* SYSTRAY */
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	XCloseDisplay(dpy);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
#ifdef TAG_PREVIEW
	for (size_t i = 0; i < LENGTH(tags); i++)
		if (mon->tagmap[i])
			XFreePixmap(dpy, mon->tagmap[i]);
	free(mon->tagmap);
	XUnmapWindow(dpy, mon->tagwin);
	XDestroyWindow(dpy, mon->tagwin);
#endif /* TAG_PREVIEW */
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
 	free(mon->pertag);
	free(mon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);
#ifdef SYSTRAY
	XWindowAttributes wa;
	XSetWindowAttributes swa;

	if (systray
	&& cme->window == systray->win
	&& cme->message_type == netatom[NetSystemTrayOP]
	&& cme->data.l[1] == 0 /* SYSTEM_TRAY_REQUEST_DOCK */
	&& cme->data.l[2]) {
		/* do our little manage() like for the systray window */
		c = ecalloc(1, sizeof(Client));

		c->win = cme->data.l[2];
		c->mon = selmon;
		c->next = systray->icons;
		systray->icons = c;

		XGetWindowAttributes(dpy, c->win, &wa);
		c->x = c->sfx = c->oldx = c->y = c->sfy = c->oldy = 0;
		c->w = c->oldw = wa.width;
		c->h = c->oldh = wa.height;
		c->oldbw = wa.border_width;
		c->bw = 0;
		c->f |= Float;
		c->tags = 1; /* reuse tags field as mapped status */
		updatesizehints(c);
		updatesystrayicongeom(c, wa.width, wa.height);
		XAddToSaveSet(dpy, c->win);
		XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
		XSetClassHint(dpy, c->win, &systrayclass);
		swa.background_pixel  = 0;
		//swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;	/* use parents background color */
		//swa.background_pixel  = 0;
		//swa.border_pixel = 0;
		//wa.background_pixel = 0;
		//swa.colormap = drw->cmap;

		XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
		XReparentWindow(dpy, c->win, systray->win, 0, 0);

		sendsystrayev(c->win, 0 /* XEMBED_EMBEDDED_NOTIFY */);
		/* FIXME are these events needed? */
		sendsystrayev(c->win, 0 /* XEMBED_FOCUS_IN */);
		sendsystrayev(c->win, 1 /* XEMBED_WINDOW_ACTIVATE */);
		sendsystrayev(c->win, 10 /* XEMBED_MODALITY_ON */);

		XSync(dpy, False);
		setclientstate(c, NormalState);
		updatestatus();
		return; /* ignore other atoms for systray */
	}
#endif /* SYSTRAY */
	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen]) {
			if (c->fakefullscreen == 2 && c->f & FS)
				c->fakefullscreen = 3;
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD */
			|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !(c->f & FS))));
		} else if (cme->data.l[1] == netatom[NetWMStateAbove]
			|| cme->data.l[2] == netatom[NetWMStateAbove])
			SETVAL(c, AlwOnTop, (cme->data.l[0] || cme->data.l[1]));
	} else if (cme->message_type == netatom[NetActiveWindow])
		if (c != selmon->sel && !(c->f & Urg))
			seturgent(c, 1);
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;
	int sw = DisplayWidth(dpy, drw->screen);
	int sh = DisplayHeight(dpy, drw->screen);

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->f & FS && c->fakefullscreen != 1)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				resizebarwin(m);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->f & Float || !selmon->lt->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->f & Float)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->f & Float)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}


Monitor *
createmon(void)
{
	Monitor *m;
	unsigned int i;

	m = ecalloc(1, sizeof(Monitor));
	m->seltags = m->oldtags = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	/* monitor flags */
	SETVAL(m, ShowBar, showbar);
	SETVAL(m, TopBar, topbar);
	SETVAL(m, HideVacant, hidevacant);
	/* gaps per tag */
	m->gappih = gappih;
	m->gappiv = gappiv;
	m->gappoh = gappoh;
	m->gappov = gappov;

	m->pertag = ecalloc(1, sizeof(Pertag));

	for (i = 0; !(m->seltags & 1 << i); i++);
	m->pertag->curtag = m->pertag->prevtag = i;

	/* the ~0 (all tags) tag */
	m->pertag->ltidxs[LENGTH(tags)] = &layouts[alltagslayout];
	m->pertag->nmasters[LENGTH(tags)] = m->nmaster;
	m->pertag->mfacts[LENGTH(tags)] = m->mfact;
	m->pertag->prevzooms[LENGTH(tags)] = NULL;
	if (gapspertag)
		m->pertag->gaps[LENGTH(tags)] = ((gappoh & 0xFF) << 0) | ((gappov & 0xFF) << 8) | ((gappih & 0xFF) << 16) | ((gappiv & 0xFF) << 24);

	for (i = 0; i < LENGTH(tags); i++) {

		/* init gaps */
		m->pertag->enablegaps |= 1 << i;

		/* init showbar */
		if (m->f & ShowBar)
			m->pertag->showbars |= 1 << i;

		/* init layouts */
		m->pertag->ltidxs[i] = &layouts[taglayouts[i]];

		/* init nmaster */
		m->pertag->nmasters[i] = m->nmaster;

		/* init mfacts */
		m->pertag->mfacts[i] = m->mfact;

		/* swap focus and zoomswap*/
		m->pertag->prevzooms[i] = NULL;

		if (gapspertag)
			m->pertag->gaps[i] = ((gappoh & 0xFF) << 0) | ((gappov & 0xFF) << 8) | ((gappih & 0xFF) << 16) | ((gappiv & 0xFF) << 24);
	}

	m->lt = m->pertag->ltidxs[m->pertag->curtag];
	strncpy(m->ltsymbol, m->lt->symbol, sizeof m->ltsymbol);

	#ifdef TAG_PREVIEW
	m->tagmap = ecalloc(LENGTH(tags), sizeof(Pixmap));
	#endif /* TAG_PREVIEW */

	return m;
}


void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
	else if ((c = swallowingclient(ev->window)))
		unmanage(c->swallowing, 1);
#ifdef SYSTRAY
	else if ((c = wintosystrayicon(ev->window))) {
		removesystrayicon(c);
		updatestatus();
	}
#endif /* SYSTRAY */
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	int x = 0, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	unsigned int a = 0, s = 0;
	Client *c;
	int bw = m->ww; /* bar width */

	if (!(m->f & ShowBar) || running < 1)
		return;

	#ifdef SYSTRAY
	bw -= m == systraytomon(m) ? sysw : 0;
	#endif /* SYSTRAY */

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) /* status is only drawn on selected monitor */
		tw = stsw;

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags == 255 || c->f & Sticky ? 0 : c->tags;
		if (c->f & Urg)
			urg |= c->tags;
	}

	for (i = 0; i < LENGTH(tags); i++) {
		/* apply 'hidevacant' only to the selected monitor */
		if (m->f & HideVacant && (!(occ & 1 << i || m->seltags & 1 << i)))
			continue;
		w = TEXTW(m->f & HideVacant ? tagsalt[i] : tags[i]);
		drw_setscheme(drw, scheme[urg & 1 << i ? SchemeUrgent : (m->seltags & 1 << i && m == selmon ? SchemeSel : SchemeNorm)]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, m->f & HideVacant ? tagsalt[i] : tags[i], 0);
		if (occ & 1 << i && !(m->f & HideVacant)) { /* don't draw these when hidevacant */
			if (urg & 1 << i) {
				/* urgent underline (top of tag) */
				drw_setscheme(drw, scheme[SchemeIndUrg]);
                		drw_rect(drw, x, 0, w, boxs, 1, 0);
			}
			/* normal underline (IndOn/Off) */
			drw_setscheme(drw, scheme[(m == selmon && selmon->sel && selmon->sel->tags & 1 << i) ? SchemeIndOn : SchemeIndOff]);
			drw_rect(drw, x, bh - boxs - 1, w, boxs + 1, 1, 0);
		}
		x += w;
	}
	/* monocle, count clients if there are more than one */
	if ((m->lt->arrange == &monocle || m->lt->arrange == &alphamonocle) && m->sel) {
		for (a = 0, s = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), a++)
			if (c == m->stack)
				s = a;
		if (!s && a)
			s = a;
		if (a > 1) {
			if (m->lt->arrange == &monocle) /* monocle */
				snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d/%d]", s, a);
			else	/* alphamonocle */
				snprintf(m->ltsymbol, sizeof m->ltsymbol, "{%d/%d}", s, a);
		}
	}
	w = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeLt]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	if ((w = bw - tw - x) > bh) {
		if (m->lt->arrange == &clear) { /* hide title in clear layout */
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
			drw_map(drw, m->barwin, 0, 0, bw, bh);
			return;
		}

		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeTitle : SchemeNorm]);
		#ifdef ICONS
			if (TEXTW(m->sel->name) + m->sel->icw > w) { /* don't center if the title + icon don't fit */
				drw_text(drw, x, 0, w, bh, lrpad / 2 + (m->sel->icon ? m->sel->icw : 0), m->sel->name, 0);
				if (m->sel->icon)
					drw_pic(drw, x, (bh - m->sel->ich) / 2, m->sel->icw, m->sel->ich, m->sel->icon);
				if (underlntitle)
					drw_rect(drw, x + (m->sel->icon ? m->sel->icw + lrpad / 2: 0), bh - boxs - 1, w - (m->sel->icon ? m->sel->icw + lrpad : 0), 1, 1, 0);
			} else { /* center window title and icon */
				drw_text(drw, x, 0, w, bh, (w - TEXTW(m->sel->name) + (m->sel->icon ? m->sel->icw + lrpad : 0)) / 2, m->sel->name, 0);
				if (m->sel->icon)
					drw_pic(drw, x + (w - TEXTW(m->sel->name) - m->sel->icw) / 2, (bh - m->sel->ich) / 2, m->sel->icw, m->sel->ich, m->sel->icon);
				if (underlntitle)
					drw_rect(drw, x + (w - TEXTW(m->sel->name) + (m->sel->icon ? m->sel->icw + lrpad : 0)) / 2, bh - boxs - 1, TEXTW(m->sel->name) - lrpad, 1, 1, 0);
			}
		#else
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
		#endif /* ICONS */
			if (m->sel->f & AlwOnTop) {
				drw_setscheme(drw, scheme[SchemeLt]);
				drw_rect(drw, x, 0, w, boxw, 1, 0);
			} else if (m->sel->f & Float)
				drw_rect(drw, bw - tw - boxw - 1, boxs, boxw, boxw, m->sel->f & Fixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	drw_map(drw, m->barwin, 0, 0, bw, bh);
}

void
drawbars(void)
{
	Monitor *m;
	for (m = mons; m; m = m->next)
		drawbar(m);
}

int
drawstatus(void)
{

	int width = selmon->ww; /* bar width */
	#ifdef SYSTRAY
	width -= selmon == systraytomon(selmon) ? sysw : 0;
	#endif /* SYSTRAY */

	int i, all = width;
	int barpad = ((bh - drw->fonts->h) / 2) - 1; //-1 so emojis render properly
	unsigned int j, len, total = 0, delimlen = TTEXTW(delimiter);

	if (!showstatus || !(selmon->f & ShowBar) || running < 1)
		return stsw = 0;

	for (j = 0; j < LENGTH(blocks); total += TTEXTW(blockoutput[j]) + delimlen, j++);

	drw_setscheme(drw, scheme[SchemeStatus]);
	drw_text(drw, width - total, 0, total, bh, 0, "", 0);

	#if INVERSED
	for (i = 0; i < LENGTH(blocks); i++)
	#else
	for (i = LENGTH(blocks) - 1; i >= 0; i--)
	#endif /* INVERSED */
	{
		if (*blockoutput[i] == '\0') /* ignore command that output NULL or '\0' */
			continue;
		drw_setscheme(drw, scheme[blocks[i].scheme]); /* set scheme */
		len  = TTEXTW(blockoutput[i]);
		all -= len;
		drw_text(drw, all, barpad, len, bh - barpad * 2, 0, blockoutput[i], 0);
		debug("drawing block '%d': '%s'\n", i, blockoutput[i]);
		/* draw delimiter */
		if (*delimiter == '\0') /* ignore no delimiter */
			continue;
		drw_setscheme(drw, scheme[SchemeDelim]);
		all -= delimlen;
		drw_text(drw, all, barpad, delimlen, bh - barpad * 2, 0, delimiter, 0);
	}

	return stsw = width - all;
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		if (!hovertofocus)
			focus(NULL);
	} else if (!c || c == selmon->sel)
		return;
	if (hovertofocus)
		focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window))) {
		drawbar(m);
#ifdef SYSTRAY
		if (m == selmon)
			updatesystray();
#endif /* SYSTRAY */
	}
}

Client *
findbefore(Client *c)
{
	Client *tmp;
	if (c == selmon->clients)
		return NULL;
	for (tmp = selmon->clients; tmp && tmp->next != c; tmp = tmp->next);
	return tmp;
}

void
focus(Client *c)
{
	Client *f;
	XWindowChanges wc;
	if (!c || !ISVISIBLE(c)) {
		for (c = selmon->stack; c && (!ISVISIBLE(c) || (c->f & Sticky && !(selmon->sel->f & Sticky))); c = c->snext);
		if (!c) /* no windows found, check for available stickies */
			for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	}

	if (selmon->sel && selmon->sel != c) {
		/* if we don't losefullscreen, we can stack fullscren clients */
		//losefullscreen(c);
		unfocus(selmon->sel, 0);
	}

	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->f & Urg)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		/* set new focused border first to avoid flickering */
		XSetWindowBorder(dpy, c->win, scheme[c->f & Float ? BorderFloat : BorderSel][ColFg].pixel);
		if (prevclient)
			XSetWindowBorder(dpy, prevclient->win, scheme[BorderNorm][ColFg].pixel);
		setfocus(c);
		if (c->mon->lt->arrange) {
			/* Move the currently focused client above the bar window */
			wc.stack_mode = Above;
			wc.sibling = c->mon->barwin;
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);

			/* Move all visible clients that are not marked as on top:
			 ** tiled: below the bar window
			 ** floating: below the selected window (avoids showing the bar) */
			wc.stack_mode = Below;
			for (f = c->mon->stack; f; f = f->snext) {
				if (f != c && !(f->f & AlwOnTop) && ISVISIBLE(f)) {
					wc.sibling = f->f & Float ? c->win : c->mon->barwin;
					XConfigureWindow(dpy, f->win, CWSibling|CWStackMode, &wc);
					wc.sibling = f->win;
				}
			}
		}

		if (c->f & UnCursor) /* put the cursor in the bottom right */
			XWarpPointer(dpy, None, root, 0, 0, 0, 0, drw->w, drw->h);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}

	selmon->sel = c;
	if (selmon->lt->arrange == alphamonocle)
		arrangemon(selmon);
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None, req = XA_ATOM;
#ifdef SYSTRAY
	/* FIXME getatomprop should return the number of items and a pointer to
	 * the stored data instead of this workaround */
	if (prop == xatom[XembedInfo])
		req = xatom[XembedInfo];
#endif /* SYSTRAY */

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, req,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
#ifdef SYSTRAY
		if (da == xatom[XembedInfo] && dl == 2)
			atom = ((Atom *)p)[1];
#endif /* SYSTRAY */
		XFree(p);
	}
	return atom;
}

unsigned long
getatom(Window w, Atom prop, long size, Atom req, unsigned char **p)
{	/* With this setup you need to manually XFree(p) */
	int format;
	unsigned long n, extra;
	Atom real;

	/* harcoded 32bits format only */
	if (XGetWindowProperty(dpy, w, prop, 0L, size, False, req,
	    &real, &format, &n, &extra, p) == Success && format == 32 && n)
		return n;

	XFree(*p);
	return 0;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	unsigned char *p = NULL;

	if (!getatom(w, wmatom[WMState], 2L, wmatom[WMState], &p))
		return -1;

	long result = *p;
	XFree(p);
	return result;
}

/* Tag preview functions */
#ifdef TAG_PREVIEW
void
getpreview(void)
{
	unsigned int occ = 0, i;
	Client *c;
	Imlib_Image image;

	for (c = selmon->clients; c; c = c->next)
		occ |= c->tags == 255 ? 0 : c->tags;

	for (i = 0; i < LENGTH(tags); i++) {
		/* searching for tags that are occupied && selected */
		if (!(occ & 1 << i) || !(selmon->seltags & 1 << i))
			continue;

		if (selmon->tagmap[i]) { /* tagmap exist, clean it */
			XFreePixmap(dpy, selmon->tagmap[i]);
			selmon->tagmap[i] = 0;
		}

		/* try to unmap the window so it doesn't show the preview on the preview */
		selmon->previewshow = 0;
		XUnmapWindow(dpy, selmon->tagwin);
		XSync(dpy, False);
		//XMoveWindow(dpy, selmon->tagwin, -selmon->mx, -selmon->my + bh);
		//XFlush(dpy);

		if (!(image = imlib_create_image(drw->w, drw->h))) {
			LOG("imlib: failed to create image, skipping.");
			continue;
		}
		imlib_context_set_image(image);
		imlib_context_set_display(dpy);
		imlib_image_set_has_alpha(1);
		imlib_context_set_blend(0);
		imlib_context_set_visual(drw->visual);
		imlib_context_set_drawable(root);

		// screen size (m{x,y,w,h}) -> window areas, without the bar (w{x,y,w,h})
		imlib_copy_drawable_to_image(0, selmon->wx, selmon->wy, selmon->ww ,selmon->wh, 0, 0, 1);
		selmon->tagmap[i] = XCreatePixmap(dpy, selmon->tagwin, selmon->mw / scalepreview, selmon->mh / scalepreview, drw->depth);
		imlib_context_set_drawable(selmon->tagmap[i]);
		imlib_render_image_part_on_drawable_at_size(0, 0, selmon->mw, selmon->mh, 0, 0, selmon->mw / scalepreview, selmon->mh / scalepreview);
		imlib_free_image();
	}
}

void
showtagpreview(unsigned int i)
{
	if (!selmon->previewshow || !selmon->tagmap[i]) {
		XUnmapWindow(dpy, selmon->tagwin);
		return;
	}

	XSetWindowBackgroundPixmap(dpy, selmon->tagwin, selmon->tagmap[i]);
	XCopyArea(dpy, selmon->tagmap[i], selmon->tagwin, drw->gc, 0, 0,
			selmon->mw / scalepreview, selmon->mh / scalepreview,
			0, 0);
	XSync(dpy, False);
	//XMapWindow(dpy, selmon->tagwin);
	XMapRaised(dpy, selmon->tagwin);
}
#endif /* TAG_PREVIEW */

void
resizebarwin(Monitor *m)
{
	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww
#ifdef SYSTRAY
		- (m == systraytomon(m) ? sysw : 0)
#endif /* SYSTRAY */
		, bh);
}

/* Systray functions */
#ifdef SYSTRAY
void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	XReparentWindow(dpy, i->win, root, 0, 0);
	free(i);
}
void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		updatestatus();
	}
}
void
sendsystrayev(Window w, long code)
{
	Atom mt = netatom[Xembed];
	XEvent ev;
	long d2 = 0, d3 = systray->win, d4 = XEMBED_EMBEDDED_VERSION;

	if (w == root) {
		mt   = xatom[Manager];
		d2   = systray->win;
		d3 = d4 = 0;
	}

	ev.type = ClientMessage;
	ev.xclient.window = w;
	ev.xclient.format = 32;
	ev.xclient.message_type = mt;
	ev.xclient.data.l[0] = CurrentTime;
	ev.xclient.data.l[1] = code;
	ev.xclient.data.l[2] = d2;
	ev.xclient.data.l[3] = d3;
	ev.xclient.data.l[4] = d4;
	XSendEvent(dpy, w, False, StructureNotifyMask, &ev);
}
Monitor *
systraytomon(Monitor *m)
{
	Monitor *t;
	int i, n;
	if (!systraypinning) {
		if (!m)
			return selmon;
		return m == selmon ? m : NULL;
	}
	for (n = 1, t = mons; t && t->next; n++, t = t->next);
	for (i = 1, t = mons; t && t->next && i < systraypinning; i++, t = t->next);
	if (systraypinningfailfirst && n < systraypinning)
		return mons;
	return t;
}
void
updatesystray(void)
{
	XSetWindowAttributes wa;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;
	unsigned int w = 1, xpad = 0, ypad = 0;

	drw_setscheme(drw, scheme[SchemeNorm]);
	for (w = 0, i = systray->icons; i; i = i->next) {
		wa.background_pixel = 0;
		//wa.background_pixel = scheme[SchemeNorm][ColBg].pixel;
		XChangeWindowAttributes(dpy, i->win, CWBackPixel, &wa);
		XMapRaised(dpy, i->win);
		i->x = w;
		XMoveResizeWindow(dpy, i->win, i->x, 0, i->w, i->h);
		w += i->w;
		if (i->next)
			w += lrpad / 2;
		if (i->mon != m)
			i->mon = m;
	}
	x -= w;
	sysw = w;
	XMoveResizeWindow(dpy, systray->win, x - xpad + 1, m->by + ypad, MAX(w, 1), bh);
	resizebarwin(selmon);
	drawstatus();
	XMapWindow(dpy, systray->win);
	XMapSubwindows(dpy, systray->win);
	XSync(dpy, False);
}

void
updatesystrayicongeom(Client *i, int w, int h)
{
	int icon_height = bh - 2;

	if (!i)
		return;

	if (w == h)
		i->w = icon_height;
	else if (h == icon_height)
		i->w = w;
	else
		i->w = (int) ((float)icon_height * ((float)w / (float)h));

	applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);

	/* force icons into the systray dimensions if they don't want to */
	if (i->h > icon_height) {
		if (i->w == i->h)
			i->w = icon_height;
		else
			i->w = (int) ((float)icon_height * ((float)i->w / (float)i->h));
		i->h = icon_height;
	}

	if (i->w > 2 * icon_height)
		i->w = icon_height;
}
void
updatesystrayiconstate(Client *i)
{
	long flags;
	int code = 0;

	if (!systray || !i || !(flags = getatomprop(i, xatom[XembedInfo])))
		return;

	if (flags & (1 << 0) /* XEMBED_MAPPED */ && !i->tags) {
		i->tags = 1;
		code = 1 /* XEMBED_WINDOW_ACTIVATE */;
		XMapRaised(dpy, i->win);
		setclientstate(i, NormalState);
	} else if (!(flags & (1 << 0) /* XEMBED_MAPPED */) && i->tags) {
		i->tags = 0;
		code = 2 /* XEMBED_WINDOW_DEACTIVATE */;
		XUnmapWindow(dpy, i->win);
		setclientstate(i, WithdrawnState);
	} else
		return;

	sendsystrayev(i->win, code);
}
Client *
wintosystrayicon(Window w)
{
	Client *i;

	if (!systray || !w)
		return NULL;

	for (i = systray->icons; i && i->win != w; i = i->next);
	return i;
}
#endif /* SYSTRAY */

/* Window Icons functions */
#ifdef ICONS
uint32_t
prealpha(uint32_t p)
{
	uint8_t a = p >> 24u;
	uint32_t rb = (a * (p & 0xFF00FFu)) >> 8u;
	uint32_t g = (a * (p & 0x00FF00u)) >> 8u;
	return (rb & 0xFF00FFu) | (g & 0x00FF00u) | (a << 24u);
}
void
geticonprop(Client *c)
{
	unsigned long n, *p = NULL, *bstp = NULL;
	unsigned int iconsize = drw->fonts->h; /* icon same height as font */
	uint32_t w, h, sz;

	c->icon = None;

	if (!(n = getatom(c->win, netatom[NetWMIcon], LONG_MAX, AnyPropertyType, (unsigned char **)&p)))
		return;

	{
		unsigned long *i;
		const unsigned long *end = p + n;
		uint32_t bstd = UINT32_MAX, d, m;

		for (i = p; i < end - 1; i += sz) {
			if ((w = *i++) > UINT16_MAX || (h = *i++) > UINT16_MAX) {
				XFree(p);
				return;
			}
			if ((sz = w * h) > end - i)
				break;
			if ((m = MAX(w, h)) >= iconsize && (d = m - iconsize) < bstd) {
				bstd = d;
				bstp = i;
			}
		}
		if (!bstp) {
			for (i = p; i < end - 1; i += sz) {
				if ((w = *i++) > UINT16_MAX || (h = *i++) > UINT16_MAX) {
					XFree(p);
					return;
				}
				if ((sz = w * h) > end - i)
					break;
				if ((d = iconsize - MAX(w, h)) < bstd) {
					bstd = d;
					bstp = i;
				}
			}
		}
	}

	if (!bstp || (w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) {
		XFree(p);
		return;
	}

	/* Picture width and height */
	if (w <= h) {
		c->ich = iconsize;
		c->icw = MAX(w * drw->fonts->h / h, 1);
	} else {
		c->ich = MAX(h * drw->fonts->h / w, 1);
		c->icw = iconsize;
	}

	uint32_t i = 0, *bstp32 = (uint32_t *)bstp;
	for (sz = w * h; i < sz; i++)
		bstp32[i] = prealpha(bstp[i]);

	c->icon = drw_picture_create_resized(drw, (char *)bstp, w, h, c->icw, c->ich);
	XFree(p);
}
void
freeicon(Client *c)
{
	if (c->icon) {
		XRenderFreePicture(dpy, c->icon);
		c->icon = None;
	}
}
void
updateicon(Client *c)
{
	freeicon(c);
	geticonprop(c);
}
#endif /* ICONS */

void
getcmd(int i, char *button)
{

	if (execlock & (1 << i)) { /* block is already running */
		debug("block '%d' with command '%s' ignored\n", i, blocks[i].command);
		return;
	}

	/* lock execution of block until current instance finishes execution */
	execlock |= (1 << i);

	if (fork() == 0) {
		dup2(pipes[i][1], STDOUT_FILENO);
		if (button)
			setenv("BLOCK_BUTTON", button, 1);
		execl("/bin/sh", "/bin/sh", "-c", blocks[i].command, (char *) NULL);
		die("demwm: block '%d', execl '%s' failed:", i, blocks[i].command);
	}
}

void
getcmds(int time)
{
	unsigned int i;

	for (i = 0; i < LENGTH(blocks); i++) {
		if (time == -1) { /* forcefully re-run all blocks */
			execlock &= ~(1 << i);
			getcmd(i, NULL);
			continue; /* skip below checks */
		}
		if (blocks[i].interval != 0 && time % blocks[i].interval == 0)
			getcmd(i, NULL);
	}
}

void
getsigcmds(unsigned int signal)
{
	unsigned int i;

	for (i = 0; i < LENGTH(blocks); i++)
		if (blocks[i].signal == signal)
			getcmd(i, NULL);
}

int
gcd(int a, int b)
{
	int temp;

	while (b > 0) {
		temp = a % b;
		a = b;
		b = temp;
	}

	return a;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING)
		strncpy(text, (char *)name.value, size - 1);
	else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
			BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++)
				/* skip modifier codes, we do that ourselves */
				if (keys[i].keysym == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							keys[i].mod | modifiers[j],
							root, True,
							GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XkbKeycodeToKeysym(dpy, ev->keycode, 0, 0);

	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

void
keyrelease(XEvent *e)
{
	combo = 0;
}

void
losefullscreen(Client *next)
{
	Client *sel = selmon->sel;
	if (!sel || !next)
		return;
	if (sel->f & FS && sel->fakefullscreen != 1 && ISVISIBLE(sel) && sel->mon == next->mon && !(next->f & Float))
		setfullscreen(sel, 0);
}

void
readxresources(void)
{
	Display *display;
	XrmDatabase d;
	XrmValue value;
	char *type, *resm;
	unsigned int i;

	if (!(display = XOpenDisplay(NULL)))
		LOG("readxresources: could not open display.");

	if (!(resm = XResourceManagerString(display)) || !(d = XrmGetStringDatabase(resm))) {
		LOG("could not open X resource database, switching to fallback colors.");

		for (i = 0; i < LENGTH(xrescolors); i++) {
			strncpy(xrescolors[i][0], fallbackcolor, 8);
			xrescolors[i][0][7] = '\0';
		}

		//XrmDestroyDatabase(d);
		XCloseDisplay(display);
		return;
	}

	for (i = 0; i < LENGTH(xrescolors); i++) {
		if (XrmGetResource(d, xrescolors[i][1], NULL, &type, &value) == True /* exist */
		&& (strnlen(value.addr, 8) == 7 && value.addr[0] == '#')) /* is a hex color */
			strncpy(xrescolors[i][0], value.addr, 8);
		else {
			LOG("could not read color '%s'.", xrescolors[i][1]);
			strncpy(xrescolors[i][0], fallbackcolor, 8);
		}
		xrescolors[i][0][7] = '\0';
	}

	XrmDestroyDatabase(d);
	XCloseDisplay(display);
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;
	//XEvent xev;

	/* do not manage (lower and skip) NET_WINDOW_TYPE_DESKTOP
	 * (desktop implementation) windows (glava, xlivebg, etc) */
	if (getatomprop(&((Client){ .win = w }), netatom[NetWMWindowType]) == netatom[NetWMWindowTypeDesktop]) {
		XMapWindow(dpy, w);
		XLowerWindow(dpy, w);
		debug("Window (%ld) has the _NET_WM_WINDOW_TYPE_DESKTOP property, ignoring.\n", w);
		return;
	}

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->pid = winpid(w);

	/* geometry */
	c->sfx = c->x = c->oldx = wa->x;
	c->sfy = c->y = c->oldy = wa->y;
	c->sfw = c->w = c->oldw = wa->width;
	c->sfh = c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;

	if (c->sfx == 0 || c->sfy == 0)
		c->sfx = c->sfy = -9999; /* placeholder */
#ifdef ICONS
	updateicon(c);
#endif /* ICONS */
	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
		/* since I don't use the rule  alwaysontop, this makes trasient
		 * windows on top of 'pop up' windows, which isn't desirable. */
		//c->alwaysontop = 1;
	} else {
		c->mon = selmon;
		applyrules(c);
		term = termforwin(c);
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
	c->bw = (c->f & Float) ? fborderpx : borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[BorderNorm][ColFg].pixel);
	configure(c); /* propagates border_width, if size doesn't change */

	SETVAL(c, AlwOnTop, getatomprop(c, netatom[NetWMState]) == netatom[NetWMStateAbove]);
	if (getatomprop(c, netatom[NetWMState]) == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	updatesizehints(c);
	updatewmhints(c);

	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!(c->f & Float)) {
		SETVAL(c, WasFloat, trans != None);
		SETVAL(c, Float, c->f & (WasFloat | Fixed));
	}
	if (c->f & Float)
		XRaiseWindow(dpy, c->win);

	/* center the window (floating) */
	if (c->f & Float || !c->mon->lt->arrange) {
		c->x = c->mon->mx + (c->mon->mw - WIDTH(c)) / 2;
		c->y = c->mon->my + (c->mon->mh - HEIGHT(c)) / 2;
	}

	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * drw->w, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon) {
		//losefullscreen(c); /* lose fullscreen when a new window is opened */
		unfocus(selmon->sel, 0);
	}
	c->mon->sel = c;
	XMapWindow(dpy, c->win);
	if (term)
		swallow(term, c);
	if (running == 1) {
		arrange(c->mon);
		focus(NULL);
	}
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;
	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

#ifdef SYSTRAY
	Client *i;
	if (systray && (i = wintosystrayicon(ev->window))) {
		sendsystrayev(i->win, 1 /* XEMBED_WINDOW_ACTIVATE */);
		updatestatus();
	}
#endif /* SYSTRAY */
	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect || !wa.depth)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	Client *sel;
	XMotionEvent *ev = &e->xmotion;

#ifdef TAG_PREVIEW
	Client *c;
	unsigned int i, x, occ = 0;

	/* FIXME when hovering the mouse over the tags and we view the tag, the
	 * preview window get's in the preview shot (screenshot) */

	if (ev->window == selmon->barwin) {
		i = x = 0;
		for (c = selmon->clients; c; c = c->next)
			occ |= c->tags == 255 ? 0 : c->tags;
		do {
			if (!(occ & 1 << i || selmon->seltags & 1 << i) && selmon->f & HideVacant)
				continue;
			x += TEXTW(selmon->f & HideVacant ? tagsalt[i] : tags[i]);
		} while (ev->x >= x && ++i < LENGTH(tags));

	     	if (i < LENGTH(tags)) {
			if (selmon->previewshow != (i + 1)
			&& !(selmon->seltags & 1 << i)) {
				selmon->previewshow = i + 1;
				showtagpreview(i);
			} else if (selmon->seltags & 1 << i) {
				selmon->previewshow = 0;
				XUnmapWindow(dpy, selmon->tagwin);
			}
		} else if (selmon->previewshow) {
			selmon->previewshow = 0;
			XUnmapWindow(dpy, selmon->tagwin);
		}
	} else if (ev->window == selmon->tagwin) {
		selmon->previewshow = 0;
		XUnmapWindow(dpy, selmon->tagwin);
	} else if (selmon->previewshow) {
		selmon->previewshow = 0;
		XUnmapWindow(dpy, selmon->tagwin);
	}

#endif /* TAG_PREVIEW */
	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		sel = selmon->sel;
		selmon = m;
		unfocus(sel, 1);
		focus(NULL);
	}
	mon = m;
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->f & Float || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	attachdefault(c);
	focus(c);
	arrange(c->mon);
}

Client *
prevtiled(Client *c)
{
	Client *p, *r;

	for (p = selmon->clients, r = NULL; p && p != c; p = p->next)
		if (!(p->f & Float) && ISVISIBLE(p))
			r = p;
	return r;
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;
	char buf[IPCSIZE];
	unsigned int i = 0;
	Arg arg = {0};
	Atom ipctom = XInternAtom(dpy, "DEMWM_IPC", False);

	if ((ev->window == root) && (ev->atom == ipctom)
	&& gettextprop(root, ipctom, buf, sizeof buf)) { /* cli functions */
		/* first two characters are the ID of the function */
		i = atoi((const char []){ buf[0], buf[1], '\0' });
		debug("index = '%d' | buf = '%s' | func = '%s'\n", i, buf, parsetable[i].name);

		switch (parsetable[i].type) {
		default: return;
		case NOOa: break;
		case INTa:
			arg.i = atoi(buf + 3); break;
		case UNIa:
			arg.ui = abs(atoi(buf + 3)); break;
		case FLTa:
			arg.f = atof(buf + 3); break;
		}

		if (parsetable[i].func == tag || parsetable[i].func == toggletag
		|| parsetable[i].func == view || parsetable[i].func == toggleview
		|| parsetable[i].func == swaptags)
			arg.ui = 1 << arg.ui;

		parsetable[i].func(&arg);
		return;
	}

#ifdef SYSTRAY
	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			c->f &= ~HintsValid;
			updatesystrayicongeom(c, c->w, c->h);
		} else if (ev->atom == xatom[XembedInfo])
			updatesystrayiconstate(c);
		updatesystray();
	}
#endif /* SYSTRAY */

	if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!(c->f & Float) && (XGetTransientForHint(dpy, c->win, &trans))) {
				if (wintoclient(trans)) {
					c->f |= Float;
					arrange(c->mon);
				} else
					c->f &= ~Float;
			}
			break;
		case XA_WM_NORMAL_HINTS:
			c->f &= ~HintsValid;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
#ifdef ICONS
		else if (ev->atom == netatom[NetWMIcon]) {
			updateicon(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
#endif /* ICONS */
	}
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
 	wc.border_width = c->bw;

	/* don't draw borders if monocle/alphamonocle/only 1 client */
	if (((nexttiled(c->mon->clients) == c && !nexttiled(c->next))
	|| (c->mon->lt->arrange == &monocle || c->mon->lt->arrange == &alphamonocle))
	&& (c->fakefullscreen == 1 || !(c->f & FS))
	&& c->mon->lt->arrange && !(c->f & Float)) {
		c->w = wc.width += c->bw * 2;
		c->h = wc.height += c->bw * 2;
		wc.border_width = 0;
	}

	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	/* since enternotify is deleted (because I don't use the mouse and I
	 * don't want the focus on the cursor, this section is not needed */
//	if (c->fakefullscreen == 1)
		/* Exception: if the client was in actual fullscreen and we
		 * exit out to fake fullscreen mode, then the focus would drift
		 * to whichever window is under the mouse cursor at the time. To
		 * avoid this we pass True to XSync which will make the X server
		 * disregard any other events in the queue thus cancelling the
		 * EnterNotify event that would otherwise have changed focus. */
//		XSync(dpy, True);
//	else
	XSync(dpy, False);
}

void
restack(Monitor *m)
{
	Client *c;
	//XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	/* unfocus floating */
	if (m->sel->f & Float || !m->lt->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			//if (!c->isfloating && ISVISIBLE(c)) {
			if (c != m->sel && !(c->f & Float) && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	//while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
	int i, bt, oldflags;
	XEvent ev;
	enum { XFD = LENGTH(blocks) };
	struct pollfd fds[] = { /* one fd for each block + X fd */
		[XFD] = { .fd = ConnectionNumber(dpy), .events = POLLIN }
	};
	if ((oldflags = fcntl(ConnectionNumber(dpy), F_GETFD)) == -1)
		die("fcntl:");
	if (fcntl(ConnectionNumber(dpy), F_SETFD, oldflags | FD_CLOEXEC) == -1)
		die("fcntl:");
	/* init blocks */
	#if INVERSED
	for (i = LENGTH(blocks) - 1; i >= 0; i--)
	#else
	for (i = 0; i < LENGTH(blocks); i++)
	#endif /* INVERSED */
	{
		pipe(pipes[i]);
		/* read end */
		if ((oldflags = fcntl(pipes[i][0], F_GETFD)) == -1)
			die("fcntl F_GETFD failed:");
		if (fcntl(pipes[i][0], F_SETFD, oldflags | FD_CLOEXEC) == -1)
			die("fcntl F_SETFD failed:");
		/* write end */
		if ((oldflags = fcntl(pipes[i][1], F_GETFD)) == -1)
			die("fcntl F_GETFD failed:");
		if (fcntl(pipes[i][1], F_SETFD, oldflags | FD_CLOEXEC) == -1)
			die("fcntl F_SETFD failed:");
		fds[i].fd = pipes[i][0];
		fds[i].events = POLLIN;
		getcmd(i, NULL);
		if (blocks[i].interval) {
			maxinterval = MAX(blocks[i].interval, maxinterval);
			sleepinterval = gcd(blocks[i].interval, sleepinterval);
		}
	}

	XSync(dpy, False);

	/* main event loop */
	while (running > 0) {

		if (!(selmon->f & ShowBar) || !showstatus) { /* bar/status hidden */
			XNextEvent(dpy, &ev);
			if (handler[ev.type])
				handler[ev.type](&ev); /* call handler */
			continue; /* skip poll() */
		}

		if ((poll(fds, LENGTH(fds), -1)) == -1) {
			if (errno == EINTR) /* signal caught */
				continue;
			die("demwm: poll:");
		}

		/* handle X display fd */
		if (fds[XFD].revents & POLLIN) {
			while (running > 0 && XPending(dpy)) {
				XNextEvent(dpy, &ev);
				if (handler[ev.type])
					handler[ev.type](&ev); /* call handler */
			}
		} else if (fds[XFD].revents & POLLHUP)
			die("demwm: poll: X fd event loop, hang up:");

		/* handle blocks */
		for (i = 0; i < LENGTH(blocks); i++) {
			if (fds[i].revents & POLLIN) {
				/* store the len of the string readed  */
				bt = read(fds[i].fd, blockoutput[i], CMDLENGTH);

				/* remove lock for the current block */
				execlock &= ~(1 << i);

				if (bt == -1) {
					LOG("read failed in block '%s':", blocks[i].command);
					perror(" failed");
					continue; /* keep reading */
				}

				if (blockoutput[i][bt - 1] == '\n') /* new ending line present */
					blockoutput[i][bt - 1] = '\0'; /* chop it off */
				else
					blockoutput[i][bt++] = '\0'; /* manually null terminate */

				drawstatus();
				drawbar(selmon);
			} else if (fds[i].revents & POLLHUP)
				die("demwm: poll: block '%d' hangup:", i);
		}
	}
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState) {
				manage(wins[i], &wa);
				setclientprop(selmon->stack);
			}
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
	running = 1;
	focus(NULL);
	arrange(NULL);
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = m->seltags; /* assign tags of target monitor */
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *) data, 2);
}

void
saveclientprop(Client *c)
{	/* send array of client info for 'seamless' restart */
	long data[EMLast] = {
		[EMFlags] = c->f,
		[EMTags]  = c->tags,
		[EMMons]  = c->mon->num,
		[EMPosx]  = c->x,
		[EMPosy]  = c->y,
	};

	XChangeProperty(dpy, c->win, XInternAtom(dpy, "DEMWM_HINTS", False),
		XA_CARDINAL, 32, PropModeReplace, (unsigned char *) data, EMLast);
}

void
setclientprop(Client *c)
{
	Monitor *m;
	unsigned long *p = NULL;

	if (!getatom(c->win, XInternAtom(dpy, "DEMWM_HINTS", False), EMLast, XA_CARDINAL, (unsigned char **)&p))
		return;

	c->f = p[EMFlags] & (LastFlag - 1);
	c->tags = p[EMTags] & TAGMASK;
	for (m = mons; m; m = m->next) {
		if (m->num == p[EMMons]) {
			c->mon = m;
			break;
		}
	}
	c->x = p[EMPosx];
	c->y = p[EMPosy];

	if (p)
		XFree(p);
}

/* vanitygaps */
void
setgaps(int oh, int ov, int ih, int iv)
{
	if (oh < 0) oh = 0;
	if (ov < 0) ov = 0;
	if (ih < 0) ih = 0;
	if (iv < 0) iv = 0;

	selmon->gappoh = oh;
	selmon->gappov = ov;
	selmon->gappih = ih;
	selmon->gappiv = iv;

	if (gapspertag) /* Pertag gaps */
		selmon->pertag->gaps[selmon->pertag->curtag] =
			((oh & 0xFF) << 0) | ((ov & 0xFF) << 8) | ((ih & 0xFF) << 16) | ((iv & 0xFF) << 24);
	arrange(selmon);
}
void
getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc)
{
	unsigned int n, oe, ie;
	Client *c;

	oe = ie = m->pertag->enablegaps & (m->seltags & ~SPTAGMASK) ? 1 : 0;

	/* get the number of clients */
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);

	/* outer gaps disabled when only one client or is disabled in the layout */
	if ((smartgaps && n == 1) || m->lt->gaps)
		oe = 0;

	*oh = m->gappoh * oe;	/* outer horizontal gap */
	*ov = m->gappov * oe;	/* outer vertical gap   */
	*ih = m->gappih * ie;	/* inner horizontal gap */
	*iv = m->gappiv * ie;	/* inner vertical gap   */
	*nc = n;		/* number of clients    */
}
void
getfacts(Monitor *m, int msize, int ssize, float *mf, float *sf, int *mr, int *sr)
{
	unsigned int n;
	float mfacts = 0, sfacts = 0;
	int mtotal = 0, stotal = 0;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
		if (n < m->nmaster)
			mfacts += c->cfact;
		else
			sfacts += c->cfact;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++)
		if (n < m->nmaster)
			mtotal += msize * (c->cfact / mfacts);
		else
			stotal += ssize * (c->cfact / sfacts);

	*mf = mfacts;		/* total factor of master area */
	*sf = sfacts;		/* total factor of stack area */
	*mr = msize - mtotal;	/* remainder (rest) of pixels after a cfacts master split */
	*sr = ssize - stotal;	/* remainder (rest) of pixels after a cfacts stack split */
}
/* vanitygaps */

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!(c->f & NeverFocus)) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
	int savestate = 0, restorestate = 0, restorefakefullscreen = 0;

	if (fullscreen &&
	((c->fakefullscreen == 0 && !(c->f & FS)) /* normal fullscreen */
	|| (c->fakefullscreen == 2))) /* fake fullscreen --> actual fullscreen */
		savestate = 1; /* go actual fullscreen */
	else if (!fullscreen) {
		if ((c->fakefullscreen == 0 && c->f & FS) /* normal fullscreen exit */
		|| (c->fakefullscreen >= 2)) /* fullscreen exit --> fake fullscreen */
			restorestate = 1; /* go back into tiled */

	/* If leaving fullscreen and the window was previously fake fullscreen
	 * (2), then restore that while staying in fullscreen. The exception to
	 * this is if we are in said state, but the client itself disables
	 * fullscreen (3) then we let the client go out of fullscreen while
	 * keeping fake fullscreen enabled (as otherwise there will be a
	 * mismatch between the client and the window manager's perception of
	 * the client's fullscreen state). */
		if (c->fakefullscreen == 2 && c->f & FS) {
			restorefakefullscreen = 1;
			c->f |= FS;
			fullscreen = 1;
		}
	}

	if (fullscreen != (c->f & FS ? 1 : 0)) { /* only send property change if necessary */
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32, PropModeReplace,
			(unsigned char *) (fullscreen ? &netatom[NetWMFullscreen] : 0), fullscreen);
		SETVAL(c, FS, fullscreen);
	}


	/* Some clients, e.g. firefox, will send a client message informing the
	 * window manager that it is going into fullscreen after receiving the
	 * above signal. This has the side effect of this function
	 * (setfullscreen) sometimes being called twice when toggling
	 * fullscreen on and off via the window manager as opposed to the
	 * application itself.  To protect against obscure issues where the
	 * client settings are stored or restored when they are not supposed to
	 * we add an additional bit-lock on the old state so that settings can
	 * only be stored and restored in that precise order. */
	if (savestate && !(c->f & FSLock)) {
		c->oldbw = c->bw;
		c->bw = 0;
		SETVAL(c, WasFloat, c->f & Float);
		c->f |= FSLock | Float;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (restorestate && (c->f & FSLock)) {
		c->f &= ~FSLock;
		c->bw = c->oldbw;
		SETVAL(c, Float, c->f & WasFloat);
		if (restorefakefullscreen || c->fakefullscreen == 3)
			c->fakefullscreen = 1;
		/* The client may have been moved to another monitor whilst in
		 * fullscreen which if tiled we address by doing a full arrange
		 * of tiled clients. If the client is floating then the height
		 * and width may be larger than the monitor's window area, so
		 * we cap that by ensuring max / min values. */
		if (c->f & Float) {
			c->x = MAX(c->mon->wx, c->oldx);
			c->y = MAX(c->mon->wy, c->oldy);
			c->w = MIN(c->mon->ww - c->x - 2*c->bw, c->oldw);
			c->h = MIN(c->mon->wh - c->y - 2*c->bw, c->oldh);
			resizeclient(c, c->x, c->y, c->w, c->h);
			restack(c->mon);
		} else
			arrange(c->mon);
	} else /* FIXME resizeclient seems to not take care of the gaps */
		arrange(c->mon);
		//resizeclient(c, c->x, c->y, c->w, c->h);
}

void
setup(void)
{
	Atom utf8string, netsupported, netwmcheck;
	Colormap cmap;
	sigset_t sm, oldsm;
	Visual *visual = NULL;
	XRenderPictFormat *fmt;
	XSetWindowAttributes wa;
	XVisualInfo tpl = { .depth = 32, .class = TrueColor }, *infos;
	const char wm[] = "demwm";
	int nitems, depth, screen;
	unsigned int i;
	int sw, sh;
	attach = attachmodes[0];

	#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec ps", NULL) == -1)
		die("pledge");
	#endif /* __OpenBSD__ */

	/* init signals */
	sigfillset(&sm);
	sigprocmask(SIG_SETMASK, &sm, &oldsm); /* prevent EINTR by blocking */
	setsignal(SIGCHLD, SIG_DFL); /* zombies */
	setsignal(SIGALRM, sigalrm); /* timer */
	/* for zombies inherited before SA_NOCLDWAIT from .xinitrc, etc */
	while (waitpid(-1, NULL, WNOHANG) > 0);
	sigprocmask(SIG_SETMASK, &oldsm, NULL);
	alarm(1); /* timer */

	/* init X and visual */
	screen = tpl.screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	infos = XGetVisualInfo(dpy, VisualScreenMask|VisualDepthMask|VisualClassMask, &tpl, &nitems);

	for (i = 0; i < nitems; i++) {
		fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = infos[i].visual;
			depth = infos[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			break; /* using ARGB */
		}
	}

	XFree(infos);

	if (!visual) {
		visual = DefaultVisual(dpy, screen);
		depth  = DefaultDepth(dpy, screen);
		cmap   = DefaultColormap(dpy, screen);
	}

	drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts))) {
		LOG("no fonts could be loaded, status bar hidden.");
		showbar = 0;
	}
	lrpad = drw->fonts->h;
	bh    = drw->fonts->h + barh; /* prevent barh being < than font size */
	updategeom();

	/* init atoms */
	netsupported                   = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netwmcheck                     = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	utf8string                     = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols]            = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete]               = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState]                = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus]            = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetWMStateAbove]       = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
	netatom[NetActiveWindow]       = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetWMName]             = XInternAtom(dpy, "_NET_WM_NAME", False);
	#ifdef ICONS
	netatom[NetWMIcon]             = XInternAtom(dpy, "_NET_WM_ICON", False);
	#endif /* ICONS */
	netatom[NetWMState]            = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMFullscreen]       = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowTypeDesktop] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	netatom[NetWMWindowType]       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	//netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList]         = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	netatom[NetWMPid]              = XInternAtom(dpy, "_NET_WM_PID", False);
	#ifdef SYSTRAY
	netatom[NetSystemTray]         = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
	netatom[NetSystemTrayOP]       = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
	netatom[NetSystemTrayOrientation] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION", False);
	netatom[NetSystemTrayOrientationHorz] = XInternAtom(dpy, "_NET_SYSTEM_TRAY_ORIENTATION_HORZ", False);
	netatom[NetSystemTrayVisual]   = XInternAtom(dpy, "_NET_SYSTEM_TRAY_VISUAL", False);
	netatom[NetWMWindowTypeDock]   = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
	xatom[Manager]                 = XInternAtom(dpy, "MANAGER", False);
	xatom[Xembed]                  = XInternAtom(dpy, "_XEMBED", False);
	xatom[XembedInfo]              = XInternAtom(dpy, "_XEMBED_INFO", False);
	#endif /* SYSTRAY */

	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove]   = drw_cur_create(drw, XC_fleur);

	/* init appearance */
	XrmInitialize();
	readxresources();
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 2);

	/* init bars */
	updatebars();
	updatestatus();

	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netwmcheck, XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) wm, sizeof wm);
	XChangeProperty(dpy, root, netwmcheck, XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netsupported, XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* XStoreName */
	XChangeProperty(dpy, root, XA_WM_NAME, utf8string, 8,
		PropModeReplace, (unsigned char *) wm, sizeof wm);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
}

void
setsignal(int sig, void (*sahandler)(int unused))
{
	const struct sigaction sa = {
		.sa_handler = sahandler,
		.sa_flags = SA_RESTART | SA_NOCLDWAIT | SA_NOCLDSTOP,
	};

	if (sigaction(sig, &sa, NULL) == -1)
		die("demwm: sigaction: signal '%d':", sig);
}


void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	SETVAL(c, Urg, urg);

	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;

	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
shift(unsigned int *tag, int i)
{
	if (i > 0) /* left circular shift */
		*tag = ((*tag << i) | (*tag >> (LENGTH(tags) - i))) & ~SPTAGMASK;
	else       /* right circular shift */
		*tag = (*tag >> (- i) | *tag << (LENGTH(tags) + i)) & ~SPTAGMASK;
}

void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt->arrange || c->f & Float) && !(c->f & FS))
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigalrm(int unused)
{
	static volatile unsigned int sleepcount = 0;

	getcmds(sleepcount);
	alarm(sleepinterval);
	sleepcount = (sleepcount + sleepinterval - 1) % maxinterval + 1;
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;

	prevclient = c;
	grabbuttons(c, 0);
	//XSetWindowBorder(dpy, c->win, scheme[BorderNorm][ColFg].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

	if (c->swallowing) {
		unswallow(c);
		return;
	}

	Client *s = swallowingclient(c->win);
	if (s) {
		free(s->swallowing);
		s->swallowing = NULL;
		arrange(m);
		focus(NULL);
		return;
	}

	detach(c);
	detachstack(c);
#ifdef ICONS
	freeicon(c);
#endif /* ICONS */
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	if (scratchpad_last_showed == c)
		scratchpad_last_showed = NULL;

	if (prevclient == c)
		prevclient = NULL;

	free(c);

	if (!s) {
		arrange(m);
		focus(NULL);
		updateclientlist();
	}
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
#ifdef SYSTRAY
	} else if ((c = wintosystrayicon(ev->window))) {
		/* KLUDGE sometimes icons occasionally unmap their windows,
		 * but do _not_ destroy them, we map those windows back */
		XMapRaised(dpy, c->win);
		removesystrayicon(c);
		updatestatus();
#endif /* SYSTRAY */
	}
}

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = drw->cmap,
		.event_mask = ButtonPressMask | ExposureMask
	#ifdef TAG_PREVIEW
		| PointerMotionMask
	#endif /* TAG_PREVIEW */
	};

#ifdef SYSTRAY
	unsigned long systrayorientation = 0; /* _NET_SYSTEM_TRAY_ORIENTATION_HORZ */

	if (!systray) { /* init systray */
		m = systraytomon(NULL);
		systray = ecalloc(1, sizeof(Systray));
		systray->win = XCreateWindow(dpy, root, m->ww, m->by, sysw, bh,
			0, drw->depth, InputOutput, drw->visual, WINMASK, &wa);
		XSelectInput(dpy, systray->win, SubstructureNotifyMask);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *) &systrayorientation, 1);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
				PropModeReplace, (unsigned char *) &drw->visual->visualid, 1);
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &netatom[NetWMWindowTypeDock], 1);
		XUnmapWindow(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendsystrayev(root, netatom[NetSystemTray]);
			XSync(dpy, False);
		}
	}
#endif /* SYSTRAY */

	for (m = mons; m; m = m->next) {
#ifdef TAG_PREVIEW
		if (!m->tagwin) {
			m->tagwin = XCreateWindow(dpy, root, m->wx, m->by + bh,
			    m->ww / 4, m->mh / 4, 0, drw->depth, InputOutput, drw->visual, WINMASK, &wa);
			XDefineCursor(dpy, m->tagwin, cursor[CurNormal]->cursor);
			//XMapRaised(dpy, m->tagwin);
			XUnmapWindow(dpy, m->tagwin);
			XSetClassHint(dpy, m->barwin, &previewclass);
		}
#endif /* TAG_PREVIEW */
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by,
		    m->ww, bh, 0, drw->depth, InputOutput, drw->visual, WINMASK, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &barclass);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->f & ShowBar) {
		m->wh -= bh;
		m->by = m->f & TopBar ? m->wy : m->wy + m->wh;
		m->wy = m->f & TopBar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
#ifdef SYSTRAY
	if (systray)
		XConfigureWindow(dpy, systray->win, CWY, &((XWindowChanges){.y = selmon->by}));
#endif /* SYSTRAY */
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;
		if (n <= nn) { /* new monitors available */
			for (i = 0; i < (nn - n); i++) {
				for (m = mons; m && m->next; m = m->next);
				if (m)
					m->next = createmon();
				else
					mons = createmon();
			}
			for (i = 0, m = mons; i < nn && m; m = m->next, i++)
				if (i >= n
				|| unique[i].x_org != m->mx || unique[i].y_org != m->my
				|| unique[i].width != m->mw || unique[i].height != m->mh)
				{
					dirty = 1;
					m->num = i;
					m->mx = m->wx = unique[i].x_org;
					m->my = m->wy = unique[i].y_org;
					m->mw = m->ww = unique[i].width;
					m->mh = m->wh = unique[i].height;
					updatebarpos(m);
				}
		} else { /* less monitors available nn < n */
			for (i = nn; i < n; i++) {
				for (m = mons; m && m->next; m = m->next);
				while ((c = m->clients)) {
					dirty = 1;
					m->clients = c->next;
					detachstack(c);
					c->mon = mons;
					attachdefault(c);
					attachstack(c);
				}
				if (m == selmon)
					selmon = mons;
				cleanupmon(m);
			}
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != drw->w || mons->mh != drw->h) {
			dirty = 1;
			mons->mw = mons->ww = drw->w;
			mons->mh = mons->wh = drw->h;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i;
	int j;
	XModifierKeymap *modmap = XGetModifierMapping(dpy);
	numlockmask = 0;

	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
			    == XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	SETVAL(c, Fixed, (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh));
	c->f |= HintsValid;
}

void
updatestatus(void)
{
#ifdef SYSTRAY
	updatesystray();
#endif /* SYSTRAY */
	drawbar(selmon);
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strncpy(c->name, broken, sizeof c->name);
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh = XGetWMHints(dpy, c->win);

	if (!wmh)
		return;

	if (wmh->flags & XUrgencyHint) {
		if (c == selmon->sel) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else {
			c->f |= Urg;
			XSetWindowBorder(dpy, c->win, scheme[BorderUrg][ColFg].pixel);
		}
	} else
		c->f &= ~Urg;

	if (wmh->flags & InputHint)
		SETVAL(c, NeverFocus, !wmh->input);
	else
		c->f &= ~NeverFocus;

	XFree(wmh);
}

pid_t
winpid(Window w)
{
	pid_t ret = 0;
	unsigned char *p = NULL;

	if (getatom(w, netatom[NetWMPid], 1L, AnyPropertyType, &p)) {
		ret = *(pid_t *)p;
		XFree(p);
		return ret;
	}

#ifdef __linux__
	static xcb_connection_t *xcon = NULL;

	if (!xcon)
		xcon = XGetXCBConnection(dpy);
	if (!xcon)
		die("demwm: cannot get xcb connection.");

	enum { _XPID = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID };
	xcb_generic_error_t *e = NULL;
	xcb_res_client_id_spec_t spec = { .client = w, .mask = _XPID };
	xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
	xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

	if (!r)
		return (pid_t)0;

	xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);

	for (; i.rem; xcb_res_client_id_value_next(&i)) {
		spec = i.data->spec;
		if (spec.mask & _XPID) {
			ret = *xcb_res_client_id_value_value(i.data);
			break;
		}
	}

	free(r);

	if (ret == (pid_t) - 1)
		ret = 0;

#endif /* __linux__ */

	return ret;
}

pid_t
getparentprocess(pid_t p)
{
	unsigned int v = 0;

#ifdef __linux__
	FILE *f;
	char buf[256];
	snprintf(buf, sizeof(buf) - 1, "/proc/%u/stat", (unsigned)p);

	if (!(f = fopen(buf, "r")))
		return 0;

	if (fscanf(f, "%*u %*s %*c %u", (unsigned *)&v) != 1)
		v = 0;

	fclose(f);
#elif __FreeBSD__
	struct kinfo_proc *proc = kinfo_getproc(p);
	if (!proc)
		return (pid_t)0;

	v = proc->ki_ppid;
	free(proc);
#endif
	return (pid_t)v;
}

int
isdescprocess(pid_t p, pid_t c)
{
	while (p != c && c != 0)
		c = getparentprocess(c);

	return (int)c;
}

Client *
termforwin(const Client *w)
{
	Client *c;
	Monitor *m;

	if (!w->pid || w->f & Terminal)
		return NULL;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->f & Terminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid))
				return c;
		}
	}

	return NULL;
}

Client *
swallowingclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->swallowing && c->swallowing->win == w)
				return c;

	return NULL;
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	LOG("fatal error: request code='%d', error code='%d'",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("demwm: another window manager is already running");
	return -1;
}

/* dynamic scratchpads */
int
scratchpad_last_showed_is_killed(void)
{
	Client *c;
	int killed = 1;

	for (c = selmon->clients; c && killed; c = c->next)
		if (c == scratchpad_last_showed)
			killed = 0;

	return killed;
}
void
scratchpad_show_client(Client *c)
{
	scratchpad_last_showed = c;
	c->tags = selmon->seltags;
	focus(c);
	arrange(selmon);
}
void
scratchpad_show_first(void)
{
	for (Client *c = selmon->clients; c && c->tags == SCRATCHPAD_MASK; c = c->next)
		scratchpad_show_client(c);
}

int
main(int argc, char *argv[])
{
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		LOG("warning, no locale support.");
	if (!(dpy = XOpenDisplay(NULL)))
		die("demwm: cannot open display.");
	root = RootWindow(dpy, DefaultScreen(dpy));

	if (argc == 2 || argc == 3) {
		char func[IPCSIZE];
		unsigned int i;
		int cmd = -1;

		for (i = 0; i < LENGTH(parsetable); i++) {
			if (!strcmp(argv[1], parsetable[i].name)) {
				cmd = i;
				break;
			}
		}
		if (cmd == -1) {
			if (!strcmp(argv[1], "version"))
				die("demwm-"VERSION);
			else if (!strcmp(argv[1], "help"))
				die("demwm: [function] (arg)");
			else
				die("'%s' is not a function.", argv[1]);
		}

		if (parsetable[cmd].type != NOOa && argc == 2)
			die("Function '%s' requires an argument.", argv[1]);

		snprintf(func, sizeof func, cmd < 10 ? "0%d %s" : "%d %s", cmd, argv[2]);
		XChangeProperty(dpy, root, XInternAtom(dpy, "DEMWM_IPC", False),
		    XInternAtom(dpy, "UTF8_STRING", False), 8, PropModeReplace,
		    (unsigned char *) func, sizeof func);
		XCloseDisplay(dpy);
		return EXIT_SUCCESS;
	}

	checkotherwm();
	setup();
	scan();
	run();
	cleanup();
	if (running == -1)
		execlp(argv[0], argv[0], (char *) NULL);
	return EXIT_SUCCESS;
}
