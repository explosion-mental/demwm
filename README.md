# Dwm
My build of dwm

![Image](PersonalBuild/ss.png "dwm")

## Patches
You can see them and 'unpatch' them if you want to.


_Little note:_ I don't use cfacts patch.

## dwm.c
dwm.def.c is not needed anymore. I'm happy with what I got.

Some little TODOS:
- systray toggleable with _#ifdef_ and _#endif_
- getenv("TERMINAL")
- DE-patch cfacts

## Mayor improvements:
- xrdb patch finally working with pywal without restarting or recompiling (Mar
  01 2021)
- Layouts at startup (Apr 04 2021)
- Gaps per tag instead of per monitor (Apr 04 2021)
- Better way of handling Pywal colors (Jun 21 2021)
- Window icons (Jul 22 2021)
