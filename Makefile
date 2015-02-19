ifndef config
  config=debug
endif

ifndef CC
  CC = gcc
endif

ifndef AR
  AR = ar
endif

ifeq ($(config),debug)
  OBJDIR = obj/debug
  TARGETDIR = bin
  TARGET = $(TARGETDIR)/fr-debug
  INCLUDES += -I. -I/usr/include/freetype2
  CFLAGS += -std=c99 -g -Wall -Wextra -msse $(DEFINES) $(INCLUDES)
  LDFLAGS +=
  LIBS += -lfreetype -lpng
  LINKCMD = $(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

ifeq ($(config),release)
  OBJDIR = obj/release
  TARGETDIR = bin
  TARGET = $(TARGETDIR)/fr
  INCLUDES += -I. -I/usr/include/freetype2
  CFLAGS += -std=c99 -O3 -Wall -Wextra -msse $(DEFINES) $(INCLUDES)
  LDFLAGS +=
  LIBS += -lfreetype -lpng
  LINKCMD = $(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS) $(LIBS)
endif

OBJECTS := \
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

$(OBJDIR)/bitmap.o: bitmap.c bitmap.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/options.o: options.c fr.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/fr.o: fr.c fr.h bitmap.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
$(OBJDIR)/main.o: main.c fr.h
	 $(CC) $(CFLAGS) -o "$@" -c "$<"
