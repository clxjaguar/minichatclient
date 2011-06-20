### Definitions

MSOURCES=main.c parsehtml.c
MOBJECTS=$(MSOURCES:.c=.o)
EXECUTABLE=minichatclient

SOURCES=cookies.c cstring.c ini.c entities.c network.c parser.c
OBJECTS=$(SOURCES:.c=.o)

TEST_SOURCES=cookies-test.c cstring-test.c ini-test.c parser-test.c
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLES=$(TEST_SOURCES:.c=)

### Active targets

# The active targets are not actual files
.PHONY : all clean mrproper mrpropre rebuild test install love

all: $(EXECUTABLE)

clean:
	@echo ---
	@echo - Cleaning the build directory...
	@echo --- 
	@rm -f *.o

mrpropre: mrproper

mrproper:
	@echo ---
	@echo - *CLEANING* the build directory '(EXEs included)'...
	@echo --- 
	@rm -f *.o $(EXECUTABLE) $(TEST_EXECUTABLES)

rebuild: mrproper all

test: $(TEST_EXECUTABLES)

install: all
	@echo ---
	@echo - Installing the programme into /usr/bin...
	@echo --- 
	cp $(EXECUTABLE) /usr/bin

love:
	@echo "... not war ?"

### Dependencies

$(EXECUTABLE): $(OBJECTS) $(MOBJECTS)
	@echo --- 
	@echo - Linking final executable $(EXECUTABLE)...
	@echo --- 
	$(CC) $(LDFLAGS) $(OBJECTS) $(MOBJECTS) -o $@

$(TEST_EXECUTABLES): $(TEST_SOURCES) $(TEST_OBJECTS) $(OBJECTS)
	@echo --- 
	@echo - Linking test executable $@...
	@echo --- 
	$(CC) $(LDFLAGS) $@.o $(OBJECTS) -o $@

