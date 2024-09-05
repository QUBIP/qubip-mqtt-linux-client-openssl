# Target executable
TARGET := main

# Build directory
BUILD_DIR := build

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

# Object files (within the build directory)
OBJS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default target
all: $(TARGET)

# Build the target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Compile source files into object files (within the build directory)
$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
