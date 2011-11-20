all: nyarc

nyarc: nyarc.c config.h
	cc -std=c99 -o nyarc nyarc.c
clean:
	rm -f nyarc

