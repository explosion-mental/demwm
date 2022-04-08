/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/wait.h>	/* dwm doesn't use wait? */
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
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
#endif /* __OpenBSD */
#ifdef TAG_PREVIEW
#include <Imlib2.h>
#endif /* TAG_PREVIEW */
#ifdef ICONS
#include <limits.h>
#include <stdint.h>
#endif /* ICONS */

#include "drw.h"
#include "util.h"
#include <X11/XF86keysym.h>	/* XF86 Keys */

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define OPAQUE                  0xffU	/* borders */
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
/* scratchpad */
#define NUMTAGS			(LENGTH(tags) + LENGTH(scratchpads))
//#define TAGMASK		((1 << LENGTH(tags)) - 1)
#define TAGMASK     		((1 << NUMTAGS) - 1)
#define SPTAG(i) 		((1 << LENGTH(tags)) << (i))
#define SPTAGMASK   		(((1 << LENGTH(scratchpads))-1) << LENGTH(tags))
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define RULE(...)		{ .monitor = -1, __VA_ARGS__ },
#define Button6			6
#define Button7			7
#ifdef SYSTRAY
/* XEMBED messages */
#define VERSION_MAJOR               0
#define VERSION_MINOR               0
#define XEMBED_MAPPED              (1 << 0)
#define XEMBED_EMBEDDED_VERSION (VERSION_MAJOR << 16) | VERSION_MINOR

enum { Manager, Xembed, XembedInfo, XLast }; /* Xembed atoms */
#endif /* SYSTRAY */

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel, SchemeUrgent, SchemeLt, SchemeTitle,
	SchemeStatus, SchemeDelim, SchemeSys, SchemeNotify, SchemeIndOff,
	SchemeIndOn, BorderNorm, BorderSel, BorderFloat, BorderUrg }; /* color schemes */
enum { Sp1, Sp2, Sp3, Sp4, Sp5, Sp6, Sp7, Sp8 }; /* scratchpads */
enum { NetSupported, NetWMName,
#ifdef ICONS
       NetWMIcon,
#endif /* ICONS */
       NetWMState, NetWMCheck, NetWMFullscreen, NetActiveWindow,
       NetWMWindowTypeDesktop, NetWMWindowType, NetWMStateAbove,
       //NetWMWindowTypeDialog,
       NetClientList,
#ifdef SYSTRAY
       NetSystemTray, NetSystemTrayOP, NetSystemTrayOrientation,
       NetSystemTrayVisual, NetWMWindowTypeDock, NetSystemTrayOrientationHorz,
#endif /* SYSTRAY */
       NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMWindowRole, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	unsigned int ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
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
	int isfixed, isurgent, neverfocus, oldstate;
	/* rules */
	int isfloating, isfullscreen, isterminal, noswallow;
	int fakefullscreen, alwaysontop, uncursor;
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
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
	const int gaps;
} Layout;

typedef struct Pertag Pertag;
struct Monitor {
	char ltsymbol[16];
	float mfact;
	int nmaster;
	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */
	int gappih;           /* horizontal gap between windows */
	int gappiv;           /* vertical gap between windows */
	int gappoh;           /* horizontal outer gaps */
	int gappov;           /* vertical outer gaps */
	unsigned int borderpx;
	unsigned int seltags;
	unsigned int sellt;
	unsigned int tagset[2];
	int showbar;
	int topbar;
	Client *clients;
	Client *sel;
	Client *stack;
	Monitor *next;
	Window barwin;
#ifdef TAG_PREVIEW
	Window tagwin;
	int previewshow;
	Pixmap tagmap[TAGNUM];
#endif /* TAG_PREVIEW */
	const Layout *lt[2];
	Pertag *pertag;
};

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	const char *wintype;
	const char *role;
	unsigned int tags;
	int isfloating;
	int isterminal;
	int noswallow;
	int isfakefullscreen;
	int uncursor;
	int monitor;
} Rule;

#ifdef SYSTRAY
typedef struct {
	Window win;
	Client *icons;
} Systray;
#endif /* SYSTRAY */

typedef struct {
	const char *color;
	const char *command;
	const unsigned int interval;
	const unsigned int signal;
} Block;

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void combotag(const Arg *arg);
static void comboview(const Arg *arg);
static Monitor *createmon(void);
static void cyclelayout(const Arg *arg);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void expose(XEvent *e);
static Client *findbefore(Client *c);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
#ifdef ICONS
static uint32_t prealpha(uint32_t p);
static Picture geticonprop(Window w, unsigned int *icw, unsigned int *ich);
static void freeicon(Client *c);
static void updateicon(Client *c);
#endif /* ICONS */
static void getcmd(int i, char *button);
static void getcmds(int time);
static void getsigcmds(unsigned int signal);
static int gcd(int a, int b);
static int getstatus(int width);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void keyrelease(XEvent *e);
static void killclient(const Arg *arg);
static void losefullscreen(Client *next);
static void fallbackcolors();
static void readxresources(void);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static Client *nexttiled(Client *c);
static void pop(Client *);
static Client *prevtiled(Client *c);
static void propertynotify(XEvent *e);
static void pushstack(const Arg *arg);
static void quit(const Arg *arg);
static void refresh(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void resizemousescroll(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
#ifdef SYSTRAY
static void resizebarwin(Monitor *m);
static Atom getatomprop(Client *c, Atom prop);
static unsigned int getsystraywidth(void);
static void removesystrayicon(Client *i);
static void resizerequest(XEvent *e);
static Monitor *systraytomon(Monitor *m);
static void updatesystray(void);
static void updatesystrayicongeom(Client *i, int w, int h);
static void updatesystrayiconstate(Client *i, XPropertyEvent *ev);
static Client *wintosystrayicon(Window w);
static int sendevent(Window w, Atom proto, int m, long d0, long d1, long d2, long d3, long d4);
#else
static int sendevent(Client *c, Atom proto);
#endif /* SYSTRAY */
static void sendmon(Client *c, Monitor *m);
static void sendstatusbar(const Arg *arg);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setcfact(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void settagsatom(Window w, unsigned int tags);
static void shift(void (*func)(const Arg *arg));
static void shifttag(const Arg *arg);
static void shifttagclients(const Arg *arg);
static void shiftview(const Arg *arg);
static void shiftviewclients(const Arg *arg);
static void shiftboth(const Arg *arg);
static void shiftswaptags(const Arg *arg);
static void showhide(Client *c);
#ifdef TAG_PREVIEW
static void showtagpreview(int tag);
static void switchtag(void);
static void updatepreview(void);
#endif /* TAG_PREVIEW */
static void sigchld(int unused);
static void sighandler(int signum);
static void sighup(int unused);
static void sigterm(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefakefullscreen(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglefullscreen(const Arg *arg);
static void togglescratch(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateblock(const Arg *arg);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewmhints(Client *c);
static void view(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xrdb(const Arg *arg);
static void xinitvisual(void);
static void zoom(const Arg *arg);
static void zoomswap(const Arg *arg);

static void scratchpad_hide(const Arg *arg);
static void scratchpad_remove(const Arg *arg);
static void scratchpad_show(const Arg *arg);
static void scratchpad_show_client(Client * c);
static int scratchpad_last_showed_is_killed (void);
static void scratchpad_show_first(void);

/* vanitygaps */
static int enablegaps = 1;	/* if not gaps per tag */
static void togglesmartgaps(const Arg *arg);
static void togglegaps(const Arg *arg);
static void defaultgaps(const Arg *arg);
static void getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc);
static void getfacts(Monitor *m, int msize, int ssize, float *mf, float *sf, int *mr, int *sr);
static void setgaps(int oh, int ov, int ih, int iv);
static void incrgaps(const Arg *arg);	/* inner + outter gaps */

static void incrigaps(const Arg *arg);	/* inner gaps (horiz + vert) */
static void incrivgaps(const Arg *arg);	/* vertical inner gaps */
static void incrihgaps(const Arg *arg);	/* horizontal inner gaps */

static void incrogaps(const Arg *arg);	/* outter gaps (horiz + vert) */
static void incrohgaps(const Arg *arg);	/* vertcal outter gaps */
static void incrovgaps(const Arg *arg);	/* horizontal outter gaps */

/* Customs */
static void togglealwaysontop(const Arg *arg);
static void swaptags(const Arg *arg);
//static void loadrandom_wall(const Arg *arg);
static void random_wall(const Arg *arg);
//static void toggletopbar(const Arg *arg);
//static void toggleborder(const Arg *arg);
static void togglevacant(const Arg *arg);
static void togglestatus(const Arg *arg);
static void spawncmd(const Arg *arg);

static pid_t getparentprocess(pid_t p);
static int isdescprocess(pid_t p, pid_t c);
static Client *swallowingclient(Window w);
static Client *termforwin(const Client *c);
static pid_t winpid(Window w);

/* variables */
static const char broken[] = "broken";
static unsigned int sleepinterval = 0, maxinterval = 0, count = 0;
static unsigned int lasttags; /* keep selected tags on restart */
static unsigned int stsw = 0; /* status width */
static unsigned int blocknum; /* blocks idx in mouse click */
static int combo = 0;         /* combo flag */
static int sw, sh;            /* X display screen geometry width, height */
static int bh, blw = 0;       /* bar geometry */
static int lrpad;             /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
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
#ifdef SYSTRAY
static Atom xatom[XLast];
static Systray *systray = NULL;
static unsigned long systrayorientation = 0; /* _NET_SYSTEM_TRAY_ORIENTATION_HORZ */
#endif /* SYSTRAY */
static Atom wmatom[WMLast], netatom[NetLast], dwmstatus, dwmtags;
static int running = 1, restart = 0;
static int depth, screen, useargb = 0;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
static xcb_connection_t *xcon;
static Visual *visual;
static Colormap cmap;
static Client *scratchpad_last_showed = NULL;

static char dmenumon[2] = "0"; /* dmenu default selected monitor */

/* various layouts to use on the config */
#include "layouts.c"

/* configuration, allows nested code to access above variables */
#include "config.h"

/* dynamic scratchpads (this selects an unused tag) */
#define SCRATCHPAD_MASK		(1 << (NUMTAGS + 1))

static char blockoutput[LENGTH(blocks)][CMDLENGTH + 1] = {0};
static int pipes[LENGTH(blocks)][2];
static int execlock = 0; /* ensure only one child process exists per block at an instance */
static int isalarm = 0, isrtsig = 0;

struct Pertag {
	unsigned int curtag, prevtag;		/* current and previous tag */
	int nmasters[LENGTH(tags) + 1];		/* number of windows in master area */
	float mfacts[LENGTH(tags) + 1];		/* mfacts per tag */
	unsigned int sellts[LENGTH(tags) + 1];	/* selected layouts */
	const Layout *ltidxs[LENGTH(tags)+1][2];/* matrix of tags and layouts indexes */
	int showbars[LENGTH(tags) + 1];		/* display bar for the current tag */
	Client *prevzooms[LENGTH(tags) + 1];	/* store zoom information */
	int enablegaps[LENGTH(tags) + 1];	/* added with vanitygaps */
	unsigned int gaps[LENGTH(tags) + 1];	/* gaps per tag */
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[NUMTAGS > 31 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };
	Atom wintype;
	char role[64];

	/* rule matching */
	c->isfloating = 0;
	c->tags = 0;
	c->alwaysontop = 0;
	c->uncursor = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;
	gettextprop(c->win, wmatom[WMWindowRole], role, sizeof(role));
	wintype  = getatomprop(c, netatom[NetWMWindowType]);

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->role || strstr(role, r->role))
		&& (!r->instance || strstr(instance, r->instance))
		&& (!r->wintype || wintype == XInternAtom(dpy, r->wintype, False)))
		{
			c->isterminal = r->isterminal;
			c->noswallow  = r->noswallow;
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			c->fakefullscreen = r->isfakefullscreen;
			c->uncursor = r->uncursor;

			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->tags = c->tags & TAGMASK ? c->tags & TAGMASK : (c->mon->tagset[c->mon->seltags] & ~SPTAGMASK);
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
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
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
	if (resizehints || (c->isfloating && floathints) || !c->mon->lt[c->mon->sellt]->arrange) {
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
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
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
	if (c->noswallow || c->isterminal)
		return;
	if (!swallowfloating && c->isfloating)
		return;

	detach(c);
	detachstack(c);

	setclientstate(c, WithdrawnState);
	XUnmapWindow(dpy, p->win);

	p->swallowing = c;
	c->mon = p->mon;

	Window w = p->win;
	p->win = c->win;
	c->win = w;
#ifdef ICONS
	updateicon(p);
#endif /* ICONS */
	updatetitle(p);

	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);

	XWindowChanges wc;
	wc.border_width = p->bw;
	XConfigureWindow(dpy, p->win, CWBorderWidth, &wc);
	XMoveResizeWindow(dpy, p->win, p->x, p->y, p->w, p->h);
	XSetWindowBorder(dpy, p->win, scheme[c->isfloating ? BorderFloat : BorderSel][ColFg].pixel);

	arrange(p->mon);
	configure(p);
	updateclientlist();
}

void
unswallow(Client *c)
{
	c->win = c->swallowing->win;

	free(c->swallowing);
	c->swallowing = NULL;

	/* unfullscreen the client */
	setfullscreen(c, 0);
#ifdef ICONS
	freeicon(c);
#endif /* ICONS */
	updatetitle(c);
	arrange(c->mon);
	XMapWindow(dpy, c->win);


	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);

	XWindowChanges wc;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
	XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	XSetWindowBorder(dpy, c->win, scheme[c->isfloating ? BorderFloat : BorderSel][ColFg].pixel);

	setclientstate(c, NormalState);
	focus(NULL);
	arrange(c->mon);
}

void
buttonpress(XEvent *e)
{
	unsigned int i, x, click, occ = 0;
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
#ifdef TAG_PREVIEW
		if (selmon->previewshow) {
			XUnmapWindow(dpy, selmon->tagwin);
				selmon->previewshow = 0;
		}
#endif /* TAG_PREVIEW */
		i = x = 0;
		for (c = m->clients; c; c = c->next)
			occ |= c->tags == 255 ? 0 : c->tags;
		do {
			/* do not reserve space for vacant tags */
			if (!(occ & 1 << i || m->tagset[m->seltags] & 1 << i) && hidevacant)
				continue;
			x += TEXTW(hidevacant ? tagsalt[i] : tags[i]);
		} while (ev->x >= x && ++i < (hidevacant ? LENGTH(tagsalt) : LENGTH(tags)));
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		} else if (ev->x < x + blw)
			click = ClkLtSymbol;
		else if (ev->x > (x = selmon->ww - stsw
			#ifdef SYSTRAY
			- getsystraywidth()
			#endif /* SYSTRAY */
			)) {
			click = ClkStatusText;
			int len, i;
			//int last = LENGTH(blocks) - 1;
			#if INVERSED
			for (i = LENGTH(blocks) - 1; i >= 0; i--)
			#else
			for (i = 0; i < LENGTH(blocks); i++)
			#endif /* INVERSED */
			{
				if (*blockoutput[i] == '\0') //ignore command that output NULL or '\0'
					continue;
				len = TEXTW(blockoutput[i]) - lrpad + TEXTW(delimiter) - lrpad;
				x += len;
				if (ev->x <= x && ev->x >= x - len) { /* if the mouse is between the block area */
					blocknum = i; /* store what block the mouse is clicking */
					break;
				}
			}
		} else
			click = ClkWinTitle;
	} else if ((c = wintoclient(ev->window))) {
		//if (ev->button != Button4 && ev->button != Button5)
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
	Arg a = {.ui = ~0};
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	view(&a);
	selmon->lt[selmon->sellt] = &foo;
	for (m = mons; m; m = m->next)
		while (m->stack)
			unmanage(m->stack, 0);
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
 	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
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
#endif /* TAG_PREVIEW */
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
#ifdef TAG_PREVIEW
	XUnmapWindow(dpy, mon->tagwin);
	XDestroyWindow(dpy, mon->tagwin);
#endif /* TAG_PREVIEW */
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

	if (systray && cme->window == systray->win && cme->message_type == netatom[NetSystemTrayOP]) {
		/* add systray icons */
		if (cme->data.l[1] == 0 /* SYSTEM_TRAY_REQUEST_DOCK */) {
			c = ecalloc(1, sizeof(Client));
			if (!(c->win = cme->data.l[2])) {
				free(c);
				return;
			}
			c->mon = selmon;
			c->next = systray->icons;
			systray->icons = c;
			XGetWindowAttributes(dpy, c->win, &wa);

			c->x = c->oldx = c->y = c->oldy = 0;
			c->w = c->oldw = wa.width;
			c->h = c->oldh = wa.height;
			c->oldbw = wa.border_width;
			c->bw = 0;
			c->isfloating = True;
			/* reuse tags field as mapped status */
			c->tags = 1;
			updatesizehints(c);
			updatesystrayicongeom(c, wa.width, wa.height);
			XAddToSaveSet(dpy, c->win);
			XSelectInput(dpy, c->win, StructureNotifyMask | PropertyChangeMask | ResizeRedirectMask);
			//XClassHint ch = {"dwmsystray", "dwmsystray"};
			//XSetClassHint(dpy, c->win, &ch);
			swa.background_pixel  = 0;
			//swa.background_pixel  = scheme[SchemeNorm][ColBg].pixel;	/* use parents background color */
			//swa.background_pixel  = 0;
			//swa.border_pixel = 0;
			//wa.background_pixel = 0;
			//swa.colormap = cmap;
			XChangeWindowAttributes(dpy, c->win, CWBackPixel, &swa);
			XReparentWindow(dpy, c->win, systray->win, 0, 0);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
					0 /* XEMBED_EMBEDDED_NOTIFY */, 0, systray->win, XEMBED_EMBEDDED_VERSION);

			/* FIXME are these events needed? */
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
					0 /* XEMBED_FOCUS_IN */, 0, systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
					1 /* XEMBED_WINDOW_ACTIVATE */, 0, systray->win, XEMBED_EMBEDDED_VERSION);
			sendevent(c->win, netatom[Xembed], StructureNotifyMask, CurrentTime,
					10 /* XEMBED_MODALITY_ON */, 0, systray->win, XEMBED_EMBEDDED_VERSION);

			XSync(dpy, False);
			updatesystray();
			setclientstate(c, NormalState);
		}
		return;
	}
#endif /* SYSTRAY */
	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen]) {
			if (c->fakefullscreen == 2 && c->isfullscreen)
				c->fakefullscreen = 3;
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
		} else if (cme->data.l[1] == netatom[NetWMStateAbove]
		|| cme->data.l[2] == netatom[NetWMStateAbove])
			c->alwaysontop = (cme->data.l[0] || cme->data.l[1]);
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
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
					if (c->isfullscreen && c->fakefullscreen != 1)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
		#ifdef SYSTRAY
				resizebarwin(m);
		#else
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
		#endif /* SYSTRAY */
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
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
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
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
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

