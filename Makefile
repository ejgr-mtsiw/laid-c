CC			:= clang
CPPFLAGS		:= -Wall -Wextra -Werror -pedantic-errors
LDFLAGS			:= -L/usr/lib -L/usr/local/lib -lm -lhdf5
BUILD			:= ./bin
OBJ_DIR			:= $(BUILD)/objects
APP_DIR			:= $(BUILD)
TARGET			:= create-hdf5-dataset
INCLUDE			:= -Iinclude/ -I/usr/local/include/
SRC			:= $(wildcard src/*.c)

OBJECTS			:= $(SRC:%.c=$(OBJ_DIR)/%.o)
DEPENDENCIES		:= $(OBJECTS:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release info

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CPPFLAGS += -DDEBUG -g
debug: all

release: CPPFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
