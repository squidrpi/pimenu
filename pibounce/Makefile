# Comilation file for RPI
PROG_NAME = pibounce
OBJS = pibounce.o
LDFLAGS = 

CC = gcc
CXX = g++
STRIP = strip

# output from sdl-config
SDL_FLAGS=-I/usr/include/SDL -I/usr/include/glib-2.0 -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include \
	-I/opt/vc/include -I/opt/vc/include/interface/vcos/pthreads \
    -I/opt/vc/include/interface/vmcs_host/linux
SDL_LIBS=-lSDL -lm -ldl -lglib-2.0

#CFLAGS = $(SDL_FLAGS) -O2 -funroll-loops -Wextra -Werror
CFLAGS = $(SDL_FLAGS) -g -funroll-loops -Wextra -Werror
CXXFLAGS = $(SDL_FLAGS) -O2 -funroll-loops -Wall
LIBS = $(SDL_LIBS)  -L/opt/vc/lib -lbcm_host -lGLESv2 -lpng

TARGET = $(PROG_NAME)

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CXX) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)
#	$(STRIP) $(TARGET)

clean:
	/bin/rm -rf *~ *.o $(TARGET)
