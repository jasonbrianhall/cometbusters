#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <ft2build.h>
#include "cometbuster.h"
#include "visualization.h"

const int MAX_VERTS = 1000000;


// Simple matrix math
typedef struct {
    float m[16];
} Mat4;

// Vertex format
typedef struct {
    float x, y;
    float r, g, b, a;
} Vertex;

// Global GL state
typedef struct {
    GLuint program;
    GLuint vao;
    GLuint vbo;
    Mat4 projection;
    float color[4];
    
    // ✅ PERF FIX: Cache uniform location at init (don't lookup every draw call)
    GLint proj_loc;
} GLRenderState;

extern GLRenderState gl_state;


void ft_init(void);
void ft_init_from_base64(void);
void ft_try_load_cjk_font(void);
void ft_cleanup(void);
unsigned int utf8_to_codepoint(const unsigned char *str, int *bytes_read);
float gl_calculate_text_width(const char *text, int font_size);
void gl_draw_text_simple(const char *text, int x, int y, int font_size);
void draw_vertices(Vertex *verts, int count, GLenum mode);
