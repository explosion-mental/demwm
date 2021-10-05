# Dwm
My build of dwm

![Image](rice.gif "dwm")

# Requirements
- In order to build dwm you need the Xlib header files.
- Xcb header files for the `swallow patch`.
## Optional
- [pywal](https://github.com/dylanaraps/pywal)
	* If not you will have to define (instead of declaring) the colors
	  variable
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
- Vanitygaps (some of the specific increase/decrease on horizontal or vertical
  functions are commented out since I use only the general one.)
- [pywal](https://github.com/dylanaraps/pywal) color syncronization by default
- More layouts
- Custom bar height
- Different color schemes for Title, Status and Layout (added 4 more shemes on
  top of the other mentioned: Urgent - Notify - Indicator on/of)
- Transparency on the bar
- A beautiful looking config.h file (at least for my taste :)
- Toggleable 'hide-vacant' patch


see: [layouts.c](https://github.com/explosion-mental/Dwm/blob/main/layouts.c)

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
Hover your mouse over a tag (you have had it viewed first) and look the contents of it.

edit [config.mk](https://github.com/explosion-mental/Dwm/blob/main/config.mk)
uncomment the line with `#TAG_PREVIEW`

# dwm_random_wall
This is a little script that changes the wallpaper to a random one and uses
(and requires) **pywal**. It also has the `-o` option that runs a `postrun`
script (you can safely delete it), which syncs zathura and dunst to the pywal
coloscheme. By default my dwm build syncs all colors to pywal colors, this only
seems "_useful_" if you wish to change wallpapers (and colorscheme) at runtime.
You should **edit** the variable `img_path` to your actual path of wallpapers
(if you use pywal then you have wallpapers, else it would be rather pointless).

Remember, the script is copied and removed by the makefile (`make install` and
`make unistall`).

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
- Make _systray_ background the same color as the bar
- depatch cfacts(?)
- <s>Make _tag previews_ more solid</s> and prevent from being shown in the preview of the preview...
- <s>systray toggleable with _#ifdef_ and _#endif_</s>
- <s>make [hide vacant patch](https://dwm.suckless.org/patches/hide_vacant_tags/)
  toggleable</s>
- <s>a better way of handling layouts at startup</s>

## Some wishing pains:
- change `dwm_random_wall` script to an actual function
- pertag cursor position. Save the state of the cursor position between tags.
- integrate dwmblocks into dwm (prob not worth it)
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
- toggleable hide vacants tags (Jul 28 2021)
- System Tray (Aug 06 2021)
- Tag previews (Aug 10 2021)
