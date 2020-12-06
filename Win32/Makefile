CC = cc
CFLAGS = -c -Wno-deprecated-declarations `pkg-config --cflags gtk+-3.0`

PNG = $(wildcard *.png)
PIXBUF = $(PNG:.png=.h)

.SUFFIXES: .png .h

.png.h:
	gdk-pixbuf-csource --name=pixbuf_inline_$* --raw $< > $@

pixbufs.o: pixbufs.c $(PIXBUF) $(PNG)
	$(CC) $(CFLAGS) $<

Win32.o: Win32.c
	$(CC) -c -Wall $<

clean:
	rm -f *.h
	rm -f *.o
