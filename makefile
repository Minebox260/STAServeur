# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g -pthread


# Executable name
TARGET = serveur_exe

# Source files
SRCS = server.c server_comm.c server_utils.c

# Object files
OBJS = $(SRCS:.c=.o)

# Header files
HEADERS = 

# Default rule
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) 

# Compiling source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $<

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
