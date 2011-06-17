################################
#### MINICHATCLIENT MAKEFILE ###
################################

all: mchatclient

rebuild: mrproper all

mchatclient: main.o cookies.o network.o parsehtml.o entities.o
	@echo "*** Linking all main objects files..."
	@gcc cookies.o network.o main.o parsehtml.o entities.o -o mchatclient

#### USED OBJECTS ####

main.o: main.c userconfig.h network.h cookies.h parsehtml.h 
	@echo "*** Compiling main.o"
	@gcc -c main.c -o main.o

network.o: network.c
	@echo "*** Compiling network.o"
	@gcc -c network.c -o network.o

cookies.o: cookies.c cookies.h
	@echo "*** Compiling cookies.o"
	@gcc -c cookies.c -o cookies.o

parsehtml.o: parsehtml.c parsehtml.h entities.h
	@echo "*** Compiling parsehtml.o"
	@gcc -c parsehtml.c -o parsehtml.o

entities.o: entities.c entities.h
	@echo "*** Compiling entities.o"
	@gcc -c entities.c -o entities.o

parser.o: parser.c parser.h parser_p.h
	@echo "*** Compiling parser.o"
	@gcc -c parser.c -o parser.o

ini.o: ini.c ini.h ini_p.h
	@echo "*** Compiling ini.o"
	@gcc -c ini.c -o ini.o

cstring.o: cstring.c cstring.h cstring_p.h
	@echo "*** Compiling cstring.o"
	@gcc -c cstring.c -o cstring.o

#### TESTS #### 

cookies-test: cookies.o cookies-test.o
	@echo "*** Linking cookies-test executable..."
	@gcc cookies.o cookies-test.o -o cookies-test

cookies-test.o: cookies-test.c
	@echo "*** Compiling cookies-test.o"
	@gcc -c cookies-test.c -o cookies-test.o

ini-test: ini.o ini-test.o
	@echo "*** Linking ini-test executable..."
	@gcc ini.o ini-test.o -o ini-test

ini-test.o: ini-test.c
	@echo "*** Compilling ini-test.o"
	@gcc -c ini-test.c -o ini-test.o

cstring-test: cstring.o cstring-test.o
	@echo "*** Linking cstring-test executable..."
	@gcc cstring.o cstring-test.o -o cstring-test

cstring-test.o: cstring-test.c
	@echo "*** Compiling cstring-test.o"
	@gcc -c cstring-test.c -o cstring-test.o

#### MISC. STUFF #### 

clean:
	@echo "*** Erasing objects files and test executables..."
	@rm -f *.o cookies-test ini-test cstring-test

mrproper: clean
	@echo "*** Erasing main executable file..."
	@rm -f mchatclient

love:
	@echo "... not war ?"
