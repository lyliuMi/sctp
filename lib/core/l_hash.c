#include "l_hash.h"
#include "l_timer.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define INITIAL_MAX 15 /* tunable == 2^n - 1 */

static hash_entry_t **alloc_array(hash_t *ht, unsigned int max)
{
    hash_entry_t **ptr = calloc(1, sizeof(*ht->array) * (max + 1));
    assert(ptr);
    return ptr;
}

hash_t *hash_make(void)
{
    hash_t *ht;
    time_t now = get_monotonic_time();

    ht = malloc(sizeof(hash_t));
    if (!ht) {
        fprintf(stderr, "malloc() failed");
        return NULL;
    }

    ht->free = NULL;
    ht->count = 0;
    ht->max = INITIAL_MAX;
    ht->seed = (unsigned int)((now >> 32) ^ now ^ 
                              (uintptr_t)ht ^ (uintptr_t)&now) - 1;
    ht->array = alloc_array(ht, ht->max);
    ht->hash_func = NULL;

    return ht;
}

hash_t *hash_make_custom(hashfunc_t hash_func)
{
    hash_t *ht = hash_make();
    if (!ht) {
        fprintf(stderr, "hash_make() failed");
        return NULL;
    }
    ht->hash_func = hash_func;
    return ht;
}

void hash_destroy(hash_t *ht)
{
    hash_entry_t *he = NULL, *next_he = NULL;

    assert(ht);
    assert(ht->array);

    hash_clear(ht);

    he = ht->free;
    while(he) {
        next_he = he->next;

        free(he);
        he = next_he;
    }

    free(ht->array);
    free(ht);
}

hash_index_t *hash_next(hash_index_t *hi)
{
    assert(hi);

    hi->this = hi->next;
    while (!hi->this) {
        if (hi->index > hi->ht->max)
            return NULL;

        hi->this = hi->ht->array[hi->index++];
    }
    hi->next = hi->this->next;
    return hi;
}

hash_index_t *hash_first(hash_t *ht)
{
    hash_index_t *hi;

    assert(ht);

    hi = &ht->iterator;

    hi->ht = ht;
    hi->index = 0;
    hi->this = NULL;
    hi->next = NULL;
    return hash_next(hi);
}

void hash_this(hash_index_t *hi,
        const void **key, int *klen, void **val)
{
    assert(hi);

    if (key)  *key  = hi->this->key;
    if (klen) *klen = hi->this->klen;
    if (val)  *val  = (void *)hi->this->val;
}

const void *hash_this_key(hash_index_t *hi)
{
    const void *key;

    hash_this(hi, &key, NULL, NULL);
    return key;
}

int hash_this_key_len(hash_index_t *hi)
{
    int klen;

    hash_this(hi, NULL, &klen, NULL);
    return klen;
}

void *hash_this_val(hash_index_t *hi)
{
    void *val;

    hash_this(hi, NULL, NULL, &val);
    return val;
}

static void expand_array(hash_t *ht)
{
    hash_index_t *hi;
    hash_entry_t **new_array;
    unsigned int new_max;

    new_max = ht->max * 2 + 1;
    new_array = alloc_array(ht, new_max);
    for (hi = hash_first(ht); hi; hi = hash_next(hi)) {
        unsigned int i = hi->this->hash & new_max;
        hi->this->next = new_array[i];
        new_array[i] = hi->this;
    }
    free(ht->array);
    ht->array = new_array;
    ht->max = new_max;
}

static unsigned int hashfunc_default(
        const char *char_key, int *klen, unsigned int hash)
{
    const unsigned char *key = (const unsigned char *)char_key;
    const unsigned char *p;
    int i;
    
    /*
     * This is the popular `times 33' hash algorithm which is used by
     * perl and also appears in Berkeley DB. This is one of the best
     * known hash functions for strings because it is both computed
     * very fast and distributes very well.
     *
     * The originator may be Dan Bernstein but the code in Berkeley DB
     * cites Chris Torek as the source. The best citation I have found
     * is "Chris Torek, Hash function for text in C, Usenet message
     * <27038@mimsy.umd.edu> in comp.lang.c , October, 1990." in Rich
     * Salz's USENIX 1992 paper about INN which can be found at
     * <http://citeseer.nj.nec.com/salz92internetnews.html>.
     *
     * The magic of number 33, i.e. why it works better than many other
     * constants, prime or not, has never been adequately explained by
     * anyone. So I try an explanation: if one experimentally tests all
     * multipliers between 1 and 256 (as I did while writing a low-level
     * data structure library some time ago) one detects that even
     * numbers are not useable at all. The remaining 128 odd numbers
     * (except for the number 1) work more or less all equally well.
     * They all distribute in an acceptable way and this way fill a hash
     * table with an average percent of approx. 86%.
     *
     * If one compares the chi^2 values of the variants (see
     * Bob Jenkins ``Hashing Frequently Asked Questions'' at
     * http://burtleburtle.net/bob/hash/hashfaq.html for a description
     * of chi^2), the number 33 not even has the best value. But the
     * number 33 and a few other equally good numbers like 17, 31, 63,
     * 127 and 129 have nevertheless a great advantage to the remaining
     * numbers in the large set of possible multipliers: their multiply
     * operation can be replaced by a faster operation based on just one
     * shift plus either a single addition or subtraction operation. And
     * because a hash function has to both distribute good _and_ has to
     * be very fast to compute, those few numbers should be preferred.
     *
     *                  -- Ralf S. Engelschall <rse@engelschall.com>
     */

    if (*klen == HASH_KEY_STRING) {
        for (p = key; *p; p++) {
            hash = hash * 33 + *p;
        }
        *klen = p - key;
    }
    else {
        for (p = key, i = *klen; i; i--, p++) {
            hash = hash * 33 + *p;
        }
    }

    return hash;
}

