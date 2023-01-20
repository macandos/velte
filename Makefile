CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -Wunused-parameter
BUILD_DIR := ./build
SRC_DIR := ./src
SRC := $(shell ls -I '*.h' $(SRC_DIR))
OBJ := $(SRC:%.c=$(BUILD_DIR)/%.o)

velte: $(OBJ)
	$(CC) $(OBJ) -o $@ 

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)