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
		 * after we add a new tag
		 * Collect (from last to first) references to all clients in the master area */
		Client **const masters = ecalloc(selmon->nmaster, sizeof(Client *));
		Client *c, *selected = selmon->sel;
		size_t i;

		/* put the master clients at the front of the list > go from
		 * the 'last' master to the 'first' */
		for (c = nexttiled(selmon->clients), i = 0; c && i < selmon->nmaster; c = nexttiled(c->next), i++) {
			masters[selmon->nmaster - (i + 1)] = c;
			if (masters[i])
				pop(masters[i]);
		}
		free(masters);

		if (selmon->sel != selected) /* don't mutate the focus */
			focus(selected);

		/* toggleview */
		selmon->seltags |= newtags;

		#ifdef TAG_PREVIEW
		getpreview();
		#endif /* TAG_PREVIEW */
	} else {
		combo = 1;
		if (newtags)
			view(&((Arg) { .ui = newtags }));
	}

	focus(NULL);
	arrange(selmon);
}

void
cyclelayout(const Arg *arg)
{
	Layout *l;
	unsigned int idx = 0;

	for (l = (Layout *)layouts; l != selmon->lt; l++, idx++);

	if (arg->i > 0) {
		if (idx < LENGTH(layouts) - 1)
			idx += arg->i;
		else
			idx = 0;
	} else {
		if (idx > 0)
			idx += arg->i;
		else
			idx = LENGTH(layouts) - 1;
	}

	setlayout(&((Arg) { .v = &layouts[idx] }));
}

void
focusmaster(const Arg *arg)
{
	Client *c;

	if (selmon->nmaster < 1 || !selmon->sel)
		return;

	if ((c = nexttiled(selmon->clients)))
		focus(c);
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

	/* Prevents focus to drift from windows that are in fake fullscreen. */
	 //if (!selmon->sel || (selmon->sel->isfullscreen && selmon->sel->fakefullscreen != 1))

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

	if (!sel || sel->f & Float)
		return;

	if (arg->i > 0) { /* pushdown */
		if ((c = nexttiled(sel->next))) {
			detach(sel);
			sel->next = c->next;
			c->next = sel;
		} else { /* move to the front */
			detach(sel);
			attachdefault(sel);
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


void
incnmaster(const Arg *arg)
{
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag] = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
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
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->f & FS && c->fakefullscreen != 1) /* no support moving fullscreen windows by mouse */
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
			else if (abs(selmon->my - ny) < snap)
				ny = selmon->my;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			else if (abs((selmon->my + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->my + selmon->wh - HEIGHT(c);
			if (!(c->f & Float) && selmon->lt->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt->arrange || c->f & Float)
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

void
quit(const Arg *arg)
{
	running = 0;
}

void
restart(const Arg *arg)
{
	running = -1;
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
	if (c->f & FS && c->fakefullscreen != 1) /* no support resizing fullscreen windows by mouse */
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
				if (!(c->f & Float) && selmon->lt->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt->arrange || c->f & Float)
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
	if (c->f & FS && c->fakefullscreen != 1) /* no support resizing fullscreen windows by mouse */
		return;
	if (!(c->f & Float) && selmon->lt->arrange)
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
	if (!selmon->lt->arrange || c->f & Float)
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

/* vanitygaps */
void
togglesmartgaps(const Arg *arg)
{
	smartgaps = !smartgaps;
	arrange(NULL);
}
void
togglegaps(const Arg *arg)
{
	selmon->pertag->enablegaps ^= selmon->seltags & ~SPTAGMASK;
	arrange(selmon);
}

void
defaultgaps(const Arg *arg)
{
	setgaps(gappoh, gappov, gappih, gappiv);
}
void
incrgaps(const Arg *arg)
{
	setgaps(selmon->gappoh + arg->i,
		selmon->gappov + arg->i,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i);
}
void
incrigaps(const Arg *arg)
{
	setgaps(selmon->gappoh,
		selmon->gappov,
		selmon->gappih + arg->i,
		selmon->gappiv + arg->i);
}
void
incrogaps(const Arg *arg)
{
	setgaps(selmon->gappoh + arg->i,
		selmon->gappov + arg->i,
		selmon->gappih,
		selmon->gappiv);
}
void
incrohgaps(const Arg *arg)
{
	setgaps(selmon->gappoh + arg->i,
		selmon->gappov,
		selmon->gappih,
		selmon->gappiv);
}
void
incrovgaps(const Arg *arg)
{
	setgaps(selmon->gappoh,
		selmon->gappov + arg->i,
		selmon->gappih,
		selmon->gappiv);
}
void
incrihgaps(const Arg *arg)
{
	setgaps(selmon->gappoh,
		selmon->gappov,
		selmon->gappih + arg->i,
		selmon->gappiv);
}
void
incrivgaps(const Arg *arg)
{
	setgaps(selmon->gappoh,
		selmon->gappov,
		selmon->gappih,
		selmon->gappiv + arg->i);
}
/* vanitygaps */

void
sendstatusbar(const Arg *arg)
{
	getcmd(blocknum, (char []){'0' + arg->i, '\0'});
}

void
setlayout(const Arg *arg)
{
	if (arg && arg->v) {
		selmon->lt = selmon->pertag->ltidxs[selmon->pertag->curtag] = (const Layout *)arg->v;
		strncpy(selmon->ltsymbol, selmon->lt->symbol, sizeof selmon->ltsymbol);
	}

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

	if (!arg || !c || !selmon->lt->arrange)
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

	if (!arg || !selmon->lt->arrange)
		return;

	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;

	if (f < 0.05 || f > 0.95)
		return;

	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag] = f;

	arrange(selmon);
}

void
shifttag(const Arg *arg)
{
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };

	if (!selmon->clients)
		return;

	shift(&shifted.ui, arg->i);
	tag(&shifted);
}

void
shifttagclients(const Arg *arg)
{

	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };
	Client *c;
	unsigned int tagmask = 0;

	for (c = selmon->clients; c; c = c->next)
		if (!(c->tags & SPTAGMASK))
			tagmask = tagmask | c->tags;

	do
		shift(&shifted.ui, arg->i);
	while (tagmask && !(shifted.ui & tagmask));

	tag(&shifted);
}

void
shiftview(const Arg *arg)
{
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };

	shift(&shifted.ui, arg->i);
	view(&shifted);
}

void
shiftviewclients(const Arg *arg)
{
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };
	Client *c;
	unsigned int tagmask = 0, filter = 0;

	for (c = selmon->clients; c; c = c->next)
		if (!(c->tags & SPTAGMASK))
			filter = filter | c->tags;

	/* TODO at least two tags are going to be with a client in order to
	 * shiftviewclient, else shiftview normally */
	tagmask = filter;

	do
		shift(&shifted.ui, arg->i);
	while (tagmask && !(shifted.ui & tagmask));

	view(&shifted);
}

void
shiftboth(const Arg *arg)
{
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };

	shift(&shifted.ui, arg->i);
	tag(&shifted);
	view(&shifted);
}