void
combotag(const Arg *arg)
{
	unsigned int newtags = arg->ui & TAGMASK;

	//unsigned int newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);

	if (!selmon->sel || !newtags)
		return;

	if (combo)
		selmon->sel->tags |= newtags;
	else {
		combo = 1;
		selmon->sel->tags = newtags;
	}

	settagsatom(selmon->sel->win, selmon->sel->tags);
	focus(NULL);
	arrange(selmon);
}

void
comboview(const Arg *arg)
{
	unsigned int newtags = arg->ui & TAGMASK;

	//unsigned int newtags = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (combo) {

		/* tagallmaster: clients in the master area should be the same
		 * after we add a new tag */
		Client *const selected = selmon->sel;
		Client **const masters = ecalloc(selmon->nmaster, sizeof(Client *));
		/* collect (from last to first) references to all clients in the master area */
		Client *c;
		size_t i;
		for (c = nexttiled(selmon->clients), i = 0; c && i < selmon->nmaster; c = nexttiled(c->next), ++i)
			masters[selmon->nmaster - (i + 1)] = c;
		/* put the master clients at the front of the list > go from
		 * the 'last' master to the 'first' */
		for (i = 0; i < selmon->nmaster; ++i)
			if (masters[i])
				pop(masters[i]);
		free(masters);
		if (selmon->sel != selected) /* don't mutate the focus */
			focus(selected);

		/* toggleview */
		selmon->tagset[selmon->seltags] |= newtags;
	} else {
		combo = 1;
		if (newtags)
			view(&((Arg) { .ui = newtags }));
	}

	focus(NULL);
	arrange(selmon);
}

Monitor *
createmon(void)
{
	Monitor *m;
	unsigned int i;

	m = ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = lasttags != 0 ? lasttags : 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->borderpx = borderpx;
	/* gaps per tag */
	m->gappih = gappih;
	m->gappiv = gappiv;
	m->gappoh = gappoh;
	m->gappov = gappov;

	m->lt[0] = taglayouts[0] && taglayouts[0] < LENGTH(layouts) ? &layouts[taglayouts[0]] : &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, taglayouts[1] && taglayouts[1] < LENGTH(layouts) ? layouts[taglayouts[1]].symbol : layouts[0].symbol, sizeof m->ltsymbol);

	if (pertag) {
		m->pertag = ecalloc(1, sizeof(Pertag));
		m->pertag->curtag = m->pertag->prevtag = 1;

		/* init layouts */
		m->pertag->ltidxs[0][0] = &layouts[0];
		for (i = 1; i <= LENGTH(tags); i++)
			m->pertag->ltidxs[i][0] = &layouts[taglayouts[i - 1]];

		for (i = 0; i <= LENGTH(tags); i++) {
			/* init nmaster */
			m->pertag->nmasters[i] = m->nmaster;

			/* init mfacts */
			m->pertag->mfacts[i] = m->mfact;

			m->pertag->sellts[i] = m->sellt;
			m->pertag->ltidxs[i][1] = m->lt[0];

			/* init showbar */
			if (pertagbar)
				m->pertag->showbars[i] = m->showbar;

			/* swap focus and zoomswap*/
			m->pertag->prevzooms[i] = NULL;

			/* compatibility with pertag and togglegaps line in createmon */
			m->pertag->enablegaps[i] = 1;

			if (gapspertag)
				m->pertag->gaps[i] = ((gappoh & 0xFF) << 0) | ((gappov & 0xFF) << 8) | ((gappih & 0xFF) << 16) | ((gappiv & 0xFF) << 24);
		}
	}	/* if pertag */
	return m;
}

