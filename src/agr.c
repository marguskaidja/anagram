#include "global.h"
#include "dict.h"
#include "finder.h"
#include "inputword.h"


int main (int argc, char *argv[])
{
    struct timeval tv1, tv2;

    gettimeofday(&tv1,NULL);

    unsigned int ncores;
    agr_inputword_t inputword;
    agr_dict_t dictionary;
    agr_finder_result_t *results;


    if (argc != 3) {
        printf ("Syntax: %s <dictionary_file> <word_to_search>\n", argv[0]);
        return 1;
    }

    agr_inputword_parse (argv[2], &inputword);

    agr_dict_load (argv[1], &dictionary);

    ncores = get_nprocs();

    results = agr_finder_find (&dictionary, &inputword, ncores);

    gettimeofday(&tv2,NULL);

    print_duration (&tv1, &tv2);

    if (results) {
        agr_finder_result_t * m = results;
        
        do {
            printf (",%.*s", m->buf_len, m->buf);
            m = m->prev;
        } while (m);

        agr_finder_freeresults (results);
    }

    printf ("\n");

    return 0;
}
