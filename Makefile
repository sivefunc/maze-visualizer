PROG_NAME = maze-visualizer
CC = gcc
LANG = c
STD = c99
OBJ_DIR = objs/
SRC_DIR = src/
OBJS = $(OBJ_DIR)maze-visualizer.o

CFLAGS= -x $(LANG) --std=$(STD) -Wall -Wextra -O3\
		$(shell pkg-config --cflags --libs sdl2)

LDFLAGS = -lm $(shell pkg-config --libs sdl2)

$(PROG_NAME) : $(OBJS)
	@$(CC) -o $(PROG_NAME) $(OBJS) $(LDFLAGS) $(CFLAGS)

$(OBJ_DIR)maze-visualizer.o : $(SRC_DIR)maze-visualizer.c
	@$(CC) $(CFLAGS) -c $(SRC_DIR)maze-visualizer.c -o $(OBJ_DIR)maze-visualizer.o

run:
	./$(PROG_NAME)

.PHONY : clean
clean :
	rm $(PROG_NAME) $(OBJS)
