
#CC=cc
CC= arm-openwrt-linux-gcc
CCFLAGS=-I../src -I../lib -I./ -I../ -Wall -ggdb -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-sign
#-DUSBFORMAT
LDFLAGS=-L./
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
TARGET=ftpcl
 
all: $(TARGET)
 
$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ -lftp
	cp $(TARGET) ../usr/mrm/bin/
 
%.o: %.c %.h
	$(CC) $(CCFLAGS) -c $<
 
%.o: %.c
	$(CC) $(CCFLAGS) -c $<
 
clean:
	rm -f *.o $(TARGET)
