###
### Interesting options:
### 	debug:			debug build
### 	cursesw (default):	cursesw (wide char, UTF-8) interface
### 	curses:			curses interface (ISO)
### 	text:			output plain text on stdout, stderr
### 	text1:			output plain text on stdout
### 	none:			do not output anything
### 	irc:			enable IRC-server support
### 	x86:			compile the x86 version under an AMD64 machine
###
### Exemples:
### 	make # will compile the program with default values
### 	make irc debug none # will compile the program with IRC support
### 			    # and no direct output
###
### Note that if you provide an explicit output module option,
### it MUST be the last option you pass.
###

### Options
CFLAGS += -Wall -Wextra -fshort-enums

# The active targets are not actual files
.PHONY : all clean mrproper mrpropre rebuild test install love \
	 irc debug curses cursesw text text1 none x86

### IRC built-in server support (default is off)
CIrc = mccirc_fake.c

irc: CIrc = mccirc.o CIrc/libcirc.o
irc: mccirc.o CIrc/libcirc.o all

### out interface (default is cursesw)
cursesw: LDFLAGS += -lncursesw
cursesw: CFLAGS += -D_X_OPEN_SOURCE_EXTENDED
cursesw: IFACE = gotcurses.c
cursesw: mchatclient

curses: LDFLAGS +=-lncurses
curses: IFACE = gotcurses.c
curses: mchatclient 

text: IFACE = gottext.c
text: mchatclient 

text1: IFACE = gottext.c
text1: CFLAGS += -DNO_STD_ERR
text1: mchatclient 

none: IFACE = gotnull.c
none: mchatclient 

### Special code for cross compiling x86 from AMD64
x86: ARCHFLAG="-m32"
x86: mchatclient.x86

### DEBUG
debug: CFLAGS += -g -ggdb -O0 -Wformat -Winit-self -Wmissing-include-dirs -Wparentheses -Wswitch-default -Wswitch-enum -Wunused-parameter -Wuninitialized -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion -DDEBUG

### Active targets

all: cursesw

clean:
	@echo --- Cleaning all objects files...
	@rm -f *.o */*.o */*/*.o

mrpropre: mrproper

mrproper: clean
	@echo --- Cleaning all binary executables files...
	@rm -f mchatclient mchatclient.x86

### note: rebuild will always rebuild the default interface
rebuild: mrproper all

test: cookies-test iface-test parser-test 

love:
	@echo "... not war ?"

### Dependencies
conf.o: conf.c conf.h display_interfaces.h
parsehtml.o: parsehtml.c parsehtml.h main.h entities.h parser.h display_interfaces.h nicklist.h
nicklist.o: nicklist.c nicklist.h main.h mccirc.h 
cookies.o: cookies.c cookies.h display_interfaces.h
network.o: network.c display_interfaces.h
main.o: main.c main.h conf.h network.h cookies.h parsehtml.h display_interfaces.h commons.h mccirc.h
gotcurses.o: gotcurses.c display_interfaces.h commons.h strfunctions.h
gottext.o: gottext.c display_interfaces.h commons.h
gotnull.o: gotnull.c display_interfaces.h commons.h
mccirc.o: mccirc.c mccirc.h CUtils/libcutils.o CIrc/libcirc.o
parser.o: parser.c parser.h parser_p.h CUtils/libcutils.o
CUtils/libcutils.o: CUtils/libcutils.c CUtils/libcutils.h CUtils/attribute.h CUtils/attribute.c CUtils/clist.h CUtils/clist.c CUtils/ini.h CUtils/ini.c CUtils/net.h CUtils/net.c CUtils/cstring.h CUtils/cstring.c
CIrc/libcirc.o: CUtils/libcutils.o CIrc/libcirc.c CIrc/irc_chan.h CIrc/irc_client.h CIrc/irc_server.h CIrc/irc_user.h CIrc/irc_chan.c CIrc/irc_client.c CIrc/irc_server.c CIrc/irc_user.c

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
	
mchatclient: $(OBJS) $(IFACE) $(CIrc)
	@echo --- Linking final executable $@...
	@$(CC) $(OBJS) $(IFACE) $(CIrc) -o $@ $(LDFLAGS)

mchatclient.x86: $(OBJS) $(IFACE) $(CIrc)
	@echo --- Linking final executable $@...
	@$(CC) $(ARCHFLAG) $(OBJS) $(IFACE) $(CIrc) -o $@ $(LDFLAGS)

### #if 0 == 1
%.o: %.c
	@echo --- Compiling $@
	@$(CC) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $(@:.o=.c) -o $@
### #endif

