#include "inputword.h"


void agr_inputword_parse (const char * str, agr_inputword_t * word)
{
    memset (word->freq_table, 0, sizeof (word->freq_table));

    for (word->len = 0; str[word->len] != '\0'; word->len++) {
        unsigned char ch;

#ifdef CASE_INSENSITIVE_SEARCH
        ch = my_tolower (str[word->len]);
#else        
        ch = str[word->len];
#endif

        word->freq_table[ch]++;
    }
}
