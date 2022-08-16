# demwm - dynamic explostion-mental's window manager
# See LICENSE file for copyright and license details.

include config.mk

SRC = drw.c demwm.c util.c
OBJ = ${SRC:.c=.o}

all: options demwm

options:
	@echo demwm build options:
	@echo "FEATURES = ${ICONS} ${SYSTRAY} ${TAG_PREVIEWS} ${XINERAMAFLAGS} ${DEBUG}"
	@echo
	@echo "CFLAGS   = ${CFLAGS}"
	@echo
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo
	@echo "CC       = ${CC}"
	@echo

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

demwm: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f demwm ${OBJ} demwm-${VERSION}.tar.gz

dist: clean
	mkdir -p demwm-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
		demwm.1 drw.h util.h ${SRC} demwm.png transient.c demwm-${VERSION}
	tar -cf demwm-${VERSION}.tar demwm-${VERSION}
	gzip demwm-${VERSION}.tar
	rm -rf demwm-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f demwm demwm_random_wall ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/demwm
	chmod 755 ${DESTDIR}${PREFIX}/bin/demwm_random_wall
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < demwm.1 > ${DESTDIR}${MANPREFIX}/man1/demwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/demwm.1

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/demwm\
		$(DESTDIR)$(PREFIX)/bin/demwm_random_wall\
		${DESTDIR}${MANPREFIX}/man1/demwm.1

.PHONY: all options clean dist install uninstall
