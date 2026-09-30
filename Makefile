#Makefile
CC := gcc
CFLAGS := -Wall -Wextra -g
BUILD_DIR = build
DOCDIR := doc

TARGET := compiler

INCLUDE_DIR := src
TEST_DIR := tests
TEST_OUT_DIR := $(BUILD_DIR)/test_out
SOURCE_DIR := src


SRCS := $(wildcard $(SOURCE_DIR)/*.c)
OBJS := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
HEADERS := $(wildcard $(INCLUDE_DIR)/*.h)

TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRCS))
TEST_DEP_OBJS := $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ 

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADERS) | dir_build
	$(CC) $(CFLAGS)  -c $< -o $@


format:
	@clang-format -i $(SRCS) $(HEADERS)

dir_build:
	@mkdir -p $(BUILD_DIR)

docs :
	doxygen

.PHONY: all clean  dir_build  docs format

clean:
	rm -rf $(BUILD_DIR) $(TARGET)  $(DOCDIR)