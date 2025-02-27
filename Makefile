CFLAGS=-fPIC
LDFLAGS=-shared

OBJDIR=./build/$(PLATFORM)
SODIR=./so/$(PLATFORM)
TESTDIR=./test/$(PLATFORM)
SRCDIR = ./src

ifeq ($(PLATFORM), windows)
    CC=x86_64-w64-mingw32-gcc
    CXX=x86_64-w64-mingw32-g++
    CFLAGS += -DPLATFORM_WINDOWS
    SERIAL_SRC=serial_comm_windows.cpp
    SHARED_LIB_EXT=dll
    BINARY_EXT=.exe
else ifeq ($(PLATFORM), linux)
    CC=gcc
    CXX=g++
    CFLAGS += -DPLATFORM_LINUX
    SERIAL_SRC=serial_comm_linux.cpp
    SHARED_LIB_EXT=so
    BINARY_EXT= 
else ifeq ($(PLATFORM), arm)
    CC=arm-linux-gnueabihf-gcc
    CXX=arm-linux-gnueabihf-g++
    CFLAGS += -DPLATFORM_LINUX
    CFLAGS += -static-libstdc++
    SERIAL_SRC=serial_comm_linux.cpp
    SHARED_LIB_EXT=so
    BINARY_EXT= 
else ifeq ($(PLATFORM), raspberrypi)
    CC=aarch64-linux-gnu-gcc
    CXX=aarch64-linux-gnu-g++
    CFLAGS += -DPLATFORM_LINUX
    SERIAL_SRC=serial_comm_linux.cpp
    SHARED_LIB_EXT=so
    BINARY_EXT= 
endif

all: directories $(SODIR)/libmotor.$(SHARED_LIB_EXT) $(SODIR)/libserial_comm.$(SHARED_LIB_EXT) $(SODIR)/libmirobot.$(SHARED_LIB_EXT) test$(BINARY_EXT)

directories:
	mkdir -p $(OBJDIR) $(SODIR)

$(OBJDIR)/serial_comm.o: $(SRCDIR)/$(SERIAL_SRC) $(SRCDIR)/serial_comm.h
	$(CXX) $(CFLAGS) -c $(SRCDIR)/$(SERIAL_SRC) -o $(OBJDIR)/serial_comm.o

$(OBJDIR)/motor.o: $(SRCDIR)/motor.cpp $(SRCDIR)/motor.h $(SRCDIR)/serial_comm.h
	$(CXX) $(CFLAGS) -c $(SRCDIR)/motor.cpp -o $(OBJDIR)/motor.o

$(OBJDIR)/mirobot.o: $(SRCDIR)/mirobot.cpp $(SRCDIR)/mirobot.h $(SRCDIR)/serial_comm.h
	$(CXX) $(CFLAGS) -c $(SRCDIR)/mirobot.cpp -o $(OBJDIR)/mirobot.o

$(SODIR)/libserial_comm.$(SHARED_LIB_EXT): $(OBJDIR)/serial_comm.o
	$(CXX) $(LDFLAGS) $(OBJDIR)/serial_comm.o -o $(SODIR)/libserial_comm.$(SHARED_LIB_EXT)

$(SODIR)/libmotor.$(SHARED_LIB_EXT): $(OBJDIR)/motor.o $(SODIR)/libserial_comm.$(SHARED_LIB_EXT)
	$(CXX) $(LDFLAGS) $(OBJDIR)/motor.o -o $(SODIR)/libmotor.$(SHARED_LIB_EXT) -L$(SODIR) -lserial_comm

$(SODIR)/libmirobot.$(SHARED_LIB_EXT): $(OBJDIR)/mirobot.o $(SODIR)/libserial_comm.$(SHARED_LIB_EXT)
	$(CXX) $(LDFLAGS) $(OBJDIR)/mirobot.o -o $(SODIR)/libmirobot.$(SHARED_LIB_EXT) -L$(SODIR) -lserial_comm

$(OBJDIR)/test.o: $(SRCDIR)/test.cpp $(SRCDIR)/motor.h $(SRCDIR)/serial_comm.h $(SRCDIR)/mirobot.h
	$(CXX) $(CFLAGS) -c $(SRCDIR)/test.cpp -o $(OBJDIR)/test.o

test$(BINARY_EXT): $(OBJDIR)/test.o $(SODIR)/libmotor.$(SHARED_LIB_EXT) $(SODIR)/libserial_comm.$(SHARED_LIB_EXT) $(SODIR)/libmirobot.$(SHARED_LIB_EXT)
	$(CXX) $(OBJDIR)/test.o -L$(SODIR) -lmotor -lserial_comm -lmirobot -o test$(BINARY_EXT)

clean:
	rm -rf ./build ./so test test.exe