void
cyclelayout(const Arg *arg)
{
	Layout *l;
	for (l = (Layout *)layouts; l != selmon->lt[selmon->sellt]; l++);
	if (arg->i > 0) {
		if (l->symbol && (l + 1)->symbol)
			setlayout(&((Arg) { .v = (l + 1) }));
		else
			setlayout(&((Arg) { .v = layouts }));
	} else {
		if (l != layouts && (l - 1)->symbol)
			setlayout(&((Arg) { .v = (l - 1) }));
		else
			setlayout(&((Arg) { .v = &layouts[LENGTH(layouts) - 2] }));
	}
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
		updatesystray();
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

	if (!m->showbar)
		return;

	#ifdef SYSTRAY
	bw -= m == systraytomon(m) ? getsystraywidth() : 0;
	#endif /* SYSTRAY */

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) /* status is only drawn on selected monitor */
		tw = getstatus(bw);

	#ifdef SYSTRAY
	resizebarwin(m);
	#endif /* SYSTRAY */

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags == 255 ? 0 : c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}

	for (i = 0; i < LENGTH(tags); i++) {
		/* apply 'hidevacant' only to the selected monitor */
		if (hidevacant && (!(occ & 1 << i || selmon->tagset[selmon->seltags] & 1 << i)))
			continue;
		w = TEXTW(hidevacant ? tagsalt[i] : tags[i]);
	        if (urg & 1 << i)
			drw_setscheme(drw, scheme[SchemeUrgent]);
		else
			drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, hidevacant ? tagsalt[i] : tags[i], urg & 1 << i);
		if (occ & 1 << i && !hidevacant) { /* don't draw these when hidevacant */
			if (urg & 1 << i) {
				/* urgent underline (top of tag) */
				drw_setscheme(drw, scheme[SchemeNotify]);
                		drw_rect(drw, x, 0, w, boxs, 1, 0);
			}
			/* normal underline (IndOn/Off) */
			drw_setscheme(drw, scheme[(m == selmon && selmon->sel && selmon->sel->tags & 1 << i) ? SchemeIndOn : SchemeIndOff]);
			drw_rect(drw, x, bh - boxs - 1, w, boxs + 1, 1, 0);
		}
		x += w;
	}
	/* monocle, count clients if there are more than one */
	if (m->lt[m->sellt]->arrange == &monocle || m->lt[m->sellt]->arrange == &alphamonocle) {
		for (a = 0, s = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), a++)
			if (c == m->stack)
				s = a;
		if (!s && a)
			s = a;
		if (a > 1) {
			if (m->lt[m->sellt]->arrange == &monocle)
				snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d/%d]", s, a);
			else
				snprintf(m->ltsymbol, sizeof m->ltsymbol, "{%d/%d}", s, a);
		}
	}
	w = blw = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeLt]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	if ((w = bw - tw - x) > bh) {
		if (m->lt[m->sellt]->arrange == &clear) { /* hide title in clear layout */
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
			if (m->sel->alwaysontop) {
				drw_setscheme(drw, scheme[SchemeLt]);
				drw_rect(drw, x, 0, w, boxw, 1, 0);
			} else if (m->sel->isfloating)
				drw_rect(drw, bw - tw - boxw - 1, boxs, boxw, boxw, m->sel->isfixed, 0);
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
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c) {
		/* if we don't losefullscreen, we can stack fullscren clients */
		//losefullscreen(c);
		unfocus(selmon->sel, 0);
	}

	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[c->isfloating ? BorderFloat : BorderSel][ColFg].pixel);
		setfocus(c);
		if (c->mon->lt[c->mon->sellt]->arrange) {
			/* Move all visible tiled clients that are not marked as on top below the bar window */
			wc.stack_mode = Below;
			wc.sibling = c->mon->barwin;
			for (f = c->mon->stack; f; f = f->snext)
				if (f != c && !f->isfloating && ISVISIBLE(f) && !f->alwaysontop) {
					XConfigureWindow(dpy, f->win, CWSibling|CWStackMode, &wc);
					wc.sibling = f->win;
				}
			/* Move the currently focused client above the bar window */
			wc.stack_mode = Above;
			wc.sibling = c->mon->barwin;
			XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);

			/* Move all visible floating windows that are not marked as on top, below the bar window */
			wc.stack_mode = Below;
			/* since most floating 'pop ups' get focus
			 * automatically, there is no need to raise (or when
			 * switching tags, re-raise) the floating windows if
			 * the user has already changed the focus to the tiled
			 * window (or another client in that case). */
			//if (c->tags == SCRATCHPAD_MASK || (c->tags & SPTAGMASK))
			//	wc.sibling = c->win;//c->win;
			//else
				wc.sibling = c->mon->barwin;//c->win;

			for (f = c->mon->stack; f; f = f->snext)
				if (f != c && f->isfloating && ISVISIBLE(f) && !f->alwaysontop) {
					XConfigureWindow(dpy, f->win, CWSibling|CWStackMode, &wc);
					wc.sibling = f->win;
				}
		}

		if (c->uncursor) /* put the cursor in the bottom right */
			XWarpPointer(dpy, None, root, 0, 0, 0, 0, sw, sh);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}

	selmon->sel = c;
	if (selmon->lt[selmon->sellt]->arrange == alphamonocle)
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

void
focusmon(const Arg *arg)
{
	Monitor *m;
	Client *sel;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	sel = selmon->sel;
	selmon = m;
	unfocus(sel, 0);
	focus(NULL);
	XWarpPointer(dpy, None, m->barwin, 0, 0, 0, 0, m->mw / 2, m->mh / 2);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	/* Note that this patch is made for dwm 6.2. If applying this on top of
	 * the latest master then the
	 * https://dwm.suckless.org/patches/alwaysfullscreen/ patch has been
	 * merged upstream, which prevents focus to drift from windows that are
	 * in fake fullscreen. To address this issue use this if statement
	 * instead:
	 if (!selmon->sel || (selmon->sel->isfullscreen && selmon->sel->fakefullscreen != 1))
	 */

	if (!selmon->sel)
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
	}
}

