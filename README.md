# Dwm
My build of dwm

![Image](rice.gif "dwm")

# Requirements
- In order to build dwm you need the Xlib header files.
- Xcb header files for the `swallow patch`.
## Optional
- [pywal](https://github.com/dylanaraps/pywal)
	* if not installed it will set colors to (hopefully) default dwm colors
- [libxft-bgra](https://github.com/uditkarode/libxft-bgra)
	* The `tags[]` array has emojis, replacing the unicode characters to
	  non-unicode will make independant of libxft-bgra.
	  [line](https://github.com/explosion-mental/Dwm/blob/410c01096547f3400d9afad7a091e3cb8c11397e/config.h#L63)
- xwallpaper
	* You can use other program to set the wallpaper,
	  [see](https://github.com/explosion-mental/Dwm/blob/main/dwm_random_wall)
- dwmblocks
	* You can disable/enable autostarting dwmblocks on `config.h`

# Recommendations
- Before updating (`git pull`), change your configs (config.h) to config.def.h
- Please look at the [docs](doc.md)

# Features
- Gaps (vanitygaps)
- [pywal](https://github.com/dylanaraps/pywal) color syncronization by default
  (xresources reading)
- Changing the schemes (colors) at run time
- More [layouts](https://github.com/explosion-mental/Dwm/blob/main/layouts.c)
- Custom bar height
- Separated border color scheme from general color scheme (which improves
  readability, in a way)
- Different color schemes for Title, Status and Layout (added 4 more shemes on
  top of the other mentioned: Urgent - Notify - Indicator on/of)
- Transparency on the bar
- A beautiful looking config.h file (at least for my taste :)
- Toggleable 'hide-vacant' patch which activates alternative tags that makes
  tag movements easy
- Dynamic and static scratchpads


# Toggleable Features
Here are the "too bloated" features which doesn't affect the workflow, but
sometimes are nice.

## Icons
Icons for the different programs that offers them.


edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk),
uncomment the line with `#ICONS`

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

# dwm_random_wall
This is a little script that changes the wallpaper to a random one and uses
(requires) **pywal**. It uses the `-o` flag for pywal that runs a `postrun`
script (you can safely delete it), which syncs zathura and dunst to the pywal
coloscheme. By default my dwm build syncs all colors to pywal colors, this only
seems "_useful_" if you wish to change wallpapers (and colorscheme) at runtime.
You should **edit** the variable `img_path` to your actual path of wallpapers
(makes sense to use `pywal` if you also use wallpapers, it would be pointless
otherwise).


Remember, the script is copied and removed by the makefile (`make install` and
`make unistall`).


_I'm working on making this a function on C so no external script would be
needed_

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
Currently I don't wish more 'features' but here are some ideas:

## Some little TODOS:
- toggle border
- depatch cfacts(?)
- Make _systray_ background the same color as the bar
- toggleable pertag. This features a workflow
  in which you can have the such 'dynamic' window management of dwm and also have a pertag
  workflow. Can this be implememnted just like the hidevacant patch? meaning, with a simple flag variable. Maybe intead of a lot of if / else we just re-arrange (re-start / re-setup) the pertag variables to default one and somehow maintain them the same while pertag is active.
	* another idea. maybe just doing all the above but only for the layout..
- <s>Make _tag previews_ more solid</s> and prevent from being shown in the preview of the preview...
- <s>systray toggleable with _#ifdef_ and _#endif_</s>
- <s>make [hide vacant patch](https://dwm.suckless.org/patches/hide_vacant_tags/)
  toggleable</s>
- <s>a better way of handling layouts at startup</s>

## Some wishing pains:
- change `dwm_random_wall` script to an actual function on C.
- pertag cursor position. Save the state of the cursor position between tags.
- integrate dwmblocks into dwm (prob not worth it).
- Make gaps an option(?)
- Can `tcc` (tiny C Compiler) be used along side with the **alpha** patch?
- <s>getenv("TERMINAL")</s>

# Mayor improvements:
- xrdb patch finally working with pywal without restarting or recompiling (Mar
  01 2021)
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
- Borders colors and general colors has different schemes (Oct 16 2021)
