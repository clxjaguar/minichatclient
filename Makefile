################################
#### MINICHATCLIENT MAKEFILE ###
################################

COMPILER = gcc 
CCFLAGS = -Wall -Wextra -fshort-enums
LIBS = -lncurses
.PHONY: all rebuild clean mrproper love

all: mchatclient

rebuild: mrproper all

mchatclient: main.o cookies.o network.o conf.o parsehtml.o entities.o parser.o clist.o cstring.o ini.o attribute.o gotcurses.o
	@echo "*** Linking all main objects files..."
	@gcc ${LIBS} cookies.o network.o main.o conf.o parsehtml.o entities.o parser.o clist.o cstring.o ini.o attribute.o gotcurses.o -o mchatclient
	@strip mchatclient

#### USED OBJECTS ####

main.o: main.c conf.h network.h cookies.h parsehtml.h display_interfaces.h commons.h
	@echo "*** Compiling main.o"
	@${COMPILER} ${CCFLAGS} -c main.c -o main.o

network.o: network.c display_interfaces.h
	@echo "*** Compiling network.o"
	@${COMPILER} ${CCFLAGS} -c network.c -o network.o

cookies.o: cookies.c cookies.h display_interfaces.h
	@echo "*** Compiling cookies.o"
	@${COMPILER} ${CCFLAGS} -c cookies.c -o cookies.o

parsehtml.o: parsehtml.c parsehtml.h entities.h parser.h clist.h display_interfaces.h
	@echo "*** Compiling parsehtml.o"
	@${COMPILER} ${CCFLAGS} -c parsehtml.c -o parsehtml.o

conf.o: conf.c conf.h display_interfaces.h
	@echo "*** Compiling conf.o"
	@${COMPILER} ${CCFLAGS} -c conf.c -o conf.o

gotcurses.o: gotcurses.c display_interfaces.h commons.h
	@echo "*** Compiling gotcurses.o"
	@${COMPILER} ${CCFLAGS} -c gotcurses.c -o gotcurses.o

# PARSING HTML ENTITIES

entities.o: entities.c entities.h
	@echo "*** Compiling entities.o"
	@${COMPILER} ${CCFLAGS} -c entities.c -o entities.o

# HTML PARSING IN MESSAGES

parser.o: parser.c parser.h
	@echo "*** Compiling parser.o"
	@${COMPILER} ${CCFLAGS} -c parser.c -o parser.o

# NOW OBJECTS USED FOR HTML PARSING IN MESSAGES

cstring.o: cstring.c cstring.h
	@echo "*** Compiling cstring.o"
	@${COMPILER} ${CCFLAGS} -c cstring.c -o cstring.o

clist.o: clist.c clist.h
	@echo "*** Compiling clist.o"
	@${COMPILER} ${CCFLAGS} -c clist.c -o clist.o

ini.o: ini.c ini.h
	@echo "*** Compiling ini.o"
	@${COMPILER} ${CCFLAGS} -c ini.c -o ini.o

attribute.o: attribute.c attribute.h
	@echo "*** Compiling attribute.o"
	@${COMPILER} ${CCFLAGS} -c attribute.c -o attribute.o


#### TESTS #### 

cookies-test: cookies.o cookies-test.o
	@echo "*** Linking cookies-test executable..."
	@${COMPILER} cookies.o cookies-test.o -o cookies-test

cookies-test.o: cookies-test.c
	@echo "*** Compiling cookies-test.o"
	@${COMPILER} ${CCFLAGS} -c cookies-test.c -o cookies-test.o

cstring-test: cstring.o cstring-test.o
	@echo "*** Linking cstring-test executable..."
	@${COMPILER} cstring.o cstring-test.o -o cstring-test

cstring-test.o: cstring-test.c
	@echo "*** Compiling cstring-test.o"
	@${COMPILER} ${CCFLAGS} -c cstring-test.c -o cstring-test.o

parser-test: parser.o parser-test.o cstring.o ini.o clist.o attribute.o
	@echo "*** Linking parser-test executable..."
	@${COMPILER} parser.o parser-test.o cstring.o ini.o clist.o attribute.o -o parser-test

parser-test.o: parser-test.c parser.h clist.h
	@echo "*** Compiling parser-test.o"
	@${COMPILER} ${CCFLAGS} -c parser-test.c -o parser-test.o

clist-test: clist.o clist-test.o
	@echo "*** Linking clist-test executable..."
	@${COMPILER} clist.o clist-test.o -o clist-test

clist-test.o: clist-test.c
	@echo "*** Compiling clist-test.o"
	@${COMPILER} ${CCFLAGS} -c clist-test.c -o clist-test.o


#### MISC. STUFF #### 

clean:
	@echo "*** Erasing objects files and test executables..."
	@rm -f *.o cookies-test ini-test cstring-test parser-test

mrproper: clean
	@echo "*** Erasing main executable file..."
	@rm -f mchatclient

love:
	@echo "... not war ?"
