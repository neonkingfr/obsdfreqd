PREFIX=/usr/local

obsdfreqd:
	clang main.c -o obsdfreqd -lm

all: obsdfreqd

clean:
	rm obsdfreqd

install: obsdfreqd
	install -o root -g wheel -m 555 obsdfreqd ${PREFIX}/sbin/obsdfreqd
	install -o root -g wheel -m 555 obsdfreqd.rc /etc/rc.d/obsdfreqd
	install -o root -g wheel -m 444 obsdfreqd.1 ${PREFIX}/man/man1/obsdfreqd.1
