# for Windows (mingw)
ifeq ($(OS),Windows_NT)
ifeq ($(MSYSTEM),MINGW32)
CFLAGS	= -g -Wall 
LIBS	= 
# Reference compiled libraries in local directory (32bit)
CFLAGS	+= -I./hidapi/win32/include/
LIBS	+= -L./hidapi/win32/lib/ -lhidapi -lsetupapi
else
CFLAGS	= -g -Wall 
LIBS	= 
# Reference compiled libraries in local directory (64bit)
CFLAGS	+= -I./hidapi/x64/include/
LIBS	+= -L./hidapi/x64/lib/ -lhidapi -lsetupapi
endif
endif

# for Linux
ifeq ($(shell uname),Linux)
CFLAGS	= -g -Wall 
LIBS	= 
# sudo apt-get install libhidapi-dev
CFLAGS	+= $(shell pkg-config --cflags hidapi-hidraw)
LIBS	+= $(shell pkg-config --libs hidapi-hidraw)
endif

# for MacOSX
ifeq ($(shell uname),Darwin)
CFLAGS	= -g -Wall 
LIBS	= 
# brew install hidapi
CFLAGS	+= -I$(shell brew --prefix hidapi)/include
LIBS	+= -L$(shell brew --prefix hidapi)/lib -lhidapi
endif

bto_hid_cmd:

bto_hid_cmd.o:	bto_hid_cmd.c
	$(CC) $(CFLAGS) -c bto_hid_cmd.c -o bto_hid_cmd.o

bto_hid_cmd:	bto_hid_cmd.o
	$(CC) $(CFLAGS) -o bto_hid_cmd  bto_hid_cmd.o $(LIBS)

clean:
	$(RM) bto_hid_cmd
	$(RM) bto_hid_cmd.o
