# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2

# OpenBSD (uncomment)
#FREETYPEINC = ${X11INC}/freetype2
#kvm to swallow on bsd
#KVMLIB = -lkvm

# debug
#DEBUG = -DDEBUG

# Icons, comment if you don't want it
ICONS = -DICONS

# Tag previews, comment if you don't want it
#TAG_PREVIEW = -DTAG_PREVIEW

# imlib2, dependency of ICONS and TAG_PREVIEW
IMLIB2LIBS = -lImlib2

# Systray, comment if you don't want it
SYSTRAY = -DSYSTRAY

# XCB libs, required by swallow
XCBLIBS = -lX11-xcb -lxcb -lxcb-res

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS} -lXrender ${XCBLIBS} ${KVMLIB} ${IMLIB2LIBS}

# flags
O_BASIC  = -march=native -Ofast -flto=auto
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${SRCVERSION}\" ${XINERAMAFLAGS} ${ICONS} ${SYSTRAY} ${TAG_PREVIEW} ${TAGNUM} ${DEBUG}
CFLAGS   = -std=c99 -pedantic -Wall -Wno-unused-function -Wno-unused-variable ${O_BASIC} ${INCS} ${CPPFLAGS}
#CFLAGS  = -g -std=c99 -pedantic -Wall -Wextra -Wno-unused-function -Og ${INCS} ${CPPFLAGS} -flto -fsanitize=address,undefined,leak
LDFLAGS  = ${LIBS}
#LDFLAGS = -g ${LIBS} ${CFLAGS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = cc
