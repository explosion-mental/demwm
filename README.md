# Dwm
My build of dwm

![Image](https://explosion-mental.codeberg.page/img/rice.gif "dwm")

_some nice screenshots [here](https://explosion-mental.codeberg.page/topics/dwm)_

# Requirements
- In order to build dwm you need the Xlib header files.
- Xcb header files for the `swallow patch`.
- Any(?) C99 compiler (gcc and tcc tested)
## Optional
- [pywal](https://github.com/dylanaraps/pywal)
	* if not installed it will set colors to (hopefully) default dwm colors
- [libxft-bgra](https://github.com/uditkarode/libxft-bgra)
	* There are emojis on config.h, replacing the unicode characters to
	  non-unicode will make independant of libxft-bgra.
- xwallpaper
	* You can use other program to set the wallpaper,
	  [see](https://github.com/explosion-mental/Dwm/blob/main/dwm_random_wall)

# Suggestions
- Before updating (`git pull`), change your configs (config.h) to config.def.h
- An incomplete [documentation](doc.md) about dwm functions.
- To redirect error mesagges to a file `exec dwm 2> "$HOME"/.cache/dwm.log`
- edit `config.mk` and change `CC = cc`, I have it default to `tcc` since it's
  very very fast and I can make changes in the go.
- edit `dwm_random_wall` to suit you

# Features
- Gaps (vanitygaps)
- [pywal](https://github.com/dylanaraps/pywal) color syncronization by default
  (xresources reading)
- Changing the schemes (colors) at run time
- More [layouts](https://github.com/explosion-mental/Dwm/blob/main/layouts.c)
- Custom bar height
- Different color schemes for Title, Status and Layout (added 4 more shemes on
  top of the other mentioned: Urgent - Notify - Indicator on/off)
- Different scheme for border instances
- Transparency on the bar
- A beautiful looking config.h file (at least for my taste :)
- Toggleable 'hide-vacant' patch, which enables alternative tags on toggle
- Dynamic and static scratchpads
- Centered title only if there is space left
- Status text handling, like dwmblocks, but asynchronous (see below)
- Windows remember their tags, they all don't stack into the first tag when
  restarting
- Remember the selected tags when restarting

# Status text
I've decided to integrate dwmblocks into dwm itself.
Why? Some reasons:
- One config file
- Remove the 'extra' program (which was a bit annoying for me)
- Dwm handles the **displaying** of the text and the **clicking** interatcion,
  no more signals for the buttons.
- This way we could expand and play around with 'blocks' (different colors for
  different blocks, for example)

## How to interact with blocks
A block is only a group that consist of 3 things: command, interval and signal.

The command is the block text, just the output of any program. For example you
could define the command as `echo "This is a block"`.

The interval is how many X seconds you want to pass before re-**run**ning the
command and update the output. Can be 0, which means never.

The signal is a number where you can re-**run** the command. Say a signal
number `Y` which, for example, I will define as `11`. Then `Y + 34` -> `11 +
34` -> `45`, and you will need this result in order to actually use the signal
with `kill`, In this case: `kill -45 $(pidof dwm)`. `pidof dwm` gets the pid of
dwm, but there is a more friendly way. This build of `dwm` sets an
_enviromental variable_ called `STATUSBAR` which value is the _pid_ of the dwm.
In short do: `kill -45 $STATUSBAR`.

Signaling mentioned above shouldn't work with BSD's, so a recommended way to
update a block with a signal is to use `xsetroot -name 1`, to update a block
with signal 1, for example.
You can do much more with `xsetroot`, see below..

## Scripts

For clicking to do anything you have to make a dedicated script which handles
the `BLOCK_BUTTON` _enviromental variable_, here an example:
```sh
case $BLOCK_BUTTON in
	1) notify-send "You've clicked mouse button $BLOCK_BUTTON" ;;
	2) notify-send "Right click" ;;
	3) notify-send "Middle click" ;;
	4) pamixer --allow-boost -i 1 ;; # volume up
	5) pamixer --allow-boost -d 1 ;; # volume down
	6) "$TERMINAL" -e "$EDITOR" "$0" ;; # edit the block
esac

[ "$(mpc status '%state%')" = 'paused' ] && echo '' && exit
```

To define the value of `BLOCK_BUTTON`, you have to edit _config.h_ mouse
buttons bindings `sendstatusbar`.


Take a look at the last line. **Before exiting it does `echo ''`**. It is
important to echo something (even `''`) to 'notify dwm' that the block has
changed.

# Manage dwm with `xsetroot`

Since dwm handles the text itself, we can use the 'name' of the root window for
other purposes, like managing dwm (similar to the `fakesignal` patch).

- Give it a function, for example `xsetroot -name togglebar`, will toggle the bar
- or a standalone `signal number` of one of the blocks, e.g `xsetroot -name 1` will update block 1
- It can accept functions that require an argument, e.g `xsetroot -name 'cyclelayout -1'` but remember to use `'` or `"` around it.


* Commands:
```sh
xsetroot -name togglebar
xsetroot -name 'cyclelayout N'	# N can be -1 or +1
xsetroot -name 'setlayout N'	# N is an index of an existing layout (0 to the last)
xsetroot -name 'view N'		# N is a tag
xsetroot -name 'tag N'		# N is a tag
xsetroot -name togglefloating
xsetroot -name togglegaps
xsetroot -name togglesmartgaps
xsetroot -name fullscreen
xsetroot -name togglevacant
xsetroot -name togglestatus
xsetroot -name killclient
xsetroot -name xrdb
xsetroot -name random_wall
xsetroot -name refresh
xsetroot -name N		# N is an existing signal of a block
```

# Toggleable Features
Here are the "too bloated" features which doesn't affect the workflow, but
sometimes are nice.

## Icons
Icons for the different programs that offers them.


edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk),
uncomment the line with `#ICONS`

**requires imlib2**

## Systray
System Tray. Blueman, screenkeys, obs, etc on your bar. You should use this if
you expect bar functionality like a Destop Enviroment.
e.g
- `nm-applet` will display NetworkManager systemtray
- `pasystray` will display PulseAudio systemtray


edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk)
uncomment the line with `#SYSTRAY`

## Tag Previews
Hover your mouse over a tag (you need to have it viewed first) and look the
contents of it.

edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk)
uncomment the line with `#TAG_PREVIEW`

**requires imlib2**

# dwm_random_wall
This is a little script chooses a random file from the `img_dir` directory
variable, runs **pywal** with the chosen image and symlinks to `bgloc` to make
it accesible to other programs.

Currently it uses `fd`, but you might not have it installed, in that case
uncomment the next line to use `find`.

The script is copied/deleted to `${PREFIX}/bin` by default, `PREFIX` path can
be changed in config.mk.

**NOTE**: I run some custom 'hooks' in `postrun` function which forks to the
background, you can delete everything and change at your liking the function.

# Patches
My little collection of patches, which aren't necessarily applied to this. You
can see them and 'unpatch' them if you want to.

Most of my modification aren't a patch. You can copy the url of the commit and
then download it.

e.g.
```
curl -sL https://github.com/explosion-mental/Dwm/commit/d8e4af59c570c4b8ef36ef8942c58ab89921900f > dwm-default-layouts-pertag.diff
```
Here are some that I converted into a patch:
- shift-tools (shift windows,tags or both either with an active client or not)

# What's Next?
Currently I don't wish more 'features' but I'm tweaking from time to time, here
are some ideas:

## Some little TODOS:
- tapresize **corners**
- toggle border
- depatch cfacts(?)
- Make _systray_ background the same color as the bar
- toggleable pertag. This features a workflow in which you can have the such
  'dynamic' window management of dwm and also have a pertag workflow. Can this
  be implememnted just like the hidevacant patch? meaning, with a simple flag
  variable. Maybe intead of a lot of if / else we just re-arrange (re-start /
  re-setup) the pertag variables to default one and somehow maintain them the
  same while pertag is active.
	* another idea. maybe just doing all the above but only for the layout..
- add `uncursor` to `Rules`: basically if window with `uncursor` enabled, will
  make the cursor go into one corner of the screen when focused (or maybe use
  any coordinates other than the window cordinates). I will find this useful
  since I have gaps, which means no clicking in the window since there are
  gaps. Could prevent accidental clicks or make it more 'safe' (don't touch
  anything).
- <s>use fork/exec/dup2 instead of popen in `getcmd` funcs (to remove delay).</s>
- <s>Make _tag previews_ more solid</s> and prevent from being shown in the preview of the preview...
- <s>systray toggleable with _#ifdef_ and _#endif_</s>
- <s>make [hide vacant patch](https://dwm.suckless.org/patches/hide_vacant_tags/)
  toggleable</s>
- <s>a better way of handling layouts at startup</s>

## Some wishing pains:
- change `dwm_random_wall` script to an actual function on C.
- pertag cursor position. Save the state of the cursor position between tags.
- Can `tcc` (tiny C Compiler) be used along side with the **alpha** patch? yes, with macros for `baralpha` and `boderalpha` compiles but if `tcc` is being used it crashes.
- <s>integrate dwmblocks into dwm with statuscmd patch.</s>
- <s>Make gaps an option(?)</s>
- <s>getenv("TERMINAL")</s>

# Mayor improvements:
- xrdb patch finally working with pywal without restarting or recompiling (Mar
  01 2021)
	* Implemented fallback colors in case any errors (Jan 27 2022)
- Layouts at startup (Apr 04 2021)
	* A better way of handling this (per monitor) (Aug 08 2021)
- Gaps per tag instead of per monitor (Apr 04 2021)
- Better way of handling Pywal colors (Jun 21 2021)
- Window icons (Jul 22 2021)
	* Icons on swallowed - unswallowed windows (Jul 23 2021)
- Toggleable hide vacants tags (Jul 28 2021)
	* Now hidevacants activates alternative tags (Oct 15 2021)
- System Tray (Aug 06 2021)
- Tag previews (Aug 10 2021)
- Borders colors and general colors now have different schemes (Oct 16 2021)
- status text (blocks) handled by dwm itself (Feb 12 2022)
	* Asynchronous block handling (Feb 25 2022)
- Windows remember their tags, useful when restarting (Feb 28 2022)
- Remember the selected tags when restarting (Mar 09 2022)
