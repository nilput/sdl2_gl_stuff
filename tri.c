#include <SDL.h>
#include "glad/glad.h"
#include "common.h"
#include "lin.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "checks.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

char keys[256] = {0};
static void check_sdl(const char *line);

struct state_s {
    char running;
    int w;
    int h;
    vec4 bg;

    SDL_Event event;
    SDL_Window *window;
    SDL_GLContext *gl;

    GLuint  prg;
    GLuint  pos_loc;
    GLuint  u_color_loc;
    GLuint  pos_buff;
    GLuint  vao;

    struct {
        float dt;
        uint64_t last;
        uint64_t freq;
        uint64_t tick;
    } time;

} static state;


#define VECMAX 1000
vec2 vectors[VECMAX];
int needs_refresh = 0;
int veccount = 0;



static void handle_event(void);
static void update(void);
static void draw(void);
static void shader_die(GLuint shd, const char *msg);
static void initialize(void);
static void push_vec(vec2);
static void normalize_v2(vec2);

int main(void)
{
    if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        die("no sdl");
    }



    check_sdl(LINEFILESTR);
    state.window = SDL_CreateWindow( "hmm", 
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL);
    if (!state.window)
        die("no window");
    check_sdl(LINEFILESTR);


    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_LoadLibrary(NULL); 



    state.gl = SDL_GL_CreateContext(state.window);
    if (!state.gl)
        die("no gl");
    check_sdl(LINEFILESTR);


    gladLoadGLLoader(SDL_GL_GetProcAddress);


    SDL_GL_SetSwapInterval(0);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    SDL_GetWindowSize(state.window, &state.w, &state.h);
    glViewport(0, 0, state.w, state.h);
    printf("w%d h%d\n", state.w, state.h);

    if(SDL_GL_MakeCurrent(state.window, state.gl) < 0){
        check_sdl(LINEFILESTR);
    }

    initialize();

    while (state.running) {
        SDL_GL_SwapWindow(state.window);
        while (SDL_PollEvent(&state.event)) {
            if (state.event.type == SDL_QUIT) 
                goto end;
            handle_event();
        }
        update();
        draw();
    }
end:


    SDL_DestroyWindow(state.window);
    SDL_GL_DeleteContext(state.gl);
    SDL_Quit();
    return 0;
}


static void handle_event(void){

    if (state.event.type == SDL_KEYDOWN || state.event.type == SDL_KEYUP){
        if (state.event.key.keysym.sym < sizeof keys)
            keys[state.event.key.keysym.sym] = state.event.type == SDL_KEYDOWN;
    }
    else if (state.event.type == SDL_WINDOWEVENT){
        SDL_GetWindowSize(state.window, &state.w, &state.h);
        glViewport(0, 0, state.w, state.h);
    }
    if (state.event.type == SDL_MOUSEBUTTONDOWN){
        vec2 v = {state.event.button.x, state.event.button.y};
        printf("(x: %f, y: %f)\n", v[0], v[1]);
        normalize_v2(v);
        printf("(x: %f, y: %f)\n", v[0], v[1]);
        push_vec(v);
    }

    if (keys[SDLK_q])
        state.running = 0;
    if (keys[SDLK_d]){
        memcpy(state.bg, (float[4]){ 0.3f, 0.5f, 1.0f, 0.0f}, sizeof(state.bg));
    }
}

void update_time()
{
    uint64_t now = SDL_GetPerformanceCounter();
    state.time.dt = (now - state.time.last) / (float) state.time.freq;
    state.time.last = now;
    if (!(state.time.tick++ % 2321))
        printf("dt: %f", state.time.dt);
}
static void update()
{
    update_time();
    for (int i=0; i < veccount; i+=3){
        char freefalling = 0;
        for (int j=0; j < 3; j++)
            freefalling += (vectors[i+j][1] > -1.0);

        if (freefalling > 1){
            for (int j=0; j < 3; j++){
                if (vectors[i+j][1] > -1.0)
                    vectors[i+j][1] -= 0.15 * state.time.dt; //gravity
            }
            needs_refresh = 1;
        }
    }
}

