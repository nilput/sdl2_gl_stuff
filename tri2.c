#include "common.h"
#include <math.h>
#include <SDL.h>
#include "glad/glad.h"
#include "checks.h"
#include "lin.h"
#include "ui.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

char keys[256] = {0};


struct state_s {
    char running;
    char fps_info;
    int w;
    int h;
    vec4 bg;

    SDL_Event event;
    SDL_Window *window;
    SDL_GLContext *gl;

    GLuint  prg;
    GLuint  a_pos_loc;
    GLuint  a_color_loc;
    GLuint  u_color_loc;
    GLuint  pos_buff;
    GLuint  vao;

    struct {
        float dt;
        float accum;
        uint64_t last;
        uint64_t freq;
        uint64_t tick;
        uint64_t frame;
    } time;

} static state;

struct vertex{
    vec2 pos;
    vec2 vel;
    uint8_t rgb[3];
};
// [ 0 .. 2 ]
int entered_vertices = 0;

#define VECMAX 1000
struct vertex vertices[VECMAX];
int needs_refresh = 0;
int trcount = 0;


static void handle_event(void);
static void update(void);
static void draw(void);
static void shader_die(GLuint shd, const char *msg);
static void prog_die(GLuint prg, const char *msg);
static void initialize(void);
static void push_vec(vec2);
static void normalize_v2(vec2);
static void dump_vertices();
static void check_sdl(const char *line);

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


static void handle_event(void)
{

    if (state.event.type == SDL_KEYDOWN || state.event.type == SDL_KEYUP){
        if (state.event.key.keysym.sym < sizeof keys)
            keys[state.event.key.keysym.sym] = state.event.type == SDL_KEYDOWN;
    }
    else if (state.event.type == SDL_WINDOWEVENT){
        SDL_GetWindowSize(state.window, &state.w, &state.h);
        glViewport(0, 0, state.w, state.h);
    }
    if (state.event.type == SDL_MOUSEBUTTONDOWN){
        int ui_handled = ui_event("Lclick", state.event.button.x, state.event.button.y);
        if (ui_handled < 0){
            vec2 v = {state.event.button.x, state.event.button.y};
            printf("(x: %f, y: %f)\n", v[0], v[1]);
            normalize_v2(v);
            printf("(x: %f, y: %f)\n", v[0], v[1]);
            push_vec(v);
        }
    }

    if (keys[SDLK_q])
        state.running = 0;
    if (keys[SDLK_f] == 1){
        keys[SDLK_f] = 2;
        state.fps_info = !state.fps_info;
    }
    if (keys[SDLK_d]){
        memcpy(state.bg, (float[4]){ 0.3f, 0.5f, 1.0f, 0.0f}, sizeof(state.bg));
        dump_vertices();
    }
}

void update_time()
{
    const float target_quanta = 1.0 / 60;
    uint64_t now = SDL_GetPerformanceCounter();
    state.time.dt = (now - state.time.last) / (float) state.time.freq;
    state.time.accum += state.time.dt;
    state.time.tick++;
    if (state.time.accum > 1.0){
        if (state.fps_info){
            fprintf(stderr, "per frame: %.2fms, fps: %ld\n",
                            state.time.accum / state.time.frame * 1000.0, 
                            state.time.frame);
        }
        state.time.frame = 0;
        state.time.accum = 0.0;
    }
    if (state.time.dt < target_quanta)
        SDL_Delay(1000 * (target_quanta - state.time.dt));

    state.time.last = now;
}

#define GRAVITY 0.15

static void rotate_triangle(struct vertex *tri)
{
    //other points
    /* vec2 rotated[3]; */
    /* float far_x = rotated[0][0]; */
    /* int far_idx; */
    /* for (int i=0; i<3; i++){ */
    /*     //relative to 'around' */
    /*     vec2_sub(rotated[i], tri->pos[i], tri->pos[]); */
    /*     if (fabs(rotated[i][0]) > far_x){ */
    /*         far_x = fabs(rotated[i][0]); */
    /*         far_idx = i; */
    /*     } */
    /* } */
    //angle of rotation
    //
    //
    //   
    //  
    // * * * * * <this 
    // *     * 
    //  *  * 
    //   *
    //   
    //
    

}

