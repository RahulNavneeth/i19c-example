CC = clang
CFLAGS = -Wall -O3 -Iinclude \
         $(shell pkg-config --cflags sdl3) \
         $(shell pkg-config --cflags sdl3-ttf) \
         -Ilib/i19c/include -Ilib/i19c/.i19c

LDFLAGS = $(shell pkg-config --libs sdl3) \
          $(shell pkg-config --libs sdl3-ttf) \
          -Llib/i19c/bin/ -li19c \
          -framework OpenGL

BIN = bin
SRC = $(shell find src/ -name "*.c")
OBJ = $(patsubst %.c, $(BIN)/%.o, $(SRC))

.PHONY: all lib dirs clean build run

all: clean lib dirs build

lib:
	cd lib/i19c && make all path=../../include/i19c

clean:
	@ rm -rf $(BIN)

dirs:
	@ mkdir $(BIN)

build: $(OBJ)
	$(CC) -o $(BIN)/main $^ $(LDFLAGS)

$(BIN)/%.o: %.c 
	mkdir -p $(dir $@)
	$(CC) -o $@ -c $< $(CFLAGS)

run: all
	$(BIN)/main
