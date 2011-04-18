all: mchatclient

rebuild: mrproper all

mchatclient: main.o cookies.o network.o parsehtml.o
	@echo "*** Linking all main objects files..."
	@gcc cookies.o network.o main.o parsehtml.o -o mchatclient

cookies-test: cookies.o cookies-test.o
	@echo "*** Linking cookies-test executable..."
	@gcc cookies.o cookies-test.o -o cookies-test

main.o: main.c
	@echo "*** Compiling main.o"
	@gcc -c main.c -o main.o

cookies-test.o: cookies-test.c
	@echo "*** Compiling cookies-test.o"
	@gcc -c cookies-test.c -o cookies-test.o

network.o: network.c
	@echo "*** Compiling network.o"
	@gcc -c network.c -o network.o

cookies.o: cookies.c cookies.h
	@echo "*** Compiling cookies.o"
	@gcc -c cookies.c -o cookies.o

parsehtml.o: parsehtml.c parsehtml.h
	@echo "*** Compiling parsehtml.o"
	@gcc -c parsehtml.c -o parsehtml.o

clean:
	@echo "*** Erasing objects files..."
	@rm -f *.o

mrproper: clean
	@echo "*** Erasing executables..."
	@rm -f mchatclient cookies-test

love:
	@echo "... not war ?"
