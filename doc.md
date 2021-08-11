
# Table of Contents

1.  [spawn](#org6b15252)
2.  [killclient](#orga807164)
3.  [TAGKEYS](#orgccf65af)
4.  [setmfact](#org642a0c4)
5.  [togglebar](#orgb0de561)
6.  [focusmon](#org1aac5af)
7.  [tagmon](#org3b9f767)

Here are most of the functions that the **user** has to touch.


<a id="org6b15252"></a>

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


<a id="orga807164"></a>

# killclient

The name describe it, kills a window client.


## Arg

It takes no argument, so you could put it like `.v = NULL` or just `0`.


## Example

    { MODKEY,	                XK_q,	killclient,		{0}		},

or

    { MODKEY,	                XK_q,	killclient,		{ .v = NULL }		},


<a id="orgccf65af"></a>

# TAGKEYS


## view


## tag


## toggleview


## toggletag


<a id="org642a0c4"></a>

# setmfact

**Sets** the **master** **factor** area, which is the size of the master(s) windows. In reality, it modifies the `mfact` (master factor area) value at runtime.


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,                       XK_h,	setmfact,	{ .f = -0.02 }		},
    { MODKEY,                       XK_l,	setmfact,	{ .f = +0.02 }		},


<a id="orgb0de561"></a>

# togglebar

(Un)hides the bar.


## Arg

No argument needed.


## Example

    { MODKEY|ControlMask,	    XK_grave,	togglebar,		{0}		},


<a id="org1aac5af"></a>

# focusmon

*Focus monitor*.


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.


## Example

    { MODKEY,                   XK_comma,	focusmon,	{ .i = -1 }		},
    { MODKEY,                  XK_period,	focusmon,	{ .i = +1 }		},


<a id="org3b9f767"></a>

# tagmon

Like `tag` but to the other monitor. Sends the window to the next monitor (display 0, 1, etc..).


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.

    I'm on display 0. To 'tagmon' another monitor, I would press the keybinding with the argument +1


## Example

    { MODKEY|ShiftMask,         XK_comma,	tagmon,		{ .i = -1 }		},
    { MODKEY|ShiftMask,        XK_period,	tagmon,		{ .i = +1 }		},

