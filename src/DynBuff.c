#include "DynBuff.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define SB_DEFAULT_INITIAL_CAPACITY 16

void sb_init(StringBuff *buf, size_t initial_capacity) {
  if (initial_capacity == 0) {
    initial_capacity = SB_DEFAULT_INITIAL_CAPACITY;
  }
  buf->data = (char *)calloc(initial_capacity, sizeof(char));
  if (buf->data == NULL) {
    perror("Error: Allocation memory failed");
    buf->capacity = 0;
    buf->length = 0;
    exit(99);
  }

  buf->capacity = initial_capacity;
  buf->length = 0;
}

void sb_append_char(StringBuff *buf, char c) {
  if (buf->length + 1 >= buf->capacity) {
    size_t new_capacity = buf->capacity * 2;

    char *new_data = (char *)realloc(buf->data, new_capacity);
    if (new_data == NULL) {
      perror("Error: Memory reallocation failure");
      exit(99);
    }
    buf->data = new_data;
    buf->capacity = new_capacity;
  }

  buf->data[buf->length] = c;
  buf->length++;
}

const char *sb_copy(const StringBuff *buf) {
  if (buf == NULL) {
    return NULL;
  };
  char *Copye = (char *)malloc(buf->length + 1);
  if (Copye == NULL) {
    exit(99);
  };
  memcpy(Copye, buf->data, buf->length);
  Copye[buf->length] = '\0';
  return (const char *)Copye;
}

void sb_remove_trailing_whitespace(StringBuff *buff) {
  buff->length -= 2;
  const size_t orig_len = buff->length;
  for (size_t i = 0; i < orig_len; i++) {

    if (buff->data[orig_len - i] == '\n') {
      return;
    }

    if (isspace(buff->data[orig_len - i])) {
      buff->length--;
      continue;
    }

    return;
  }
}

void sb_append_str(StringBuff *buf, const char *str) {
  if (str == NULL) {
    return;
  }
  while (*str) {
    sb_append_char(buf, *str++);
  }
}
