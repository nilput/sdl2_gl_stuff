
DEPS = glad/glad.o
SRC = $(shell find . -maxdepth 1 -name '*.c')
OBJ = $(patsubst %.c, %.o, $(SRC))
PROGS = $(patsubst %.o, %, $(OBJ))

LDFLAGS = 
CFLAGS := -Wall -Wpedantic -g3 -O0  $(shell pkg-config --cflags sdl2) $(shell pkg-config --cflags gl) -I3dparty
LDLIBS := $(shell pkg-config --libs sdl2)  $(shell pkg-config --libs gl) $(DEPS) -ldl

.PHONY: build all clean
build: $(DEPS)
build: $(PROGS)

all: $(PROGS)
clean:
	@rm -f $(OBJ) $(PROGS) $(DEPS)
