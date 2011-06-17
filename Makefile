SOURCES=main.c cookies.c cstring.c ini.c entities.c network.c parsehtml.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=minichatclient

tests = cookies-test.c cstring-test.c ini-test.c
bintests = cookie-test ini-test cstring-test

# 'all' and 'clean' and 'test' are not files
.PHONY : all clean test

# Default goal, invoked by "make" without parameters
all: $(SOURCES) $(EXECUTABLE)

rebuild: clean all

$(EXECUTABLE): $(OBJECTS)
	@echo --- 
	@echo - Linking final executable $(EXECUTABLE)...
	@echo --- 
	@$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Use those ".d" makefiles -- the "-" before inlude is there
# so it won't print an error message if the .d file is still
# not created
-include $(SOURCES:.c=.d)

# Compile the test executables
test: $(bintests)

# Clean the temporary files
clean:
	@echo ---
	@echo - Cleaning the build directory...
	@echo --- 
	@rm *.o *.d $(EXECUTABLE) 2>/dev/null || true

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

