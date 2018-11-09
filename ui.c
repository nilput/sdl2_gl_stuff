#include <stddef.h>
#include "common.h"
#include "glad/glad.h"
#include "font.h"
#include "checks.h"
#include "ui.h"
struct ui ui = {0};

#define UI_A_POS 0
#define UI_A_COL 1
float identity_mat4[16] = { 1.0, 0.0, 0.0, 0.0,
                            0.0, 1.0, 0.0, 0.0,
                            0.0, 0.0, 1.0, 0.0,
                            0.0, 0.0, 0.0, 1.0 };
float screen_mat4[16] =   { 0 };

int ui_initialize()
{

    const char * vsh_src = 
        "#version 330\n"
        "layout(location = 0) in ivec2  a_pos;\n"
        "layout(location = 1) in ivec4 a_color;\n"
        "uniform mat4 u_mat;\n"
        "out vec4 v_color;\n"
        "void main(void){\n"
        "   gl_Position = u_mat * vec4(vec2(a_pos.xy), 1.0, 1.0);\n"
        "   v_color = vec4(a_color) / 255.0;\n"
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
    ui.prg = glCreateProgram();
    if( ui.prg < 0){
        ui.last_error = "prg < -1";
        return -1;
    }
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
    if (status != GL_TRUE){
        ui.last_error = shader_log(vsh);
        return -1;
    }
    glGetShaderiv(fgsh, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE){
        ui.last_error = shader_log(fgsh);
        return -1;
    }
    glAttachShader(ui.prg, vsh);
    glAttachShader(ui.prg, fgsh);
    glDeleteShader(vsh);
    glDeleteShader(fgsh);
    glLinkProgram(ui.prg);
    glGetProgramiv(ui.prg, GL_LINK_STATUS, &status);
    if (status != GL_TRUE){
        ui.last_error = "program linking failed";
        return -1;
    }
    glFlush();
    
    glUseProgram(ui.prg);
    glGenBuffers(1, &ui.vbo);
    glGenVertexArrays(1, &ui.vao);

    glBindVertexArray(ui.vao); 

    ui.umat_loc = glGetUniformLocation(ui.prg, "u_mat");

    glEnableVertexAttribArray(UI_A_POS);
    glEnableVertexAttribArray(UI_A_COL);
    glEnableVertexAttribArray(ui.umat_loc);


    /* check_gl(LINEFILESTR); */
    return 0;
}

void ui_gl_restore_state(){
    glUseProgram(ui.prg);
    glBindVertexArray(ui.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ui.vbo);
    glVertexAttribIPointer(UI_A_POS, 2, GL_UNSIGNED_SHORT,  sizeof(uvec2), (void *) offsetof(uvec2, x));
    glVertexAttribIPointer(UI_A_COL, 4, GL_UNSIGNED_BYTE, sizeof(uvec2), (void *) offsetof(uvec2, rgb));
    if (ui.screen_width < 1.0 || ui.screen_height < 1.0){ //instead of dividing by 0
        ui.last_error = "invalid screen size";
        memcpy(screen_mat4, identity_mat4, sizeof screen_mat4);
    }
    else{
#define m screen_mat4
                                //scale                                    //translate
        m[0 ] = 1.0 / (ui.screen_width / 2.0); m[1] = 0.0;   m[2 ] = 0.0; m[3 ] = -1.0;
        m[4 ] = 0.0; m[5 ] = -1.0 / (ui.screen_height / 2.0); m[6 ] = 0.0; m[7 ] =  1.0;
        m[8 ] = 0.0; m[9 ] =  0.0;                            m[10] = 1.0; m[11] =  0.0;
        m[12] = 0.0; m[13] =  0.0;                            m[14] = 0.0; m[15] =  1.0;
#undef m
    }
    glUniformMatrix4fv(ui.umat_loc, 1, GL_TRUE, screen_mat4);
    check_gl(LINEFILESTR);
}

void ui_set_screen_dim(uint16_t w, uint16_t h)
{
    ui.cached = false;
    ui.screen_width = w;
    ui.screen_height = h;
}

static void set_color(uint8_t rgb[4], uint32_t color)
{
    //alpha, blue, green red
    //input: 0xFFEEBBCC
    //red is FF
    rgb[3] = (color & ((int32_t) 0xFF << 0 )) >> 0;
    rgb[2] = (color & ((int32_t) 0xFF << 8 )) >> 8;
    rgb[1] = (color & ((int32_t) 0xFF << 16)) >> 16;
    rgb[0] = (color & ((int32_t) 0xFF << 24)) >> 24;
}

