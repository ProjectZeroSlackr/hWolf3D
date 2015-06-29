ifdef IPOD
CC = arm-uclinux-elf-gcc
else
CC = gcc
endif
#CC=/opt/intel/compiler60/ia32/bin/icc

#CFLAGS = -g -Wall
CFLAGS = -g -O2 -Wall
#CFLAGS = -Wall -O6 -fomit-frame-pointer -ffast-math -funroll-loops -march=pentiumpro
#CFLAGS = -g -Wall -W -pedantic -std=c99
#CFLAGS = -Os -Wall -pedantic
#CFLAGS = -Os -Wall -fomit-frame-pointer -ffast-math -march=pentiumpro
#CFLAGS=-O3 -xiMK -tpp6 -c99 -wp_ipo -g
OBJS = objs.o misc.o id_ca.o id_vh.o id_us.o \
	wl_act1.o wl_act2.o wl_act3.o wl_agent.o wl_game.o \
	wl_inter.o wl_menu.o wl_play.o wl_state.o wl_text.o wl_main.o \
	wl_debug.o vi_comm.o sd_comm.o
ROBJS = wl_draw.o
SOBJS = $(OBJS) $(ROBJS) vi_svga.o
XOBJS = $(OBJS) $(ROBJS) vi_xlib.o
DOBJS = $(OBJS) $(ROBJS) vi_sdl.o
HOBJS = $(OBJS) $(ROBJS) vi_hotdog.o

#LDLIBS = -lm -wp_ipo
LDLIBS = -lm

# no sound
OBJS += sd_null.o
# sound using OSS
#OBJS += sd_oss.o fmopl.o
#CFLAGS += -D_REENTRANT
#LDLIBS += -lpthread

ifdef IPOD
CFLAGS += -I../hotdog -DIPOD
else
CFLAGS += $(shell sdl-config --cflags)
endif

ifdef WOLF
ifdef SHARE
CFLAGS += -DWMODE=0
else
CFLAGS += -DWMODE=1
endif
else
ifdef SPEAR
ifdef SHARE
CFLAGS += -DWMODE=2
else
CFLAGS += -DWMODE=3
endif
endif
endif

SLDLIBS = $(LDLIBS) -lvga
XLDLIBS = $(LDLIBS) -L/usr/X11R6/lib -lX11 -lXext
DLDLIBS = $(LDLIBS) $(shell sdl-config --libs)
HLDLIBS = $(LDLIBS) -L../hotdog/ipod -lhotdog -elf2flt

NASM = nasm

.SUFFIXES: .asm

#all:	swolf3d xwolf3d sdlwolf3d
ifdef IPOD
all:	hwolf3d
else
all:	sdlwolf3d
endif

$(SOBJS): version.h id_heads.h wl_def.h
$(XOBJS): version.h id_heads.h wl_def.h
$(DOBJS): version.h id_heads.h wl_def.h

.asm.o:
	$(NASM) -f elf -o $@ $<

swolf3d: $(SOBJS)
	$(CC) -o swolf3d $(SOBJS) $(SLDLIBS) 

xwolf3d: $(XOBJS)
	$(CC) -o xwolf3d $(XOBJS) $(XLDLIBS)

sdlwolf3d: $(DOBJS)
	$(CC) -o sdlwolf3d $(DOBJS) $(DLDLIBS)

hwolf3d:  $(HOBJS)
	$(CC) -o hwolf3d $(HOBJS) $(HLDLIBS)

clean:
	rm -rf swolf3d xwolf3d sdlwolf3d hwolf3d *.gdb *.o *.il

distclean: clean
	rm -rf *~ DEADJOE

forced: distclean all
	rm -rf *.o
