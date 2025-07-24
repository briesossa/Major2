CC = gcc
CFLAGS = -Wall -g
TARGET = newshell

all: $(TARGET)

$(TARGET) : newshell.c
	$(CC) $(CFLAGS) -o $(TARGET) newshell.c

clean:
	rm -f $(TARGET)
