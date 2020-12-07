PREFIX=/usr/local
CFLAGS=
LDFLAGS=-lkvm
CC=cc

after: after.c
	$CC $LDFLAGS $CFLAGS -o after after.c

clean:V:
	rm after