void
spawn(const Arg *arg)
{
	if (fork() == 0) {
		setsid();
		execvp(((const char **)arg->v)[0], (char *const *)arg->v);
		die("demwm: execvp '%s':", ((const char **)arg->v)[0]);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
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
	if (c->f & FS) {
		c->f &= ~FS;
		sendmon(c, dirtomon(arg->i));
		c->f |= FS;
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
	unsigned int i;

	selmon->f ^= ShowBar;

	for (i = 0; i < LENGTH(tags); i++)
		if (selmon->f & ShowBar)
			selmon->pertag->showbars |= 1 << i;
		else
			selmon->pertag->showbars &= ~(1 << i);

	updatebarpos(selmon);
	resizebarwin(selmon);
	arrange(selmon);
}

void
toggletagbar(const Arg *arg)
{
	selmon->f ^= ShowBar;

	if (selmon->f & ShowBar)
		selmon->pertag->showbars |= selmon->seltags & ~SPTAGMASK;
	else
		selmon->pertag->showbars &= ~selmon->seltags & ~SPTAGMASK;

	updatebarpos(selmon);
	focus(NULL);
	resizebarwin(selmon);
	arrange(selmon);
}

void
togglefakefullscreen(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!c)
		return;

	if (c->fakefullscreen != 1 && c->f & FS) { /* exit fullscreen --> fake fullscreen */
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
	XWindowChanges wc;

	if (!c)
		return;
	if (c->f & FS && c->fakefullscreen != 1) /* no support for fullscreen windows */
		return;
	/* if floating: toggle it, if fixed: true */
	SETVAL(c, Float, ((c->f & Float) ^ Float) || (c->f & Fixed));

	if (c->f & Float) {
		wc.border_width = c->bw = fborderpx;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XSetWindowBorder(dpy, c->win, scheme[BorderFloat][ColFg].pixel);

		if (c->sfx == -9999) /* first time floating */
			resize(c, c->x, c->y, c->w, c->h, 0);
		else	/* restore last known float dimensions */
			resize(c, c->sfx, c->sfy, c->sfw, c->sfh, 0);
	} else {
		wc.border_width = c->bw = borderpx;
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc);
		XSetWindowBorder(dpy, c->win, scheme[BorderSel][ColFg].pixel);

		/* save last known float dimensions */
		c->sfx = c->x; c->sfy = c->y;
		c->sfw = c->w; c->sfh = c->h;
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
		setfullscreen(c, !(c->f & FS ? 1 : 0));
}

void
togglescratch(const Arg *arg)
{
	Client *c, *next, *last = NULL, *found = NULL, *monclients = NULL;
	Monitor *mon;
	unsigned int scratchtag = SPTAG(arg->ui);

	//TODO remove unneeded operations from original renamed-scratchpads

	/* Loop through all monitor and if any of them have a scratchpad, use it:
	    - hidden scratchpads will be shown
	    - shown scratchpads will be hidden, unless they are being moved to the current monitor
	    - the scratchpads will be moved to the current monitor if they all reside on the same monitor
	    - multiple scratchpads residing on separate monitors will be left in place (NO)
	 */
	for (mon = mons; mon; mon = mon->next) {
		for (c = mon->stack; c; c = next) {
			next = c->snext;

			if (!(c->tags & scratchtag)) /* not a THE scratchpad */
				continue;

			/* Record the first found scratchpad client for focus purposes, but prioritise the
			   scratchpad on the current monitor if one exists */
			if (!found || (mon == selmon && found->mon != selmon))
				found = c;

			/* If scratchpad clients reside on another monitor and we are moving them across then
			   as we are looping through monitors we could be moving a client to a monitor that has
			   not been processed yet, hence we could be processing a scratchpad twice. To avoid
			   this we detach them and add them to a temporary list (monclients) which is to be
			   processed later. */

			if (c->mon != selmon) {
				detach(c);
				detachstack(c);
				c->next = NULL;
				/* Note that we are adding clients at the end of the list, this is to preserve the
				   order of clients as they were on the adjacent monitor (relevant when tiled) */
				if (last)
					last = last->next = c;
				else
					last = monclients = c;
			} else { /* the scratchpad it's on the same monitor */
				XSetWindowBorder(dpy, c->win, scheme[c->f & Float ? BorderFloat : BorderSel][0].pixel);
				c->mon->seltags ^= scratchtag;
				if (c->f & Float)
					XRaiseWindow(dpy, c->win);
			}
		}
	}

	/* Attach moved scratchpad clients on the selected monitor */
	for (c = monclients; c; c = next) {
		next = c->next;
		mon = c->mon;
		c->mon = selmon;
		c->tags = scratchtag;

		/* Attach scratchpad clients from other monitors at the bottom of the stack */
		if (selmon->clients) {
			for (last = selmon->clients; last && last->next; last = last->next);
			last->next = c;
		} else
			selmon->clients = c;
		c->next = NULL;
		attachstack(c);

		/* Center floating scratchpad windows when moved from one monitor to another */
		if (c->f & Float) {
			if (c->w > selmon->ww)
				c->w = selmon->ww - c->bw * 2;
			if (c->h > selmon->wh)
				c->h = selmon->wh - c->bw * 2;

			/* maintain the position but make it relative to the new monitor dimention */
			c->x = c->mon->wx + (c->x - mon->wx) * ((double)(abs(c->mon->ww - WIDTH(c))) / MAX(abs(mon->ww - WIDTH(c)), 1));
			c->y = c->mon->wy + (c->y - mon->wy) * ((double)(abs(c->mon->wh - HEIGHT(c))) / MAX(abs(mon->wh - HEIGHT(c)), 1));

			resizeclient(c, c->x, c->y, c->w, c->h);
			XRaiseWindow(dpy, c->win);
		}

		/* if it isn't selected alrd, tag it */
		if (!(selmon->seltags & scratchtag))
			selmon->seltags ^= scratchtag;
	}

	if (found) {
		focus(ISVISIBLE(found) ? found : NULL);
		arrange(NULL);
	} else {
		selmon->seltags |= scratchtag;
		spawn(&((Arg) { .v = scratchpads[arg->ui] }));
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
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->seltags ^ (arg->ui & TAGMASK);
	int i;

	if (newtagset) {
		#ifdef TAG_PREVIEW
		getpreview();
		#endif /* TAG_PREVIEW */
		selmon->seltags = newtagset;

		if (newtagset == ~0) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			selmon->pertag->curtag = LENGTH(tags);
		}
		/* test if the user did not select the same tag */
		if (!(newtagset & 1 << (selmon->pertag->curtag))) {
			selmon->pertag->prevtag = selmon->pertag->curtag;
			for (i=0; !(newtagset & 1 << i); i++) ;
			selmon->pertag->curtag = i;
		}

		/* apply settings for this view */
		selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
		selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
		selmon->lt = selmon->pertag->ltidxs[selmon->pertag->curtag];
		if ((selmon->f & ShowBar ? 1 : 0) != (selmon->pertag->showbars & (selmon->seltags & ~SPTAGMASK) ? 1 : 0))
			toggletagbar(NULL);
		else {
			focus(NULL);
			arrange(selmon);
		}
	}
}

void
updateblock(const Arg *arg)
{
	getsigcmds(arg->ui);
	updatestatus();
}

void
updateallblocks(const Arg *arg)
{
	getcmds(-1);
	updatestatus();
}

void
view(const Arg *arg)
{
	int i;
	unsigned int tmp;

	if ((arg->ui & TAGMASK) == selmon->seltags)
 		return;
	#ifdef TAG_PREVIEW
	getpreview();
	#endif /* TAG_PREVIEW */

	/* toggle sel tagset */
	tmp = selmon->oldtags;
	selmon->oldtags = selmon->seltags;
	selmon->seltags = tmp;

	if (arg->ui & TAGMASK) {
		selmon->seltags = arg->ui & TAGMASK;
		selmon->pertag->prevtag = selmon->pertag->curtag;

		if (arg->ui == ~0)
			selmon->pertag->curtag = LENGTH(tags);
		else {
			for (i = 0; !(arg->ui & 1 << i); i++);
			selmon->pertag->curtag = i;
		}
	} else {
		tmp = selmon->pertag->prevtag;
		selmon->pertag->prevtag = selmon->pertag->curtag;
		selmon->pertag->curtag = tmp;
	}
	selmon->nmaster = selmon->pertag->nmasters[selmon->pertag->curtag];
	selmon->mfact = selmon->pertag->mfacts[selmon->pertag->curtag];
	selmon->lt = selmon->pertag->ltidxs[selmon->pertag->curtag];

	if (gapspertag) { /* store gaps in 2 bits */
		selmon->gappoh = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff) >> 0;
		selmon->gappov = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff00) >> 8;
		selmon->gappih = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff0000) >> 16;
		selmon->gappiv = (selmon->pertag->gaps[selmon->pertag->curtag] & 0xff000000) >> 24;
	}

	if ((selmon->f & ShowBar ? 1 : 0) != (selmon->pertag->showbars & (selmon->seltags & ~SPTAGMASK) ? 1 : 0))
		toggletagbar(NULL);
	else {
		focus(NULL);
		arrange(selmon);
	}
}

