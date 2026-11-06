CC := clang
CFLAGS := -std=gnu17 -Wall -Wextra -MMD -MP

TARGET := bin/web-server
BUILD_DIR := build
SRC_DIR := src

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(TARGET)
all: CFLAGS += -O3

debug: $(TARGET)
debug: CFLAGS += -g -O0

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(dir $(TARGET))

-include $(DEPS)

.PHONY: clean debug all