void
pushstack(const Arg *arg)
{
	Client *c, *sel = selmon->sel;

	if (!sel || sel->isfloating)
		return;

	if (arg->i > 0) { /* pushdown */
		if ((c = nexttiled(sel->next))) {
			detach(sel);
			sel->next = c->next;
			c->next = sel;
		} else { /* move to the front */
			detach(sel);
			attach(sel);
		}
	} else { /* pushup */
		if ((c = prevtiled(sel)) && c != nexttiled(selmon->clients)) {
			detach(sel);
			sel->next = c;
			for (c = selmon->clients; c->next != sel->next; c = c->next);
			c->next = sel;
		} else { /* move to the end */
			for (c = sel; c->next; c = c->next);
			if (sel != c) {
				detach(sel);
				sel->next = NULL;
				c->next = sel;
			}
		}
	}
	focus(sel);
	arrange(selmon);
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
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

/* Tag preview functions */
#ifdef TAG_PREVIEW
void
updatepreview(void)
{
	Monitor *m;

	XSetWindowAttributes wa = {
		.override_redirect = True,
		.event_mask = ButtonPressMask|ExposureMask,
	//	.background_pixmap = 0,
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = cmap
	};
	for (m = mons; m; m = m->next) {
		m->tagwin = XCreateWindow(dpy, root, m->wx, m->by + bh, m->ww / 4, m->mh / 4, 0, depth,
		                          CopyFromParent, visual,
		                          CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);
		XDefineCursor(dpy, m->tagwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->tagwin);
		XUnmapWindow(dpy, m->tagwin);
	}
}

void
switchtag(void)
{
  	int i;
	unsigned int occ = 0;
	Client *c;
	Imlib_Image image;

	for (c = selmon->clients; c; c = c->next)
		occ |= c->tags == 255 && hidevacant ? 0 : c->tags;
	//	occ |= c->tags == 255 ? 0 : c->tags;
	for (i = 0; i < LENGTH(tags); i++) {
		if (selmon->tagset[selmon->seltags] & 1 << i) {
                  	if (selmon->tagmap[i] != 0) {
 				XFreePixmap(dpy, selmon->tagmap[i]);
 				selmon->tagmap[i] = 0;
 			}
			/* do not reserve space for vacant tags */
			//if (!(occ & 1 << i || selmon->tagset[selmon->seltags] & 1 << i) && hidevacant) {
			//	continue;

	//	if (hidevacant) {
	//		/* do not draw vacant tags */
	//		if (!(occ & 1 << i || selmon->tagset[selmon->seltags] & 1 << i))
	//		continue;
	//	}
			if (occ & 1 << i) {
				image = imlib_create_image(sw, sh);
				imlib_context_set_image(image);
				imlib_context_set_display(dpy);
				imlib_image_set_has_alpha(1);
				imlib_context_set_blend(0);
				imlib_context_set_visual(visual);

				//imlib_context_set_visual(drw->visual);
				imlib_context_set_drawable(root);
				// screen size (m{x,y,w,h}) -> window areas, without the bar (w{x,y,w,h})
				imlib_copy_drawable_to_image(0, selmon->wx, selmon->wy, selmon->ww ,selmon->wh, 0, 0, 1);
				selmon->tagmap[i] = XCreatePixmap(dpy, selmon->tagwin, selmon->mw / scalepreview, selmon->mh / scalepreview, depth);
				imlib_context_set_drawable(selmon->tagmap[i]);
				imlib_render_image_part_on_drawable_at_size(0, 0, selmon->mw, selmon->mh, 0, 0, selmon->mw / scalepreview, selmon->mh / scalepreview);
				imlib_free_image();
			}
		}
	}
}
void
showtagpreview(int tag)
{
	if (!selmon->previewshow) {
		XUnmapWindow(dpy, selmon->tagwin);
		return;
	}
        if (selmon->tagmap[tag]) {
		XSetWindowBackgroundPixmap(dpy, selmon->tagwin, selmon->tagmap[tag]);
		XCopyArea(dpy, selmon->tagmap[tag], selmon->tagwin, drw->gc, 0, 0, selmon->mw / scalepreview, selmon->mh / scalepreview, 0, 0);
		XSync(dpy, False);
		XMapWindow(dpy, selmon->tagwin);
	} else
		XUnmapWindow(dpy, selmon->tagwin);
}
#endif /* TAG_PREVIEW */

/* Systray functions */
#ifdef SYSTRAY
void
resizebarwin(Monitor *m)
{
	unsigned int w = m->ww;
	if (m == systraytomon(m))
		w -= getsystraywidth();
	XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, w, bh);
}
unsigned int
getsystraywidth()
{
	unsigned int w = 0;
	Client *i;
	if (!systray)
		return 0;
	for (i = systray->icons; i; w += i->w + lrpad / 2, i = i->next);
	return w ? w - lrpad / 2 : 0;
}
void
removesystrayicon(Client *i)
{
	Client **ii;

	if (!i)
		return;
	for (ii = &systray->icons; *ii && *ii != i; ii = &(*ii)->next);
	if (ii)
		*ii = i->next;
	free(i);
}
void
resizerequest(XEvent *e)
{
	XResizeRequestEvent *ev = &e->xresizerequest;
	Client *i;

	if ((i = wintosystrayicon(ev->window))) {
		updatesystrayicongeom(i, ev->width, ev->height);
		updatesystray();
	}
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
	//XWindowChanges wc;
	Client *i;
	Monitor *m = systraytomon(NULL);
	unsigned int x = m->mx + m->mw;
	unsigned int w = 1, xpad = 0, ypad = 0;

	//if (systray)
	//	return;

	if (!systray) {
		/* init systray */
		systray = ecalloc(1, sizeof(Systray));

		wa.override_redirect = True; //ignore the window
		wa.event_mask = ButtonPressMask|ExposureMask;
		wa.border_pixel = 0;
		wa.background_pixel = 0;
		wa.colormap = cmap;
		systray->win = XCreateWindow(dpy, root, x - xpad + lrpad / 2, m->by + ypad,
				w, bh, 0, depth, InputOutput, visual,
				CWOverrideRedirect|CWBorderPixel|CWBackPixel|CWColormap|CWEventMask, &wa); // CWBackPixmap
		//systray->win = XCreateSimpleWindow(dpy, root, x, m->by, w, bh, 0, 0, scheme[SchemeNorm][ColBg].pixel);

		XSelectInput(dpy, systray->win, SubstructureNotifyMask);

		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayOrientation], XA_CARDINAL, 32,
				PropModeReplace, (unsigned char *) &systrayorientation, 1);
		XChangeProperty(dpy, systray->win, netatom[NetSystemTrayVisual], XA_VISUALID, 32,
				PropModeReplace, (unsigned char *) &visual->visualid, 1);
		XChangeProperty(dpy, systray->win, netatom[NetWMWindowType], XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &netatom[NetWMWindowTypeDock], 1);

		XMapRaised(dpy, systray->win);
		XSetSelectionOwner(dpy, netatom[NetSystemTray], systray->win, CurrentTime);
		if (XGetSelectionOwner(dpy, netatom[NetSystemTray]) == systray->win) {
			sendevent(root, xatom[Manager], StructureNotifyMask, CurrentTime, netatom[NetSystemTray], systray->win, 0, 0);
			XSync(dpy, False);
		} else {
			fprintf(stderr, "dwm: unable to obtain system tray.\n");
			free(systray);
			systray = NULL;
			return;
		}
	}


	//wc.stack_mode = Above;
	//wc.sibling = m->barwin;
	//XConfigureWindow(dpy, systray->win, CWSibling|CWStackMode, &wc);

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
	XMoveResizeWindow(dpy, systray->win, x - xpad, m->by + ypad, MAX(w, 1), bh);

	//wc.stack_mode = Above;
	//wc.sibling = m->barwin;
	//wc.x = x - xpad;
	//wc.y = m->by + ypad;
	//wc.width = w;
	//wc.height = bh;
	//XConfigureWindow(dpy, systray->win, CWX|CWY|CWWidth|CWHeight|CWSibling|CWStackMode, &wc);
	//XMapWindow(dpy, systray->win);
	//XMapSubwindows(dpy, systray->win);
	//drawbar(m);
	// this doesn't seem to work
	XSetForeground(dpy, drw->gc, scheme[SchemeNorm][ColBg].pixel);
	XFillRectangle(dpy, systray->win, drw->gc, 0, 0, w, bh);
	XSync(dpy, False);
	//XSetForeground(drw->dpy, drw->gc, scheme[bar->borderscheme][ColBorder].pixel);
	//XFillRectangle(drw->dpy, drw->drawable, drw->gc, 0, 0, bar->bw, bar->bh);
	resizebarwin(m);
}
void
updatesystrayicongeom(Client *i, int w, int h)
{
	int icon_height = bh - 2;

	if (i) {
		i->h = icon_height;
		if (w == h)
			i->w = icon_height;
		else if (h == icon_height)
			i->w = w;
		else
			i->w = (int) ((float)(icon_height) * ((float)w / (float)h));
		applysizehints(i, &(i->x), &(i->y), &(i->w), &(i->h), False);
		/* force icons into the systray dimensions if they don't want to */
		if (i->h > icon_height) {
			if (i->w == i->h)
				i->w = icon_height;
			else
				i->w = (int) ((float)(icon_height) * ((float)i->w / (float)i->h));
			i->h = icon_height;
		}
		if (i->w > 2 * icon_height)
			i->w = icon_height;
	}
}
void
updatesystrayiconstate(Client *i, XPropertyEvent *ev)
{
	long flags;
	int code = 0;

	if (!systray || !i || ev->atom != xatom[XembedInfo] ||
		!(flags = getatomprop(i, xatom[XembedInfo])))
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
	sendevent(i->win, xatom[Xembed], StructureNotifyMask, CurrentTime, code, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
}
Client *
wintosystrayicon(Window w) {
	Client *i = NULL;

	if (!systray)
		return NULL;
	if (!w)
		return i;
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
Picture
geticonprop(Window win, unsigned int *picw, unsigned int *pich)
{
	int format;
	unsigned long n, extra, *p = NULL;
	unsigned int iconsize;
	Atom real;

	if (XGetWindowProperty(dpy, win, netatom[NetWMIcon], 0L, LONG_MAX, False, AnyPropertyType,
						   &real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return None;

	if (n == 0 || format != 32) { XFree(p); return None; }

	iconsize = drw->fonts->h;

	unsigned long *bstp = NULL;
	uint32_t w, h, sz;
	{
		unsigned long *i;
		const unsigned long *end = p + n;
		uint32_t bstd = UINT32_MAX, d, m;
		for (i = p; i < end - 1; i += sz) {
			if ((w = *i++) > UINT16_MAX || (h = *i++) > UINT16_MAX) { XFree(p); return None; }
			if ((sz = w * h) > end - i) break;
			if ((m = w > h ? w : h) >= iconsize && (d = m - iconsize) < bstd) { bstd = d; bstp = i; }
		}
		if (!bstp) {
			for (i = p; i < end - 1; i += sz) {
				if ((w = *i++) > UINT16_MAX || (h = *i++) > UINT16_MAX) { XFree(p); return None; }
				if ((sz = w * h) > end - i) break;
				if ((d = iconsize - (w > h ? w : h)) < bstd) { bstd = d; bstp = i; }
			}
		}
		if (!bstp) { XFree(p); return None; }
	}

	if ((w = *(bstp - 2)) == 0 || (h = *(bstp - 1)) == 0) { XFree(p); return None; }

	uint32_t icw, ich;
	if (w <= h) {
		ich = iconsize;
		icw = w * drw->fonts->h / h;
		if (icw == 0) icw = 1;
	} else {
		icw = iconsize;
		ich = h * drw->fonts->h / w;
		if (ich == 0) ich = 1;
	}
	*picw = icw; *pich = ich;

	uint32_t i, *bstp32 = (uint32_t *)bstp;
	for (sz = w * h, i = 0; i < sz; ++i)
		bstp32[i] = prealpha(bstp[i]);

	Picture ret = drw_picture_create_resized(drw, (char *)bstp, w, h, icw, ich);
	XFree(p);

	return ret;
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
	c->icon = geticonprop(c->win, &c->icw, &c->ich);
}
#endif /* ICONS */

static void
remove_all(char *str, char to_remove)
{
	char *read = str, *write = str;
	do {
		while (*read == to_remove) read++;
		*write++ = *read;
		read++;
	} while (*(read-1));
}

void
getcmd(int i, char *button)
{
	if (!selmon->showbar || !showstatus)
		return;

	if (execlock & 1 << i) { /* block is already running */
		fprintf(stderr, "dwm: ignoring block %d, command %s\n", i, blocks[i].command);
		return;
	}

	/* lock execution of block until current instance finishes execution */
	execlock |= 1 << i;

	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		dup2(pipes[i][1], STDOUT_FILENO);
		close(pipes[i][0]);
		close(pipes[i][1]);

		if (button)
			setenv("BLOCK_BUTTON", button, 1);
		execlp("/bin/sh", "sh", "-c", blocks[i].command, (char *) NULL);
		fprintf(stderr, "dwm: block %d, execlp %s", i, blocks[i].command);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
getcmds(int time)
{
	int i;
	for (i = 0; i < LENGTH(blocks); i++) {
		if ((blocks[i].interval != 0 && time % blocks[i].interval == 0) || time == -1)
			getcmd(i, NULL);
	}
}

void
getsigcmds(unsigned int signal)
{
	int i;
	for (i = 0; i < LENGTH(blocks); i++) {
		if (blocks[i].signal == signal)
			getcmd(i, NULL);
	}
}

int
getstatus(int width)
{
	int i, len, all = width, delimlen = TEXTW(delimiter) - lrpad;
	char fgcol[8];
				/* fg		bg */
	const char *cols[8] = 	{ fgcol, colors[SchemeStatus][ColBg] };

	if (!showstatus)
		return stsw = 0;

	#if INVERSED
	for (i = 0; i < LENGTH(blocks); i++)
	#else
	for (i = LENGTH(blocks) - 1; i >= 0; i--)
	#endif /* INVERSED */
	{
		if (*blockoutput[i] == '\0') /* ignore command that output NULL or '\0' */
			continue;
		strcpy(fgcol, blocks[i].color);
		/* re-load the scheme with the new colors */
		scheme[SchemeStatus] = drw_scm_create(drw, cols, alphas[SchemeStatus], 2);
		drw_setscheme(drw, scheme[SchemeStatus]); /* 're-set' the scheme */
		len = TEXTW(blockoutput[i]) - lrpad;
		all -= len;
		drw_text(drw, all, 0, len, bh, 0, blockoutput[i], 0);
		/* draw delimiter */
		if (delimiter == '\0') /* ignore no delimiter */
			continue;
		drw_setscheme(drw, scheme[SchemeDelim]);
		all -= delimlen;
		drw_text(drw, all, 0, delimlen, bh, 0, delimiter, 0);
	}

	return stsw = width - all;
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
	else {
		if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
			strncpy(text, *list, size - 1);
			XFreeStringList(list);
		}
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
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
incnmaster(const Arg *arg)
{
	if (pertag)
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
	else
		selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
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
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
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
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
#ifdef SYSTRAY
	if (!sendevent(selmon->sel->win, wmatom[WMDelete], NoEventMask, wmatom[WMDelete], CurrentTime, 0, 0, 0)) {
#else
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
#endif /* SYSTRAY */
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
losefullscreen(Client *next)
{
	Client *sel = selmon->sel;
	if (!sel || !next)
		return;
	if (sel->isfullscreen && sel->fakefullscreen != 1 && ISVISIBLE(sel) && sel->mon == next->mon && !next->isfloating)
		setfullscreen(sel, 0);
}

/* Wrapper function to load colors, since it appears at least 3 times. The
 * logic is: if 'fallback' value is 0, load the colors normally (pywal); but if
 * it does has a value, then use fallback colors */
void
fallbackcolors(void)
{
	int i;

	strcpy(bg_wal, "#444444");
	strcpy(fg_wal, "#bbbbbb");
	strcpy(color0, "#222222");
	strcpy(color1, "#eeeeee");
	strcpy(color2, "#005577");
	strcpy(color3, "#222222");
	strcpy(color4, "#eeeeee");
	strcpy(color5, "#222222");
	strcpy(color6, "#222222");
	strcpy(color7, "#222222");
	strcpy(color8, "#222222");
	strcpy(cursor_wal, "#222222");
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 2);
	fprintf(stderr, "dwm: could not get Xresources colors, switching to fallback colors.\n");
}

void
xrdbloadcolor(XrmDatabase xrdb, const char *name, char *var)
{
	XrmValue value;
	char *type;
	int i;

	if (XrmGetResource(xrdb, name, NULL, &type, &value) == True) { /* exist */
		if (strnlen(value.addr, 8) == 7 && value.addr[0] == '#') { /* is a hex color */
			for (i = 1; i < 7; i++) {
				if ((value.addr[i] < 48)
				|| (value.addr[i] > 57 && value.addr[i] < 65)
				|| (value.addr[i] > 70 && value.addr[i] < 97)
				|| (value.addr[i] > 102))
					break;
			}
			if (i == 7) {
				strncpy(var, value.addr, 7);
				var[7] = '\0';
			}
		}
        }
}


void
readxresources(void)
{
	Display *display;
	char *resm;
	XrmDatabase xrdb;

	display = XOpenDisplay(NULL);
	resm = XResourceManagerString(display);

	if (!resm)
		return;

	xrdb = XrmGetStringDatabase(resm);
	if (xrdb != NULL) {
		xrdbloadcolor(xrdb, "background", bg_wal);
		xrdbloadcolor(xrdb, "foreground", fg_wal);
		xrdbloadcolor(xrdb, "cursor", cursor_wal);
		xrdbloadcolor(xrdb, "color0", color0);
		xrdbloadcolor(xrdb, "color1", color1);
		xrdbloadcolor(xrdb, "color2", color2);
		xrdbloadcolor(xrdb, "color3", color3);
		xrdbloadcolor(xrdb, "color4", color4);
		xrdbloadcolor(xrdb, "color5", color5);
		xrdbloadcolor(xrdb, "color6", color6);
		xrdbloadcolor(xrdb, "color7", color7);
		xrdbloadcolor(xrdb, "color8", color8);
 		XrmDestroyDatabase(xrdb);	/* close the database */
	} else
		fallbackcolors();

	XCloseDisplay(display);
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL, *term = NULL;
	Window trans = None;
	XWindowChanges wc;
	int format;
	unsigned int *ptags;
	unsigned long n, extra;
	Atom atom;
	//XEvent xev;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	c->pid = winpid(w);

	/* do not manage (lower and skip) NET_WINDOW_TYPE_DESKTOP
	 * (desktop implementation) windows (glava, xlivebg, etc) */
	if (getatomprop(c, netatom[NetWMWindowType]) == netatom[NetWMWindowTypeDesktop]) {
		XMapWindow(dpy, c->win);
		XLowerWindow(dpy, c->win);
		free(c);
		return;
	}

	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;
	c->cfact = 1.0;

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
		if (XGetWindowProperty(dpy, c->win, dwmtags, 0L, 1L, False, XA_CARDINAL,
		&atom, &format, &n, &extra, (unsigned char **)&ptags) == Success && n == 1 && *ptags != 0) {
			c->tags = *ptags;
			XFree(ptags);
		}
	}
	settagsatom(c->win, c->tags);

	if (c->x + WIDTH(c) > c->mon->mx + c->mon->mw)
		c->x = c->mon->mx + c->mon->mw - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->my + c->mon->mh)
		c->y = c->mon->my + c->mon->mh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->mx);
	/* only fix client y-offset, if the client center might cover the bar */
	c->y = MAX(c->y, ((c->mon->by == c->mon->my) && (c->x + (c->w / 2) >= c->mon->wx)
		&& (c->x + (c->w / 2) < c->mon->wx + c->mon->ww)) ? bh : c->mon->my);
//	c->bw = borderpx;
	c->bw = c->isfloating ? fborderpx : c->mon->borderpx;
	/* center the window (floating) */
	c->x = c->mon->mx + (c->mon->mw - WIDTH(c)) / 2;
	c->y = c->mon->my + (c->mon->mh - HEIGHT(c)) / 2;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[BorderNorm][ColFg].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	if (getatomprop(c, netatom[NetWMState]) == netatom[NetWMStateAbove])
		c->alwaysontop = 1;
	if (getatomprop(c, netatom[NetWMState]) == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	updatesizehints(c);
	updatewmhints(c);

	/* store float size and position */
	c->sfx = c->x;
	c->sfy = c->y;
	c->sfw = c->w;
	c->sfh = c->h;

	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = t || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon) {
		losefullscreen(c);
		unfocus(selmon->sel, 0);
	}
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	if (term)
		swallow(term, c);
	focus(NULL);
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
		sendevent(i->win, netatom[Xembed], StructureNotifyMask,
			CurrentTime, 1 /* XEMBED_WINDOW_ACTIVATE */, 0,
			systray->win, XEMBED_EMBEDDED_VERSION);
		updatesystray();
	}
#endif /* SYSTRAY */
	if (!XGetWindowAttributes(dpy, ev->window, &wa))
		return;
/* github.com/bakkeby/patches/commit/67c8bcefafbed8d0f122bb91b6d253919727b60e */
	if (!wa.depth)
		return;
	if (wa.override_redirect)
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
	unsigned int i, x;
	if (ev->window == selmon->barwin) {
		i = x = 0;
		do
	     		x += TEXTW(tags[i]);
		while (ev->x >= x && ++i < LENGTH(tags));

	     	if (i < LENGTH(tags)) {
			if ((i + 1) != selmon->previewshow
			&& !(selmon->tagset[selmon->seltags] & 1 << i)) {
	     			selmon->previewshow = i + 1;
	     			showtagpreview(i);
			} else if (selmon->tagset[selmon->seltags] & 1 << i) {
				selmon->previewshow = 0;
				showtagpreview(0);
		  	}
		} else if (selmon->previewshow != 0) {
	     		selmon->previewshow = 0;
	     		showtagpreview(0);
	     	}
	} else if (selmon->previewshow != 0) {
		selmon->previewshow = 0;
	     	showtagpreview(0);
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

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

Client *
prevtiled(Client *c)
{
	Client *p, *r;

	for (p = selmon->clients, r = NULL; p && p != c; p = p->next)
		if (!p->isfloating && ISVISIBLE(p))
			r = p;
	return r;
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

#ifdef SYSTRAY
	if ((c = wintosystrayicon(ev->window))) {
		if (ev->atom == XA_WM_NORMAL_HINTS) {
			updatesizehints(c);
			updatesystrayicongeom(c, c->w, c->h);
		} else
			updatesystrayiconstate(c, ev);
		updatesystray();
	}
#endif /* SYSTRAY */

	if ((ev->window == root)) { /* root events */
		if (ev->atom == dwmstatus) /* update bar */
			updatestatus();
		if (ev->atom == XA_WM_NAME) { /* 'fake' signal */
			char n[32], *buf;
			int arg;
			if (gettextprop(root, XA_WM_NAME, n, sizeof(n))) {
				/* divide into 2 args separated by the first space */
				for (int i = 0; i <= strnlen(n, 32); i++) {
					if (n[i] == ' ') {
						buf = n;
						buf += i + 1; /* chop the first 'word' */
						arg = atoi(buf); /* store as an int */

						n[i] = '\0'; /* chop every after the space */
						break;
					}
				}

				/* parsing commands */
				if (!strcmp(n, "cyclelayout"))
					cyclelayout(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrgaps"))
					incrgaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrogaps"))
					incrogaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrohgaps"))
					incrohgaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrovgaps"))
					incrovgaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrigaps"))
					incrigaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrihgaps"))
					incrihgaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "incrivgaps"))
					incrivgaps(&((Arg) { .i = arg }));
				else if (!strcmp(n, "defaultgaps"))
					defaultgaps(NULL);
				else if (!strcmp(n, "killclient"))
					killclient(NULL);
				else if (!strcmp(n, "random_wall"))
					random_wall(NULL);
				else if (!strcmp(n, "refresh"))
					refresh(NULL);
				else if (!strcmp(n, "setlayout"))
					setlayout(&((Arg) { .v = &layouts[arg] }));
				else if (!strcmp(n, "tag"))
					tag(&((Arg) { .ui = 1 << arg }));
				else if (!strcmp(n, "togglebar"))
					togglebar(NULL);
				else if (!strcmp(n, "togglefloating"))
					togglefloating(NULL);
				else if (!strcmp(n, "togglefullscreen"))
					togglefullscreen(NULL);
				else if (!strcmp(n, "togglefakefullscreen"))
					togglefakefullscreen(NULL);
				else if (!strcmp(n, "togglegaps"))
					togglegaps(NULL);
				else if (!strcmp(n, "togglesmartgaps"))
					togglesmartgaps(NULL);
				else if (!strcmp(n, "togglevacant"))
					togglevacant(NULL);
				else if (!strcmp(n, "togglestatus"))
					togglestatus(NULL);
				else if (!strcmp(n, "toggletag"))
					toggletag(&((Arg) { .ui = 1 << arg }));
				else if (!strcmp(n, "view"))
					view(&((Arg) { .ui = 1 << arg }));
				else if (!strcmp(n, "xrdb"))
					xrdb(NULL);
				else if (!strcmp(n, "zoom"))
					zoom(NULL);
				else if (!strcmp(n, "zoomswap"))
					zoomswap(NULL);
				else if (atoi(n) > 0) { /* more than 0 it's a signal */
					getsigcmds(atoi(n));
					updatestatus();
				}
			}
		}
	}
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			updatesizehints(c);
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

void
quit(const Arg *arg)
{
	running = 0;
}

void
refresh(const Arg *arg)
{
	restart = 1;
	running = 0;
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
	|| (c->mon->lt[c->mon->sellt]->arrange == &monocle
	||  c->mon->lt[c->mon->sellt]->arrange == &alphamonocle))
	&& (c->fakefullscreen == 1 || !c->isfullscreen)
	&& !c->isfloating
	&& c->mon->lt[c->mon->sellt]->arrange != NULL) {
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
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	int ocx2, ocy2, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	int horizcorner, vertcorner;
	int di;
	unsigned int dui;
	Window dummy;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	ocx2 = c->x + c->w;
	ocy2 = c->y + c->h;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!XQueryPointer (dpy, c->win, &dummy, &dummy, &di, &di, &nx, &ny, &dui))
	       return;
	horizcorner = nx < c->w / 2;
	vertcorner = ny < c->h / 2;
	XWarpPointer (dpy, None, c->win, 0, 0, 0, 0,
		      horizcorner ? (-c->bw) : (c->w + c->bw - 1),
		      vertcorner ? (-c->bw) : (c->h + c->bw - 1));

	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			nx = horizcorner ? ev.xmotion.x : c->x;
			ny = vertcorner ? ev.xmotion.y : c->y;
			nw = MAX(horizcorner ? (ocx2 - nx) : (ev.xmotion.x - ocx - 2 * c->bw + 1), 1);
			nh = MAX(vertcorner ? (ocy2 - ny) : (ev.xmotion.y - ocy - 2 * c->bw + 1), 1);

			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
		      horizcorner ? (-c->bw) : (c->w + c->bw - 1),
		      vertcorner ? (-c->bw) : (c->h + c->bw - 1));
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
resizemousescroll(const Arg *arg)
{
	int nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	int dw = *((int*)arg->v + 1);
	int dh = *(int*)arg->v;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support resizing fullscreen windows by mouse */
		return;
	if (!c->isfloating && selmon->lt[selmon->sellt]->arrange)
		togglefloating(NULL);
	restack(selmon);
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	nw = MAX(c->w + dw, 1);
	nh = MAX(c->h + dh, 1);
	//if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
	//&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
	//{
	//	if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
	//	&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
	//		togglefloating(NULL);
	//}
	if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
		resize(c, c->x, c->y, nw, nh, 1);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
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
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
			//if (c != m->sel && !c->isfloating && ISVISIBLE(c)) {
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
	XEvent ev;
	int i;
	struct pollfd fds[LENGTH(blocks) + 1] = {0};

	fds[0].fd = ConnectionNumber(dpy);
	fds[0].events = POLLIN;

	#if INVERSED
	for (i = LENGTH(blocks) - 1; i >= 0; i--)
	#else
	for (i = 0; i < LENGTH(blocks); i++)
	#endif /* INVERSED */
	{
		pipe(pipes[i]);
		fds[i + 1].fd = pipes[i][0];
		fds[i + 1].events = POLLIN;
		getcmd(i, NULL);
		if (blocks[i].interval) {
			maxinterval = MAX(blocks[i].interval, maxinterval);
			sleepinterval = gcd(blocks[i].interval, sleepinterval);
		}
	}

	alarm(sleepinterval);
	/* main event loop */
	XSync(dpy, False);
	while (running) {

		if ((poll(fds, LENGTH(blocks) + 1, -1)) == -1) {
			if (errno == EINTR) { /* signal caught */
				if (isalarm) /* SIGALRM */
					isalarm = 0; /* restore isalarm */
				else if (isrtsig) /* blocks signal SIGRTMIN + i */
					isrtsig = 0;
				else
					fprintf(stderr, "dwm: poll INTERRUPTED by a signal (EINTR)\n");
				continue;
			}
			fprintf(stderr, "dwm: poll ");
			perror("failed");
			exit(EXIT_FAILURE);
		}

		/* handle display fd */
		if (fds[0].revents & POLLIN) {
			while (running && XPending(dpy)) {
				XNextEvent(dpy, &ev);
				if (handler[ev.type])
					handler[ev.type](&ev); /* call handler */
			}
		} else if (fds[0].revents & POLLHUP) {
			fprintf(stderr, "dwm: main event loop, hang up\n");
			perror(" failed");
			exit(1);
		}

		/* handle blocks */
		for (i = 0; i < LENGTH(blocks); i++) {
			if (fds[i + 1].revents & POLLIN) {
				/* empty buffer with CMDLENGTH + 1 byte for the null terminator */
				int bt = read(fds[i + 1].fd, blockoutput[i], CMDLENGTH);
				/* remove lock for the current block */
				execlock &= ~(1 << i);

				if (bt == -1) { /* if read failed */
					fprintf(stderr, "dwm: read failed in block %s\n", blocks[i].command);
					perror(" failed");
					continue;
				}

				if (blockoutput[i][bt - 1] == '\n') /* chop off ending new line, if one is present */
					blockoutput[i][bt - 1] = '\0';
				else /* NULL terminate the string */
					blockoutput[i][bt++] = '\0';

				drawbar(selmon);
			} else if (fds[i + 1].revents & POLLHUP) {
				fprintf(stderr, "dwm: blocks hangup\n");
				perror(" failed");
				exit(1);
			}
		}
	}

	/* close the pipes after running */
	for (i = 0; i < LENGTH(blocks); i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
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
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
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
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
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
togglesmartgaps(const Arg *arg)
{
	smartgaps = !smartgaps;
	arrange(NULL);
}
void
togglegaps(const Arg *arg)
{
	if (pertag)
		selmon->pertag->enablegaps[selmon->pertag->curtag] =
			!selmon->pertag->enablegaps[selmon->pertag->curtag];
	else
		enablegaps = !enablegaps;
	arrange(NULL);
}
void
defaultgaps(const Arg *arg)
{
	setgaps(gappoh, gappov, gappih, gappiv);
}
void
incrgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh + arg->i,
		selmon->gappov + arg->i,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i
	);
}
void
getgaps(Monitor *m, int *oh, int *ov, int *ih, int *iv, unsigned int *nc)
{
	unsigned int n, oe, ie;
	Client *c;
	if (pertag)
		oe = ie = selmon->pertag->enablegaps[selmon->pertag->curtag];
	else
		oe = ie = enablegaps;
	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (smartgaps && n == 1) {
		oe = 0;     /* outer gaps disabled when only one client */
	}

	/* disable gaps if they are disabled */
	if (selmon->lt[selmon->sellt]->gaps)
		oe = 0;

	*oh = m->gappoh*oe; /* outer horizontal gap */
	*ov = m->gappov*oe; /* outer vertical gap   */
	*ih = m->gappih*ie; /* inner horizontal gap */
	*iv = m->gappiv*ie; /* inner vertical gap   */
	*nc = n;            /* number of clients    */
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
	*mr = msize - mtotal;	/* the remainder (rest) of pixels after a cfacts master split */
	*sr = ssize - stotal;	/* the remainder (rest) of pixels after a cfacts stack split */
}
void
incrigaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh,
		selmon->gappov,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i
	);
}
void
incrogaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh + arg->i,
		selmon->gappov + arg->i,
		selmon->gappih,
		selmon->gappiv
	);
}
void
incrohgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh + arg->i,
		selmon->gappov,
		selmon->gappih,
		selmon->gappiv
	);
}
void
incrovgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh,
		selmon->gappov + arg->i,
		selmon->gappih,
		selmon->gappiv
	);
}
void
incrihgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh,
		selmon->gappov,
		selmon->gappih + arg->i,
		selmon->gappiv
	);
}
void
incrivgaps(const Arg *arg)
{
	setgaps(
		selmon->gappoh,
		selmon->gappov,
		selmon->gappih,
		selmon->gappiv + arg->i
	);
}
/* vanitygaps */