void
xrdb(const Arg *arg)
{
	unsigned int i;
	readxresources();

	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], alphas[i], 2);
	drawstatus();
	focus(NULL);
	arrange(NULL);
}

void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt->arrange || !c || !(c->f & Float))
		return;
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
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
	if (c && c->f & Float)
		togglefloating(NULL);

	if (!c->mon->lt->arrange
	|| (c && c->f & Float) || !c)
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
	attachdefault(c);
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
	tag(&((Arg){.ui = SCRATCHPAD_MASK}));
}
void
scratchpad_remove(const Arg *arg)
{
	if (selmon->sel && scratchpad_last_showed
	&& selmon->sel == scratchpad_last_showed)
		scratchpad_last_showed = NULL;
}
void
scratchpad_show(const Arg *arg)
{
	Client *c;
	int found_current = 0;
	int found_next = 0;

	if (scratchpad_last_showed == NULL || scratchpad_last_showed_is_killed())
		scratchpad_show_first();
	else {
		if (scratchpad_last_showed->tags != SCRATCHPAD_MASK) {
			scratchpad_last_showed->tags = SCRATCHPAD_MASK;
			focus(NULL);
			arrange(selmon);
		} else {
			for (c = selmon->clients; c; c = c->next) {
				if (found_current == 0) {
					if (c == scratchpad_last_showed) {
						found_current = 1;
						continue;
					}
				} else {
					if (c->tags == SCRATCHPAD_MASK) {
						found_next = 1;
						scratchpad_show_client(c);
						break;
					}
				}
			}
			if (found_next == 0)
				scratchpad_show_first();
		}
	}
}

