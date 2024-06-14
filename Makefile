CC := gcc
CFLAGS := -g3 -ggdb
RM := rm -fr

BUILD_DIR := ./build
INCLUDE_DIR := ./src
SOURCE_DIR := ./src
OUT_DIR := ./out

default: all

all: main



$(BUILD_DIR)/main.o: $(SOURCE_DIR)/main.c 
	$(CC) -c -I$(INCLUDE_DIR) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/logger.o: $(SOURCE_DIR)/logger.c 
	$(CC) -c -I$(INCLUDE_DIR) $(CFLAGS) $^ -o $@


main: $(BUILD_DIR)/main.o $(BUILD_DIR)/logger.o
	$(CC) $^ -o $(OUT_DIR)/$@


clean:
	$(RM) $(OUT_DIR)/* $(BUILD_DIR)/*