static void update()
{
    update_time();
    for (int i=0; i < trcount*3; i+=3){
        char freefalling = 0;
        int pivot;
        for (int j=0; j < 3; j++)
            if (vertices[i+j].pos[1] > -1.0)
                freefalling++;
            else 
                pivot = i;

        if (freefalling >= 3){
            for (int j=0; j < 3; j++){
                vertices[i+j].pos[1] -= GRAVITY * state.time.dt;
            }
        }
        else if (freefalling > 1){
            /* rotate_triangle(&triangles[i]); */
        }
        else
            continue;
        needs_refresh = 1;
    }
}

static void push_vec(vec2 v)
{
    int dest = trcount * 3;
    memcpy(vertices[dest + entered_vertices].pos, v, sizeof(vec2));
    entered_vertices++;
    if (entered_vertices >= 3){
        int r = rand();
        entered_vertices = 0;
        memset(vertices[dest].vel, 0, sizeof(vec2));
        vertices[dest].rgb[0] = (uint8_t)(r);
        vertices[dest].rgb[1] = (uint8_t)(r >> 8);
        vertices[dest].rgb[2] = (uint8_t)(r >> 16);

        for(int i=1; i<3; i++){
            memcpy(vertices[dest + i].rgb, vertices[dest].rgb, sizeof(char[3]));
            memcpy(vertices[dest + i].vel, vertices[dest].vel, sizeof(vec2));
        }
        trcount++;
        needs_refresh = 1;
    }
}
static void normalize_v2(vec2 v)
{
    v[0] = (v[0] / state.w - 0.5) * 2.0;
    v[1] = (v[1] / state.h - 0.5) * -2.0;
}


static void tri_restore_gl_state()
{
    glUseProgram(state.prg);
    glBindVertexArray(state.vao);

}

static void draw_polygons()
{
    if (needs_refresh){
        needs_refresh = 0;
        glBindVertexArray(state.vao);
        glBindBuffer(GL_ARRAY_BUFFER, state.pos_buff);
        glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex) * trcount * 3, vertices, GL_STATIC_DRAW);

        //idx, size, type, normalize?, stride, offset
        glVertexAttribPointer(state.a_pos_loc,   2, GL_FLOAT,         GL_FALSE, 20, (void *) 0);

        /* glVertexAttribPointer(state.a_color_loc, 3, GL_UNSIGNED_BYTE, GL_FALSE, 20, (void *) 16 ); */

        /* VertexAttribIPointer( uint index, int size, enum type, */
        /* sizei stride, const void *pointer ); */
        glVertexAttribIPointer(state.a_color_loc, 3, GL_UNSIGNED_BYTE, 20, (void *) 16 ); 

        check_gl(LINEFILESTR);

        /* printf("updated\n"); */
    }
    glDrawArrays(GL_TRIANGLES, 0, trcount * 3);
}