/* Customs */
void
togglealwaysontop(const Arg *arg)
{
	if (!selmon->sel)
		return;
	selmon->sel->f ^= AlwOnTop;
	focus(NULL);
}

void
togglevacant(const Arg *arg)
{
	selmon->f ^= HideVacant;
	drawbar(selmon);
}

void
movfh_setmfact(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->f & Float)
		movefloathorz(&((Arg) { .i = arg->f > 0 ? movefloat : -movefloat }));
	else
		setmfact(arg);
}

void
movfv_pushstack(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->f & Float)
		movefloatvert(&((Arg) { .i = arg->i > 0 ? movefloat : -movefloat }));
	else
		pushstack(arg);
}

void
movefloathorz(const Arg *arg)
{
	if (!(selmon->sel->f & Float) || selmon->sel->f & FS || !selmon->sel)
		return;

	resize(selmon->sel, selmon->sel->x + arg->i, selmon->sel->y, selmon->sel->w, selmon->sel->h, 1);
}

void
movefloatvert(const Arg *arg)
{
	if (!(selmon->sel->f & Float) || selmon->sel->f & FS || !selmon->sel)
		return;

	resize(selmon->sel, selmon->sel->x, selmon->sel->y + arg->i, selmon->sel->w, selmon->sel->h, 1);
}