int
#ifdef SYSTRAY
sendevent(Window w, Atom proto, int mask, long d0, long d1, long d2, long d3, long d4)
#else
sendevent(Client *c, Atom proto)
#endif /* SYSTRAY */
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;
#ifdef SYSTRAY
	Atom mt;
	if (proto == wmatom[WMTakeFocus] || proto == wmatom[WMDelete]) {
		mt = wmatom[WMProtocols];
		if (XGetWMProtocols(dpy, w, &protocols, &n)) {
			while (!exists && n--)
				exists = protocols[n] == proto;
			XFree(protocols);
		}
	} else {
		exists = True;
		mt = proto;
	}
#else
	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
#endif /* SYSTRAY */
	if (exists) {
		ev.type = ClientMessage;
#ifdef SYSTRAY
		ev.xclient.window = w;
		ev.xclient.message_type = mt;
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = d0;
		ev.xclient.data.l[1] = d1;
		ev.xclient.data.l[2] = d2;
		ev.xclient.data.l[3] = d3;
		ev.xclient.data.l[4] = d4;
		XSendEvent(dpy, w, False, mask, &ev);
#else
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
#endif /* SYSTRAY */
	}
	return exists;
}

void
sendstatusbar(const Arg *arg)
{
	char button[2] = { '0' + arg->i & 0xff, '\0' };
	getcmd(blocknum, button);
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
#ifdef SYSTRAY
	sendevent(c->win, wmatom[WMTakeFocus], NoEventMask, wmatom[WMTakeFocus], CurrentTime, 0, 0, 0);
#else
	sendevent(c, wmatom[WMTakeFocus]);
#endif /* SYSTRAY */
}