static void draw()
{
    static float bg0 = 0.0;
    state.time.frame++;
    glClearColor(state.bg[0], state.bg[1], state.bg[2], state.bg[3]);
    if (bg0 != (state.bg[0]+ state.bg[1]+ state.bg[2]+ state.bg[3])){
        bg0 = (state.bg[0]+ state.bg[1]+ state.bg[2]+ state.bg[3]);
        printf("bg: %f %f %f %f\n", state.bg[0], state.bg[1], state.bg[2], state.bg[3]);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    draw_polygons();


    ui_flush();
    /* for (int i=100; i<200; i++){ */
    /*     for (int j=100; j<200; j++){ */
    /*         ui_put_pixel(i, j, 0xFFFFFFFF); */
    /*     } */
    /* } */
    ui_display();
    tri_restore_gl_state();
}


static void dump_vertices()
{
    printf("trcount: %d\n", trcount);
    printf("struct vertex: \n"
            "   size: %lu\n"
            "   sizeof pos:%lu       offsetof pos: %lu\n"
            "   sizeof vel:%lu       offsetof vel: %lu\n"
            "   sizeof rgb:%lu       offsetof rgb: %lu\n", sizeof(struct vertex),
            sizeof(vec2), offsetof(struct vertex, pos),
            sizeof(vec2), offsetof(struct vertex, vel),
            sizeof(char[3]), offsetof(struct vertex, rgb));
    for (int i=0; i < trcount; i++){
            printf("%d:\n", i);
            printf("\trgb: %hhu %hhu %hhu\n", vertices[i*3].rgb[0], vertices[i*3].rgb[1], vertices[i*3].rgb[2]);
        for (int j = 0; j<3; j++){
            printf("\t%f %f\n", vertices[i*3+j].pos[0], vertices[i*3+j].pos[1]);
        }
    }
}

void *cb_triangle(void *_)
{
    push_vec((float [2]) {0.2f, 0.2f});
    push_vec((float [2]) {0.2f, 0.4f});
    push_vec((float [2]) {0.4f, 0.2f});
    return NULL;
}
void *cb_rectangle(void *_)
{
    push_vec((float [2]) {-0.2f, -0.2f});
    push_vec((float [2]) {-0.2f, -0.4f});
    push_vec((float [2]) {-0.4f, -0.2f});
    push_vec((float [2]) {-0.2f, -0.4f});
    push_vec((float [2]) {-0.4f, -0.2f});
    push_vec((float [2]) {-0.4f, -0.4f});
    //cheat and set same color
    for (int i=trcount*3-1; i>trcount*3-3-1; i--){
        memcpy(vertices[i].rgb, vertices[i-3].rgb, sizeof(char[3]));

    }
    return NULL;
}

static void initialize()
{
    state.running = 1;


    memcpy(state.bg, (float[4]){0.0f, 0.5f, 1.0f, 0.0f}, sizeof(state.bg));
    state.time.freq = SDL_GetPerformanceFrequency();
    state.time.last = SDL_GetPerformanceCounter();
    srand(0xBADBEEF0);
    const char * vsh_src = 
        "#version 330\n"
        "in vec2  a_pos;\n"
        "in ivec3 a_color;\n"
        "out vec4 v_color;\n"
        "void main(void){\n"
        "   gl_Position = vec4(a_pos.xy, 1, 1);\n"
        "   v_color = vec4(a_color, 255.0) / 255.0;\n"
        "}\n";
    const char * fgsh_src = 
        "#version 330\n"
        "precision mediump float;\n"
        "in vec4 v_color;\n"
        "out vec4 color;\n"
        "uniform vec4 u_color;\n"
        "void main(void){\n"
        "   color = v_color;\n"
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
        prog_die(prg,"program linking failed");
    glFlush();
    
    glUseProgram(prg);
    glGenBuffers(1, &state.pos_buff);
    glGenVertexArrays(1, &state.vao);
    glBindVertexArray(state.vao);
    state.a_pos_loc   = glGetAttribLocation(state.prg, "a_pos");
    state.a_color_loc = glGetAttribLocation(state.prg, "a_color");
    state.u_color_loc = glGetUniformLocation(state.prg, "u_color");
    check_gl(LINEFILESTR);

    if ((int) state.vao < 0         ||
        (int) state.u_color_loc < 0 ||
        (int) state.a_color_loc < 0 ||
        (int) state.a_pos_loc < 0     ||
        (int) state.pos_buff < 0  )  
    {

        printf(
            "vao: %d\n"
            "u_color_loc: %d\n"
            "a_color_loc: %d\n"
            "a_pos_loc: %d\n"
            "pos_buff: %d\n", state.vao, state.u_color_loc, state.a_color_loc, state.a_pos_loc, state.pos_buff);

        /* die("state.u_color_loc < 0 || state.a_pos_loc < 0 || state.pos_buff < 0"); */
    }
    glUniform4f(state.u_color_loc, 0.2, 0.2, 0.2, 1.0);

    glEnableVertexAttribArray(state.a_pos_loc);
    glEnableVertexAttribArray(state.a_color_loc);

    push_vec((float [2]) {0.2f, 0.2f});
    push_vec((float [2]) {0.2f, 0.4f});
    push_vec((float [2]) {0.4f, 0.2f});

    push_vec((float [2]) {-0.9f, -0.9f});
    push_vec((float [2]) {-0.8f, -0.9f});
    push_vec((float [2]) {-0.8f, -0.8f});

    dump_vertices();


    check_gl(LINEFILESTR);


    if ( ui_initialize() < 0){
        die(ui_last_error());
    }
    ui_set_screen_dim(state.w, state.h);
    //ui_create_button(int x, int y, int w, int h, const char *label);
    //ui_register_callback(int element_id, const char *event, cb_func func);
    int rec = ui_create_button(100, 100, 200, 100, "rectangle");
    int tri = ui_create_button(310, 100, 200, 100, "triangle");
    ui_register_callback(tri, "Lclick", cb_triangle);
    ui_register_callback(rec, "Lclick", cb_rectangle);
    
}

static void check_sdl(const char *line){
   const char *m = SDL_GetError(); 
   if (m){
        printf( "SDL LAST ERROR: %s\n"
                "at %s\n", m, line);
   }
}
