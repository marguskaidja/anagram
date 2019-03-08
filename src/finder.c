#include "finder.h"

#define WORKER_MIN_BUFLEN 16

typedef struct {
    unsigned char * buf;
    size_t buf_len;
    agr_inputword_t * word;
    agr_finder_result_t **results;
    pthread_mutex_t *results_sync_mutex;
} agr_finder_workerproc_arg_t;

// Size is 32bit or 64bit, depending of architecture
typedef unsigned long apr_scanblock_t;

// Detect machine endianess
static unsigned short __dummy = 0x0001;
static unsigned char *check_endianness = (char*)&__dummy;

inline static void check_line (agr_finder_workerproc_arg_t * worker_arg, unsigned char *ln_start, size_t ln_len, AGR_INPUTWORD_FREQTYPE * freq_copy, int restore_freq_copy)
{
    if (ln_len) {
        if (ln_start[ln_len-1] == '\r') {
            ln_len--;
        }

        if (ln_len == worker_arg->word->len) {
            size_t pos;
            
            unsigned char * tmpp = ln_start;
            unsigned char ch;

            for (pos = 0; pos < ln_len; pos++) {
                ch = ln_start[pos];

#ifdef CASE_INSENSITIVE_SEARCH
                ch = my_tolower (ch);
#endif

                if (freq_copy[ch] == 0) {
                    ln_start = NULL;
                    break;
                }

                freq_copy[ch]--;
            }

            if (restore_freq_copy) {
                while (pos > 0) {
                    pos--;
                    
                    ch = tmpp[pos];

#ifdef CASE_INSENSITIVE_SEARCH
                    ch = my_tolower (ch);
#endif

                    freq_copy[ch]++;
                }
            }

            if (ln_start) {
                // Alloc structure for match
                agr_finder_result_t * new_result = malloc (sizeof (agr_finder_result_t));

                if (!new_result) {
                    BAILOUT ("malloc(%i) failed", sizeof (agr_finder_result_t));
                }
        
                new_result->buf = ln_start;
                new_result->buf_len = ln_len;
        
                pthread_mutex_lock (worker_arg->results_sync_mutex);
        
                new_result->prev = *worker_arg->results;
                *worker_arg->results = new_result;
        
                pthread_mutex_unlock (worker_arg->results_sync_mutex);
            }
        }
    }    
}

static void *agr_finder_workerproc (agr_finder_workerproc_arg_t * worker_arg)
{
    AGR_INPUTWORD_FREQTYPE freq_copy[256];    
    size_t offs = 0;
    size_t aligned_boundary;
    unsigned char * ln_start = worker_arg->buf;
    apr_scanblock_t * scanbuf = (apr_scanblock_t*)worker_arg->buf;
    unsigned char * buf_end = worker_arg->buf + worker_arg->buf_len - 1;
    size_t ln_len;
    int is_little_endian = *check_endianness;

    memcpy (freq_copy, worker_arg->word->freq_table, sizeof (worker_arg->word->freq_table));

    // Calc boundary in which data blocks can be loaded to CPU registry with single instruction
    aligned_boundary = worker_arg->buf_len - (worker_arg->buf_len % (sizeof (apr_scanblock_t)));

    do {
        apr_scanblock_t block;
        int nbyte = 0;

        // If offs is within boundary, then load data into CPU register with single instruction
        if (offs < aligned_boundary) {
            block = *scanbuf;
        } else {

            // Buffer exhausted
            if (offs >= worker_arg->buf_len) {
                break;
            }

            // Otherwise use memcpy to transfer at most sizeof (apr_scanblock_t)-1 bytes.
            //
            // It's slower, but executed only once when we process the trailing data.
            block = 0;
            memcpy (&block, scanbuf, worker_arg->buf_len - offs);
        }

        // Use bitwise arithmetic to quickly find LF characters in the block:
        // All bytes which weren't 0x0A the beginning, will be zeroed after following operations.
        // All nonzero bytes in block are LF (0x0A) characters.
        // (https://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord)
        block ^= 0x0A0A0A0A0A0A0A0AULL;
        block  = (block - 0x0101010101010101ULL) & ~block & 0x8080808080808080ULL;

        // Iterate until one or more byte in block is non-zero
        while (block) {

            // Count the number of zero bits to findout the exact position of
            // the next LF character.
            int num_zero_bytes = (is_little_endian ? __builtin_ctzll (block) : __builtin_clzll (block));

            // Divide by 8 (shr 3) to get the actual byte position
            num_zero_bytes >>= 3;

            nbyte += num_zero_bytes;

            if (nbyte < sizeof (apr_scanblock_t)) {    
                ln_len = ((size_t)scanbuf - (size_t)ln_start) + nbyte;

                check_line (worker_arg, ln_start, ln_len, freq_copy, 1);

                nbyte++;
                ln_start = ((unsigned char*)scanbuf) + nbyte;

                // Shift all processed bytes out of the block so
                // we can query next LF position.
                num_zero_bytes = ((num_zero_bytes + 1) << 3);

                if (is_little_endian) {
                    block >>= num_zero_bytes;
                } else {
                    block <<= num_zero_bytes;
                }

            } else {
                // No more LF characters in this block
                break;
            }
        }

        offs += sizeof (apr_scanblock_t);
        scanbuf++;
    } while (1);

    if (ln_start <= buf_end) {
        check_line (worker_arg, ln_start, buf_end - ln_start + 1, freq_copy, 0);
    }

    return NULL;
}