void
setfullscreen(Client *c, int fullscreen)
{
	int savestate = 0, restorestate = 0, restorefakefullscreen = 0;

	if ((c->fakefullscreen == 0 && fullscreen && !c->isfullscreen) /* normal fullscreen */
			|| (c->fakefullscreen == 2 && fullscreen)) /* fake fullscreen --> actual fullscreen */
		savestate = 1; /* go actual fullscreen */
	else if ((c->fakefullscreen == 0 && !fullscreen && c->isfullscreen) /* normal fullscreen exit */
			|| (c->fakefullscreen >= 2 && !fullscreen)) /* fullscreen exit --> fake fullscreen */
		restorestate = 1; /* go back into tiled */

	/* If leaving fullscreen and the window was previously fake fullscreen
	 * (2), then restore that while staying in fullscreen. The exception to
	 * this is if we are in said state, but the client itself disables
	 * fullscreen (3) then we let the client go out of fullscreen while
	 * keeping fake fullscreen enabled (as otherwise there will be a
	 * mismatch between the client and the window manager's perception of
	 * the client's fullscreen state). */
	if (c->fakefullscreen == 2 && !fullscreen && c->isfullscreen) {
		restorefakefullscreen = 1;
		c->isfullscreen = 1;
		fullscreen = 1;
	}

	if (fullscreen != c->isfullscreen) { /* only send property change if necessary */
		if (fullscreen)
			XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
				PropModeReplace, (unsigned char *) &netatom[NetWMFullscreen], 1);
		else
			XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
				PropModeReplace, (unsigned char *) 0, 0);
	}

	c->isfullscreen = fullscreen;

	/* Some clients, e.g. firefox, will send a client message informing the
	 * window manager that it is going into fullscreen after receiving the
	 * above signal. This has the side effect of this function
	 * (setfullscreen) sometimes being called twice when toggling
	 * fullscreen on and off via the window manager as opposed to the
	 * application itself.  To protect against obscure issues where the
	 * client settings are stored or restored when they are not supposed to
	 * we add an additional bit-lock on the old state so that settings can
	 * only be stored and restored in that precise order. */
	if (savestate && !(c->oldstate & (1 << 1))) {
		c->oldbw = c->bw;
		c->oldstate = c->isfloating | (1 << 1);
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (restorestate && (c->oldstate & (1 << 1))) {
		c->bw = c->oldbw;
		c->isfloating = c->oldstate = c->oldstate & 1;
		if (restorefakefullscreen || c->fakefullscreen == 3)
			c->fakefullscreen = 1;
		/* The client may have been moved to another monitor whilst in
		 * fullscreen which if tiled we address by doing a full arrange
		 * of tiled clients. If the client is floating then the height
		 * and width may be larger than the monitor's window area, so
		 * we cap that by ensuring max / min values. */
		if (c->isfloating) {
			c->x = MAX(c->mon->wx, c->oldx);
			c->y = MAX(c->mon->wy, c->oldy);
			c->w = MIN(c->mon->ww - c->x - 2*c->bw, c->oldw);
			c->h = MIN(c->mon->wh - c->y - 2*c->bw, c->oldh);
			resizeclient(c, c->x, c->y, c->w, c->h);
			restack(c->mon);
		} else
			arrange(c->mon);
	} else
		resizeclient(c, c->x, c->y, c->w, c->h);
}

void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
		if (pertag) {
			selmon->pertag->sellts[selmon->pertag->curtag] ^= 1;
			selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
		} else
			selmon->sellt ^= 1;
	}
	if (pertag) {
		if (arg && arg->v) {
			selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt] = (Layout *)arg->v;
			selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
		}
	} else {
		if (arg && arg->v)
			selmon->lt[selmon->sellt] = (Layout *)arg->v;
	}

	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

void
setcfact(const Arg *arg)
{
	float f;
	Client *c;

	c = selmon->sel;

	if (!arg || !c || !selmon->lt[selmon->sellt]->arrange)
		return;
	if (!arg->f)
		f = 1.0;
	else if (arg->f > 4.0) // set fact absolutely
		f = arg->f - 4.0;
	else
		f = arg->f + c->cfact;
	if (f < 0.25)
		f = 0.25;
	else if (f > 4.0)
		f = 4.0;
	c->cfact = f;
	arrange(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	if (pertag)
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;
	else
		selmon->mfact = f;
	arrange(selmon);
}

void
sigalrm(int unused)
{
	isalarm = 1;
	getcmds(count);
	alarm(sleepinterval);
	count = (count + sleepinterval - 1) % maxinterval + 1;
}

void
setup(void)
{
	XSetWindowAttributes wa;
	Atom utf8string;
	int i;

	/* clean up any zombies immediately */
	sigchld(0);

	/* init signals handlers */
	signal(SIGHUP, sighup);   /* restart */
	signal(SIGTERM, sigterm); /* exit */

	struct sigaction al;
	al.sa_handler = sigalrm;
	al.sa_flags = SA_RESTART;
	sigemptyset(&al.sa_mask);
	sigaction(SIGALRM, &al, NULL);

	/* handle defined real time signals */
	struct sigaction sa;
	sa.sa_handler = sighandler;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	for (i = 0; i < LENGTH(blocks); i++)
		if (blocks[i].signal)
			sigaction(SIGRTMIN + blocks[i].signal, &sa, NULL);

	/* pid as an enviromental variable */
	char envpid[16];
	snprintf(envpid, LENGTH(envpid), "%d", getpid());
	setenv("STATUSBAR", envpid, 1);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	xinitvisual();
	drw = drw_create(dpy, screen, root, sw, sh, visual, depth, cmap);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts))) {
		fprintf(stderr, "dwm: no fonts could be loaded, status bar hidden.\n");
		showbar = 0;
	}
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + barh; /* prevent barh being < than font size */
	updategeom();

	/* init atoms */
	utf8string                     = XInternAtom(dpy, "UTF8_STRING", False);
	dwmstatus                      = XInternAtom(dpy, "UPDATE_DWM_STATUSBAR", False);
	dwmtags                        = XInternAtom(dpy, "_DWM_TAGS", False);
	wmatom[WMProtocols]            = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete]               = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState]                = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus]            = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	wmatom[WMWindowRole]           = XInternAtom(dpy, "WM_WINDOW_ROLE", False);
	netatom[NetWMStateAbove]       = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
	netatom[NetActiveWindow]       = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported]          = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName]             = XInternAtom(dpy, "_NET_WM_NAME", False);
	#ifdef ICONS
	netatom[NetWMIcon]             = XInternAtom(dpy, "_NET_WM_ICON", False);
	#endif /* ICONS */
	netatom[NetWMState]            = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck]            = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen]       = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowTypeDesktop] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
	netatom[NetWMWindowType]       = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	//netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList]         = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
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
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	if (restart) {	/* keep the colors the same */
		xrdb(NULL);
		restart = 0;
	} else	/* refresh colors */
		random_wall(NULL);

	/* init system tray */
	#ifdef SYSTRAY
	updatesystray();
	#endif /* SYSTRAY */

	/* init bars */
	updatebars();
	updatestatus();
	#ifdef TAG_PREVIEW
	updatepreview(); //Is this for the current tag?
	#endif /* TAG_PREVIEW */
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
}


void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
settagsatom(Window w, unsigned int tags)
{
	XChangeProperty(dpy, w, dwmtags, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char*)&tags, 1);
}

