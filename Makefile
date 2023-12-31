CC = gcc

SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
INC_DIR = include

TARGET = bin/run
MAIN = src/main.c
SRC := $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(SRC))
LIB := $(patsubst $(SRC_DIR)/%.c, $(LIB_DIR)/lib%.so,$(SRC))


all: $(TARGET)

$(TARGET): $(LIB) $(MAIN)
	gcc -g -L$(LIB_DIR) -Wl,-rpath,'$$ORIGIN/../$(LIB_DIR)' -lui -lcontroller -o bin/run src/main.c -I$(INC_DIR)

$(LIB_DIR)/lib%.so: $(OBJ_DIR)/%.o
	$(CC) -shared -o $@ $< -lusb-1.0 -lSDL2 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c -fPIC $< -o $@

clean:
	rm -rf $(OBJ_DIR)/* $(LIB) $(TARGET)


.PHONY: all clean
