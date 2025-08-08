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

cameras_set.o: main_window.h memory.h protocol.h settings.h ultimatte.h

control_window.o: cameras_set.h controller.h free_d.h main_window.h memory.h protocol.h ptz.h sw_p_08.h tally.h trackball.h

controller.o: logging.h network_header.h protocol.h

error.o: cameras_set.h logging.h memory.h network_header.h ptz.h

free_d.o: cameras_set.h logging.h network_header.h protocol.h

interface.o: cameras_set.h main_window.h memory.h ptz.h settings.h

logging.o: f_sync.h tally.h

main_window.o: cameras_set.h control_window.h controller.h free_d.h interface.h logging.h memory.h protocol.h ptz.h settings.h sw_p_08.h tally.h trackball.h ultimatte.h update_notification.h

memory.o: cameras_set.h controller.h interface.h logging.h main_window.h memory.h protocol.h ptz.h settings.h sw_p_08.h ultimatte.h

protocol.o: error.h logging.h network_header.h update_notification.h

ptz.o: cameras_set.h control_window.h controller.h error.h free_d.h interface.h main_window.h memory.h protocol.h settings.h sw_p_08.h tally.h ultimatte.h

settings.o: cameras_set.h control_window.h controller.h error.h f_sync.h free_d.h interface.h logging.h main_window.h memory.h protocol.h ptz.h sw_p_08.h tally.h trackball.h ultimatte.h update_notification.h

sw_p_08.o: cameras_set.h control_window.h logging.h main_window.h memory.h network_header.h protocol.h ptz.h settings.h

tally.o: cameras_set.h control_window.h interface.h logging.h memory.h network_header.h protocol.h ptz.h ultimatte.h

trackball.o: main_window.h settings.h

ultimatte.o: logging.h network_header.h

update_notification.o: cameras_set.h control_window.h error.h free_d.h memory.h network_header.h protocol.h ptz.h

%.o: %.c ptz.h %.h
	$(CC) $(CFLAGS) $<

Win32/pixbufs.o: $(PNG) $(PIXBUF)
	@(cd Win32 && $(MAKE) pixbufs.o)

Win32/Win32.o: Win32/Win32.c
	@(cd Win32 && $(MAKE) Win32.o)

$(PROG).exe: $(OBJS) Win32/pixbufs.o Win32/Win32.o
	$(CC) -o $@ $^ $(LDFLAGS) -lwsock32

install-win64: $(PROG).exe
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

