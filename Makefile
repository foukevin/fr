ifndef config
  config=debug
endif

ifndef CC
  CC = gcc
endif

ifndef AR
  AR = ar
endif

ifndef platform
  platform = $(shell uname -s)
endif
export platform

INCLUDES += -I.
CFLAGS = -Wall -Wextra `freetype-config --cflags` `libpng-config --cflags`
LIBS += `freetype-config --libs` `libpng-config --libs`

ifeq ($(config),debug)
  OBJDIR = obj/debug
  TARGETDIR = bin
  TARGET = $(TARGETDIR)/fr-debug
  CFLAGS += -std=c99 -g -Wall -Wextra -msse $(DEFINES) $(INCLUDES)
  LDFLAGS +=
  LINKCMD = $(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

ifeq ($(config),release)
  OBJDIR = obj/release
  TARGETDIR = bin
  TARGET = $(TARGETDIR)/fr
  CFLAGS += -std=c99 -O3 -Wall -Wextra -msse $(DEFINES) $(INCLUDES)
  LDFLAGS +=
  LINKCMD = $(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

OBJECTS := \
	$(OBJDIR)/error.o \
	$(OBJDIR)/bitmap.o \
	$(OBJDIR)/options.o \
	$(OBJDIR)/fr.o \
	$(OBJDIR)/main.o

.PHONY: clean

all: $(TARGETDIR) $(OBJDIR) $(TARGET)

$(TARGET): $(OBJECTS)
	$(LINKCMD)

$(TARGETDIR):
	mkdir -p $(TARGETDIR)

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -f  $(TARGET)
	rm -rf $(OBJDIR)

$(OBJDIR)/error.o: error.c error.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/bitmap.o: bitmap.c bitmap.h error.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/options.o: options.c fr.h error.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/fr.o: fr.c fr.h bitmap.h raster_font.h error.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/main.o: main.c fr.h error.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
