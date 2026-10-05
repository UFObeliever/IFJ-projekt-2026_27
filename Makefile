#Makefile
CC := gcc
CFLAGS := -Wall -Wextra  -DNDEBUG -O3 #release
DEBUG_FLAGS := -Wall -Wextra  -g
BUILD_DIR = build
DOCDIR := doc

TARGET := compiler
DEBUG_TARGET := compiler-debug
INCLUDE_DIR := src
TEST_DIR := tests
TEST_OUT_DIR := $(BUILD_DIR)/test_out
SOURCE_DIR := src


SRCS := $(wildcard $(SOURCE_DIR)/*.c)
OBJS := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEBUG_OBJS := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/debug/%.o, $(SRCS))
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_DEP_OBJS := $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

all: $(TARGET)

debug: $(DEBUG_TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ 

$(DEBUG_TARGET): $(DEBUG_OBJS) | dir_debug
	$(CC) $(DEBUG_CFLAGS) $(DEBUG_OBJS) -o $@ 

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) | dir_build
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/debug/%.o: $(SOURCE_DIR)/%.c $(HEADERS) | dir_debug
	$(CC) $(DEBUG_CFLAGS) -c $< -o $@


format:
	@clang-format -i $(SRCS) $(HEADERS)

dir_build:
	@mkdir -p $(BUILD_DIR)

dir_debug: | dir_build
	@mkdir -p $(BUILD_DIR)/debug

tidy-check:
	@clang-tidy $(SRCS) -- $(CFLAGS)

tidy-fix:
	@clang-tidy $(SRCS) --fix -- $(CFLAGS)

docs :
	doxygen

.PHONY: all clean  dir_build  docs format tidy-check tidy-fix

clean:
	rm -rf $(BUILD_DIR) $(TARGET)  $(DOCDIR) $(DEBUG_TARGET)