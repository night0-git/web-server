CC ?= clang
CFLAGS := -Wall -Wextra -MMD -MP

TARGET := bin/web-server
BUILD_DIR := build
SRC_DIR := src

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: clean run debug all

all: release

debug: CFLAGS += -g -O0
release: CFLAGS += -O3

debug release: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(dir $(TARGET))

-include $(DEPS)