TARGET = coaster
LIBS = -framework OpenGL -framework GLUT
CC = gcc
CFLAGS = -g -Wall

.PHONY: default run clean

default: run
all: default

OBJECTS = $(patsubst %.c, .objectFiles/%.o, $(wildcard *.c))
HEADERS = $(wildcard *.h)

.objectFiles/%.o: %.c $(HEADERS)
	mkdir -p .objectFiles
	$(CC) $(CFLAGS) -c -w $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	-rm -f *.o
	-rm -f .objectFiles
	-rm -f $(TARGET)
