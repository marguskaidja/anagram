#include "dict.h"

void agr_dict_load (const char * filename, agr_dict_t * dict)
{
    size_t i;
    int dict_fd = open (filename, O_RDONLY | O_NOATIME);

    if (dict_fd == -1) {
        BAILOUT("%s", filename);
    }

    dict->buf_len = lseek (dict_fd, 0, SEEK_END);
    lseek (dict_fd, 0, SEEK_SET);

    dict->buf = mmap (NULL, dict->buf_len, PROT_READ, MAP_PRIVATE | MAP_POPULATE, dict_fd, 0);

    if (!dict->buf) {
        BAILOUT("mmap() failed");
    }

    close (dict_fd);
}


void agr_dict_unload (agr_dict_t * dict)
{
    if (dict->buf) {
        munmap (dict->buf, dict->buf_len);
        dict->buf = NULL;
    }
}
