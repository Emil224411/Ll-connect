# paths
PREFIX = /usr/local

# libs
SDLLIBS = -lSDL2 -lSDL2_ttf

#CFLAGS   = -g -std=c99 -pedantic -Wall -O0
CFLAGS   = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os
LDFLAGS  = ${SDLLIBS}

# compiler and linker
CC = gcc

SRC = main.c controller.c ui.c
OBJ = ${SRC:.c=.o}

all: Ll-connect

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: controller.h ui.h

Ll-connect: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

clean:
	rm -f ${OBJ} Ll-connect

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	mkdir -p ${DESTDIR}${PREFIX}/share/fonts/
	cp -f ./HackNerdFont-Regular.ttf ${DESTDIR}${PREFIX}/share/fonts/
	cp -f Ll-connect ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/Ll-connect

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/Ll-connect ${DESTDIR}${PREFIX}/share/fonts/HackNerdFont-Regular.ttf

.PHONY: all clean install uninstall
