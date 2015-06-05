all::

### --- CONFIGURATION SECTION ---
# CFLAGS and LDFLAGS are for the users to override from the command line.

CFLAGS = -g -O2 -Wall
LDFLAGS =
ALL_CFLAGS = $(CPPFLAGS) $(CFLAGS)
ALL_LDFLAGS = $(LDFLAGS)
STRIP ?= strip

# Variables

CC = cc
AR = ar
RM = rm -f
INSTALL = install
FREETYPE_CFLAGS = $(shell freetype-config --cflags)
FREETYPE_LIBS = $(shell freetype-config --libs)
LIBPNG_CFLAGS = $(shell libpng-config --cflags)
LIBPNG_LIBS = $(shell libpng-config --libs)

### --- END CONFIGURATION SECTION ---

BASIC_CFLAGS = -I.
BASIC_LDFLAGS =

# Guard against environment variables
PROGRAM_OBJS =
PROGRAM =

PROGRAM_OBJS += error.o
PROGRAM_OBJS += bitmap.o
PROGRAM_OBJS += options.o

# Binary suffix, set to .exe for Windows builds
X =

PROGRAM = fr$X

OBJECTS = $(PROGRAM_OBJS) fr.o

# Libraries

EXTLIBS =

BASIC_CFLAGS += $(FREETYPE_CFLAGS) $(LIBPNG_CFLAGS)
EXTLIBS += $(FREETYPE_LIBS) $(LIBPNG_LIBS)

LIBS = $(EXTLIBS)

### Cleaning rules

.PHONY: clean

ALL_CFLAGS += $(BASIC_CFLAGS)
ALL_LDFLAGS += $(BASIC_LDFLAGS)

clean:
	$(RM) $(OBJECTS)

### Build rules

.PHONY: all strip

all:: $(PROGRAM)

strip: $(PROGRAM)
	$(STRIP) $(STRIP_OPTS) $^

.SUFFIXES:

$(OBJECTS): %.o: %.c
	$(CC) -o $*.o -c $(ALL_CFLAGS) $(EXTRA_CPPFLAGS) $<

$(PROGRAM): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(ALL_LDFLAGS) $(LIBS)