static void push_vec(vec2 v)
{
    needs_refresh = 1;
    memcpy(vectors + veccount++, v, sizeof(vec2));
}
static void normalize_v2(vec2 v){
    v[0] = (v[0] / state.w - 0.5) * 2.0;
    v[1] = (v[1] / state.h - 0.5) * -2.0;
}
static void draw_polygons()
{
    if (needs_refresh){
        needs_refresh = 0;
        glBindVertexArray(state.vao);
        glBindBuffer(GL_ARRAY_BUFFER, state.pos_buff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * veccount, vectors, GL_STATIC_DRAW);
        glEnableVertexAttribArray(state.pos_loc);
        glVertexAttribPointer(state.pos_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }
    glDrawArrays(GL_TRIANGLES, 0, veccount);
}
static void draw(){
    static float bg0 = 0.0;
    glClearColor(state.bg[0], state.bg[1], state.bg[2], state.bg[3]);
    if (bg0 != (state.bg[0]+ state.bg[1]+ state.bg[2]+ state.bg[3])){
        bg0 = (state.bg[0]+ state.bg[1]+ state.bg[2]+ state.bg[3]);
        printf("bg: %f %f %f %f\n", state.bg[0], state.bg[1], state.bg[2], state.bg[3]);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_polygons();
}



static void initialize(){
    state.running = 1;
    memcpy(state.bg, (float[4]){0.0f, 0.5f, 1.0f, 0.0f}, sizeof(state.bg));
    state.time.freq = SDL_GetPerformanceFrequency();
    state.time.last = SDL_GetPerformanceCounter();
    const char * vsh_src = 
        "#version 330\n"
        "in vec2 pos;\n"
        "void main(void){\n"
        "gl_Position = vec4(pos, 1, 1);\n"
        "}\n";
    const char * fgsh_src = 
        "#version 330\n"
        "precision mediump float;\n"
        "out vec4 color;\n"
        "uniform vec4 u_color;\n"
        "void main(void){\n"
        "color = u_color;\n"
        "}\n";
    GLuint prg = glCreateProgram();
    state.prg = prg;
    if( prg < 0)
        die("prog < 0");
    GLuint vsh = glCreateShader(GL_VERTEX_SHADER);
    GLuint fgsh = glCreateShader(GL_FRAGMENT_SHADER);

    int vshlen = strlen(vsh_src);
    int fgshlen = strlen(fgsh_src);
    glShaderSource(vsh, 1, &vsh_src, &vshlen);
    glShaderSource(fgsh, 1, &fgsh_src, &fgshlen);
    glCompileShader(vsh);
    glCompileShader(fgsh);
    GLint status;
    glGetShaderiv(vsh, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
        shader_die(vsh, "vsh");
    glGetShaderiv(fgsh, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
        shader_die(fgsh, "fgsh");
    glAttachShader(prg, vsh);
    glAttachShader(prg, fgsh);
    glDeleteShader(vsh);
    glDeleteShader(fgsh);

    glLinkProgram(prg);
    glGetProgramiv(prg, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
        die("program linking failed");
    glFlush();
    
    glUseProgram(prg);
    glGenBuffers(1, &state.pos_buff);
    glGenVertexArrays(1, &state.vao);
    state.u_color_loc = glGetUniformLocation(state.prg, "u_color");
    state.pos_loc = glGetAttribLocation(state.prg, "pos");
    check_gl(LINEFILESTR);

    if ((int) state.vao < 0 || (int) state.u_color_loc < 0 || (int) state.pos_loc < 0 || (int) state.pos_buff < 0){
        printf(
            "vao: %d\n"
            "u_color_loc: %d\n"
            "pos_loc: %d\n"
            "pos_buff: %d\n", state.vao, state.u_color_loc, state.pos_loc, state.pos_buff);
        die("state.u_color_loc < 0 || state.pos_loc < 0 || state.pos_buff < 0");
    }
    glUniform4f(state.u_color_loc, 0.5, 0.5, 0.5, 0.3);



    glBindVertexArray(state.vao);
    glEnableVertexAttribArray(state.pos_loc);
    glBindBuffer(GL_ARRAY_BUFFER, state.pos_buff);
    //void glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized,
    //                           GLsizei stride, const GLvoid * pointer);
    glVertexAttribPointer(state.pos_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);

    push_vec((float [2]) {0.2f, 0.2f});
    push_vec((float [2]) {0.2f, 0.4f});
    push_vec((float [2]) {0.4f, 0.2f});

    push_vec((float [2]) {0.2f, 0.2f});
    push_vec((float [2]) {0.2f, 0.4f});
    push_vec((float [2]) {0.4f, 0.2f});
    for (int i=0; i<veccount; i++){
        printf("%f %f %f\n", vectors[i][0], vectors[i][1], vectors[i][2]);
    }


    check_gl(LINEFILESTR);
    
}

static void check_sdl(const char *line){
   const char *m = SDL_GetError(); 
   if (m){
        printf( "SDL LAST ERROR: %s\n"
                "at %s\n", m, line);
   }
}
