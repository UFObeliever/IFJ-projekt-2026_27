#ifndef DYNBUFF_H
#define DYNBUFF_H
#include <stdlib.h>

typedef struct {
  char *data;      // pointer to data (owns)
  size_t length;   // length of the string
  size_t capacity; // capacity for string
} StringBuff;

/**
 * @brief Funkce incializuje buffer
 *
 * @param buf: pointer na buffer
 * @param initial_capacity: počáteční velikost bufferu
 */
void sb_init(StringBuff *buf, size_t initial_capacity);

/**
 * @brief Funkce ukládájící znak do bufferu
 *
 * @param buf: pointer na buffer
 * @param c: charakter
 */
void sb_append_char(StringBuff *buf, char c);

static inline void sb_reset(StringBuff *buf) { buf->length = 0; }

static inline void sb_free(StringBuff *buf) {
  free(buf->data);
  buf->data = NULL;
};

const char *sb_copy(const StringBuff *buf);

/**
 * @brief Funkce odstraňující whitespace
 *
 * @param buf: pointer na buffer
 */
void sb_remove_trailing_whitespace(StringBuff *buff);

/**
 * @brief Funkce ukládájící string do bufferu
 *
 * @param buf: pointer na buffer
 * @param c: charakter
 */
void sb_append_str(StringBuff *buf, const char *str);
#endif
