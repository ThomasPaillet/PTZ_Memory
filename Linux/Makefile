CC = cc
CFLAGS = -c -Wall -D_REENTRANT `pkg-config --cflags gtk+-3.0`

gresources.o: gresources.c gresources.h
	$(CC) $(CFLAGS) $<

gresources.c: gresources.xml $(shell glib-compile-resources --generate-dependencies --sourcedir=../ gresources.xml)
	glib-compile-resources --generate-source --sourcedir=../ gresources.xml

gresources.h: gresources.xml $(shell glib-compile-resources --generate-dependencies --sourcedir=../ gresources.xml)
	glib-compile-resources --generate-header --sourcedir=../ gresources.xml

clean:
	rm -f gresources.h
	rm -f gresources.c
	rm -f *.o
