MSOURCES=main.c parsehtml.c
MOBJECTS=$(MSOURCES:.c=.o)
EXECUTABLE=minichatclient

SOURCES=cookies.c cstring.c ini.c entities.c network.c
OBJECTS=$(SOURCES:.c=.o)

TEST_SOURCES=cookies-test.c cstring-test.c ini-test.c
TEST_OBJECTS=$(TEST_SOURCES:.c=.o)
TEST_EXECUTABLES=cookies-test cstring-test ini-test

# 'all' and 'clean' and 'test' are not files
.PHONY : all clean test

# Default goal, invoked by "make" without parameters
all: $(MSOURCES) $(SOURCES) $(EXECUTABLE)

rebuild: clean all

$(EXECUTABLE): $(OBJECTS) $(MOBJECTS)
	@echo --- 
	@echo - Linking final executable $(EXECUTABLE)...
	@echo --- 
	$(CC) $(LDFLAGS) $(OBJECTS) $(MOBJECTS) -o $@

# Compile the test executables
test: $(TEST_EXECUTABLES)

$(TEST_EXECUTABLES): $(TEST_SOURCES) $(TEST_OBJECTS) $(OBJECTS)
	@echo --- 
	@echo - Linking test executable $@...
	@echo --- 
	$(CC) $(LDFLAGS) $@.o $(OBJECTS) -o $@

# Use those ".d" makefiles -- the "-" before inlude is there
# so it won't print an error message if the .d file is still
# not created
-include $(MSOURCES:.c=.d) $(SOURCES:.c=.d)

# Clean the temporary files
clean:
	@echo ---
	@echo - Cleaning the build directory...
	@echo --- 
	@rm *.o *.d $(EXECUTABLE) $(TEST_EXECUTABLES) 2>/dev/null || true

# create a ".d" makefile for each C source file
# with its required dependencies
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o \1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# Private headers support
%.c: %_p.h
%.o: %_p.h

love:
	@echo "... not war ?"

