PREFIX=/usr/local
CFLAGS=
LDFLAGS=-lkvm
CC=cc

after: after.c
	$CC $LDFLAGS $CFLAGS -o after after.c

install:V:
	install -c -s -m 0755 after ${PREFIX}/bin/after

installman:V:
	install -c after.1 ${PREFIX}/man/man1/after.1

clean:V:
	rm after

readme:V:
	mandoc -T ascii | col -bx > README
