#include "doubly_list.h"
#include "mempool.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct my_data{
    lnode_t lnode;
    int id;
};
int log_test_domain_id;
static MEM_POOL(pool, struct my_data);

extern void log_test02();
void test01()
{
    DOUBLY_LIST(h);
    DOUBLY_LIST_INIT(&h);
    struct my_data *a = malloc(sizeof(struct my_data) * 10);
    for(int i = 0; i < 10; i++)
    {
        printf("%#x ", a+i);
        doubly_list_add(&h, a+i);
    }
}

void test02()
{
    struct my_data *a = NULL;
    mempool_init(&pool, 10);
    for(int i = 0; i < 10; i++)
    {
        mempool_alloc(&pool, &a);
        printf("%#x ", a);
        a->id += (100+i);
        printf("head = %d, tail = %d, size = %d, avail = %d \n", 
                pool.head, pool.tail, pool.size, pool.avail);
    }

    mempool_free(&pool, a);
    printf("head = %d, tail = %d, size = %d, avail = %d \n", 
            pool.head, pool.tail, pool.size, pool.avail);
    // mempool_alloc(&pool, &a);
    printf("%#x ", a);
    log_info("test pool");
}

int main(int argc, char *argv[])
{
    main_init();

    main_test();

    main_destroy();
    return 0;
}

void main_init()
{
    log_init();
    log_install_domain(&log_test_domain_id, "log_test", LOG_DEFAULT);
}

void main_test()
{
    test02();
    log_test02();
}

void main_destroy()
{
    mempool_destroy(&pool);
    log_final();
}