inline static size_t length_to_lf (unsigned char *buf, size_t buf_len)
{
    apr_scanblock_t * scanbuf = (apr_scanblock_t*)buf;
    size_t aligned_boundary = buf_len - (buf_len % (sizeof (apr_scanblock_t)));
    size_t offs;
    int is_little_endian = *check_endianness;

    for (offs = 0; offs < aligned_boundary; offs += sizeof (apr_scanblock_t), scanbuf++) {
        apr_scanblock_t block = *scanbuf ^ 0x0A0A0A0A0A0A0A0AULL;

        block = (block - 0x0101010101010101ULL) & ~block & 0x8080808080808080ULL;

        if (block) {
            int num_zero_bytes = (is_little_endian ? __builtin_ctzll (block) : __builtin_clzll (block));

            return offs + (num_zero_bytes >> 3) + 1;
        }
    }

    for (buf = (unsigned char*)scanbuf; offs < buf_len; offs++, buf++) {
        if (*buf == '\n') {
            return offs + 1;
        }
    }

    return buf_len;
}

agr_finder_result_t *agr_finder_find (agr_dict_t *dict, agr_inputword_t *word, unsigned int max_workers)
{
    int i, nworkers;
    size_t worker_default_buf_len;
    pthread_t * threads = NULL;
    pthread_mutex_t results_sync_mutex = PTHREAD_MUTEX_INITIALIZER;
    agr_finder_result_t * results = NULL;
    unsigned char * buf;
    unsigned char * buf_end;
    size_t offs;
    int pthread_errno;

    if (!dict->buf_len) {
        return NULL;
    }

    worker_default_buf_len = (dict->buf_len / max_workers) + ((dict->buf_len % max_workers) / max_workers);

    // Give at least WORKER_MIN_BUFLEN bytes per worker
    if (worker_default_buf_len < WORKER_MIN_BUFLEN) {
        max_workers = MAX(dict->buf_len / WORKER_MIN_BUFLEN,1);
        worker_default_buf_len = WORKER_MIN_BUFLEN;
    } else if (max_workers == 0) {
        max_workers = 1;
    }

    // Keep first worker in main thread and the rest of the workers in separate threads
    if (max_workers > 1) {
        threads =  malloc ((max_workers - 1) * sizeof (pthread_t));

        if (!threads) {
            BAILOUT ("malloc(%i) failed", (max_workers - 1) * sizeof (pthread_t));
        }
    }

    agr_finder_workerproc_arg_t * worker_args = malloc (max_workers * sizeof (agr_finder_workerproc_arg_t));

    if (!worker_args) {
        BAILOUT ("malloc(%i) failed", max_workers * sizeof (agr_finder_workerproc_arg_t));
    }

    buf = dict->buf;
    buf_end = buf + dict->buf_len - 1;
    offs = 0;

    for (i = 0; i < max_workers && offs < dict->buf_len; i++) {

        worker_args[i].buf = &buf[offs];

        if (i == max_workers - 1) {
            worker_args[i].buf_len = dict->buf_len - offs;
        } else {
            // Seek to the next LF to avoid splitting
            // the buffer in the middle of word
            size_t offs_lf_search_start = offs + worker_default_buf_len - 1;

            if (offs_lf_search_start >= dict->buf_len) {
                worker_args[i].buf_len = dict->buf_len - offs;
            } else {
                worker_args[i].buf_len = offs_lf_search_start - offs + length_to_lf (&buf[offs_lf_search_start], dict->buf_len - offs_lf_search_start);
            }

            offs += worker_args[i].buf_len;
        }

        worker_args[i].word = word;
        worker_args[i].results_sync_mutex = &results_sync_mutex;
        worker_args[i].results = &results;

        if (i > 0) {
            if ( (pthread_errno = pthread_create (&threads[i-1], NULL, (void *(*)(void *))&agr_finder_workerproc, &worker_args[i])) != 0) {
                BAILOUT_ERRNO (pthread_errno, "pthread_create() failed");
            }
        }
    }

    nworkers = i;

    agr_finder_workerproc (&worker_args[0]);

    for (i = 0; i < nworkers-1; i++) {
        if ( (pthread_errno = pthread_join (threads[i], NULL)) != 0) {
            BAILOUT_ERRNO (pthread_errno, "pthread_join() failed");
        }
    }

    free (worker_args);
    free (threads);

    return results;
}

void agr_finder_freeresults (agr_finder_result_t * results)
{
    while (results) {
        agr_finder_result_t *tmp = results;
        results = results->prev;
        free (tmp);
    }
}