unsigned int _hashfunc_default(const char *char_key, int *klen)
{
    return hashfunc_default(char_key, klen, 0);
}

static hash_entry_t **find_entry(hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    hash_entry_t **hep, *he;
    unsigned int hash;

    if (ht->hash_func)
        hash = ht->hash_func(key, &klen);
    else
        hash = hashfunc_default(key, &klen, ht->seed);

    /* scan linked list */
    for (hep = &ht->array[hash & ht->max], he = *hep;
         he; hep = &he->next, he = *hep) {
        if (he->hash == hash
            && he->klen == klen
            && memcmp(he->key, key, klen) == 0)
            break;
    }
    if (he || !val)
        return hep;

    /* add a new entry for non-NULL values */
    if ((he = ht->free) != NULL)
        ht->free = he->next;
    else {
        he = malloc(sizeof(*he));
        assert(he);
    }
    he->next = NULL;
    he->hash = hash;
    he->key  = key;
    he->klen = klen;
    he->val  = val;
    *hep = he;
    ht->count++;
    return hep;
}

void *hash_get_debug(hash_t *ht,
        const void *key, int klen, const char *file_line)
{
    hash_entry_t *he;

    assert(ht);
    assert(key);
    assert(klen);

    he = *find_entry(ht, key, klen, NULL, file_line);
    if (he)
        return (void *)he->val;
    else
        return NULL;
}

void hash_set_debug(hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    hash_entry_t **hep;

    assert(ht);
    assert(key);
    assert(klen);

    hep = find_entry(ht, key, klen, val, file_line);
    if (*hep) {
        if (!val) {
            /* delete entry */
            hash_entry_t *old = *hep;
            *hep = (*hep)->next;
            old->next = ht->free;
            ht->free = old;
            --ht->count;
        } else {
            /* replace entry */
            (*hep)->val = val;
            /* check that the collision rate isn't too high */
            if (ht->count > ht->max) {
                expand_array(ht);
            }
        }
    }
    /* else key not present and val==NULL */
}

void *hash_get_or_set_debug(hash_t *ht,
        const void *key, int klen, const void *val, const char *file_line)
{
    hash_entry_t **hep;

    assert(ht);
    assert(key);
    assert(klen);

    hep = find_entry(ht, key, klen, val, file_line);
    if (*hep) {
        val = (*hep)->val;
        /* check that the collision rate isn't too high */
        if (ht->count > ht->max) {
            expand_array(ht);
        }
        return (void *)val;
    }
    /* else key not present and val==NULL */
    return NULL;
}

unsigned int hash_count(hash_t *ht)
{
    assert(ht);
    return ht->count;
}

void hash_clear(hash_t *ht)
{
    hash_index_t *hi;

    assert(ht);

    for (hi = hash_first(ht); hi; hi = hash_next(hi))
        hash_set(ht, hi->this->key, hi->this->klen, NULL);
}

/* This is basically the following...
 * for every element in hash table {
 *    comp elemeny.key, element.value
 * }
 *
 * Like with table_do, the comp callback is called for each and every
 * element of the hash table.
 */
int hash_do(hash_do_callback_fn_t *comp,
                             void *rec, const hash_t *ht)
{
    hash_index_t  hix;
    hash_index_t *hi;
    int rv, dorv  = 1;

    hix.ht    = (hash_t *)ht;
    hix.index = 0;
    hix.this  = NULL;
    hix.next  = NULL;

    if ((hi = hash_next(&hix))) {
        /* Scan the entire table */
        do {
            rv = (*comp)(rec, hi->this->key, hi->this->klen, hi->this->val);
        } while (rv && (hi = hash_next(hi)));

        if (rv == 0) {
            dorv = 0;
        }
    }
    return dorv;
}
