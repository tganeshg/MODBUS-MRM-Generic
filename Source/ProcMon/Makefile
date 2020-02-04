
CC=arm-openwrt-linux-gcc
#CC=cc
CCFLAGS=-I./ -Wall -ggdb -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-sign -DUSBFORMAT
LDFLAGS=
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
TARGET=procMon
 
all: $(TARGET)
 
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^
	cp $(TARGET) ../usr/mrm/bin/
 
%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $<
 
%.o: %.c
	$(CC) $(CCFLAGS) -c $<
 
clean:
	rm -f *.o $(TARGET)