void
shifttag(const Arg *arg)
{
	Arg shifted;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;
	if (!selmon->clients)
		return;
	if (arg->i > 0)	/* left circular shift */
		shifted.ui = ((shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
	else	/* right circular shift */
		shifted.ui = (shifted.ui >> (- arg->i) | shifted.ui << (LENGTH(tags) + arg->i)) & ~SPTAGMASK;
	tag(&shifted);
}

void
shifttagclients(const Arg *arg)
{

	Arg shifted;
	Client *c;
	unsigned int tagmask = 0;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;

	for (c = selmon->clients; c; c = c->next)
		if (!(c->tags & SPTAGMASK))
			tagmask = tagmask | c->tags;

	if (arg->i > 0)	/* left circular shift */
		do
			shifted.ui = ((shifted.ui << arg->i)
			   | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
		while (tagmask && !(shifted.ui & tagmask));
	else		/* right circular shift */
		do
			shifted.ui = (shifted.ui >> (- arg->i)
			   | shifted.ui << (LENGTH(tags) + arg->i)) & ~SPTAGMASK;
		while (tagmask && !(shifted.ui & tagmask));
	tag(&shifted);
}


void
shift(void (*func)(const Arg *arg))
{
//	Arg shifted;
//	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;
//
//	if ((*func)(arg->i) > 0)	/* left circular shift */
//		shifted.ui = ((shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
//	else	/* right circular shift */
//		shifted.ui = (shifted.ui >> (- arg->i) | shifted.ui << (LENGTH(tags) + arg->i)) & ~SPTAGMASK;
//	func(&shifted);
}

void
shiftview(const Arg *arg)
{
	Arg shifted;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;

	if (arg->i > 0)	/* left circular shift */
		shifted.ui = ((shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
	else	/* right circular shift */
		shifted.ui = (shifted.ui >> (- arg->i) | shifted.ui << (LENGTH(tags) + arg->i)) & ~SPTAGMASK;
	view(&shifted);
}

void
shiftviewclients(const Arg *arg)
{
	Arg shifted;
	Client *c;
	unsigned int tagmask = 0, filter = 0;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;

	for (c = selmon->clients; c; c = c->next)
		if (!(c->tags & SPTAGMASK))
			filter = filter | c->tags;

	/* FIXME at least two tags are going to be with a client in order to
	 * shiftviewclient, else shiftview normally */
	//if (filter > LENGTH(tags))
	tagmask = filter;

	if (arg->i > 0)	/* left circular shift */
		do {
			shifted.ui = (shifted.ui << arg->i)
			   | (shifted.ui >> (LENGTH(tags) - arg->i));
			shifted.ui &= ~SPTAGMASK;
		} while (tagmask && !(shifted.ui & tagmask));
	else		/* right circular shift */
		do {
			shifted.ui = (shifted.ui >> (- arg->i)
			   | shifted.ui << (LENGTH(tags) + arg->i));
			shifted.ui &= ~SPTAGMASK;
		} while (tagmask && !(shifted.ui & tagmask));
	view(&shifted);
}

void
shiftboth(const Arg *arg)
{
	Arg shifted;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;

	if (arg->i > 0)	/* left circular shift */
		shifted.ui = ((shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
	else		/* right circular shift */
		shifted.ui = ((shifted.ui >> (- arg->i) | shifted.ui << (LENGTH(tags) + arg->i))) & ~SPTAGMASK;
	tag(&shifted);
	view(&shifted);
}


void
showhide(Client *c)
{
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
sigchld(int unused)
{
	//if (signal(SIGCHLD, sigchld) == SIG_ERR)
	//	die("can't install SIGCHLD handler:");
	//while (0 < waitpid(-1, NULL, WNOHANG));
	struct sigaction sa;
	sa.sa_handler = sigchld;
	sa.sa_flags = SA_NOCLDSTOP | SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGCHLD, &sa, 0) == -1)
		die("can't install SIGCHLD handler:");
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
sighandler(int signum)
{
	isrtsig = 1;
	getsigcmds(signum-SIGRTMIN);
}

void
sighup(int unused)
{
	restart = 1;
	running = 0;
}

void
sigterm(int unused)
{
	running = 0;
}

void
spawn(const Arg *arg)
{
	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	if (arg->v == clip) /* fix for rofi */
		dmenumon[0] = '\0';

	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		fprintf(stderr, "dwm: execvp %s", ((char **)arg->v)[0]);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

/* wrapper for exec which appends NULL to the variable */
void
spawncmd(const Arg *arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		char shcmd[1024];
		strcpy(shcmd, arg->v);
		char *command[] = { "/bin/sh", "-c", shcmd, NULL };
		execvp(command[0], command);
		fprintf(stderr, "dwm: execvp %s", *command);
		perror(" failed");
		exit(EXIT_SUCCESS);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		settagsatom(selmon->sel->win, selmon->sel->tags);
		focus(NULL);
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c || !mons->next)
		return;
	if (c->isfullscreen) {
		c->isfullscreen = 0;
		sendmon(c, dirtomon(arg->i));
		c->isfullscreen = 1;
		if (c->fakefullscreen != 1) {
			resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
			XRaiseWindow(dpy, c->win);
		}
	} else
		sendmon(c, dirtomon(arg->i));
}

void
togglebar(const Arg *arg)
{
	if (pertag && pertagbar)
		selmon->showbar = selmon->pertag->showbars[selmon->pertag->curtag] = !selmon->showbar;
	else
		selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
#ifdef SYSTRAY
	resizebarwin(selmon);
	XWindowChanges wc;
	if (!selmon->showbar)
		wc.y = -bh;
	else if (selmon->showbar) {
		wc.y = 0;
		if (!selmon->topbar)
			wc.y = selmon->mh - bh;
	}
	XConfigureWindow(dpy, systray->win, CWY, &wc);
#else
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
#endif /* SYSTRAY */
	arrange(selmon);
	getcmds(-1);
}

void
togglefakefullscreen(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c)
		return;

	if (c->fakefullscreen != 1 && c->isfullscreen) { /* exit fullscreen --> fake fullscreen */
		c->fakefullscreen = 2;
		setfullscreen(c, 0);
	} else if (c->fakefullscreen == 1) {
		setfullscreen(c, 0);
		c->fakefullscreen = 0;
	} else {
		c->fakefullscreen = 1;
		setfullscreen(c, 1);
	}
}

void
togglefloating(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c)
		return;
	if (c->isfullscreen && c->fakefullscreen != 1) /* no support for fullscreen windows */
		return;
	c->isfloating = !c->isfloating || c->isfixed;

	if (selmon->sel->isfloating) {
		selmon->sel->bw = fborderpx;
		configure(selmon->sel);
		/* apply float color, since oldbw has the old color */
		XSetWindowBorder(dpy, c->win, scheme[BorderFloat][ColFg].pixel);

		/* restore last known float dimensions */
		resize(selmon->sel, selmon->sel->sfx, selmon->sel->sfy,
		       selmon->sel->sfw, selmon->sel->sfh, 0);
	} else {
		selmon->sel->bw = borderpx;
		configure(selmon->sel);
		/* save last known float dimensions */
		selmon->sel->sfx = selmon->sel->x;
		selmon->sel->sfy = selmon->sel->y;
		selmon->sel->sfw = selmon->sel->w;
		selmon->sel->sfh = selmon->sel->h;
		XSetWindowBorder(dpy, c->win, scheme[BorderSel][ColFg].pixel);
	}

	arrange(c->mon);
}

void
togglefullscreen(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!c)
		return;

	if (c->fakefullscreen == 1) { /* fake fullscreen --> fullscreen */
		c->fakefullscreen = 2;
		setfullscreen(c, 1);
	} else
		setfullscreen(c, !c->isfullscreen);
}

void
togglescratch(const Arg *arg)
{
	Client *c;
	unsigned int found = 0, scratchtag = SPTAG(arg->ui);
	Arg sparg = {.v = scratchpads[arg->ui] };

	for (c = selmon->clients; c && !(found = c->tags & scratchtag); c = c->next);
	if (found) {
		unsigned int newtagset = selmon->tagset[selmon->seltags] ^ scratchtag;
		if (newtagset) {
			selmon->tagset[selmon->seltags] = newtagset;
			focus(NULL);
			arrange(selmon);
		}
		if (ISVISIBLE(c)) {
			focus(c);
			restack(selmon);
		}
	} else {
		selmon->tagset[selmon->seltags] |= scratchtag;
		spawn(&sparg);
	}
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		settagsatom(selmon->sel->win, selmon->sel->tags);
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);
	int i;

	if (newtagset) {
		#ifdef TAG_PREVIEW
		switchtag();
		#endif /* TAG_PREVIEW */
		selmon->tagset[selmon->seltags] = newtagset;

		if (pertag) {
			if (newtagset == ~0) {
				selmon->pertag->prevtag = selmon->pertag->curtag;
				selmon->pertag->curtag = 0;
			}
			/* test if the user did not select the same tag */
			if (!(newtagset & 1 << (selmon->pertag->curtag - 1))) {
				selmon->pertag->prevtag = selmon->pertag->curtag;
				for (i=0; !(newtagset & 1 << i); i++) ;
				selmon->pertag->curtag = i + 1;
			}

			/* apply settings for this view */
			selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
			selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
			selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
			selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
			selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];
			if (pertagbar && selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
				togglebar(NULL);
		}

		focus(NULL);
		arrange(selmon);
	}
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	//c->alwaysontop = 0;
	XSetWindowBorder(dpy, c->win, scheme[BorderNorm][ColFg].pixel);
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
		updatesystray();
#endif /* SYSTRAY */
	}
}

void
updatebars(void)
{
	unsigned int w;
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixel = 0,
		.border_pixel = 0,
		.colormap = cmap,
		.event_mask = ButtonPressMask|ExposureMask
	#ifdef TAG_PREVIEW
		PointerMotionMask
	#endif /* TAG_PREVIEW */
	};
	XClassHint ch = {"dwm", "dwm"};

	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		w = m->ww;
#ifdef SYSTRAY
		//unsigned int w = m == systraytomon(m) ? w - getsystraywidth() : m->ww;
		if (m == systraytomon(m))
			w -= getsystraywidth();
#endif /* SYSTRAY */
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by,
		   w, bh, 0, depth, InputOutput, visual,
		   CWOverrideRedirect|CWBackPixel|CWBorderPixel|CWColormap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
#ifdef SYSTRAY
		if (m == systraytomon(m))
			XMapRaised(dpy, systray->win);
#endif /* SYSTRAY */
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateblock(const Arg *arg)
{
	getsigcmds(arg->ui);
	updatestatus();
}

void
updateclientlist()
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
					attach(c);
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
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
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
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
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
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
}

void
updatestatus(void)
{
	drawbar(selmon);
#ifdef SYSTRAY
	updatesystray();
#endif /* SYSTRAY */
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else {
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
			if (c->isurgent) /* set border to urgent */
				XSetWindowBorder(dpy, c->win, scheme[BorderUrg][ColFg].pixel);
		}
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

void
xinitvisual(void)
{
	int nitems, i;
	long masks = VisualScreenMask | VisualDepthMask | VisualClassMask;
	XRenderPictFormat *fmt;
	XVisualInfo *infos, tpl = { .screen = screen, .depth = 32, .class = TrueColor };

	infos = XGetVisualInfo(dpy, masks, &tpl, &nitems);
	visual = NULL;
	for(i = 0; i < nitems; i ++) {
		fmt = XRenderFindVisualFormat(dpy, infos[i].visual);
		if (fmt->type == PictTypeDirect && fmt->direct.alphaMask) {
			visual = infos[i].visual;
			depth = infos[i].depth;
			cmap = XCreateColormap(dpy, root, visual, AllocNone);
			useargb = 1;
			break;
		}
	}

	XFree(infos);
	if (!visual) {
		visual = DefaultVisual(dpy, screen);
		depth = DefaultDepth(dpy, screen);
		cmap = DefaultColormap(dpy, screen);
	}
}

void
view(const Arg *arg)
{
	int i;
	unsigned int tmptag;

	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
 		return;
	#ifdef TAG_PREVIEW
	switchtag();
	#endif /* TAG_PREVIEW */
	selmon->seltags ^= 1; /* toggle sel tagset */

	if (pertag) {
		if (arg->ui & TAGMASK) {
			selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
			selmon->pertag->prevtag = selmon->pertag->curtag;

			if (arg->ui == ~0)
				selmon->pertag->curtag = 0;
			else {
				for (i=0; !(arg->ui & 1 << i); i++) ;
				selmon->pertag->curtag = i + 1;
			}
		} else {
			tmptag = selmon->pertag->prevtag;
			selmon->pertag->prevtag = selmon->pertag->curtag;
			selmon->pertag->curtag = tmptag;
		}
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
		selmon->sellt = selmon->pertag->sellts[selmon->pertag->curtag];
		selmon->lt[selmon->sellt] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt];
		selmon->lt[selmon->sellt^1] = selmon->pertag->ltidxs[selmon->pertag->curtag][selmon->sellt^1];

		if (gapspertag) {
			selmon->gappoh = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff) >> 0;
			selmon->gappov = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff00) >> 8;
			selmon->gappih = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff0000) >> 16;
			selmon->gappiv = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff000000) >> 24;
		}

		if (pertagbar && selmon->showbar != selmon->pertag->showbars[selmon->pertag->curtag])
			togglebar(NULL);
	} else if (arg->ui & TAGMASK) /* if pertag */
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	focus(NULL);
	arrange(selmon);
}

