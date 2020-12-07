PREFIX=/usr/local
CFLAGS=
LDFLAGS=-lkvm
CC=cc

after: after.c
	$CC $LDFLAGS $CFLAGS -o after after.c

install:V:
	install -D -m 0755 after ${PREFIX}/bin/after

clean:V:
	rm after
