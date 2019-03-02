#ifndef AGR_FINDER_H
#define AGR_FINDER_H

#include "global.h"
#include "dict.h"
#include "inputword.h"

typedef struct {
    unsigned char * buf;
    size_t buf_len;
    void * prev;
} agr_finder_result_t;


agr_finder_result_t *agr_finder_find (agr_dict_t *, agr_inputword_t *, unsigned int);
void agr_finder_freeresults (agr_finder_result_t *);

#endif
