#ifndef COMMOH
#define COMMOH
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void die(const char *msg, ...){
    
    va_list va;
    va_start(va, msg);
    vfprintf(stderr, msg, va);
    fprintf(stderr, "\n");
    va_end(va);

    exit(1);
}

static void dlogf(char *msg, ...){
    
    va_list va;
    va_start(va, msg);
    vfprintf(stderr, msg, va);
    va_end(va);
}

static const char *lfcat(const char *f, int l){
    static char buff[1024];
    snprintf(buff, 1024-1, "%s:%d", f, l);
    return buff;
}

#define LINEFILESTR lfcat(__FILE__, __LINE__)


#endif
