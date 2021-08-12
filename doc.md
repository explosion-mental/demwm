
# Table of Contents

1.  [TAGKEYS](#orgacebd07)
2.  [spawn](#orgf88fee3)
3.  [killclient](#org54d024f)
4.  [focusstack](#orge7699ff)
5.  [setmfact](#orgc51e595)
6.  [zoom](#org0f3d9dd)
7.  [togglebar](#org18c433a)
8.  [togglefloating](#org834ed1b)
9.  [focusmon](#org0068a82)
10. [tagmon](#orgea59c8d)
11. [incnmaster](#org08398b0)

Here are most of the functions that the **user** has to touch.


<a id="orgacebd07"></a>

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


<a id="orgf88fee3"></a>

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


<a id="org54d024f"></a>

# killclient

The name describe it, kills a window client.


## Arg

It takes no argument, so you could put it like `.v = NULL` or just `0`.


## Example

    { MODKEY,	                XK_q,	killclient,		{0}		},

or

    { MODKEY,	                XK_q,	killclient,		{ .v = NULL }		},


<a id="orge7699ff"></a>

# focusstack

More like, &rsquo;focus client&rsquo;. Focuses the next/prev client (window).


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,		XK_j,	focusstack,	{ .i = +1 }	},
    { MODKEY,		XK_k,	focusstack,	{ .i = -1 }	},


<a id="orgc51e595"></a>

# setmfact

**Sets** the **master** **factor** area, which is the size of the master(s) windows. In reality, it modifies the `mfact` (master factor area) value at runtime.


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,                       XK_h,	setmfact,	{ .f = -0.02 }		},
    { MODKEY,                       XK_l,	setmfact,	{ .f = +0.02 }		},


<a id="org0f3d9dd"></a>

# zoom

swaps between the first window on the master stack to the current window. If you are already on the first master window, it uses the second master window.


## Arg

It takes no argument.


## Example

    { MODKEY,                       XK_w,	zoom,			{0}		},


<a id="org18c433a"></a>

# togglebar

(Un)hides the bar.


## Arg

No argument needed.


## Example

    { MODKEY|ControlMask,	    XK_grave,	togglebar,		{0}		},


<a id="org834ed1b"></a>

# togglefloating

Toggles floating behaviour on windows. This depends on the current state of the window.


## Arg

It takes no argument.


## Example

    { MODKEY|ShiftMask,     XK_space,	togglefloating,		{0}		},


<a id="org0068a82"></a>

# focusmon

*Focus monitor*.


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.


## Example

    { MODKEY,                   XK_comma,	focusmon,	{ .i = -1 }		},
    { MODKEY,                  XK_period,	focusmon,	{ .i = +1 }		},


<a id="orgea59c8d"></a>

# tagmon

Like `tag` but to the other monitor. Sends the window to the next monitor (display 0, 1, etc..).


## Arg

It takes an *integer* argument and its bidirectional.

+1/-1 &rsquo;*adds/sutracts*&rsquo; **1** to the current value of the display.

    I'm on display 0. To 'tagmon' another monitor, I would press the keybinding with the argument +1


## Example

    { MODKEY|ShiftMask,         XK_comma,	tagmon,		{ .i = -1 }		},
    { MODKEY|ShiftMask,        XK_period,	tagmon,		{ .i = +1 }		},


<a id="org08398b0"></a>

# incnmaster

Increase master, sutracts/adds the first slave window to the master stack (left side on default tile layout).


## Arg

It takes an *integer* argument and its bidirectional.


## Example

    { MODKEY,           XK_semicolon,	incnmaster,	{ .i = +1 }		},
    { MODKEY|ShiftMask, XK_semicolon,	incnmaster,	{ .i = -1 }		},

Here `MODKEY + ;` adds the first window on the slave stack and `MODKEY + Shift + ;` retrieves the last window on the master stack to the slave stack.

