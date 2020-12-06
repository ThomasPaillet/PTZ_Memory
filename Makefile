CC = cc
CFLAGS = -c -Wall -D_REENTRANT `pkg-config --cflags gtk+-3.0`
LDFLAGS = -lm `pkg-config --libs gtk+-3.0` -ljpeg

PROG = PTZ-Memory
BINDIR = $(PROG)

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

PNG = $(wildcard Win32/*.png)
PIXBUF = $(wildcard Win32/*.h)

$(PROG): Linux/gresources.o $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

Linux/gresources.o: Linux/gresources.xml
	@(cd Linux && $(MAKE) gresources.o)

ptz.o: ptz.h

cameras_set.o: ptz.h

control_window.o: ptz.h

settings.o: ptz.h

protocol.o: ptz.h

controller.o: ptz.h

update_notification.o: ptz.h

sw_p_08.o: ptz.h

tally.o: ptz.h

error.o: ptz.h

main_window.o: ptz.h

.c.o:
	$(CC) $(CFLAGS) $<

Win32/pixbufs.o: $(PNG) $(PIXBUF)
	@(cd Win32 && $(MAKE) pixbufs.o)

Win32/Win32.o: Win32/Win32.c
	@(cd Win32 && $(MAKE) Win32.o)

$(PROG).exe: $(OBJS) Win32/pixbufs.o Win32/Win32.o
	$(CC) -o $@ $^ $(LDFLAGS) -lwsock32

install-win32: $(PROG).exe
	strip --strip-unneeded $(PROG).exe
	@mkdir -p c:/$(BINDIR)/resources
	cp -Ru resources/* c:/$(BINDIR)/resources
	cp -u $(PROG).exe $(PROG).ico COPYING c:/$(BINDIR)
	@$(SHELL) Win32/install-gtk-dll $(BINDIR)

clean:
	rm -f *.o

clean-all:
	@(cd Linux && $(MAKE) clean)
	@(cd Win32 && $(MAKE) clean)
	rm -f *.o
	rm -f $(PROG)