void
togglestatus(const Arg *arg)
{
	showstatus = !showstatus;

	if (selmon->f & ShowBar && showstatus)
		getcmds(-1);
	updatestatus();
}

void
shiftswaptags(const Arg *arg)
{
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };

	shift(&shifted.ui, arg->i);
	swaptags(&shifted);
}

void
swaptags(const Arg *arg)
{
	Client *c;
	unsigned int newtag = arg->ui & TAGMASK;
	unsigned int curtag = selmon->seltags & ~SPTAGMASK;

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

void
toggletopbar(const Arg *arg)
{
	selmon->f ^= TopBar;

	updatebarpos(selmon);
	resizebarwin(selmon);
	arrange(selmon);
}

void
togglesticky(const Arg *arg)
{
	if (!selmon->sel)
		return;
	selmon->sel->f ^= Sticky;
	focus(NULL);
	arrange(selmon);
}

void
previewtag(const Arg *arg)
{
	#ifdef TAG_PREVIEW
	if (selmon->previewshow != (arg->ui + 1))
		selmon->previewshow = arg->ui + 1;
	else
		selmon->previewshow = 0;
	showtagpreview(arg->ui);
	#endif /* TAG_PREVIEW */
}

void
shiftpreview(const Arg *arg)
{
	//TODO needs an index, currenlty it only shows the adjacent tagpreviews
	#ifdef TAG_PREVIEW
	Arg shifted = { .ui = selmon->seltags & ~SPTAGMASK };
	Client *c;
	unsigned int tagmask = 0;

	for (c = selmon->clients; c; c = c->next)
		if (!(c->tags & SPTAGMASK))
			tagmask |= c->tags;

	do
		shift(&shifted.ui, arg->i);
	while (tagmask && !(shifted.ui & tagmask));

	//redo <<
	shifted.ui = ffs(shifted.ui) - 1;
	previewtag(&shifted);
	#endif /* TAG_PREVIEW */
}

void
swapfocus(const Arg *arg)
{
	static int swapfs = -1;
	//Client *c = selmon->clients;

	//for (; c && c != prevclient; c = c->next);

	//if (c == prevclient && prevclient) {
	//	if (c->tags && ((c->tags & TAGMASK) != selmon->seltags))
	//		view(&((Arg){.ui = c->tags & TAGMASK}));
	//	focus(c);
	//	restack(c->mon);
	//}
	/// Workaround in edge cases with scratchpads..
	swapfs = !swapfs;
	focusstack(&((Arg){.i = swapfs}));
}

void /* change attach */
attachwhere(const Arg *arg)
{
	attach = (void *)arg->v;
}

void
cycleattach(const Arg *arg)
{
	unsigned int idx = 0;

	if (arg->i > 0) {
		if (idx < LENGTH(attachmodes) - 1)
			idx += arg->i;
		else
			idx = 0;
	} else {
		if (idx > 0)
			idx += arg->i;
		else
			idx = LENGTH(attachmodes) - 1;
	}

	attach = attachmodes[idx];
}
