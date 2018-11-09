#ifndef CHECKS_H
#define CHECKS_H
#include "check_gl.h"

static const char * shader_log(GLuint shd)
{
    static char log[1024];
    glGetShaderInfoLog(shd, sizeof log, NULL, log);
    return log;
}
static const char * prog_log(GLuint prg)
{
    static char log[1024];
    glGetProgramInfoLog(prg, sizeof log, NULL, log);
    return log;
}
static void shader_die(GLuint shd, const char *msg)
{
    die("%s shader compilation failed\n%s", msg, shader_log(shd));
}
static void prog_die(GLuint prg, const char *msg)
{
    die("%s program compilation failed\n%s", msg, prog_log(prg));
}

#endif
