.PHONY: all clean

CFLAGS=-Wall -Wextra -Werror
LDFLAGS=`pkg-config --libs glfw3` `pkg-config --libs vulkan`

all: physEngine

clean:
	rm -f physEngine
	rm -f *.o

physEngine: physEngine.o
	gcc -o physEngine physEngine.o $(LDFLAGS)

physEngine.o: physEngine.c
	gcc -c -o physEngine.o physEngine.c $(CFLAGS)
