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

# 'all' and 'clean' and 'test' are not files
.PHONY : all clean rebuild test install love

all: $(EXECUTABLE)

clean:
	@echo ---
	@echo - Cleaning the build directory...
	@echo --- 
	@rm *.o $(EXECUTABLE) $(TEST_EXECUTABLES) 2>/dev/null || true


rebuild: clean all

test: $(TEST_EXECUTABLES)

install: all
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

