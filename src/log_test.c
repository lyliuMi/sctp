#include "doubly_list.h"
#include "mempool.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

extern int log_test_domain_id;
#undef LOG_DOMAIN
#define LOG_DOMAIN log_test_domain_id

void log_test02()
{
    // log_install_domain(&pool_domain_id, "pool", LOG_DEFAULT);
    log_info("start log_test");
}