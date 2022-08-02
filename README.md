# Dynamic Explosion Mental's Window Manager

![Image](https://explosion-mental.codeberg.page/img/screensus/rice.gif "demwm")

_some nice screenshots [here](https://explosion-mental.codeberg.page/topics/demwm.html#Screenshots)_

# Requirements
- In order to build demwm you need the Xlib header files.
- Xcb header files for the `swallow patch`.
- Any(?) C99 compiler (gcc and tcc tested)
## Optional
- [pywal](https://github.com/dylanaraps/pywal)
	* if not installed it will set colors to a fallback one defined in config.h
- [libxft-bgra](https://github.com/uditkarode/libxft-bgra)
	* There are emojis on config.h, replacing the unicode characters to
	  non-unicode will make independant of libxft-bgra.
- xwallpaper, dunst, glava, etc
	* There is a script that calls these programs, nothing will crash if
	  not installed, please take a look and modify
	  [demwm_random_wall](#demwm_random_wall)

# Suggestions
- Refer to the man page
  [online](https://explosion-mental.codeberg.page/topics/demwm-man.html) or in
  this repo
- To redirect error mesagges to a file `exec demwm 2> "$HOME/.cache/demwm.log"`
- edit `config.mk` and change `CC = cc`, I have it default to `tcc` since it's
  very very fast and I can make changes in the go.
- edit `demwm_random_wall` to suit you
- If you are looking for Workspaces, then use [dusk](https://github.com/bakkeby/dusk)
- On `xinitrc` you should call `demwm_random_wall` or `xrdb` or any other
  program that **set**s the Xresources that demwm would read (without
  backgrounding it '`&`'). Example of a xinitrc:
  ```sh
  demwm_random_wall
  exec demwm
  ```

# Features
- Gaps (vanitygaps)
- Xresources color reading, configurable in config.h. Syncs with
  [pywal](https://github.com/dylanaraps/pywal) colors by default.
- Changing the schemes (colors) at run time
- More [layouts](https://github.com/explosion-mental/Dwm/blob/main/layouts.c)
- Custom bar height
- Different color schemes: Normal, Selected, Title, Status, Layout, Urgent and
  **Ind**icator schemes (underlines in tags)
- Different schemes for border instances
- Transparency while keeping the text opaque on the bar
- A beautiful looking config.h file (at least for my taste :)
- Toggleable 'hide-vacant' patch, which enables alternative tags on toggle
- Dynamic scratchpads: any window can be added or removed as a scratchpad
- Static scratchpads: defined in config.h and move between monitors
- Centered title only if there is space left
- Status text handling, like dwmblocks, but asynchronous (see below)
- Windows remember their tags, they all don't stack into the first tag when
  restarting
- Remember the selected tags when restarting
- Click to focus, removed the hover to focus functionality.
- Focus tiled windows and C them on top of floating if they have focus.

# Toggleable Features
Here are the "too bloated" features which doesn't affect the workflow, but
sometimes are nice. These features are avaliable and optional via CPP (won't be
compiled in if you don't need it).

Edit `config.mk` and uncomment the necesary lines to enable them.

## Icons
Window icons on the title bar, if the program has one to offer.

Uncomment the line with `#ICONS`
**requires imlib2**

## Systray
System Tray. Blueman, screenkeys, obs, etc on your bar. You should use this if
you expect bar functionality like a Destop Enviroment.
e.g
- `nm-applet` will display NetworkManager systemtray
- `pasystray` will display PulseAudio systemtray


Uncomment the line with `#SYSTRAY`

## Tag Previews
Hover your mouse over a tag (you need to have it viewed first) and look the
contents of it.

Uncomment the line with `#TAG_PREVIEW`
**requires imlib2**

# demwm_random_wall
This is a little script chooses a random file from the `img_dir` directory
variable, runs **pywal** with the chosen image and symlinks to `bgloc`. Then
makes the current `demwm` instance execute `xrdb` to refresh the colors, by
calling `xsetroot -name xrdb`.

The script is copied/deleted to `${PREFIX}/bin` by default, `PREFIX` path can
be changed in config.mk.

This script is not a dependecy.

# Patches branch
My little collection of patches, which aren't necessarily applied to this.

Most of my modification aren't a patch. You can copy the url of the commit and
then download it.

e.g.

```
curl -sL https://github.com/explosion-mental/demwm/commit/d8e4af59c570c4b8ef36ef8942c58ab89921900f > dwm-default-layouts-pertag.diff
```

Here are some that I converted into a patch:
- shift-tools (shift windows,tags or both either with an active client or not)
- integrated status text <s>(TODO: add the `poll` version)</s>
- gruvbox theme (for the bar)
- truecenteredtitle

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
- Make a window 'transient' like with a keybinding. Usecase for this will be to
  have a video playing (any window) in a 'non-intrusive' position, say the
  bottom right, and make it impossible to focus that window; initially thought only by keybinding (focusstack),
  but I'm open for make a flag to also ignore mouse buttons.
- <s>add `uncursor` to `Rules`: basically if window with `uncursor` enabled, will
  make the cursor go into one corner of the screen when focused (or maybe use
  any coordinates other than the window cordinates). I will find this useful
  since I have gaps, which means no clicking in the window since there are
  gaps. Could prevent accidental clicks or make it more 'safe' (don't touch
  anything).</s>
- <s>use fork/exec/dup2 instead of popen in `getcmd` funcs (to remove delay).</s>
- <s>Make _tag previews_ more solid</s> and prevent from being shown in the preview of the preview...
- <s>systray toggleable with _#ifdef_ and _#endif_</s>
- <s>make [hide vacant patch](https://dwm.suckless.org/patches/hide_vacant_tags/)
  toggleable</s>
- <s>a better way of handling layouts at startup</s>

## Some wishing pains:
- new logo (?)
- <s>Write a proper manpage</s>
- <s>s/dwm/demwm/</s>
- <s>Rename the proyect</s>
- pertag cursor position. Save the state of the cursor position between tags.
- <s>change `dwm_random_wall` script to an actual function on C.</s> Why? being a script is much easier to tweak.
- <s>Can `tcc` (tiny C Compiler) be used along side with the **alpha** patch? yes, with macros for `baralpha` and `boderalpha` compiles but if `tcc` is being used it crashes.</s>
- <s>integrate dwmblocks into dwm with statuscmd patch.</s>
- <s>Make gaps an option(?)</s>
- <s>getenv("TERMINAL")</s> Tbh the macro it's better option

# Mayor improvements:
- xrdb patch finally working with pywal without restarting or recompiling (Mar 01 2021)
	* Implemented fallback colors in case any errors (Jan 27 2022)
		* Now it's configurable, not hardcoded to pywal (Jun 13 2022)
- Layouts at startup (Apr 04 2021)
	* A better way of handling this (per monitor) (Aug 08 2021)
- Gaps per tag instead of per monitor (Apr 04 2021)
- Better way of handling Pywal colors (Jun 21 2021)
- Window icons (Jul 22 2021)
	* Icons on swallowed - unswallowed windows (Jul 23 2021)
- Toggleable hide vacants tags (Jul 28 2021)
	* Now hidevacants activates alternative tags (Oct 15 2021)
		* Hidevacants multimonitor friendly (Jun 06 2022)
- System Tray (Aug 06 2021)
- Tag previews (Aug 10 2021)
- Borders colors and general colors now have different schemes (Oct 16 2021)
- status text (blocks) handled by dwm itself (Feb 12 2022)
	* Asynchronous block handling (Feb 25 2022)
	* Do not call `poll` if the bar is not in use (Apr 12 2022)
- Call dwm functions with `xsetroot -name` [(Feb 21 2022)](https://codeberg.org/explosion-mental/Dwm/commit/f03519c70c5c9ada10afda3b6175ea5dfa9b1755)
- Windows remember their tags, useful when restarting (Feb 28 2022)
- Fixed some patches bugs that the compiler fixes (Mar 06 2022)
- Remember the selected tags when restarting (Mar 09 2022)
- uncursor window rule (Mar 18 2022)
- Static scratchpads automatically move between monitors (Jun 12 2022)
