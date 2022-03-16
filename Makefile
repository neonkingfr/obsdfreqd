all:
	clang main.c -o obsdfreqd -lm

clean:
	rm obsdfreqd
