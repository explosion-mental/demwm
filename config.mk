# dwm version
VERSION = 6.2

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

# Icons, comment if you don't want it
ICONS = -DICONS
IMLIB2LIBS = -lImlib2

# Systray, comment if you don't want it
SYSTRAY = -DSYSTRAY

# Tag previews, comment if you don't want it
#TAG_PREVIEW = -DTAG_PREVIEW
#TAGNUM = -DTAGNUM=9	#number of tags you got

# XCB libs, required by swallow
XCBLIBS = -lX11-xcb -lxcb -lxcb-res

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS} -lXrender ${XCBLIBS} ${KVMLIB} ${IMLIB2LIBS}

# flags
CPPFLAGS = -D_DEFAULT_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS} ${ICONS} ${SYSTRAY} ${TAG_PREVIEW} ${TAGNUM}
#CFLAGS   = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
CFLAGS   = -march=native -mtune=native -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Wno-unused-function -flto -O3 ${INCS} ${CPPFLAGS}
#CFLAGS   = -std=c99 -O3 -march=native -pedantic -Wall -Wno-deprecated-declarations -Os ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = gcc
