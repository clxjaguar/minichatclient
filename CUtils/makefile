### This makefile can be customized with the GTK and DEBUG environment variable.
###
### If GTK is GTK2, we will compile and link against gtk+ 2.0.
### If GTK is GTK3, we will compile and link against gtk+ 3.0.
### I.E.: "make all GTK=GTK2" (default is GTK2)
###
### If DEBUG is 1, we will check against all possible warnings.
### I.E.: "make all DEBUG=1"
###
### If CROSS is win32, we will cross-compile to target WIN32 (using mxe)
### I.E.: "make all CROSS=win32"
### (you can also specify the version of mingw to use:
### "make all CROSS=win32 MGW=i686-pc-mingw32"
### but then, please remember to set the PKG_CONFIG_PATH_i686-pc-mingw32
### or similar variable)
###
### If CROSS is x86, we will cross-compile to target x86 (from AMD64)
### I.E.: "make all CROSS=x86"

VERSION=1.0.0

### Options
# GTK+ and GtkGLExt, pthread
O_GTK=0
O_OPENGL=0
O_PTHREADS=0

### Extra CFLAGS
CFLAGS += -Wall -Wextra

# Executable name, main source file, sources, test source files
NAME=libcutils
MSOURCES=
SOURCES=ini.c attribute.c clist.c cstring.c net.c
#TEST_SOURCES=test/ini-test.c test/clist-test.c test/cstring-test.c test/net-test.c
TEST_SOURCES=test/test.c
TEST_ASO=test/test.A test/test.SO
###

### MXE special code (to allow cross compilation to win32)
ifeq ($(CROSS), win32)
	ifeq ($(MGW)a, a)
		MGW=i686-pc-mingw32
	endif
	
	# note: if you override MGW, you must specify the pkg path yourself
	PKG_CONFIG_PATH_i686-pc-mingw32:=mxe/usr/$(MGW)/lib/pkgconfig
	PATH:=mxe/usr/bin:$(PATH)
	
	LDFLAGS += -mwindows
	EXECUTABLE:=$(EXECUTABLE).exe
	CC=$(MGW)-gcc
	LD=$(MGW)-ld
	AR=$(MGW)-ar
	PKG_CONFIG=$(MGW)-pkg-config
else
	### Special code for cross compiling x86 from AMD64
	ifeq ($(CROSS), x86)
		EXECUTABLE:=$(EXECUTABLE).x86
		ARCHFLAG="-m32"
	endif
	###
	PKG_CONFIG=pkg-config
endif
###

### DEBUG
ifeq ($(DEBUG), 1)
	CFLAGS += -g -ggdb -O0 -fshort-enums -Wformat -Winit-self -Wmissing-include-dirs -Wparentheses -Wswitch-default -Wswitch-enum -Wunused-parameter -Wuninitialized -Wundef -Wshadow -Wpointer-arith -Wbad-function-cast -Wcast-qual -Wwrite-strings -Wconversion
endif
###

### GTK flags
ifeq ($(O_GTK), 1)
	# export-dynamic not supported on PE targets
	ifneq ($(CROSS), win32)
		CFLAGS += -export-dynamic
		LDFLAGS += -export-dynamic
	endif
	ifeq ($(GTK), GTK3)
		CFLAGS += `$(PKG_CONFIG) --cflags gtk+-3.0`
		LDFLAGS += `$(PKG_CONFIG) --libs gtk+-3.0`
	else
		CFLAGS += `$(PKG_CONFIG) --cflags gtk+-2.0`
		LDFLAGS += `$(PKG_CONFIG) --libs gtk+-2.0`
	endif
endif
###

### GtkGlExt flags
ifeq ($(O_OPENGL), 1)
	CFLAGS += `$(PKG_CONFIG) --cflags gtkglext-1.0`
	LDFLAGS += `$(PKG_CONFIG) --libs gtkglext-1.0`
endif
###

### pthread support
ifeq ($(O_PTHREADS), 1)
	LDFLAGS += -lpthread
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

all: $(NAME).a $(NAME).so.$(VERSION)

clean:
	@echo --- Cleaning the build directory...
	@rm -f *.o */*.o */*/*.o

mrpropre: mrproper

mrproper: clean
	@echo --- *CLEANING* the build directory '(EXEs included)'...
	@rm $(NAME).a $(NAME).so.$(VERSION) $(EXECUTABLE) $(TEST_EXECUTABLES) $(TEST_ASO) 2>/dev/null || echo >/dev/null

rebuild: mrproper all

test: $(TEST_EXECUTABLES) $(TEST_ASO)

install: all
	@echo --- Installing the programme into /usr/bin...
	cp $(EXECUTABLE) /usr/bin

love:
	@echo "... not war ?"

### Dependencies
SHARED=-shared -fPIC

%.o: %.c
	@echo --- $@
	@$(CC) $(SHARED) $(ARCHFLAG) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(TARGET_ARCH) -c $^ -o $@

$(NAME): $(OBJECTS) $(MOBJECTS)
	@echo === Linking final binary $(EXECUTABLE)...
	@$(CC) $(SHARED) $(ARCHFLAG) $(OBJECTS) $(MOBJECTS) -o $@ $(LDFLAGS)

$(NAME).a: $(OBJECTS)
	@echo === Linking final binary $(NAME).a...
	@$(AR) rcs $(NAME).a $(OBJECTS)

$(NAME).so.$(VERSION): $(OBJECTS)
	@echo === Linking final binary $(NAME).so.$(VERSION)...
	@$(CC) --shared -fPIC $(ARCHFLAG) $(OBJECTS) $(MOBJECTS) -o $@ $(LDFLAGS)

$(TEST_EXECUTABLES): $(TEST_OBJECTS) $(OBJECTS)
	@echo === Linking test executable $@...
	@$(CC) $(ARCHFLAG) $@.o $(OBJECTS) -o $@ $(LDFLAGS)

%.A: $(TEST_OBJECTS) $(NAME).a
	@echo === Linking .A test executable $@...
	@$(CC) $(ARCHFLAG) $(TEST_OBJECTS) $(NAME).a -o $@ $(LDFLAGS)

%.SO: $(TEST_OBJECTS) $(NAME).so.$(VERSION)
	@echo === Linking .so test executable $@...
	@$(CC) $(ARCHFLAG) $(TEST_OBJECTS) $(NAME).so.$(VERSION) -o $@ $(LDFLAGS)

