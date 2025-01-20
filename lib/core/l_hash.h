#ifndef _L_HASH_H
#define _L_HASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HASH_KEY_STRING     (-1)

typedef struct hash_entry_t hash_entry_t;
typedef struct hash_t hash_t;
typedef struct hash_index_t hash_index_t;
typedef unsigned int (*hashfunc_t)(const char *key, int *klen);

struct hash_entry_t 
{
    hash_entry_t    *next;
    unsigned int        hash;
    const void          *key;
    int                 klen;
    const void          *val;
};

struct hash_index_t 
{
    hash_t          *ht;
    hash_entry_t    *this, *next;
    unsigned int        index;
};

struct hash_t 
{
    hash_entry_t    **array;
    hash_index_t    iterator;  /* For ogs_hash_first(NULL, ...) */
    unsigned int        count, max, seed;
    hashfunc_t      hash_func;
    hash_entry_t    *free;  /* List of recycled entries */
};


unsigned int _hashfunc_default(const char *key, int *klen);

hash_t *hash_make(void);
hash_t *hash_make_custom(hashfunc_t ogs_hash_func);
void hash_destroy(hash_t *ht);

#define hash_set(ht, key, klen, val) \
    hash_set_debug(ht, key, klen, val, __FILE__)
void hash_set_debug(hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line);
#define hash_get(ht, key, klen) \
    hash_get_debug(ht, key, klen, __FILE__)
void *hash_get_debug(hash_t *ht,
        const void *key, int klen, const char *file_line);
#define hash_get_or_set(ht, key, klen, val) \
    hash_get_or_set_debug(ht, key, klen, val, __FILE__)
void *hash_get_or_set_debug(hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line);

hash_index_t *hash_first(hash_t *ht);
hash_index_t *hash_next(hash_index_t *hi);
void hash_this(hash_index_t *hi, 
        const void **key, int *klen, void **val);

const void* hash_this_key(hash_index_t *hi);
int hash_this_key_len(hash_index_t *hi);
void* hash_this_val(hash_index_t *hi);
unsigned int hash_count(hash_t *ht);
void hash_clear(hash_t *ht);

typedef int (hash_do_callback_fn_t)(
        void *rec, const void *key, int klen, const void *value);

int hash_do(hash_do_callback_fn_t *comp,
        void *rec, const hash_t *ht);

#ifdef __cplusplus
}
#endif

#endif