#ifndef AGR_INPUTWORD_H
#define AGR_INPUTWORD_H

#include "global.h"

#define AGR_INPUTWORD_FREQTYPE      unsigned char

typedef struct {
    unsigned int len;
    AGR_INPUTWORD_FREQTYPE freq_table[256];
} agr_inputword_t;

void agr_inputword_parse (const char *, agr_inputword_t *);


#endif
