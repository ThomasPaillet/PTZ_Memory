CC = cc
CFLAGS = -c -Wall -D_REENTRANT `pkg-config --cflags gtk+-3.0`
#CFLAGS = -c -mwin32 -Wall -D_REENTRANT `pkg-config --cflags gtk+-3.0`
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

cameras_set.o: main_window.h memory.h protocol.h settings.h

control_window.o: cameras_set.h controller.h main_window.h memory.h protocol.h ptz.h sw_p_08.h tally.h trackball.h

controller.o: protocol.h

error.o: cameras_set.h memory.h ptz.h

interface.o: cameras_set.h main_window.h memory.h ptz.h settings.h

main_window.o: cameras_set.h control_window.h controller.h error.h interface.h memory.h protocol.h ptz.h settings.h sw_p_08.h tally.h trackball.h update_notification.h

memory.o: cameras_set.h controller.h interface.h main_window.h memory.h protocol.h ptz.h settings.h

protocol.o: error.h update_notification.h

ptz.o: control_window.h controller.h error.h interface.h main_window.h memory.h protocol.h settings.h sw_p_08.h tally.h

settings.o: cameras_set.h control_window.h controller.h interface.h main_window.h memory.h protocol.h ptz.h sw_p_08.h tally.h trackball.h update_notification.h

sw_p_08.o: cameras_set.h control_window.h main_window.h memory.h protocol.h ptz.h settings.h

tally.o: cameras_set.h control_window.h interface.h memory.h protocol.h ptz.h

trackball.o: main_window.h settings.h

update_notification.o: cameras_set.h control_window.h error.h memory.h protocol.h ptz.h

%.o: %.c ptz.h %.h
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