int ui_put_pixel_rgb_array(uint16_t x, uint16_t y, uint8_t rgb[4])
{
    ui.cached = 0;
    if(ui.n_pixels >= PIX_MAX)
        return -1;
    ui.data.pixels[ui.n_pixels].x = x;
    ui.data.pixels[ui.n_pixels].y = y;
    memcpy(ui.data.pixels[ui.n_pixels].rgb, rgb, sizeof(uint8_t[4]));
    return ui.n_pixels++;
}

static int ui_put_vertex_rgb_array(uint16_t x, uint16_t y, uint8_t rgb[4])
{
    ui.cached = 0;
    if(ui.n_vertices >= PIX_MAX)
        return -1;
    ui.data.vertices[ui.n_vertices].x = x;
    ui.data.vertices[ui.n_vertices].y = y;
    memcpy(ui.data.vertices[ui.n_vertices].rgb, rgb, sizeof(uint8_t[4]));
    return ui.n_vertices++;
}

int ui_put_pixel(uint16_t x, uint16_t y, uint32_t color)
{
    ui.cached = 0;
    if(ui.n_pixels >= PIX_MAX)
        return -1;
    ui.data.pixels[ui.n_pixels].x = x;
    ui.data.pixels[ui.n_pixels].y = y;
    set_color(ui.data.pixels[ui.n_pixels].rgb, color);
    return ui.n_pixels++;
}

int ui_put_rect(urect *rect)
{
    ui.cached = 0;
    if (ui.n_rects >= RECT_MAX)
        return -1;
    memcpy(ui.data.rects + ui.n_rects, rect, sizeof(urect));
    return ui.n_rects++;
}

int ui_draw_text(uint16_t x, uint16_t y, const char *text, uint8_t rgb[4])
{
    if (!text || y > ui.screen_height || y < 0 || x > ui.screen_height || x < 0 || !rgb){
        ui.last_error = "invalid parameter to ui_draw_text()";
        return -1;
    }

    y += 13;
    while (*text){
        char t = *text;
        char pixrow;
        if (t >= 32 && t < 127){
            for (int i=0; i<13; i++){ //row
                pixrow = RASTERS[t-32][i];
                for(int j=7; j>=0; j--){ //column
                    if (pixrow & 1U)
                        ui_put_pixel_rgb_array(x+j, y-i, rgb);
                    pixrow = pixrow >> 1;
                }
            }
        }
        x += 8 + RASTERS_SPACING;
        text++;
    }
    return 0;
}
uint16_t ui_textwidth(int len){
    return len * 8.0 + len * RASTERS_SPACING;
}
uint16_t ui_textheight(int len){
    return 13;
}
inline static void draw_button(int button_id)
{
    struct ui_button *button = (struct ui_button*) &ui.ui_elems[button_id];
    ui_put_rect(&button->rect);
    if (!button->text)
        return;
    uint16_t sx, sy;
    sx = button->rect.x +button->rect.w / 2 - ui_textwidth(button->textlen) / 2;
    sy = button->rect.y +button->rect.h / 2 - ui_textheight(button->textlen) / 2;
    ui_draw_text(sx, sy, button->text, button->text_color);
}
//adds a null terminator
static char *copy_to_blob(const char *str, int len)
{
    if (ui.bloblen + len + 1 >= STR_BLOB_MAX)
        return NULL;
    char *at = ui.data.strblob + ui.bloblen;
    memcpy(at, str, len);
    at[len] = '\0';
    ui.bloblen += len + 1;
    return at;
}

int ui_render()
{

    //fix TODO
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvec2) * ui.n_vertices, ui.data.vertices, GL_STATIC_DRAW);
    int n_points = ui.n_vertices - ui.first_point_vertex;
    glDrawArrays(GL_TRIANGLES, 0, ui.first_point_vertex);
    glDrawArrays(GL_POINTS, ui.first_point_vertex, n_points);

    return 0;
}

