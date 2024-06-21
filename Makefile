CC = gcc

PREFIX = /usr/local

SRC_DIR = src
OBJ_DIR = obj
LIB_DIR = lib
INC_DIR = include

EXECUTABLE = Ll-connect
MAIN = src/main.c
SRC := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c))
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o,$(SRC))
LIB := $(patsubst $(SRC_DIR)/%.c, $(LIB_DIR)/lib%.so,$(SRC))

all: bin/$(EXECUTABLE)

bin/$(EXECUTABLE): $(LIB_DIR)/libui.so $(LIB_DIR)/libcontroller.so $(MAIN)
	gcc -g -I$(INC_DIR) -L$(LIB_DIR) -Wl,-rpath,'$$ORIGIN/../$(LIB_DIR)' -lcontroller -lui -lSDL2 -o bin/$(EXECUTABLE) $(MAIN)
	mkdir -p $(HOME)/.config/Ll-connect-config

$(LIB_DIR)/libui.so: $(OBJ_DIR)/ui.o
	$(CC) -g -shared -o $@ $< -lSDL2 -lSDL2_ttf

$(LIB_DIR)/libcontroller.so: $(OBJ_DIR)/controller.o
	$(CC) -g -shared -o $@ $<

$(OBJ_DIR)/ui.o: $(SRC_DIR)/ui.c
	$(CC) -g -c -fPIC $< -o $@

$(OBJ_DIR)/controller.o: $(SRC_DIR)/controller.c
	$(CC) -g -c -fPIC $< -o $@

install: bin/$(EXECUTABLE)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp bin/$(EXECUTABLE) $(DESTDIR)$(PREFIX)/bin/
	mkdir -p $(DESTDIR)$(PREFIX)/lib
	mkdir -p $(DESTDIR)$(PREFIX)/include
	cp $(LIB_DIR)/libcontroller.so $(DESTDIR)$(PREFIX)/lib/
	cp $(LIB_DIR)/libui.so $(DESTDIR)$(PREFIX)/lib/

clean:
	rm -f $(OBJ) $(LIB) bin/$(EXECUTABLE)
clean_install:
	rm -f $(DESTDIR)$(PREFIX)/lib/libcontroller.so 
	rm -f $(DESTDIR)$(PREFIX)/lib/libui.so
	rm -f $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)


.PHONY: all clean install clean_install
