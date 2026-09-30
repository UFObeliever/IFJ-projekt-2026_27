#Makefile
CC := gcc
CFLAGS := -Wall -Wextra -g
BUILD_DIR = build
DOCDIR := doc

TARGET := compiler

INCLUDE_DIR := .
#TEST_DIR := tests
#TEST_COMMON_DIR := $(TEST_DIR)/test_files
#TEST_OUT_DIR := $(BUILD_DIR)/test_out
SOURCE_DIR := .


SRCS := $(wildcard $(SOURCE_DIR)/*.c)
OBJS := $(patsubst $(SOURCE_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
HEADDERS := $(wildcard $(INCLUDE_DIR)/*.h)

#TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)
#TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.o, $(TEST_SRCS))
#TEST_DEP_OBJS := $(filter-out $(BUILD_DIR)/main.o, $(OBJS))

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ 

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(HEADDERS) | dir_build
	$(CC) $(CFLAGS)  -c $< -o $@ $(LDFLAGS)

##$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c $(HEADDERS) | dir_build
#	$(CC) $(CFLAGS) -c $< -o $@ 

#test_lexer: $(BUILD_DIR)/lexer_test.o $(TEST_DEP_OBJS) | dir_test
#	$(CC) $(CFLAGS)  $(TEST_DEP_OBJS) $<  -o $(BUILD_DIR)/lexer_test $(LDFLAGS)
#	./$(BUILD_DIR)/lexer_test < $(TEST_COMMON_DIR)/lexer_test_in.txt > $(TEST_OUT_DIR)/lexer_test_current_out.txt
#	diff --strip-trailing-cr $(TEST_OUT_DIR)/lexer_test_current_out.txt $(TEST_COMMON_DIR)/lexer_test_expected_out.txt

dir_build:
	@mkdir -p $(BUILD_DIR)

#dir_test: dir_build
#	@mkdir -p $(TEST_OUT_DIR)

docs :
	doxygen

.PHONY: all clean  dir_build  docs

clean:
	rm -rf $(BUILD_DIR) $(TARGET)  $(DOCDIR)