.PHONY: clean

HOSTNAME := $(shell hostname)
SANDMAN := $(findstring sandman,$(HOSTNAME))
BROADY2 := $(findstring broady2,$(HOSTNAME))
BROADY3 := $(findstring broady3,$(HOSTNAME))
REMOTE_MACHINE := $(or $(SANDMAN),$(BROADY2),$(BROADY3))


CFLAGS =
LDFLAGS =

ifdef REMOTE_MACHINE
    # CC = /various/common_tools/gcc-6.4.0/bin/gcc
    # CC = /various/dgal/gcc-8.3.0/bin/gcc
    CC = ${HOME}/Documents/gcc_current/bin/gcc

    library = lib
    CFLAGS = -DREMOTE_MACHINE -I "$(library)"
else
    # CC = gcc
    CC = ${HOME}/Documents/gcc_current/bin/gcc
    # CC = /home/jim/Documents/gcc/gcc-8.3.0/bin/g++
    library = lib
    CFLAGS = -I "$(library)"
endif


obj_path = ./obj


GRAIN_SIZE = 1
# GRAIN_SIZE = 64
# GRAIN_SIZE = 1024


CFLAGS += -Wall -Wextra -fopenmp
CFLAGS += -Wno-unused
# CFLAGS += -g3 -fno-omit-frame-pointer
CFLAGS += -O3
# CFLAGS += -DDEBUG

CFLAGS += -DGRAIN_SIZE=$(GRAIN_SIZE)


LDFLAGS +=


all: bfs.exe


CSRC = env.c gomp_polymer.c loop_partitioners.c io.c frontier.c bfs.c
HSRC = env.h gomp_polymer.h loop_partitioners.h io.h frontier.h bfs.h

COBJ = $(patsubst %,$(obj_path)/%,$(CSRC:.c=.o))



bfs.exe: $(COBJ)
	$(CC) $(CFLAGS) $(COBJ) -o $@ $(LDFLAGS)


$(obj_path)/%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@


clean: 
	$(RM) *.exe $(obj_path)/*.o a.out

