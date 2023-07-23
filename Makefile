CC = cc
LD = ld

CFLAGS = -O3 -Wall -Iinclude
LIBS = -lm

all: init ps2save-extract icon2png

init:
	@if test ! -d obj/ ; then mkdir obj ; fi

ps2save-extract: obj/main.o obj/armax.o obj/xps.o obj/cbs.o obj/psu.o obj/mcs.o obj/lzari.o obj/miniz_tinfl.o
	$(CC) $(LIBS) -o ps2save-extract obj/main.o obj/armax.o obj/xps.o obj/cbs.o obj/psu.o obj/mcs.o obj/lzari.o obj/miniz_tinfl.o

icon2png:
	$(CC) $(CFLAGS) $(LIBS) -o icon2png src/icon2png.c

obj/main.o:
	$(CC) $(CFLAGS) -c -o obj/main.o src/main.c

obj/armax.o:
	$(CC) $(CFLAGS) -c -o obj/armax.o src/armax.c

obj/xps.o:
	$(CC) $(CFLAGS) -c -o obj/xps.o src/xps.c

obj/cbs.o:
	$(CC) $(CFLAGS) -c -o obj/cbs.o src/cbs.c

obj/psu.o:
	$(CC) $(CFLAGS) -c -o obj/psu.o src/psu.c

obj/mcs.o:
	$(CC) $(CFLAGS) -c -o obj/mcs.o src/mcs.c

obj/lzari.o:
	$(CC) $(CFLAGS) -c -o obj/lzari.o src/lzari.c

obj/miniz_tinfl.o:
	$(CC) $(CFLAGS) -c -o obj/miniz_tinfl.o src/miniz_tinfl.c

clean:
	@rm -Rf obj/ ps2save-extract icon2png
