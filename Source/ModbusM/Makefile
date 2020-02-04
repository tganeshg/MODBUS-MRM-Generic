#export PATH=/home/ganesh/Programs/SAM9X25/openwrt/staging_dir/toolchain-arm_arm926ej-s_gcc-5.3.0_musl-1.1.16_eabi/bin:$PATH
#CC=cc
CC=arm-openwrt-linux-gcc
CCFLAGS=-I./ -Wall -ggdb -Wno-unused-variable -Wno-unused-but-set-variable -Wno-pointer-sign -DUSBFORMAT
LDFLAGS=
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
TARGET=modbus
 
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
