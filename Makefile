# Compiler and Flags
CC = gcc
CFLAGS = -I$(IDIR) -Wall -Wextra -g

# Included libraries
LIBS = -lssl -lcrypto
SDIR = ./src
IDIR = ./include

# Directories
SDIR = src
IDIR = include

# Files
SRCS = $(SDIR)/imap_fetch.c $(SDIR)/utils.c
HDRS = $(IDIR)/utils.h
OBJS = $(SRCS:.c=.o)

# Target executable name
TARGET = imap_fetch

# Default target
.PHONY: all
all: $(TARGET)

# Compile code and link to create the final program
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# Pattern rule to build object files
$(SDIR)/%.o: $(SDIR)/%.c $(HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)