pid_t
winpid(Window w)
{
	pid_t result = 0;

#ifdef __linux__
	xcb_res_client_id_spec_t spec = {0};
	spec.client = w;
	spec.mask = XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID;

	xcb_generic_error_t *e = NULL;
	xcb_res_query_client_ids_cookie_t c = xcb_res_query_client_ids(xcon, 1, &spec);
	xcb_res_query_client_ids_reply_t *r = xcb_res_query_client_ids_reply(xcon, c, &e);

	if (!r)
		return (pid_t)0;

	xcb_res_client_id_value_iterator_t i = xcb_res_query_client_ids_ids_iterator(r);
	for (; i.rem; xcb_res_client_id_value_next(&i)) {
		spec = i.data->spec;
		if (spec.mask & XCB_RES_CLIENT_ID_MASK_LOCAL_CLIENT_PID) {
			uint32_t *t = xcb_res_client_id_value_value(i.data);
			result = *t;
			break;
		}
	}

	free(r);

	if (result == (pid_t)-1)
		result = 0;

#endif /* __linux__ */

#ifdef __OpenBSD__
        Atom type;
        int format;
        unsigned long len, bytes;
        unsigned char *prop;
        pid_t ret;

	if (XGetWindowProperty(dpy, w, XInternAtom(dpy, "_NET_WM_PID", 0), 0,
				1, False, AnyPropertyType, &type, &format,
				&len, &bytes, &prop) != Success || !prop)
               return 0;

        ret = *(pid_t*)prop;
        XFree(prop);
        result = ret;

#endif /* __OpenBSD__ */
	return result;
}

pid_t
getparentprocess(pid_t p)
{
	unsigned int v = 0;

#if defined(__linux__)
	FILE *f;
	char buf[256];
	snprintf(buf, sizeof(buf) - 1, "/proc/%u/stat", (unsigned)p);

	if (!(f = fopen(buf, "r")))
		return 0;

	//fscanf(f, "%*u %*s %*c %u", &v);
	if (fscanf(f, "%*u %*s %*c %u", (unsigned *)&v) != 1)
		v = (pid_t)0;
	fclose(f);
#elif defined(__FreeBSD__)
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

	if (!w->pid || w->isterminal)
		return NULL;

	for (m = mons; m; m = m->next) {
		for (c = m->clients; c; c = c->next) {
			if (c->isterminal && !c->swallowing && c->pid && isdescprocess(c->pid, w->pid))
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
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
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
	die("dwm: another window manager is already running");
	return -1;
}

void
xrdb(const Arg *arg)
{
	unsigned int i;
	readxresources();

	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 2);
	focus(NULL);
	arrange(NULL);
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange
	|| (selmon->sel && selmon->sel->isfloating))
		return;
	if (c == nexttiled(selmon->clients))
		if (!c || !(c = nexttiled(c->next)))
			return;
	pop(c);
}

void
zoomswap(const Arg *arg)
{
	Client *c = selmon->sel;
	if (arg && arg->v)
		c = (Client*)arg->v;
	if (!c)
		return;
	Client *at = NULL, *cold, *cprevious = NULL,
	       *p = c->mon->pertag->prevzooms[c->mon->pertag->curtag];

	/* if it's floating, make the floating window the new master (which
	 * means making it not floating) */
	if (c && c->isfloating)
		togglefloating(&((Arg) { .v = c }));

	if (!c->mon->lt[c->mon->sellt]->arrange
	|| (c && c->isfloating) || !c)
		return;
	if (c == nexttiled(c->mon->clients)) {
		at = findbefore(p);
		if (at)
			cprevious = nexttiled(at->next);
		if (!cprevious || cprevious != p) {
			p = NULL;
			if (!c || !(c = nexttiled(c->next)))
				return;
		} else
			c = cprevious;
	}
	cold = nexttiled(c->mon->clients);
	if (c != cold && !at)
		at = findbefore(c);
	detach(c);
	attach(c);
	/* swap windows instead of pushing the previous one down */
	if (c != cold && at) {
		p = cold;
		if (cold && at != cold) {
			detach(cold);
			cold->next = at->next;
			at->next = cold;
		}
	}
	focus(c);
	arrange(c->mon);
}

/* dynamic scratchpads */
void
scratchpad_hide(const Arg *arg)
{
	if (selmon->sel) {
		selmon->sel->tags = SCRATCHPAD_MASK;
		selmon->sel->isfloating = 1;
		focus(NULL);
		arrange(selmon);
	}
}
int
scratchpad_last_showed_is_killed(void)
{
	int killed = 1;
	for (Client *c = selmon->clients; c != NULL; c = c->next)
		if (c == scratchpad_last_showed) {
			killed = 0;
			break;
		}
	return killed;
}
void
scratchpad_remove(const Arg *arg)
{
	if (selmon->sel && scratchpad_last_showed != NULL && selmon->sel == scratchpad_last_showed)
		scratchpad_last_showed = NULL;
}
void
scratchpad_show(const Arg *arg)
{
	Client *c;
	if (scratchpad_last_showed == NULL || scratchpad_last_showed_is_killed())
		scratchpad_show_first();
	else {
		if (scratchpad_last_showed->tags != SCRATCHPAD_MASK) {
			scratchpad_last_showed -> tags = SCRATCHPAD_MASK;
			focus(NULL);
			arrange(selmon);
		} else {
			int found_current = 0;
			int found_next = 0;
			for (c = selmon->clients; c != NULL; c = c->next) {
				if (found_current == 0) {
					if (c == scratchpad_last_showed) {
						found_current = 1;
						continue;
					}
				} else {
					if (c->tags == SCRATCHPAD_MASK) {
						found_next = 1;
						scratchpad_show_client (c);
						break;
					}
				}
			}
			if (found_next == 0)
				scratchpad_show_first ();
		}
	}
}
void
scratchpad_show_client(Client *c)
{
	scratchpad_last_showed = c;
	c->tags = selmon->tagset[selmon->seltags];
	focus(c);
	arrange(selmon);
}
void
scratchpad_show_first(void)
{
	for (Client *c = selmon->clients; c != NULL; c = c->next) {
		if (c->tags == SCRATCHPAD_MASK) {
			scratchpad_show_client (c);
			break;
		}
	}
}


/* Customs */
void
togglealwaysontop(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!selmon->sel->isfloating) /* only floating clients can be always on top */
		togglefloating(NULL);
	selmon->sel->alwaysontop = !selmon->sel->alwaysontop;
	focus(NULL);
}

void
togglevacant(const Arg *arg)
{
	hidevacant = !hidevacant;
	drawbar(selmon);
}

static int oldborder;
//TODO
void
toggleborder(const Arg *arg)
{
	Client *c;
	//int prev_borderpx = selmon->borderpx;

	oldborder = selmon->borderpx;
	if (selmon->borderpx == 0)
		selmon->borderpx = oldborder;
	else {
		oldborder = selmon->borderpx;
		selmon->borderpx = 0;
	}
	for (c = selmon->clients; c; c = c->next) {
		if (borderpx == 0)
			c->bw = selmon->borderpx;
		else
			c->bw = selmon->borderpx = 0;
		//if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
		//	resize(c, c->x, c->y, c->w-(arg->i*2), c->h-(arg->i*2), 0);
		//}
	}


	//if (oldborder == 0) {
	//	oldborder  = borderpx;
	//	borderpx = 0;
	//} else {
	//	borderpx  = oldborder;
	//}
	arrange(NULL);
}

void
togglestatus(const Arg *arg)
{
	showstatus = !showstatus;
	getcmds(-1);
	updatestatus();
}

void
shiftswaptags(const Arg *arg)
{
	Arg shifted;
	shifted.ui = selmon->tagset[selmon->seltags] & ~SPTAGMASK;
	if (arg->i > 0)	/* left circular shift */
		shifted.ui = ((shifted.ui << arg->i) | (shifted.ui >> (LENGTH(tags) - arg->i))) & ~SPTAGMASK;
	else		/* right circular shift */
		shifted.ui = ((shifted.ui >> (- arg->i) | shifted.ui << (LENGTH(tags) + arg->i))) & ~SPTAGMASK;
	swaptags(&shifted);
}
void
swaptags(const Arg *arg)
{
	Client *c;
	unsigned int newtag = arg->ui & TAGMASK;
	unsigned int curtag = selmon->tagset[selmon->seltags] & ~SPTAGMASK;

	if (newtag == curtag || !curtag || (curtag & (curtag-1)))
		return;

	for (c = selmon->clients; c != NULL; c = c->next) {
		if ((c->tags & newtag) || (c->tags & curtag))
			c->tags ^= curtag ^ newtag;

		if (!c->tags)
			c->tags = newtag;
	}

	//selmon->tagset[selmon->seltags] = newtag;

	focus(NULL);
	arrange(selmon);
}
//void
//random_wall(const Arg *arg)
//{
	//char *img = "/home/faber/Media/Pictures/Wallpapers";
	//char *bgloc = "${XDG_DATA_HOME:-$HOME/.local/share/}/bg";
	//symlink()

	//img_path="$HOME/Media/Pictures/Wallpapers"
////	const char bglock = "";
////	if ((bclock = getenv("XDG_DATA_HOME/bg")) == NULL)
////		bglock = getenv("HOME/.local/bg");
//	//loadrandom_wall(NULL);
//	//wait(NULL);
//	system("dwm_random_wall");
//	xrdb(NULL);
	//char cmd[50];
	//if (tag)
	//	snprintf(cmd, sizeof(cmd), "%s --hide --show %d &", preview_path, tag);
	//else
	//	snprintf(cmd, sizeof(cmd), "%s --hide &", preview_path);
	//system(cmd);
//}
void
random_wall(const Arg *arg)
{
	if (system("dwm_random_wall") != 0)
		fallbackcolors();
	else
		xrdb(NULL);
}
/*void
toggletopbar(const Arg *arg)
{
	topbar = !topbar;
	updatebarpos(selmon);
	drawbar(selmon);
	//arrange(NULL);
}*/
/* end of Customs */

int
main(int argc, char *argv[])
{
	if (argc == 3 && !strcmp("--restart", argv[1])) {
			restart = 1;
			lasttags = atoi(argv[2]);
	} else if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	if (!(xcon = XGetXCBConnection(dpy)))
		die("dwm: cannot get xcb connection");
	checkotherwm();
	XrmInitialize();
	setup();
	#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec ps", NULL) == -1) die("pledge");
	#endif /* __OpenBSD__ */
	scan();
	run();
	char t[12];
	snprintf(t, sizeof(t), "%d", selmon->tagset[selmon->seltags]);
	if (restart) execlp(argv[0], argv[0], "--restart", t, (char *) NULL);
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
