# Target executable
TARGET := main

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -pedantic -g -pthread

# Path to OpenSSL header files
CFLAGS += -Iopenssl-3.2.2/ -Imqtt/ -Imqtt/MQTTPacket/

# Percorso agli header files di nanoModbus
CFLAGS += -InanoMODBUS/

# Libraries
LIBS := -lstdc++ -Lopenssl-3.2.2 -lssl -lcrypto

# Source files
#SRCS := main.c modbus_task.c mqtt_task.c nanoMODBUS/nanomodbus.c utilis.c mqtt/MQTTInterface.c mqtt/MQTTClient.c
SRCS := $(wildcard *.c nanoMODBUS/*.c mqtt/*.c mqtt/MQTTPacket/*.c)

# Object files
OBJS := $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
