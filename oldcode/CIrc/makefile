### This makefile can be customized with the GTK and DEBUG environment variable.
###
### If GTK is GTK2, we will compile and link against gtk+ 2.0.
### If GTK is GTK3, we will compile and link against gtk+ 3.0.
### I.E.: "GTK=GTK2 make all"
###
### If DEBUG is 1, we will check against all possible warnings.
### I.E.: "DEBUG=1 make all"

### Options (Gtk+ and GtkGLExt, pthread)
O_GTK=0
O_OPENGL=0
#LDFLAGS += -lpthread
###

### CUtils:
CUtils=../CUtils/libcutils.o
#CUtils=../CUtils/clist.o ../CUtils/cstring.o ../CUtils/net.o
#CUtils=../CUtils/libcutils.a
#CUtils=../CUtils/libcutils.so.1.0.0

### Definitions
CFLAGS += -Wall -Wextra

MSOURCES=irc-send.c
MOBJECTS=$(MSOURCES:.c=.o)
EXECUTABLE=$(MSOURCES:.c=)

SOURCES=irc_client.c irc_server.c irc_user.c irc_chan.c
OBJECTS=$(SOURCES:.c=.o) $(CUtils)

TEST_SOURCES=
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLES=$(TEST_SOURCES:.c=)

ifeq ($(DEBUG), 1)
	CFLAGS += -g -ggdb -O0 -fshort-enums -Wformat -Winit-self -Wmissing-include-dirs -Wparentheses -Wswitch-default -Wswitch-enum -Wunused-parameter -Wuninitialized -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion
endif

### GTK flags
ifeq ($(O_GTK), 1)
	ifeq ($(GTK), GTK3)
		GTKFLAGS += -export-dynamic `pkg-config --libs --cflags gtk+-3.0`
	else
		GTKFLAGS += -export-dynamic `pkg-config --libs --cflags gtk+-2.0`
	endif
	CFLAGS += $(GTKFLAGS)
	LDFLAGS += $(GTKFLAGS)
endif
###

### GtkGlExt flags
ifeq ($(O_OPENGL), 1)
	GTKGLEXT += `pkg-config --cflags --libs gtkglext-1.0`
	CFLAGS += $(GTKGLEXT)
	LDFLAGS += $(GTKGLEXT)
endif
###

### Active targets

# The active targets are not actual files
.PHONY : all clean mrproper mrpropre rebuild test install love

all: $(EXECUTABLE)
	@rm $(MOBJECTS)

clean:
	@echo --- Cleaning the build directory...
	@rm -f *.o */*.o */*/*.o

mrpropre: mrproper

mrproper: clean
	@echo --- *CLEANING* the build directory '(EXEs included)'...
	@rm $(EXECUTABLE) $(TEST_EXECUTABLES) 2>/dev/null || echo >/dev/null

rebuild: mrproper all

test: $(TEST_EXECUTABLES)

install: all
	@echo --- Installing the programme into /usr/bin...
	cp $(EXECUTABLE) /usr/bin

love:
	@echo "... not war ?"

### Dependencies

%.o: %.c
	@echo --- $@
	@$(CC) $(SHARED) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $^ -o $@

$(EXECUTABLE): $(OBJECTS) $(MOBJECTS)
	@echo === Linking final binary $(EXECUTABLE)...
	@$(CC) $(SHARED) $(ARCHFLAG) $(OBJECTS) $(MOBJECTS) -o $@ $(LDFLAGS)

$(TEST_EXECUTABLES): $(TEST_OBJECTS) $(OBJECTS)
	@echo === Linking test executable $@...
	@$(CC) $(ARCHFLAG) $@.o $(OBJECTS) -o $@ $(LDFLAGS)