int ui_display(void)
{
    ui_gl_restore_state();
    check_gl(LINEFILESTR);
    if (ui.cached)
        return ui_render();
    ui.n_vertices = 0;

    for (int i=0; i<ui.n_ui; i++){
        switch(ui.ui_elems[i].head.type){
            case UI_BUTTON:
                draw_button(i);
            break;
            default:
                ui.last_error = "unknown ui element type.";
                return -1;
            break;
        }
    }

    //rects -> pixels
    for (int i=0; i<ui.n_rects; i++){
        //emit 6 vertices per rectangle
        //using put pixel even though it will not be drawn as a pixel!
        urect *r = &ui.data.rects[i];
        //     w
        // (1)     (2)
        //              h
        // (3)     (4)
        // 1, 3, 2, 3, 4, 2
        ui_put_vertex_rgb_array(r->x,      r->y,      r->rgb);
        ui_put_vertex_rgb_array(r->x,      r->y+r->h, r->rgb);
        ui_put_vertex_rgb_array(r->x+r->w, r->y,      r->rgb);
        ui_put_vertex_rgb_array(r->x,      r->y+r->h, r->rgb);
        ui_put_vertex_rgb_array(r->x+r->w, r->y+r->h, r->rgb);
        ui_put_vertex_rgb_array(r->x+r->w, r->y,      r->rgb);

    }
    ui.first_point_vertex = ui.n_vertices;
    //CHECK IF THERE IS SPACE
    memcpy(ui.data.vertices + ui.n_vertices, ui.data.pixels, ui.n_pixels * sizeof(uvec2));
    ui.n_vertices += ui.n_pixels;

    return ui_render();
}

static int rect_contains(urect *rect, int x, int y){
    return ((x > rect->x) && (x < rect->x+rect->w)) &&
           ((y > rect->y) && (y < rect->y+rect->h));
    
}

int ui_handle_event(const char *event, int ui_id){
    int callback_id = ui.ui_elems[ui_id].head.callback_id;
    struct callback_info *cb_inf;
    while (callback_id >= 0){
        cb_inf = &ui.cb[callback_id];
        if (cb_inf->event && (strcmp(event, cb_inf->event) == 0)){
            cb_inf->cb(NULL);
            return 0;
        }
        callback_id = cb_inf->next;
    }
    ui.last_error = "didnt find event";
    return -1;
}

int ui_event(const char *event, int x, int y)
{
    int s;
    for (int i=0; i<ui.n_ui; i++){
        switch(ui.ui_elems[i].head.type){
            case UI_BUTTON:
                if (rect_contains( &((struct ui_button *) &ui.ui_elems[i])->rect, x, y)){
                    s = ui_handle_event(event, i);
                    if (s < 0)
                        return s;
                    return i;
                }
            break;
            default:
                ui.last_error = "unknown ui element type.";
                return -2;
            break;
        }
    }
    return -1;

}
void ui_flush()
{
    ui.n_pixels = 0;
    ui.n_rects = 0;
    ui.n_vertices = 0;
    ui.cached = 0;
}
int ui_create_button(int x, int y, int w, int h, const char *label)
{
    if (ui.n_ui >= UI_MAX)
        return -1;
    struct ui_button *button = (struct ui_button *) &ui.ui_elems[ui.n_ui];
    set_color(button->rect.rgb, COLOR_HBLACK);
    set_color(button->text_color, COLOR_WHITE);
    button->rect.x = x;
    button->rect.y = y;
    button->rect.w = w;
    button->rect.h = h;
    uint16_t len = strlen(label);
    button->text = copy_to_blob(label, len);
    button->textlen = len;
    button->head.callback_id = -1;
    button->head.type = UI_BUTTON;
    return ui.n_ui++;
}

int ui_register_callback(int element_id, const char *event, cb_func func)
{
    if (ui.n_cb >= CB_MAX || element_id < 0 || element_id > ui.n_ui){
        ui.last_error = "ui.n_cb+1 >= CB_MAX";
        return -1;
    }
    const char *event_str = copy_to_blob(event, strlen(event));
    if (!event_str){
        ui.last_error = "strblob full";
        return -1;
    }
    struct callback_info *new_cb = &ui.cb[ui.n_cb];
    new_cb->event = event_str;
    new_cb->cb = func;
    ui_element *target = &ui.ui_elems[element_id];
    //link new cb to prev cb (can be -1)
    new_cb->next = target->head.callback_id;
    //link ui_element to new cb
    target->head.callback_id = ui.n_cb;
    return ui.n_cb++;
}

const char *ui_last_error(void){
    if (!ui.last_error)
        ui.last_error = "";
    return ui.last_error;
}

