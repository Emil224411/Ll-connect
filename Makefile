CC = gcc

SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
INC_DIR = include

TARGET = bin/run
MAIN = src/main.c
SRC := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(SRC))
LIB := $(patsubst $(SRC_DIR)/%.c, $(LIB_DIR)/lib%.so,$(SRC))

all: $(TARGET)

$(TARGET): $(LIB_DIR)/libui.so $(LIB_DIR)/libcontroller.so $(MAIN)
	gcc -g -I$(INC_DIR) -L$(LIB_DIR) -Wl,-rpath,'$$ORIGIN/../$(LIB_DIR)' -lcontroller -lui -lSDL2 -o bin/run src/main.c

$(LIB_DIR)/libui.so: $(OBJ_DIR)/ui.o
	$(CC) -g -shared -o $@ $< -lSDL2 -lSDL2_ttf

$(LIB_DIR)/libcontroller.so: $(OBJ_DIR)/controller.o
	$(CC) -g -shared -o $@ $<

$(OBJ_DIR)/ui.o: $(SRC_DIR)/ui.c
	$(CC) -g -c -fPIC $< -o $@

$(OBJ_DIR)/controller.o: $(SRC_DIR)/controller.c
	$(CC) -g -c -fPIC $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(LIB) $(TARGET)


.PHONY: all clean
