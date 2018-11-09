#ifndef UI_H
#define UI_H

#include "lin.h"

struct ui_head;
#define PIX_MAX         100000 //700kb
#define RECT_MAX        256 
#define CB_MAX          100      //max callbacks
#define UI_MAX          100  
#define STR_BLOB_MAX    10000   //10kb
#define UI_V_MAX        200000 //1400kb

#define COLOR_WHITE     0xFFFFFFFF
#define COLOR_HBLACK    0x00000088
#define COLOR_BLACK     0x000000FF


typedef struct {
    uint16_t x, y;
    uint8_t  rgb[4];

} uvec2;

typedef struct {
    uint16_t x, y, w, h;
    uint8_t  rgb[4];
} urect;

enum ui_type{
    UI_BUTTON,
    UI_LABEL,
};

struct ui_head{
    enum ui_type type;
    int callback_id;
};


typedef void * (*cb_func)(void *arg);
struct callback_info;
struct callback_info{
    const char *event;
    cb_func cb;
    int next;
};

struct ui_button{
    struct ui_head head;
    urect rect;
    const char *text; //ptr to somewhere in strblob
    uint16_t textlen;
    uint8_t text_color[4];
};

struct ui_label{
    struct ui_head head;
    uvec2 pos;
    const char *text; //ptr to somewhere in strblob
};

union ui_element{
    struct ui_head head;
    struct ui_button _;
    struct ui_label __;
};

typedef union ui_element ui_element;

struct ui{
    GLuint prg;
    GLuint vao;
    GLuint vbo;
    GLuint umat_loc;
    int screen_width;
    int screen_height;
    char cached;
    int n_ui;
    int n_cb;
    int n_pixels;
    int n_vertices; //pixels + (rects * 6)
    int n_rects;
    int bloblen;
    int first_point_vertex; //whats before it is rectangles

    ui_element ui_elems[UI_MAX];
    struct callback_info cb[CB_MAX];
    struct {
        urect rects[RECT_MAX];
        uvec2 pixels[PIX_MAX];
        char strblob[STR_BLOB_MAX];
        uvec2 vertices[UI_V_MAX];
    } data;
    const char *last_error;
};


int ui_display(void);
int ui_initialize();
void ui_set_screen_dim(uint16_t w, uint16_t h);
int ui_create_button(int x, int y, int w, int h, const char *label);
int ui_put_pixel(uint16_t x, uint16_t y, uint32_t color);
int ui_put_pixel_rgb_array(uint16_t x, uint16_t y, uint8_t rgb[4]);
int ui_register_callback(int element_id, const char *event, cb_func func);
const char *ui_last_error(void);
void ui_flush();
int ui_event(const char *event, int x, int y);

#endif
