/**
 * @file dyn_buff.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní dynamického bufferu pro řetězce.
 *
 * Buffer se podle potřeby zvětšuje. Používá ho lexer při sestavování lexémů a
 * generátor při skládání výstupního kódu.
 *
 * @author Matěj Okruhlica (xokruhm00)
 */

#ifndef DYNBUFF_H
#define DYNBUFF_H
#include <stdlib.h>

/**
 * @brief Dynamický buffer pro řetězec
 *
 * Obsah není ukončený nulovým znakem, platných je prvních `length` znaků.
 */
typedef struct {
  /** Ukazatel na data (buffer je vlastní) */
  char *data;
  /** Počet uložených znaků */
  size_t length;
  /** Kapacita pole `data` ve znacích */
  size_t capacity;
} StringBuff;

/**
 * @brief Inicializuje buffer
 *
 * Alokuje pole `data` o zadané kapacitě (nula znamená výchozích 16 znaků) a
 * nastaví délku na nulu. Při nedostatku paměti program skončí s kódem 99.
 *
 * @param buf buffer, který se inicializuje
 * @param initial_capacity počáteční kapacita ve znacích
 */
void sb_init(StringBuff *buf, size_t initial_capacity);

/**
 * @brief Připojí znak na konec bufferu
 *
 * Podle potřeby kapacitu zdvojnásobí. Při selhání alokace program skončí s
 * kódem 99.
 *
 * @param buf buffer
 * @param c přidávaný znak
 */
void sb_append_char(StringBuff *buf, char c);

/**
 * @brief Vyprázdní buffer
 *
 * Nastaví délku na nulu, paměť zůstává alokovaná pro další použití.
 *
 * @param buf buffer
 */
static inline void sb_reset(StringBuff *buf) { buf->length = 0; }

/**
 * @brief Uvolní paměť bufferu
 *
 * Ukazatel `data` se nastaví na NULL. Buffer lze znovu použít až po sb_init().
 *
 * @param buf buffer
 */
static inline void sb_free(StringBuff *buf) {
  free(buf->data);
  buf->data = NULL;
};

/**
 * @brief Vytvoří kopii obsahu bufferu jako řetězec
 *
 * Kopie je ukončená nulovým znakem a leží na haldě, uvolňuje ji volající
 * (free). Při nedostatku paměti program skončí s kódem 99.
 *
 * @param buf kopírovaný buffer
 * @return ukazatel na kopii, pro `buf` rovné NULL vrací NULL
 */
const char *sb_copy(const StringBuff *buf);

/**
 * @brief Odstraní bílé znaky z konce bufferu
 *
 * Délku bufferu nejdřív zkrátí o dva znaky. Funkce se zatím nikde nepoužívá.
 *
 * @param buff buffer
 */
void sb_remove_trailing_whitespace(StringBuff *buff);

/**
 * @brief Připojí řetězec na konec bufferu
 *
 * Řetězec NULL se ignoruje.
 *
 * @param buf buffer
 * @param str řetězec ukončený nulovým znakem
 */
void sb_append_str(StringBuff *buf, const char *str);
#endif
