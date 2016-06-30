### If DEBUG is 1, we will check against all possible warnings.
### I.E.: "make all DEBUG=1"
###
### If CROSS is x86, we will cross-compile to target x86 (from AMD64)
### I.E.: "make all CROSS=x86"
###
### If OUT is curses,  we will use curses (ISO transliteration only)
### If OUT is cursesw, we will use cursesw (widechar support, able to support UTF-8 terminals). this is default.
### If OUT is text,    we will use pure text mode -- stdin/stdout/stderr
### If OUT is text1,   we will use pure text mode -- stdin/stdout (no stderr)
### If OUT is null,    we will use a dummy output (i.e., no output)
### I.E.: "make all OUT=text"

### Special code to support auto-crashing when a nickname is set to NULL
ifeq ($(CRASH_NO_NICK), 1)
	CFLAGS += -DCRASH_NO_NICK
endif

LDFLAGS += -lssl -lcrypto

### Special code for interface mode (ncurses(w) or pure text)
ifeq ($(OUT), curses)
	LDFLAGS += -lncurses
	IFACE=gotcurses.c
else ifeq ($(OUT), text)
	IFACE=gottext.c
else ifeq ($(OUT), text1)
	IFACE=gottext.c
	CFLAGS += -DNO_STD_ERR
else ifeq ($(OUT), null)
	IFACE=gotnull.c
else
	LDFLAGS += -lncursesw
	CFLAGS += -D_X_OPEN_SOURCE_EXTENDED
	IFACE=gotcurses.c
endif
###

# Executable name, main source file, sources files
EXECUTABLE=mchatclient
MSOURCES=main.c
SOURCES=conf.c parsehtml.c nicklist.c cookies.c entities.c network.c parser.c strfunctions.c ircserver.c CUtils/libcutils.c $(IFACE)
TEST_SOURCES=cookies-test.c iface-test.c parser-test.c
###

### Special code for cross compiling x86 from AMD64
ifeq ($(CROSS), x86)
	EXECUTABLE:=$(EXECUTABLE).x86
	ARCHFLAG="-m32"
endif
###

### CFLAGS and DEBUG
CFLAGS += -Wall -Wextra -fshort-enums

ifeq ($(DEBUG), 1)
	CFLAGS += -g -ggdb -O0 -Wformat -Winit-self -Wmissing-include-dirs -Wparentheses -Wswitch-default -Wswitch-enum -Wunused-parameter -Wuninitialized -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion
endif
###

### inferred vars
MOBJECTS=$(MSOURCES:.c=.o)
OBJECTS=$(SOURCES:.c=.o)
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLES=$(TEST_SOURCES:.c=)
###

### Active targets

# The active targets are not actual files
.PHONY : all clean mrproper mrpropre rebuild test install love

all: $(EXECUTABLE)

clean:
	@echo --- Cleaning all objects files...
	@rm -f *.o */*.o */*/*.o

mrpropre: mrproper

mrproper: clean
	@echo --- Cleaning all binary executables files...
	@rm $(EXECUTABLE) $(TEST_EXECUTABLES) 2>/dev/null || echo >/dev/null

rebuild: mrproper all

test: $(TEST_EXECUTABLES)

love:
	@echo "... not war ?"

### Dependencies
conf.o: conf.c conf.h display_interfaces.h
parsehtml.o: parsehtml.c parsehtml.h main.h entities.h parser.h display_interfaces.h nicklist.h strfunctions.h cookies.h
nicklist.o: nicklist.c nicklist.h display_interfaces.h main.h ircserver.h strfunctions.h
cookies.o: cookies.c cookies.h display_interfaces.h
network.o: network.c display_interfaces.h
main.o: main.c main.h conf.h network.h cookies.h parsehtml.h strfunctions.h display_interfaces.h commons.h ircserver.h nicklist.h
gotcurses.o: gotcurses.c display_interfaces.h commons.h strfunctions.h
gottext.o: gottext.c display_interfaces.h commons.h
gotnull.o: gotnull.c display_interfaces.h commons.h
parser.o: parser.c parser.h CUtils/libcutils.o
ircserver.o: ircserver.c ircserver.h display_interfaces.h nicklist.h strfunctions.h
CUtils/libcutils.o: CUtils/libcutils.c CUtils/libcutils.h CUtils/attribute.h CUtils/attribute.c CUtils/clist.h CUtils/clist.c CUtils/ini.h CUtils/ini.c CUtils/net.h CUtils/net.c CUtils/cstring.h CUtils/cstring.c

%.o: %.c
	@echo --- Compiling $@
	@#$(CC) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $^ -o $@
	@$(CC) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $(@:.o=.c) -o $@

$(EXECUTABLE): $(OBJECTS) $(MOBJECTS)
	@echo --- Linking final executable $(EXECUTABLE)...
	@$(CC) $(ARCHFLAG) $(OBJECTS) $(MOBJECTS) -o $@ $(LDFLAGS)
ifneq ($(DEBUG), 1)
	@echo "--- Stripping $(EXECUTABLE)"
	@strip $(EXECUTABLE)
endif

$(TEST_EXECUTABLES): $(TEST_SOURCES) $(TEST_OBJECTS) $(OBJECTS)
	@echo --- Linking test executable $@...
	@$(LD) $(ARCHFLAG) $@.o $(OBJECTS) -o $@ $(LDFLAGS)
