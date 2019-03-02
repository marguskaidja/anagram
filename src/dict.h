#ifndef AGR_DICT_H
#define AGR_DICT_H

#include "global.h"

typedef struct {
    unsigned char * buf;
    size_t buf_len;
} agr_dict_t;

void agr_dict_load (const char *, agr_dict_t *);
void agr_dict_unload (agr_dict_t *);


#endif
