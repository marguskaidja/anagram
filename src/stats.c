#include "global.h"
#include "dict.h"
#include "finder.h"
#include "inputword.h"





int main (int argc, char *argv[])
{
    struct timeval tv1, tv2;
    unsigned int ncores;
    agr_dict_t dictionary = {};
    char *buf;
    size_t offs = 0;

    // Start timing
    gettimeofday(&tv1,NULL);

    if (argc != 2) {
        printf ("Syntax: %s <dictionary_file>\n", argv[0]);
        exit (1);
    }

    agr_dict_load (argv[1], &dictionary);

    ncores = get_nprocs();

    // Loop over all words and resolve it's anagrams
    buf = dictionary.buf;

    while (offs < dictionary.buf_len) {
        agr_finder_result_t *results;
        size_t word_start, word_len;
        agr_inputword_t inputword;

        word_start = offs;
        
        // Search word boundary
        do {
            if (buf[offs] == '\r' || buf[offs] == '\n') {
                break;
            }

            offs++;
        } while (offs < dictionary.buf_len);

        word_len = offs - word_start;

        if (word_len) {
            char * input_str;
            int anagram_cnt = 0;

            memset (&inputword, 0, sizeof (agr_inputword_t));

            input_str = strndup (&buf[word_start], word_len);

            agr_inputword_parse (input_str, &inputword);

            printf ("%s\t", input_str);

            results = agr_finder_find (&dictionary, &inputword, ncores);

            if (results) {
                agr_finder_result_t * m = results;

                do {
                    printf ("%.*s", m->buf_len, m->buf);

                    if (m->prev) {
                        printf (",");
                    }

                    m = m->prev;
                    anagram_cnt++;
                } while (m);

                agr_finder_freeresults (results);
            }

            printf ("\t%i\n", anagram_cnt);

            free (input_str);
        }
        
        if (offs < dictionary.buf_len) {
            //printf ("%c", buf[offs]);
        }
        
        offs++;
    }


    // End timing
    gettimeofday(&tv2,NULL);

    // Print results
    print_duration (&tv1, &tv2);
    printf ("\n");


    return 0;
}
