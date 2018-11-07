#ifndef COMMOH
#define COMMOH
#include <stdio.h>
#include <stdarg.h>

static void die(char *msg, ...){
    
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

const char *lfcat(const char *f, int l){
    static char buff[1024];
    snprintf(buff, 1024-1, "%s:%d", f, l);
    return buff;
}

#define LINEFILESTR lfcat(__FILE__, __LINE__)

void check_sdl(const char *line){
   const char *m = SDL_GetError(); 
   if (m){
        printf( "SDL LAST ERROR: %s\n"
                "at %s\n", m, line);
   }

}

#endif
