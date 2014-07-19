# awm - alpt's window manager
# AlpT (@freaknet.org)

# dwm - dynamic window manager
# © 2006-2007 Anselm R. Garbe, Sander van Dijk

include config.mk

SRC += awm.c
OBJ = ${SRC:.c=.o}

FSRC += fdock.c
FOBJ = ${FSRC:.c=.o}

all: options awm fdock

options:
	@echo awm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

awm: ${OBJ}
	@echo CC -o $@
	@${CC} -o $@ ${OBJ} ${LDFLAGS}

fdock: ${FOBJ}
	@echo CC -o $@
	@${CC} -o $@ ${FOBJ} ${LDFLAGS}
clean:
	@echo cleaning
	@rm -f fdock awm ${OBJ} awm-${VERSION}.tar.gz

dist: clean
	@echo creating dist tarball
	@mkdir -p awm-${VERSION}
	@cp -R LICENSE Makefile README config.h config.mk scripts \
		${SRC} awm-${VERSION}
	@mkdir awm-${VERSION}/scripts
	@cp -R scripts/* awm-${VERSION}/scripts
	@tar -cf awm-${VERSION}.tar awm-${VERSION}
	@gzip awm-${VERSION}.tar
	@rm -rf awm-${VERSION}

install: all
	@echo installing executable file to ${DESTDIR}${PREFIX}/bin
	@mkdir -p ${DESTDIR}${PREFIX}/bin
	@cp -f awm ${DESTDIR}${PREFIX}/bin
	@chmod 755 ${DESTDIR}${PREFIX}/bin/awm
	@echo installing manual page to ${DESTDIR}${MANPREFIX}/man1
	@mkdir -p ${DESTDIR}${MANPREFIX}/man1

uninstall:
	@echo removing executable file from ${DESTDIR}${PREFIX}/bin
	@rm -f ${DESTDIR}${PREFIX}/bin/awm
	@echo removing manual page from ${DESTDIR}${MANPREFIX}/man1

.PHONY: all options clean dist install uninstall
