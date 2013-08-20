### Env Options:
### 	IRC=1			enable IRC-server support
### 	CROSS=x86		compile the x86 version under an AMD64 machine
### 	DEBUG=1			debug build
###
### Output modules:
### 	cursesw (default):	cursesw (wide char, UTF-8) interface
### 	curses:			curses interface (ISO)
### 	text:			output plain text on stdout, stderr
### 	text1:			output plain text on stdout
### 	none:			do not output anything
###
### Exemples:
### 	make # will compile the program with default values
### 	make IRC=1 DEBUG=1 none # will compile the program with IRC support
### 				# and no direct output
###

### Options
CFLAGS += -Wall -Wextra -fshort-enums

### Special code for cross compiling x86 from AMD64
#if 0 == 1
ifeq ($(CROSS), x86)
#fi
#if $(CROSS) == x86
	ARCHFLAG="-m32"
#fi
#if 0 == 1
endif
#fi

### DEBUG
#if 0 == 1
ifeq ($(DEBUG), 1)
#fi
#if $(DEBUG) == 1
	CFLAGS += -g -ggdb -O0 -Wformat -Winit-self -Wmissing-include-dirs -Wparentheses -Wswitch-default -Wswitch-enum -Wunused-parameter -Wuninitialized -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion -DDEBUG
#fi
#if 0 == 1
endif
#fi


### IRC built-in server support (default is off)
#if 0 == 1
ifeq ($(IRC), 1)
#fi
#if $(IRC) == 1
	CIrc = mccirc.o CIrc/libcirc.o
#if 0 == 1
else
#fi
#else
	CIrc = mccirc_fake.o
#fi
#if 0 == 1
endif
#fi

# The active targets are not actual files
.PHONY : all clean mrproper mrprorebuild test install love \
	 irc debug curses cursesw text text1 none x86

### default options
all: cursesw

### out interface (default is cursesw)
cursesw: LDFLAGS += -lncursesw
cursesw: CFLAGS += -D_X_OPEN_SOURCE_EXTENDED
cursesw: IFACE = gotcurses.c
cursesw: gotcurses.o mchatclient

curses: LDFLAGS +=-lncurses
curses: IFACE = gotcurses.c
curses: gotcurses.o mchatclient 

text: IFACE = gottext.c
text: gottext.o mchatclient 

text1: IFACE = gottext.c
text1: CFLAGS += -DNO_STD_ERR
text1: gottext.o mchatclient 

none: IFACE = gotnull.c
none: gotnull.o mchatclient 

### Active targets
clean:
	@echo --- Cleaning all objects files...
	@rm -f *.o */*.o */*/*.o

mrpropre: mrproper

mrproper: clean
	@echo --- Cleaning all binary executables files...
	@rm -f mchatclient mchatclient.x86
	@rm -f iface-test cookies-test parser-test

### note: rebuild will always rebuild the default interface
rebuild: mrproper all

test: cookies-test iface-test parser-test 

love:
	@echo "... not war ?"

### Test dependencies
cookies-test: cookies-test.o gottext.o cookies.o CUtils/libcutils.o
	@echo --- Linking test executable $@...
	@$(CC) cookies-test.o gottext.o cookies.o CUtils/libcutils.o -o $@ $(LDFLAGS)

iface-test: iface-test.o display_interfaces.h CUtils/libcutils.o gottext.o
	@echo --- Linking test executable $@...
	@$(CC) iface-test.o CUtils/libcutils.o gottext.o -o $@ $(LDFLAGS)

parser-test: parser-test.o parser.o CUtils/libcutils.o
	@echo --- Linking test executable $@...
	@$(CC) parser-test.o parser.o CUtils/libcutils.o -o $@ $(LDFLAGS)


### Main dependencies
OBJS = main.o conf.o parsehtml.o nicklist.o cookies.o entities.o network.o parser.o strfunctions.o CUtils/libcutils.o
	
mchatclient: $(OBJS) $(CIrc)
	@echo --- Linking final executable $@...
	@$(CC) $(ARCHFLAG) $(OBJS) $(IFACE) $(CIrc) -o $@ $(LDFLAGS)

%.o: %.c
	@echo --- Compiling $@
	@$(CC) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $(@:.o=.c) -o $@

### Implied dependencies
conf.c: display_interfaces.h 
cookies-test.c: cookies.h 
cookies.c: cookies.h display_interfaces.h 
entities.c: entities.h 
gotcurses.c: display_interfaces.h commons.h strfunctions.h conf.h 
gotnull.c: commons.h 
gottext.c: CUtils/net.h CUtils/cstring.h display_interfaces.h commons.h 
iface-test.c: display_interfaces.h CUtils/cstring.h 
main.c: main.h entities.h cookies.h network.h parsehtml.h conf.h commons.h display_interfaces.h strfunctions.h mccirc.h 
main.h: mccirc.h 
mccirc.c: mccirc.h CUtils/cstring.h CUtils/clist.h CIrc/irc_server.h CIrc/irc_chan.h 
mccirc.h: CIrc/irc_server.h CUtils/clist.h 
mccirc_fake.c: mccirc.h 
network.c: display_interfaces.h network.h 
nicklist.c: display_interfaces.h mccirc.h main.h 
parsehtml.c: entities.h parsehtml.h nicklist.h parser.h display_interfaces.h main.h 
parser-test.c: parser.h CUtils/cstring.h 
parser.c: CUtils/cstring.h CUtils/clist.h parser.h parser_p.h CUtils/ini.h 
parser.h: CUtils/clist.h 
parser_p.h: CUtils/ini.h 
strfunctions.c: strfunctions.h 

