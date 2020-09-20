.PHONY: all debug clean

CFLAGS=-Wall -Wextra -Werror -c
CFLAGS.P=$(CFLAGS) -DNDEBUG
CFLAGS.D=$(CFLAGS) -g
LDFLAGS=`pkg-config --libs glfw3` `pkg-config --libs vulkan`

all: physEngine

debug: physEngine.d

clean:
	rm -f physEngine
	rm -f *.o

physEngine: physEngine.o
	gcc -o physEngine physEngine.o $(LDFLAGS)

physEngine.o: physEngine.c
	gcc -c -o physEngine.o physEngine.c $(CFLAGS.P)

physEngine.d: physEngine.d.o
	gcc -o physEngine.d physEngine.d.o $(LDFLAGS)

physEngine.d.o: physEngine.c
	gcc -o physEngine.d.o physEngine.c $(CFLAGS.D)
