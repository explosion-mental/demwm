# Dwm
My build of dwm

![Image](rice.gif "dwm")

# config.h?...
Yes.. Please look at the [docs](doc.md)

# Features/patches
- vanitygaps (some of the specific increase/decrease on horizontal or vertical functions are commented out since I use only the general one.)
- [pywal](https://github.com/dylanaraps/pywal) color syncronization by default
- more layouts
- custom bar height
- Different color schemes for Title, Status and Layout (added 4 more shemes on top of the other mentioned: Urgent - Notify - Indicator on/of)
- Transparency on the bar
- A beautiful looking config.h file (at least for my taste :)


see: [layouts.c](https://github.com/explosion-mental/Dwm/blob/main/layouts.c)

## dwm_random_wall
This is a little script that changes the wallpaper to a random one and uses
pywal. By default my dwm build syncs all colors to pywal colors. You should
**edit** the variable `img_path` to your actual path of wallpapers.

Remember, the script is copied and removed automatically by the makefile (`make
install` and `make unistall`).

_Note_: you need **pywal**

## Toggleable Features
Here are the "Too bloated" features which doesn't affect the workflow, but
sometimes are nice.

### Icons
Icons for the different programs that offers them.


edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk),
uncomment the line with `#ICONS`

### Systray
System Tray. Blueman, screenkeys, obs, etc on your bar. You should use this if
you expect bar functionality like a Destop Enviroment.
e.g
- `nm-applet` will display NetworkManager systemtray
- `pasystray` will display PulseAudio systemtray


edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk)
uncomment the line with `#SYSTRAY`

### Tag Previews
Hover your mouse over a tag (you have had it viewed first) and look the contents of it.

edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk)
uncomment the line with `#TAG_PREVIEW`

## Patches
My little collection of patches, which aren't necessarily applied to this. You
can see them and 'unpatch' them if you want to.

Most of my modification aren't made a patch yet, but you can copy the url of
the commit and then download it.

e.g.
```
curl -sL https://github.com/explosion-mental/Dwm/commit/d8e4af59c570c4b8ef36ef8942c58ab89921900f
> dwm-default-layouts-pertag.diff
```
, but here are some that I converted into a
patch:
- shift-tools (shift windows,tags or both either with an active client or not)

## dwm.c
dwm.def.c is not needed anymore. I'm happy with what I got.

Some little TODOS:
- toggle border
- Make 'tag previews' more solid
- Make systray background more solid
- <s>systray toggleable with _#ifdef_ and _#endif_</s>
- depatch cfacts
- <s>make [hide vacant patch](https://dwm.suckless.org/patches/hide_vacant_tags/)
  toggleable</s>
- <s>a better way of handling layouts at startup</s>

Some wishing pains:
- integrate dwmblocks into dwm (prob not worth it)
- Make gaps an option(?)
- getenv("TERMINAL")

## Mayor improvements:
- xrdb patch finally working with pywal without restarting or recompiling (Mar
  01 2021)
- Layouts at startup (Apr 04 2021)
	* A better way of handling this (per monitor) (Aug 08 2021)
- Gaps per tag instead of per monitor (Apr 04 2021)
- Better way of handling Pywal colors (Jun 21 2021)
- Window icons (Jul 22 2021)
	* Icons on swallowed - unswallowed windows (Jul 23 2021)
- toggleable hide vacants tags (Jul 28 2021)
- System Tray (Aug 06 2021)
- Tag previews (Aug 10 2021)
