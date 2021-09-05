
# Table of Contents

1.  [TAGKEYS](#org004ce04)
2.  [spawn](#org77dcd86)
3.  [killclient](#org40e491c)
4.  [focusstack](#org8856c7e)
5.  [setmfact](#org544e95a)
6.  [zoom](#org45a3b63)
7.  [togglebar](#org4f3ca3c)
8.  [togglefloating](#org4987883)
9.  [focusmon](#orga641de8)
10. [tagmon](#org0292be9)
11. [incnmaster](#org520ef38)
12. [Scratchpads](#org439a58b)

Here are most of the functions that the **user** has to touch.


<a id="org004ce04"></a>

# TAGKEYS

These functions are elegantly put into the TAGKEYS macro. They are self-explanatory and most of the time you don&rsquo;t want to change the bindings, so you can skip this section.


## view

View the contents of a tag, you can think of it like moving to a tag.


## tag

&ldquo;tag&rdquo; the current window to a tag. Basically like moving the windows to a &rsquo;workspace&rsquo; or a &ldquo;*different desktop*&rdquo;, which is only a way of thinking since dwm doesn&rsquo;t implements those.


## toggleview

Like view but it can stack, meaning you can &rsquo;view&rsquo; multiple tags. This is toggleable.


## toggletag

Like tag but it can stack, meaning you can &rsquo;tag&rsquo; multiple tags. This is toggleable.


<a id="org77dcd86"></a>

# spawn

A wrapper for *exec* function. This executes shell commands.


## Arg

It takes a *void* argument (a variable) or by using the macro **SHCMD**.

    #define SHCMD(cmd)	spawn, { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }


## Example

    static const char *termcmd[] = { terminalcmd, NULL };
    static Key keys[] = {
    	/* modifier(s)      key         function        argument        */
    	{ MODKEY            XK_Return,  spawn,      { .v = termcmd }    },
    	{ MODKEY            XK_w,       SHCMD("$BROWSER")               },
    };


<a id="org40e491c"></a>

# killclient

The name describe it, kills a window client.


## Arg

It takes no argument, so you could put it like `.v = NULL` or just `0`.


## Example

    { MODKEY,	                XK_q,	killclient,		{0}		},

or

    { MODKEY,	                XK_q,	killclient,		{ .v = NULL }		},


<a id="org8856c7e"></a>

# focusstack

More like, &rsquo;focus client&rsquo;. Focuses the next/prev client (window).


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,		XK_j,	focusstack,	{ .i = +1 }	},
    { MODKEY,		XK_k,	focusstack,	{ .i = -1 }	},


<a id="org544e95a"></a>

# setmfact

**Sets** the **master** **factor** area, which is the size of the master(s) windows. In reality, it modifies the `mfact` (master factor area) value at runtime.


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,                       XK_h,	setmfact,	{ .f = -0.02 }		},
    { MODKEY,                       XK_l,	setmfact,	{ .f = +0.02 }		},


<a id="org45a3b63"></a>

# zoom

swaps between the first window on the master stack to the current window. If you are already on the first master window, it uses the second master window.


## Arg

It takes no argument.


## Example

    { MODKEY,                       XK_w,	zoom,			{0}		},


<a id="org4f3ca3c"></a>

# togglebar

(Un)hides the bar.


## Arg

No argument needed.


## Example

    { MODKEY|ControlMask,	    XK_grave,	togglebar,		{0}		},


<a id="org4987883"></a>

# togglefloating

Toggles floating behaviour on windows. This depends on the current state of the window.


## Arg

It takes no argument.


## Example

    { MODKEY|ShiftMask,     XK_space,	togglefloating,		{0}		},


<a id="orga641de8"></a>

# focusmon

*Focus monitor*.


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.


## Example

    { MODKEY,                   XK_comma,	focusmon,	{ .i = -1 }		},
    { MODKEY,                  XK_period,	focusmon,	{ .i = +1 }		},


<a id="org0292be9"></a>

# tagmon

Like `tag` but to the other monitor. Sends the window to the next monitor (display 0, 1, etc..).


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.

    I'm on display 0. To 'tagmon' another monitor, I would press the keybinding with the argument +1


## Example

    { MODKEY|ShiftMask,         XK_comma,	tagmon,		{ .i = -1 }		},
    { MODKEY|ShiftMask,        XK_period,	tagmon,		{ .i = +1 }		},


<a id="org520ef38"></a>

# incnmaster

Increase master, sutracts/adds the first slave window to the master stack (left side on default tile layout).


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,           XK_semicolon,	incnmaster,	{ .i = +1 }		},
    { MODKEY|ShiftMask, XK_semicolon,	incnmaster,	{ .i = -1 }		},

Here `MODKEY + ;` adds the first window on the slave stack and `MODKEY + Shift + ;` retrieves the last window on the master stack to the slave stack.


<a id="org439a58b"></a>

# Scratchpads

I have two patches: *scratchpads* and *dynamic scratchpads*. Both of these do different functions so they work well so here goes a bit of explanation:


## scratchpads

Allows you  to *store* a client on a tag that isn&rsquo;t visible. When you call that client by pressing a keybinding you basically **toggleview** that tag and you will see the client store.


## dynamic scratchpads

