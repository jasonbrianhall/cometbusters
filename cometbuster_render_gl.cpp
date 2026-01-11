#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <pango/pango.h>
#include "cometbuster.h"
#include "visualization.h"

#ifdef ExternalSound
#include "audio_wad.h"
#endif

#include "Monospace.h"  // Your TTF-converted font


static GLuint global_vao = 0;

// Simple matrix math
typedef struct {
    float m[16];
} Mat4;

static Mat4 mat4_identity(void) {
    Mat4 m = {0};
    m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.0f;
    return m;
}

static Mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far) {
    Mat4 m = mat4_identity();
    m.m[0] = 2.0f / (right - left);
    m.m[5] = 2.0f / (top - bottom);
    m.m[10] = -2.0f / (far - near);
    m.m[12] = -(right + left) / (right - left);
    m.m[13] = -(top + bottom) / (top - bottom);
    m.m[14] = -(far + near) / (far - near);
    return m;
}

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
} GLRenderState;

static GLRenderState gl_state = {0};

// Shader sources
static const char *vertex_shader = 
    "#version 330 core\n"
    "layout(location = 0) in vec2 position;\n"
    "layout(location = 1) in vec4 color;\n"
    "uniform mat4 projection;\n"
    "out vec4 vertexColor;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(position, 0.0, 1.0);\n"
    "    vertexColor = color;\n"
    "}\n";

static const char *fragment_shader =
    "#version 330 core\n"
    "in vec4 vertexColor;\n"
    "out vec4 FragColor;\n"
    "void main() {\n"
    "    FragColor = vertexColor;\n"
    "}\n";

static GLuint compile_shader(const char *src, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);
    
    int success;
    char log[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, log);
        fprintf(stderr, "[GL] Shader compile error: %s\n", log);
    }
    return shader;
}

static GLuint create_program(const char *vs_src, const char *fs_src) {
    GLuint vs = compile_shader(vs_src, GL_VERTEX_SHADER);
    GLuint fs = compile_shader(fs_src, GL_FRAGMENT_SHADER);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    
    int success;
    char log[512];
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(prog, 512, NULL, log);
        fprintf(stderr, "[GL] Program link error: %s\n", log);
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

static void gl_init(void) {
    if (gl_state.program) return;
    
    fprintf(stderr, "[GL] Initializing modern GL 3.3+ renderer\n");
    glGenVertexArrays(1, &global_vao);
    
    gl_state.program = create_program(vertex_shader, fragment_shader);
    
    glGenVertexArrays(1, &gl_state.vao);
    glGenBuffers(1, &gl_state.vbo);
    
    glBindVertexArray(gl_state.vao);
    glBindBuffer(GL_ARRAY_BUFFER, gl_state.vbo);
    glBufferData(GL_ARRAY_BUFFER, 100000 * sizeof(Vertex), NULL, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, x));
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, r));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    gl_state.color[0] = 1.0f;
    gl_state.color[1] = 1.0f;
    gl_state.color[2] = 1.0f;
    gl_state.color[3] = 1.0f;
    
    fprintf(stderr, "[GL] GL 3.3+ renderer initialized\n");
}

static void draw_vertices(Vertex *verts, int count, GLenum mode) {
    glUseProgram(gl_state.program);
    
    GLint proj_loc = glGetUniformLocation(gl_state.program, "projection");
    glUniformMatrix4fv(proj_loc, 1, GL_FALSE, gl_state.projection.m);
    
    glBindBuffer(GL_ARRAY_BUFFER, gl_state.vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(Vertex), verts);
    
    glBindVertexArray(gl_state.vao);
    glDrawArrays(mode, 0, count);
}

// ============================================================================
// HIGH-LEVEL DRAWING API
// ============================================================================

void gl_setup_2d_projection(int width, int height) {
    // Always use 1920x1080 logical game coordinates (matches Cairo normalization)
    // This ensures mouse tracking and rendering match Cairo exactly
    (void)width;   // Unused - we always use 1920x1080
    (void)height;  // Unused - we always use 1920x1080
    gl_state.projection = mat4_ortho(0, 1920, 1080, 0, -1, 1);
}

void gl_restore_projection(void) {
    // No-op in modern GL
}

void gl_set_color(float r, float g, float b) {
    gl_state.color[0] = r;
    gl_state.color[1] = g;
    gl_state.color[2] = b;
    gl_state.color[3] = 1.0f;
}

void gl_set_color_alpha(float r, float g, float b, float a) {
    gl_state.color[0] = r;
    gl_state.color[1] = g;
    gl_state.color[2] = b;
    gl_state.color[3] = a;
}

// ============================================================================
// TEXT RENDERING - Monospace.h Font Support
// ============================================================================

static void gl_draw_text_simple(const char *text, int x, int y, int font_size) {
    if (!text || !text[0]) return;
    
    // Monospace.h has max height
    float ref_height = (float)FONT_MAX_HEIGHT;
    float scale = (float)font_size / ref_height;
    
    float current_x = (float)x;
    float baseline_y = (float)y;  // y is the baseline
    
    // Build vertex array
    static Vertex verts[100000];
    int vert_count = 0;
    
    for (int i = 0; text[i] && vert_count < 99990; i++) {
        unsigned char ch = (unsigned char)text[i];
        
        // Get glyph from font
        const GlyphData *glyph = get_glyph(ch);
        if (!glyph || !glyph->bitmap) {
            continue;
        }
        
        float glyph_width = (float)glyph->width * scale;
        float pixel_scale = scale;
        
        // Use glyph's vertical offset for proper baseline alignment
        // yoffset is the distance from baseline to top of glyph
        float glyph_top = baseline_y - (float)glyph->yoffset * scale;
        
        // Render each pixel in the glyph
        for (int row = 0; row < glyph->height; row++) {
            float pixel_y = glyph_top + row * pixel_scale;
            
            for (int col = 0; col < glyph->width; col++) {
                unsigned char pixel = glyph->bitmap[row * glyph->width + col];
                
                if (pixel > 3 && vert_count < 99994) {
                    float pixel_x = current_x + col * pixel_scale;
                    float alpha = gl_state.color[3] * ((float)pixel / 255.0f);
                    
                    float r = gl_state.color[0];
                    float g = gl_state.color[1];
                    float b = gl_state.color[2];
                    
                    // Triangle 1
                    verts[vert_count++] = {pixel_x, pixel_y, r, g, b, alpha};
                    verts[vert_count++] = {pixel_x + pixel_scale, pixel_y, r, g, b, alpha};
                    verts[vert_count++] = {pixel_x + pixel_scale, pixel_y + pixel_scale, r, g, b, alpha};
                    
                    // Triangle 2
                    verts[vert_count++] = {pixel_x, pixel_y, r, g, b, alpha};
                    verts[vert_count++] = {pixel_x + pixel_scale, pixel_y + pixel_scale, r, g, b, alpha};
                    verts[vert_count++] = {pixel_x, pixel_y + pixel_scale, r, g, b, alpha};
                }
            }
        }
        
        // Move to next character position
        current_x += glyph_width;
    }
    
    // Draw all glyphs at once
    if (vert_count > 0) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        draw_vertices(verts, vert_count, GL_TRIANGLES);
    }
}


void gl_draw_line(float x1, float y1, float x2, float y2, float width) {
    glLineWidth(width);
    Vertex verts[2] = {
        {x1, y1, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x2, y2, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 2, GL_LINES);
}

void gl_draw_rect_filled(float x, float y, float width, float height) {
    Vertex verts[6] = {
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 6, GL_TRIANGLES);
}

void gl_draw_rect_outline(float x, float y, float width, float height, float line_width) {
    glLineWidth(line_width);
    Vertex verts[5] = {
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x + width, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y + height, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]},
        {x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]}
    };
    draw_vertices(verts, 5, GL_LINE_STRIP);
}

void gl_draw_circle(float cx, float cy, float radius, int segments) {
    Vertex *verts = malloc((segments + 2) * sizeof(Vertex));
    verts[0] = (Vertex){cx, cy, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        verts[i+1] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    draw_vertices(verts, segments + 2, GL_TRIANGLE_FAN);
    free(verts);
}

void gl_draw_circle_outline(float cx, float cy, float radius, float line_width, int segments) {
    glLineWidth(line_width);
    Vertex *verts = malloc((segments + 1) * sizeof(Vertex));
    
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        verts[i] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    draw_vertices(verts, segments + 1, GL_LINE_STRIP);
    free(verts);
}

void gl_draw_polygon(float *points, int num_points, int filled) {
    Vertex *verts = malloc(num_points * sizeof(Vertex));
    for (int i = 0; i < num_points; i++) {
        verts[i] = (Vertex){points[i*2], points[i*2+1], gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    draw_vertices(verts, num_points, filled ? GL_TRIANGLES : GL_LINE_STRIP);
    free(verts);
}

void gl_draw_polyline(float *points, int num_points, float line_width) {
    glLineWidth(line_width);
    Vertex *verts = malloc(num_points * sizeof(Vertex));
    for (int i = 0; i < num_points; i++) {
        verts[i] = (Vertex){points[i*2], points[i*2+1], gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    draw_vertices(verts, num_points, GL_LINE_STRIP);
    free(verts);
}

// Helper to draw transformed polygon outline for comets
static void draw_comet_polygon(Comet *c, double points[][2], int num_points, float line_width) {
    if (!c) return;
    
    // Allocate for points + closing point
    Vertex *verts = malloc((num_points + 1) * sizeof(Vertex));
    
    float angle = c->base_angle + c->rotation * M_PI / 180.0f;
    float cos_a = cosf(angle);
    float sin_a = sinf(angle);
    
    // Transform and add all points
    for (int j = 0; j < num_points; j++) {
        // Rotate point around origin
        float x = (float)(points[j][0] * cos_a - points[j][1] * sin_a);
        float y = (float)(points[j][0] * sin_a + points[j][1] * cos_a);
        
        // Translate to comet position
        x += (float)c->x;
        y += (float)c->y;
        
        verts[j] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    // Close the polygon by repeating first vertex
    verts[num_points] = verts[0];
    
    glLineWidth(line_width);
    // Use GL_LINE_STRIP with num_points+1 to close the polygon
    draw_vertices(verts, num_points + 1, GL_LINE_STRIP);
    free(verts);
}

// ✓ VECTOR-BASED ASTEROIDS (like original Asteroids arcade game)
// Matches the cairo version exactly
void draw_comet_buster_comets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        
        // Skip inactive (destroyed) comets - DO NOT RENDER THEM
        if (!c->active) continue;
        
        // Set color and line width
        gl_set_color(c->color[0], c->color[1], c->color[2]);
        
        double radius = c->radius;
        
        // Use rotation_speed as a shape variant seed (deterministic but varied)
        int shape_variant = (int)c->rotation_speed % 3;
        
        if (c->size == COMET_MEGA) {
            // Mega asteroid: giant 12+ pointed shape that appears on boss waves (wave % 5 == 0)
            // Thicker lines for emphasis
            
            if (shape_variant == 0) {
                // Complex mega variant 1
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.8, radius * 0.55},
                    {radius * 0.6, radius * 0.9},
                    {radius * 0.2, radius * 0.95},
                    {-radius * 0.4, radius * 0.85},
                    {-radius * 0.75, radius * 0.65},
                    {-radius * 0.95, radius * 0.2},
                    {-radius * 0.9, -radius * 0.35},
                    {-radius * 0.6, -radius * 0.8},
                    {-radius * 0.1, -radius * 0.95},
                    {radius * 0.5, -radius * 0.85},
                    {radius * 0.85, -radius * 0.5}
                };
                draw_comet_polygon(c, points, 12, 3.5f);
            } else if (shape_variant == 1) {
                // Complex mega variant 2
                double points[][2] = {
                    {radius * 0.95, radius * 0.15},
                    {radius * 0.7, radius * 0.75},
                    {radius * 0.3, radius * 0.95},
                    {-radius * 0.2, radius * 0.9},
                    {-radius * 0.65, radius * 0.75},
                    {-radius * 0.9, radius * 0.3},
                    {-radius * 0.95, -radius * 0.2},
                    {-radius * 0.75, -radius * 0.7},
                    {-radius * 0.35, -radius * 0.92},
                    {radius * 0.15, -radius * 0.95},
                    {radius * 0.65, -radius * 0.75},
                    {radius * 0.9, -radius * 0.35}
                };
                draw_comet_polygon(c, points, 12, 3.5f);
            } else {
                // Complex mega variant 3
                double points[][2] = {
                    {radius, -radius * 0.1},
                    {radius * 0.8, radius * 0.6},
                    {radius * 0.5, radius * 0.88},
                    {radius * 0.1, radius * 0.96},
                    {-radius * 0.35, radius * 0.88},
                    {-radius * 0.7, radius * 0.7},
                    {-radius * 0.95, radius * 0.15},
                    {-radius * 0.88, -radius * 0.4},
                    {-radius * 0.55, -radius * 0.85},
                    {-radius * 0.05, -radius * 0.96},
                    {radius * 0.6, -radius * 0.8},
                    {radius * 0.9, -radius * 0.4}
                };
                draw_comet_polygon(c, points, 12, 3.5f);
            }
        } else if (c->size == COMET_LARGE) {
            // Large asteroid: 10-pointed jagged circle
            if (shape_variant == 0) {
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.8, radius * 0.6},
                    {radius * 0.4, radius * 0.92},
                    {-radius * 0.2, radius * 0.95},
                    {-radius * 0.7, radius * 0.7},
                    {-radius * 0.95, radius * 0.1},
                    {-radius * 0.85, -radius * 0.5},
                    {-radius * 0.3, -radius * 0.95},
                    {radius * 0.3, -radius * 0.92},
                    {radius * 0.85, -radius * 0.5}
                };
                draw_comet_polygon(c, points, 10, 2.5f);
            } else if (shape_variant == 1) {
                double points[][2] = {
                    {radius * 0.95, radius * 0.2},
                    {radius * 0.65, radius * 0.8},
                    {radius * 0.1, radius * 0.98},
                    {-radius * 0.5, radius * 0.85},
                    {-radius * 0.9, radius * 0.35},
                    {-radius * 0.95, -radius * 0.15},
                    {-radius * 0.6, -radius * 0.8},
                    {radius * 0.1, -radius * 0.98},
                    {radius * 0.65, -radius * 0.75},
                    {radius * 0.9, -radius * 0.3}
                };
                draw_comet_polygon(c, points, 10, 2.5f);
            } else {
                double points[][2] = {
                    {radius * 0.9, -radius * 0.3},
                    {radius * 0.7, radius * 0.7},
                    {radius * 0.2, radius * 0.98},
                    {-radius * 0.4, radius * 0.92},
                    {-radius * 0.85, radius * 0.5},
                    {-radius * 0.98, -radius * 0.1},
                    {-radius * 0.55, -radius * 0.85},
                    {radius * 0.15, -radius * 0.99},
                    {radius * 0.75, -radius * 0.65},
                    {radius * 0.95, radius * 0.15}
                };
                draw_comet_polygon(c, points, 10, 2.5f);
            }
        } else if (c->size == COMET_MEDIUM) {
            // Medium asteroid: 8-pointed
            if (shape_variant == 0) {
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.7, radius * 0.7},
                    {0, radius},
                    {-radius * 0.7, radius * 0.7},
                    {-radius, 0},
                    {-radius * 0.7, -radius * 0.7},
                    {0, -radius},
                    {radius * 0.7, -radius * 0.7}
                };
                draw_comet_polygon(c, points, 8, 2.0f);
            } else if (shape_variant == 1) {
                double points[][2] = {
                    {radius * 0.95, radius * 0.3},
                    {radius * 0.5, radius * 0.85},
                    {-radius * 0.2, radius * 0.98},
                    {-radius * 0.8, radius * 0.6},
                    {-radius * 0.95, -radius * 0.3},
                    {-radius * 0.5, -radius * 0.85},
                    {radius * 0.2, -radius * 0.98},
                    {radius * 0.8, -radius * 0.6}
                };
                draw_comet_polygon(c, points, 8, 2.0f);
            } else {
                double points[][2] = {
                    {radius * 0.85, -radius * 0.5},
                    {radius * 0.6, radius * 0.8},
                    {-radius * 0.3, radius * 0.95},
                    {-radius * 0.9, radius * 0.35},
                    {-radius * 0.8, -radius * 0.6},
                    {-radius * 0.2, -radius * 0.98},
                    {radius * 0.7, -radius * 0.7},
                    {radius * 0.95, radius * 0.3}
                };
                draw_comet_polygon(c, points, 8, 2.0f);
            }
        } else if (c->size == COMET_SMALL) {
            // Small asteroid: 6-pointed
            double points[][2] = {
                {radius, 0},
                {radius * 0.5, radius * 0.866},
                {-radius * 0.5, radius * 0.866},
                {-radius, 0},
                {-radius * 0.5, -radius * 0.866},
                {radius * 0.5, -radius * 0.866}
            };
            draw_comet_polygon(c, points, 6, 1.5f);
        }
    }
}

void draw_comet_buster_gl(Visualizer *visualizer, void *cr) {
    if (!visualizer) return;
    
    gl_init();
    
    CometBusterGame *game = &visualizer->comet_buster;
    int width = visualizer->width;
    int height = visualizer->height;
    
    gl_setup_2d_projection(width, height);
    
#ifdef ExternalSound
    // Draw splash screen if active
    if (game->splash_screen_active) {
        comet_buster_draw_splash_screen_gl(game, cr, width, height);
        return;  // Don't draw game yet
    }
#endif
    
    // Background is already set in on_realize, no need to draw again
    
    // Draw grid (extended 50 pixels to the right)
    gl_set_color(0.1f, 0.15f, 0.35f);
    glLineWidth(0.5f);
    
    for (int i = 0; i <= width + 50; i += 50) {
        gl_draw_line(i, 0, i, height, 0.5f);
    }
    for (int i = 0; i <= height; i += 50) {
        gl_draw_line(0, i, width + 50, i, 0.5f);
    }
    
    // Draw game elements
    draw_comet_buster_comets_gl(game, cr, width, height);
    draw_comet_buster_bullets_gl(game, cr, width, height);
    draw_comet_buster_enemy_ships_gl(game, cr, width, height);
    draw_comet_buster_ufos_gl(game, cr, width, height);
    
    // Draw boss (either Spawn Queen or regular Death Star)
    if (game->boss_active) {
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            draw_spawn_queen_boss_gl(&game->spawn_queen, cr, width, height);
        } else if (game->boss.active) {
            if (game->current_wave % 30 == 5) {
               draw_comet_buster_boss_gl(&game->boss, cr, width, height);
           } else if (game->current_wave % 30 == 10) {
               draw_spawn_queen_boss_gl(&game->spawn_queen, cr, width, height);
           } else if (game->current_wave % 30 == 15) {
              draw_void_nexus_boss_gl(&game->boss, cr, width, height);
           } else if (game->current_wave % 30 == 20) {
              draw_harbinger_boss_gl(&game->boss, cr, width, height);
           } else if (game->current_wave % 30 == 25) {
              draw_star_vortex_boss_gl(&game->boss, cr, width, height);
           } else if (game->current_wave % 30 == 0) {
              draw_singularity_boss_gl(&game->boss, cr, width, height);
           }
       }
    }
    
    draw_comet_buster_enemy_bullets_gl(game, cr, width, height);
    draw_comet_buster_canisters_gl(game, cr, width, height);
    draw_comet_buster_missile_pickups_gl(game, cr, width, height);
    draw_comet_buster_bomb_pickups_gl(game, cr, width, height);
    draw_comet_buster_missiles_gl(game, cr, width, height);
    draw_comet_buster_bombs_gl(game, cr, width, height);
    draw_comet_buster_particles_gl(game, cr, width, height);
    draw_comet_buster_ship_gl(game, cr, width, height);
    
    // Draw boss explosion effect
    boss_explosion_draw_gl(&game->boss_explosion_effect, cr);
    
    // Draw HUD
    draw_comet_buster_hud_gl(game, cr, width, height);
    
    // Draw game over
    if (game->game_over) {
        draw_comet_buster_game_over_gl(game, cr, width, height);
    }
}

void draw_comet_buster_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->bullet_count; i++) {
        Bullet *b = &game->bullets[i];
        if (!b->active) continue;
        
        // Draw bullet as yellow diamond (4 points)
        // Points: (x+size, y), (x, y+size), (x-size, y), (x, y-size)
        double size = 3.0;
        Vertex diamond_verts[5] = {
            {(float)(b->x + size), (float)b->y, 1.0f, 1.0f, 0.0f, 1.0f},     // Right
            {(float)b->x, (float)(b->y + size), 1.0f, 1.0f, 0.0f, 1.0f},    // Bottom
            {(float)(b->x - size), (float)b->y, 1.0f, 1.0f, 0.0f, 1.0f},    // Left
            {(float)b->x, (float)(b->y - size), 1.0f, 1.0f, 0.0f, 1.0f},    // Top
            {(float)(b->x + size), (float)b->y, 1.0f, 1.0f, 0.0f, 1.0f}     // Close back to right
        };
        
        // Draw filled diamond
        gl_set_color(1.0f, 1.0f, 0.0f);
        draw_vertices(diamond_verts, 5, GL_TRIANGLE_FAN);
        
        // Draw bullet trail (classic Asteroids style)
        double trail_length = 5.0;
        double norm_len = sqrt(b->vx * b->vx + b->vy * b->vy);
        if (norm_len > 0.1) {
            double trail_x = b->x - (b->vx / norm_len) * trail_length;
            double trail_y = b->y - (b->vy / norm_len) * trail_length;
            
            gl_set_color_alpha(1.0f, 1.0f, 0.0f, 0.3f);
            glLineWidth(0.5f);
            gl_draw_line((float)trail_x, (float)trail_y, (float)b->x, (float)b->y, 0.5f);
        }
    }
}

void draw_comet_buster_enemy_ships_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        if (!ship->active) continue;
        
        // Determine color based on ship type
        float ship_r = 0.2f, ship_g = 0.6f, ship_b = 1.0f;  // Default: blue (patrol, type 0)
        
        switch (ship->ship_type) {
            case 1:  // Aggressive red ship
                ship_r = 1.0f; ship_g = 0.0f; ship_b = 0.0f;
                break;
            case 2:  // Hunter green ship
                ship_r = 0.2f; ship_g = 1.0f; ship_b = 0.2f;
                break;
            case 3:  // Sentinel purple ship
                ship_r = 0.8f; ship_g = 0.2f; ship_b = 1.0f;
                break;
            case 4:  // Brown coat elite cyan ship
                ship_r = 0.0f; ship_g = 0.9f; ship_b = 1.0f;
                break;
            case 5:  // Juggernaut gold ship
                ship_r = 1.0f; ship_g = 0.84f; ship_b = 0.0f;
                break;
        }
        
        // Determine size based on ship type
        double ship_size;
        if (ship->ship_type == 5) {
            ship_size = 36.0;  // Juggernaut: 3x
        } else if (ship->ship_type == 4) {
            ship_size = 18.0;  // Brown coat: 1.5x
        } else {
            ship_size = 12.0;  // All others: 1x
        }
        
        // Transform triangle points
        float cos_a = cosf((float)ship->angle);
        float sin_a = sinf((float)ship->angle);
        
        // Triangle points in local coordinates:
        // Front: (ship_size, 0)
        // Back left: (-ship_size, -ship_size/1.5)
        // Back right: (-ship_size, ship_size/1.5)
        double local_points[3][2] = {
            {ship_size, 0},
            {-ship_size, -ship_size / 1.5},
            {-ship_size, ship_size / 1.5}
        };
        
        // Transform to world coordinates
        Vertex ship_verts[4];
        for (int j = 0; j < 3; j++) {
            float x = (float)local_points[j][0];
            float y = (float)local_points[j][1];
            
            // Rotate by ship angle
            float rotated_x = x * cos_a - y * sin_a;
            float rotated_y = x * sin_a + y * cos_a;
            
            // Translate to ship position
            rotated_x += (float)ship->x;
            rotated_y += (float)ship->y;
            
            ship_verts[j] = (Vertex){rotated_x, rotated_y, ship_r, ship_g, ship_b, 1.0f};
        }
        
        // Close the polygon
        ship_verts[3] = ship_verts[0];
        
        // Draw filled triangle
        gl_set_color(ship_r, ship_g, ship_b);
        draw_vertices(ship_verts, 3, GL_TRIANGLE_FAN);
        
        // Draw outline
        glLineWidth(1.5f);
        gl_set_color(ship_r, ship_g, ship_b);
        draw_vertices(ship_verts, 4, GL_LINE_STRIP);
        
        // Draw health indicator (green line at top of ship)
        double health_x = ship->x + ship_size - 5.0;
        double health_y_top = ship->y - ship_size - 3.0;
        double health_y_bottom = ship->y - ship_size;
        
        gl_set_color(0.2f, 1.0f, 0.2f);
        glLineWidth(1.0f);
        gl_draw_line((float)health_x, (float)health_y_top, (float)health_x, (float)health_y_bottom, 1.0f);
        
        // Draw Juggernaut health bar (for type 5 only)
        if (ship->ship_type == 5) {
            double bar_width = 60.0;
            double bar_height = 8.0;
            double bar_x = ship->x - bar_width / 2.0;
            double bar_y = ship->y + 50.0;
            
            // Background bar
            gl_set_color(0.3f, 0.3f, 0.3f);
            gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height);
            
            // Outline
            glLineWidth(1.0f);
            gl_set_color(0.3f, 0.3f, 0.3f);
            gl_draw_rect_outline((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height, 1.0f);
            
            // Health fill (green to yellow to red)
            double health_ratio = (double)ship->health / 10.0;
            if (health_ratio < 0.0) health_ratio = 0.0;
            if (health_ratio > 1.0) health_ratio = 1.0;
            
            float bar_r, bar_g, bar_b;
            if (health_ratio > 0.5) {
                // Green to yellow (high health)
                bar_r = (1.0f - (float)health_ratio) * 2.0f;
                bar_g = 1.0f;
                bar_b = 0.0f;
            } else {
                // Yellow to red (low health)
                bar_r = 1.0f;
                bar_g = (float)health_ratio * 2.0f;
                bar_b = 0.0f;
            }
            
            gl_set_color(bar_r, bar_g, bar_b);
            double fill_width = (bar_width - 2.0) * health_ratio;
            gl_draw_rect_filled((float)(bar_x + 1.0), (float)(bar_y + 1.0), (float)fill_width, (float)(bar_height - 2.0));
        }
        
        // ========== SHIELD CIRCLE ==========
        if (ship->shield_health > 0) {
            // Determine shield color based on ship type
            float shield_r = 0.2f, shield_g = 0.6f, shield_b = 1.0f;
            float shield_alpha = 0.5f;
            
            switch (ship->ship_type) {
                case 1:  // Red ship shield: orange/red
                    shield_r = 1.0f; shield_g = 0.5f; shield_b = 0.0f;
                    break;
                case 2:  // Green ship shield: bright green
                    shield_r = 0.5f; shield_g = 1.0f; shield_b = 0.5f;
                    break;
                case 3:  // Sentinel purple shield: bright purple
                    shield_r = 0.8f; shield_g = 0.4f; shield_b = 1.0f;
                    break;
                case 4:  // Brown coat cyan shield - bright cyan
                    shield_r = 0.2f; shield_g = 0.9f; shield_b = 1.0f;
                    shield_alpha = 0.6f;  // Brighter for brown coat
                    break;
                case 5:  // Juggernaut gold shield - golden glow
                    shield_r = 1.0f; shield_g = 0.9f; shield_b = 0.2f;
                    break;
            }
            
            // Determine shield radius based on ship type
            double shield_radius;
            if (ship->ship_type == 5) {
                shield_radius = 50.0;  // Larger shield for juggernaut
            } else if (ship->ship_type == 4) {
                shield_radius = 24.0;  // Slightly larger for brown coat
            } else {
                shield_radius = 22.0;  // Standard shield radius
            }
            
            // Draw shield circle
            gl_set_color_alpha(shield_r, shield_g, shield_b, shield_alpha);
            gl_draw_circle_outline((float)ship->x, (float)ship->y, (float)shield_radius, 2.0f, 24);
            
            // Draw shield impact flash
            if (ship->shield_impact_timer > 0) {
                float flash_alpha = (float)(ship->shield_impact_timer / 0.2);
                if (flash_alpha > 1.0f) flash_alpha = 1.0f;
                
                double impact_x = 22.0 * cos(ship->shield_impact_angle);
                double impact_y = 22.0 * sin(ship->shield_impact_angle);
                
                // Bright white flash circle
                gl_set_color_alpha(1.0f, 1.0f, 1.0f, flash_alpha * 0.8f);
                gl_draw_circle((float)(ship->x + impact_x), (float)(ship->y + impact_y), 4.0f, 12);
                
                // Expanding rings
                double ring_radius = 6.0 + (1.0 - flash_alpha) * 10.0;
                gl_set_color_alpha(1.0f, 1.0f, 1.0f, flash_alpha * 0.4f);
                gl_draw_circle_outline((float)(ship->x + impact_x), (float)(ship->y + impact_y), (float)ring_radius, 1.0f, 16);
            }
        }
    }
}

void draw_comet_buster_ufos_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *ufo = &game->ufos[i];
        if (!ufo->active) continue;
        
        // Set color based on damage state
        float ufo_r = 0.0f, ufo_g = 1.0f, ufo_b = 1.0f;  // Cyan
        if (ufo->damage_flash_timer > 0) {
            ufo_r = 1.0f; ufo_g = 1.0f; ufo_b = 1.0f;  // White when hit
        }
        
        // UFO dimensions
        double dome_width = 40.0;
        double dome_height = 20.0;
        double porthole_radius = 3.5;
        double porthole_spacing = 14.0;
        double fin_width = 5.0;
        double fin_height = 6.0;
        
        // ========== DOME (Top semicircle) ==========
        gl_set_color(ufo_r, ufo_g, ufo_b);
        glLineWidth(1.5f);
        
        // Draw top dome - semicircle
        int dome_segments = 20;
        Vertex dome_verts[21];
        for (int j = 0; j <= dome_segments; j++) {
            double angle = M_PI + (j * M_PI / dome_segments);  // From PI to 2*PI (top half)
            float x = (float)(ufo->x + (dome_width / 2.0) * cos(angle));
            float y = (float)(ufo->y + (dome_width / 2.0) * sin(angle));
            dome_verts[j] = (Vertex){x, y, ufo_r, ufo_g, ufo_b, 1.0f};
        }
        draw_vertices(dome_verts, dome_segments + 1, GL_LINE_STRIP);
        
        // ========== FLAT LINE (where dome meets body) ==========
        gl_draw_line((float)(ufo->x - dome_width / 2.0), (float)ufo->y,
                     (float)(ufo->x + dome_width / 2.0), (float)ufo->y, 1.5f);
        
        // ========== THREE PORTHOLES ==========
        // Left porthole
        gl_set_color(ufo_r, ufo_g, ufo_b);
        gl_draw_circle_outline((float)(ufo->x - porthole_spacing), 
                              (float)(ufo->y + dome_height / 2.0), 
                              (float)porthole_radius, 1.5f, 12);
        
        // Center porthole
        gl_draw_circle_outline((float)ufo->x, 
                              (float)(ufo->y + dome_height / 2.0), 
                              (float)porthole_radius, 1.5f, 12);
        
        // Right porthole
        gl_draw_circle_outline((float)(ufo->x + porthole_spacing), 
                              (float)(ufo->y + dome_height / 2.0), 
                              (float)porthole_radius, 1.5f, 12);
        
        // ========== FINS ==========
        // Left fin
        gl_draw_rect_outline((float)(ufo->x - porthole_spacing - fin_width / 2.0),
                            (float)(ufo->y + dome_height / 2.0),
                            (float)fin_width, (float)fin_height, 1.5f);
        
        // Right fin
        gl_draw_rect_outline((float)(ufo->x + porthole_spacing - fin_width / 2.0),
                            (float)(ufo->y + dome_height / 2.0),
                            (float)fin_width, (float)fin_height, 1.5f);
        
        // ========== HEALTH INDICATOR DOTS ==========
        if (ufo->health < ufo->max_health) {
            for (int h = 0; h < ufo->max_health; h++) {
                if (h < ufo->health) {
                    gl_set_color(0.0f, 1.0f, 1.0f);  // Cyan for healthy
                } else {
                    gl_set_color(0.3f, 0.3f, 0.3f);  // Dark gray for missing
                }
                
                double dot_x = ufo->x - 10.0 + h * 10.0;
                double dot_y = ufo->y - dome_height - 8.0;
                gl_draw_circle((float)dot_x, (float)dot_y, 2.0f, 12);
            }
        }
    }
}

void draw_comet_buster_enemy_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        Bullet *b = &game->enemy_bullets[i];
        if (!b->active) continue;
        gl_set_color(0.0f, 1.0f, 1.0f);  // Cyan - matches Cairo version
        gl_draw_circle(b->x, b->y, 3.0f, 8);
    }
}

void draw_comet_buster_canisters_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->canister_count; i++) {
        Canister *c = &game->canisters[i];
        if (!c->active) continue;
        
        // Alpha based on remaining lifetime (fade out near end)
        float alpha = 1.0f;
        if (c->lifetime < 2.0) {
            alpha = (float)(c->lifetime / 2.0);
        }
        
        // Draw shield shape as a simple circle for now (easier to debug)
        // Darker cyan fill
        gl_set_color_alpha(0.0f, 0.6f, 0.8f, alpha * 0.3f);
        gl_draw_circle(c->x, c->y, 12.0f, 24);
        
        // Bright cyan outline
        gl_set_color_alpha(0.0f, 1.0f, 1.0f, alpha);
        gl_draw_circle_outline(c->x, c->y, 12.0f, 2.0f, 24);
        
        // Draw medical cross in center
        gl_set_color_alpha(0.0f, 1.0f, 1.0f, alpha);
        float cross_size = 5.0f;
        gl_draw_line(c->x - cross_size, (float)c->y, c->x + cross_size, (float)c->y, 1.5f);
        gl_draw_line((float)c->x, c->y - cross_size, (float)c->x, c->y + cross_size, 1.5f);
    }
}

void draw_comet_buster_missile_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr; (void)width; (void)height;
    
    for (int i = 0; i < game->missile_pickup_count; i++) {
        MissilePickup *pickup = &game->missile_pickups[i];
        if (!pickup->active) continue;
        
        // Alpha fade out in last 2 seconds
        float alpha = 1.0f;
        if (pickup->lifetime < 2.0) {
            alpha = (float)(pickup->lifetime / 2.0);
        }
        
        double size = 10.0;
        
        // Draw X shape (orange lines)
        gl_set_color_alpha(1.0f, 0.65f, 0.0f, alpha);
        gl_draw_line((float)(pickup->x - size), (float)(pickup->y - size), 
                     (float)(pickup->x + size), (float)(pickup->y + size), 2.5f);
        gl_draw_line((float)(pickup->x + size), (float)(pickup->y - size), 
                     (float)(pickup->x - size), (float)(pickup->y + size), 2.5f);
        
        // Draw circle outline (orange)
        gl_draw_circle_outline(pickup->x, pickup->y, size + 4.0, 1.5f, 24);
        
        // Draw yellow filled circle in center
        gl_set_color_alpha(1.0f, 1.0f, 0.0f, alpha);
        gl_draw_circle(pickup->x, pickup->y, 2.5f, 12);
    }
}

void draw_comet_buster_bomb_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr; (void)width; (void)height;
    
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        BombPickup *p = &game->bomb_pickups[i];
        if (!p->active) continue;
        
        // Alpha fade out in last 2 seconds
        float alpha = 1.0f;
        if (p->lifetime < 2.0) {
            alpha = (float)(p->lifetime / 2.0);
        }
        
        // Draw main bomb circle (orange/yellow)
        gl_set_color_alpha(1.0f, 0.7f, 0.0f, alpha);
        gl_draw_circle(p->x, p->y, 12.0f, 24);
        
        // Draw fuse (line sticking out from top)
        gl_set_color_alpha(0.8f, 0.4f, 0.2f, alpha);
        gl_draw_line((float)p->x, (float)p->y - 12.0f, (float)p->x, (float)p->y - 22.0f, 1.5f);
    }
}

void draw_comet_buster_missiles_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    // Color scheme for each missile type
    // Type 0: CYAN - Target furthest comet
    // Type 1: RED - Target ships and boss
    // Type 2: GREEN - Target closest comets
    // Type 3: MAGENTA - Target comets ~400px away
    // Type 4: ORANGE - Target comets 200-600px away
    
    for (int i = 0; i < game->missile_count; i++) {
        Missile *missile = &game->missiles[i];
        if (!missile->active) continue;
        
        // Calculate alpha fade based on lifetime
        float alpha = 1.0f;
        if (missile->lifetime < 0.5) {
            alpha = (float)(missile->lifetime / 0.5);
        }
        
        // Determine fill color based on missile type
        float fill_r = 1.0f, fill_g = 1.0f, fill_b = 0.2f;  // Default yellow
        float stroke_r = 1.0f, stroke_g = 0.8f, stroke_b = 0.0f;  // Default orange
        
        switch (missile->missile_type) {
            case 0:  // Furthest comet - CYAN
                fill_r = 0.2f; fill_g = 1.0f; fill_b = 1.0f;
                stroke_r = 0.0f; stroke_g = 0.8f; stroke_b = 1.0f;
                break;
            case 1:  // Ships and boss - RED
                fill_r = 1.0f; fill_g = 0.2f; fill_b = 0.2f;
                stroke_r = 1.0f; stroke_g = 0.0f; stroke_b = 0.0f;
                break;
            case 2:  // Closest comets - GREEN
                fill_r = 0.2f; fill_g = 1.0f; fill_b = 0.2f;
                stroke_r = 0.0f; stroke_g = 0.8f; stroke_b = 0.0f;
                break;
            case 3:  // Comets ~400px away - MAGENTA
                fill_r = 1.0f; fill_g = 0.2f; fill_b = 1.0f;
                stroke_r = 0.8f; stroke_g = 0.0f; stroke_b = 0.8f;
                break;
            case 4:  // Comets 200-600px away - ORANGE
                fill_r = 1.0f; fill_g = 0.6f; fill_b = 0.0f;
                stroke_r = 1.0f; stroke_g = 0.4f; stroke_b = 0.0f;
                break;
        }
        
        // Missile diamond shape (kite):
        // Points: (8,0), (-4,3), (-2,0), (-4,-3)
        float points[8] = {
            8.0f,  0.0f,    // Front point
            -4.0f, 3.0f,    // Bottom right
            -2.0f, 0.0f,    // Middle notch
            -4.0f, -3.0f    // Top right
        };
        
        // Rotate points around missile angle
        float cos_a = cosf((float)missile->angle);
        float sin_a = sinf((float)missile->angle);
        
        Vertex verts[4];
        for (int j = 0; j < 4; j++) {
            float x = points[j*2];
            float y = points[j*2+1];
            
            // Rotate point
            float rotated_x = x * cos_a - y * sin_a;
            float rotated_y = x * sin_a + y * cos_a;
            
            // Translate to missile position
            rotated_x += (float)missile->x;
            rotated_y += (float)missile->y;
            
            verts[j] = (Vertex){rotated_x, rotated_y, fill_r, fill_g, fill_b, alpha};
        }
        
        // Draw filled missile
        gl_set_color_alpha(fill_r, fill_g, fill_b, alpha);
        draw_vertices(verts, 4, GL_TRIANGLE_FAN);
        
        // Draw missile outline
        gl_set_color_alpha(stroke_r, stroke_g, stroke_b, alpha);
        glLineWidth(1.5f);
        
        // Create outline vertices (repeat first to close)
        Vertex outline_verts[5];
        for (int j = 0; j < 4; j++) {
            outline_verts[j] = verts[j];
        }
        outline_verts[4] = verts[0];  // Close the shape
        
        draw_vertices(outline_verts, 5, GL_LINE_STRIP);
    }
}

void draw_comet_buster_bombs_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr; (void)width; (void)height;
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *bomb = &game->bombs[i];
        if (!bomb->active) continue;
        
        if (!bomb->detonated) {
            // Pulsing effect based on time left
            double pulse = 1.0 - (bomb->lifetime / bomb->max_lifetime) * 0.3;  // Pulses as countdown happens
            
            // Main bomb body - orange circle with pulsing
            gl_set_color_alpha((float)(1.0 * pulse), 0.6f, 0.0f, 1.0f);
            gl_draw_circle(bomb->x, bomb->y, 15.0f, 24);
            
            // Outline - yellow
            gl_set_color(1.0f, 0.8f, 0.0f);
            gl_draw_circle_outline(bomb->x, bomb->y, 15.0f, 2.0f, 24);
            
            // Fuse - brown line sticking up
            gl_set_color(0.7f, 0.3f, 0.1f);
            gl_draw_line(bomb->x, bomb->y - 15.0f, bomb->x, bomb->y - 28.0f, 2.0f);
            
            // Spark at fuse - yellow circle
            gl_set_color(1.0f, 1.0f, 0.2f);
            gl_draw_circle(bomb->x, bomb->y - 28.0f, 3.0f, 12);
            
            // Countdown text - white number
            gl_set_color(1.0f, 1.0f, 1.0f);
            int countdown = (int)(bomb->lifetime + 1);
            if (countdown < 1) countdown = 1;
            
            char text[8];
            snprintf(text, sizeof(text), "%d", countdown);
            
            // Draw countdown text centered on bomb
            gl_draw_text_simple(text, (int)bomb->x - 3, (int)bomb->y - 2, 12);
        } else {
            // Draw explosion wave
            // Expanding circle with fading opacity
            double wave_progress = bomb->wave_radius / bomb->wave_max_radius;
            double opacity = 1.0 - wave_progress;  // Fades out as it expands
            
            gl_set_color_alpha(1.0f, 0.5f, 0.0f, (float)opacity);
            gl_draw_circle_outline(bomb->x, bomb->y, bomb->wave_radius, 2.0f, 24);
        }
    }
}

void draw_comet_buster_particles_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->particle_count; i++) {
        Particle *p = &game->particles[i];
        if (!p->active) continue;
        double alpha = p->lifetime / p->max_lifetime;
        gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha);
        gl_draw_circle(p->x, p->y, p->size, 6);
    }
}

void draw_comet_buster_ship_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    (void)width;
    (void)height;
    
    // ========== MAIN SHIP BODY ==========
    double ship_size = 12.0;
    float cos_a = cosf((float)game->ship_angle);
    float sin_a = sinf((float)game->ship_angle);
    
    // Ship triangle points (in local coordinates):
    // (12,0), (-12,-12), (-3.6,0), (-12,12), close back to (12,0)
    double local_points[4][2] = {
        {ship_size, 0},           // Front point
        {-ship_size, -ship_size}, // Top left
        {-ship_size * 0.3, 0},    // Side notch
        {-ship_size, ship_size}   // Bottom left
    };
    
    // Transform points to world coordinates
    Vertex ship_verts[5];
    for (int i = 0; i < 4; i++) {
        float x = (float)local_points[i][0];
        float y = (float)local_points[i][1];
        
        // Rotate by ship angle
        float rotated_x = x * cos_a - y * sin_a;
        float rotated_y = x * sin_a + y * cos_a;
        
        // Translate to ship position
        rotated_x += (float)game->ship_x;
        rotated_y += (float)game->ship_y;
        
        // Determine color with invulnerability flash
        float r = 0.0f, g = 1.0f, b = 0.0f;
        float a = 1.0f;
        
        if (game->invulnerability_time > 0) {
            float flash = sinf((float)game->invulnerability_time * 10.0f) * 0.5f + 0.5f;
            a = flash;
        }
        
        ship_verts[i] = (Vertex){rotated_x, rotated_y, r, g, b, a};
    }
    
    // Close the polygon by repeating first vertex (like cairo_close_path)
    ship_verts[4] = ship_verts[0];
    
    // Draw outline only - NO FILL (just stroke like Cairo version)
    gl_set_color(0.0f, 1.0f, 0.0f);
    glLineWidth(2.0f);
    draw_vertices(ship_verts, 5, GL_LINE_STRIP);
    
    // ========== MUZZLE FLASH ==========
    if (game->muzzle_flash_timer > 0) {
        float alpha = (float)(game->muzzle_flash_timer / 0.1);
        if (alpha > 1.0f) alpha = 1.0f;
        
        // Muzzle flash triangle: (12,0), (12+20,-5), (12+20,5), close
        double muzzle_local[3][2] = {
            {ship_size, 0},
            {ship_size + 20.0, -5.0},
            {ship_size + 20.0, 5.0}
        };
        
        Vertex muzzle_verts[3];
        for (int i = 0; i < 3; i++) {
            float x = (float)muzzle_local[i][0];
            float y = (float)muzzle_local[i][1];
            
            float rotated_x = x * cos_a - y * sin_a;
            float rotated_y = x * sin_a + y * cos_a;
            
            rotated_x += (float)game->ship_x;
            rotated_y += (float)game->ship_y;
            
            muzzle_verts[i] = (Vertex){rotated_x, rotated_y, 1.0f, 1.0f, 0.0f, alpha};
        }
        
        gl_set_color_alpha(1.0f, 1.0f, 0.0f, alpha);
        draw_vertices(muzzle_verts, 3, GL_TRIANGLE_FAN);
    }
    
    // ========== BURNER/THRUSTER EFFECT ==========
    if (game->burner_intensity > 0.01) {
        // Flicker effect
        float flicker = 0.7f + 0.3f * sinf((float)game->burner_intensity * 20.0f);
        float effective_intensity = (float)game->burner_intensity * flicker;
        
        // Calculate speed for length multiplier
        double speed = sqrt(game->ship_vx * game->ship_vx + game->ship_vy * game->ship_vy);
        double length_mult = 1.0 + (speed / 150.0) * 0.5;
        
        double flame_length = 30.0 * effective_intensity * length_mult;
        double flame_width = 8.0 * effective_intensity;
        
        // Main burner flame points (pointing backward from back of ship)
        // Ship back is at x = -ship_size (-12), flame extends from there
        double burner_local[4][2] = {
            {-ship_size, -flame_width},                          // Top at back of ship
            {-ship_size - flame_length, -flame_width * 0.3},    // Top tip of flame
            {-ship_size - flame_length, flame_width * 0.3},     // Bottom tip of flame
            {-ship_size, flame_width}                            // Bottom at back of ship
        };
        
        Vertex burner_verts[4];
        for (int i = 0; i < 4; i++) {
            float x = (float)burner_local[i][0];
            float y = (float)burner_local[i][1];
            
            float rotated_x = x * cos_a - y * sin_a;
            float rotated_y = x * sin_a + y * cos_a;
            
            rotated_x += (float)game->ship_x;
            rotated_y += (float)game->ship_y;
            
            // Flame color gradient: yellow -> orange -> red
            float color_r = 1.0f;
            float color_g = 0.7f * (1.0f - (float)i / 3.0f);  // Fade orange
            float color_b = 0.0f;
            
            burner_verts[i] = (Vertex){rotated_x, rotated_y, color_r, color_g, color_b, effective_intensity * 0.7f};
        }
        
        gl_set_color_alpha(1.0f, 0.7f, 0.0f, effective_intensity * 0.7f);
        draw_vertices(burner_verts, 4, GL_TRIANGLE_FAN);
    }
    
    // ========== SHIELD CIRCLE ==========
    if (game->shield_health > 0) {
        float shield_alpha = (float)game->shield_health / (float)game->max_shield_health;
        
        // Shield color: cyan when healthy, orange when medium, red when critical
        float shield_r = 0.0f, shield_g = 1.0f, shield_b = 1.0f;
        if (game->shield_health >= 2) {
            shield_r = 0.0f; shield_g = 1.0f; shield_b = 1.0f;  // Cyan
        } else if (game->shield_health >= 1) {
            shield_r = 1.0f; shield_g = 0.8f; shield_b = 0.0f;  // Orange
        } else {
            shield_r = 1.0f; shield_g = 0.3f; shield_b = 0.3f;  // Red
        }
        
        // Draw shield circle
        gl_set_color_alpha(shield_r, shield_g, shield_b, shield_alpha * 0.6f);
        gl_draw_circle_outline((float)game->ship_x, (float)game->ship_y, 28.0f, 2.5f, 24);
        
        // Draw shield segment pips
        glLineWidth(1.5f);
        double segment_angle = (2.0 * M_PI) / game->max_shield_health;
        
        for (int i = 0; i < game->shield_health; i++) {
            double angle = (i * segment_angle) - (M_PI / 2.0);
            double x1 = 24.0 * cos(angle);
            double y1 = 24.0 * sin(angle);
            double x2 = 32.0 * cos(angle);
            double y2 = 32.0 * sin(angle);
            
            gl_draw_line((float)(game->ship_x + x1), (float)(game->ship_y + y1),
                        (float)(game->ship_x + x2), (float)(game->ship_y + y2),
                        1.5f);
        }
        
        // Draw impact flash
        if (game->shield_impact_timer > 0) {
            float flash_alpha = (float)(game->shield_impact_timer / 0.2);
            if (flash_alpha > 1.0f) flash_alpha = 1.0f;
            
            double impact_x = 28.0 * cos(game->shield_impact_angle);
            double impact_y = 28.0 * sin(game->shield_impact_angle);
            
            // Bright white flash
            gl_set_color_alpha(1.0f, 1.0f, 1.0f, flash_alpha * 0.8f);
            gl_draw_circle((float)(game->ship_x + impact_x), (float)(game->ship_y + impact_y), 5.0f, 12);
            
            // Expanding rings
            double ring_radius = 8.0 + (1.0 - flash_alpha) * 12.0;
            gl_set_color_alpha(1.0f, 1.0f, 1.0f, flash_alpha * 0.4f);
            gl_draw_circle_outline((float)(game->ship_x + impact_x), (float)(game->ship_y + impact_y), 
                                  (float)ring_radius, 1.0f, 16);
        }
    }
}

void draw_comet_buster_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)cr; (void)width; (void)height;
    
    double body_radius = 35.0;
    
    // Main body - large dark gray circle
    gl_set_color(0.3f, 0.3f, 0.4f);
    gl_draw_circle(boss->x, boss->y, (float)body_radius, 32);
    
    // Highlight if taking damage
    if (boss->damage_flash_timer > 0) {
        gl_set_color_alpha(1.0f, 0.5f, 0.5f, 0.7f);
        gl_draw_circle(boss->x, boss->y, (float)body_radius, 32);
    }
    
    // Outer ring - metallic look
    gl_set_color_alpha(0.6f, 0.6f, 0.7f, 0.8f);
    gl_draw_circle_outline(boss->x, boss->y, (float)body_radius, 2.5f, 32);
    
    // Draw rotating pattern on the boss (8 lines radiating)
    gl_set_color(0.8f, 0.8f, 0.9f);
    for (int i = 0; i < 8; i++) {
        double angle = (i * 2.0 * M_PI / 8.0);
        double x1 = cos(angle) * 20.0;
        double y1 = sin(angle) * 20.0;
        double x2 = cos(angle) * 30.0;
        double y2 = sin(angle) * 30.0;
        
        gl_draw_line((float)(boss->x + x1), (float)(boss->y + y1),
                     (float)(boss->x + x2), (float)(boss->y + y2), 1.5f);
    }
    
    // Core/center glow - red pulsing
    double core_radius = 8.0;
    gl_set_color(1.0f, 0.2f, 0.2f);
    gl_draw_circle(boss->x, boss->y, (float)core_radius, 16);
    
    // Inner glow around core
    gl_set_color_alpha(1.0f, 0.3f, 0.3f, 0.6f);
    gl_draw_circle_outline(boss->x, boss->y, (float)(core_radius + 3.0), 1.5f, 16);
    
    // Draw health bar above boss
    double bar_width = 80.0;
    double bar_height = 6.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 50.0;
    
    // Background (gray)
    gl_set_color(0.3f, 0.3f, 0.3f);
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height);
    
    // Health fill (red)
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(1.0f, 0.2f, 0.2f);
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)(bar_width * health_ratio), (float)bar_height);
    
    // Border (white)
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height, 1.0f);
    
    // Draw shield if active
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_radius = 50.0;
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        
        // Outer shield glow (pulsing)
        float pulse_alpha = 0.3f + 0.1f * (float)sin(boss->shield_impact_timer * 10.0);
        gl_set_color_alpha(0.0f, 0.8f, 1.0f, pulse_alpha);
        gl_draw_circle(boss->x, boss->y, (float)shield_radius, 32);
        
        // Shield outline
        gl_set_color_alpha(0.0f, 1.0f, 1.0f, 0.8f);
        gl_draw_circle_outline(boss->x, boss->y, (float)shield_radius, 2.0f, 32);
        
        // Shield segments
        gl_set_color(0.0f, 1.0f, 1.0f);
        int num_segments = 12;
        for (int i = 0; i < num_segments; i++) {
            if (i < (int)(num_segments * shield_ratio)) {
                double angle = (i * 2.0 * M_PI / num_segments);
                double x1 = cos(angle) * (shield_radius - 3.0);
                double y1 = sin(angle) * (shield_radius - 3.0);
                double x2 = cos(angle) * (shield_radius + 3.0);
                double y2 = sin(angle) * (shield_radius + 3.0);
                
                gl_draw_line((float)(boss->x + x1), (float)(boss->y + y1),
                             (float)(boss->x + x2), (float)(boss->y + y2), 1.5f);
            }
        }
    }
    
    // Draw phase indicator
    const char *phase_text = "";
    float phase_r = 1.0f, phase_g = 1.0f, phase_b = 0.5f;
    
    if (boss->phase == 0) {
        phase_text = "NORMAL";
        phase_r = 1.0f; phase_g = 1.0f; phase_b = 0.5f;  // Yellow
    } else if (boss->phase == 1) {
        phase_text = "SHIELDED";
        phase_r = 0.0f; phase_g = 1.0f; phase_b = 1.0f;  // Cyan
    } else {
        phase_text = "ENRAGED!";
        phase_r = 1.0f; phase_g = 0.2f; phase_b = 0.2f;  // Red
    }
    
    gl_set_color(phase_r, phase_g, phase_b);
    gl_draw_text_simple(phase_text, (int)(boss->x - 25), (int)(boss->y - 25), 10);
}

void draw_spawn_queen_boss_gl(SpawnQueenBoss *queen, void *cr, int width, int height) {
    if (!queen || !queen->active) return;
    (void)cr; (void)width; (void)height;
    
    double major_axis = 70.0;
    double minor_axis = 45.0;
    int segments = 32;
    
    // Build vertices for the elliptical body
    Vertex body_verts[32];
    for (int i = 0; i < segments; i++) {
        double angle = 2.0 * M_PI * i / segments;
        float cos_a = (float)cos(angle);
        float sin_a = (float)sin(angle);
        
        // Rotate the point around the boss center
        float cos_rot = (float)cos(queen->rotation * M_PI / 180.0);
        float sin_rot = (float)sin(queen->rotation * M_PI / 180.0);
        
        float x = cos_a * (float)major_axis;
        float y = sin_a * (float)minor_axis;
        
        // Apply rotation
        float rx = x * cos_rot - y * sin_rot;
        float ry = x * sin_rot + y * cos_rot;
        
        body_verts[i] = {(float)queen->x + rx, (float)queen->y + ry, 0.7f, 0.3f, 0.8f, 1.0f};
    }
    
    // Draw filled magenta body
    draw_vertices(body_verts, segments, GL_POLYGON);
    
    // Draw rotating darker oval pattern on the body (shows rotation)
    gl_set_color_alpha(0.4f, 0.1f, 0.5f, 0.6f);
    double pattern_major = 50.0;
    double pattern_minor = 20.0;
    Vertex pattern_verts[32];
    for (int i = 0; i < segments; i++) {
        double angle = 2.0 * M_PI * i / segments;
        float cos_a = (float)cos(angle);
        float sin_a = (float)sin(angle);
        
        // Rotate the pattern point
        float cos_rot = (float)cos(queen->rotation * M_PI / 180.0);
        float sin_rot = (float)sin(queen->rotation * M_PI / 180.0);
        
        float x = cos_a * (float)pattern_major;
        float y = sin_a * (float)pattern_minor;
        
        // Apply rotation
        float rx = x * cos_rot - y * sin_rot;
        float ry = x * sin_rot + y * cos_rot;
        
        pattern_verts[i] = {(float)queen->x + rx, (float)queen->y + ry, 0.4f, 0.1f, 0.5f, 0.6f};
    }
    draw_vertices(pattern_verts, segments, GL_POLYGON);
    
    // Draw cyan outline ring
    gl_set_color_alpha(0.0f, 1.0f, 1.0f, 0.7f);
    gl_draw_circle_outline(queen->x, queen->y, (float)major_axis, 2.5f, segments);
    
    // Draw light cyan glow background
    gl_set_color_alpha(0.0f, 0.8f, 1.0f, 0.2f);
    gl_draw_circle(queen->x, queen->y, (float)(major_axis + 10.0), segments);
    
    // Spawn ports - 6 around equator with pulsing glow
    double port_radius = 6.0;
    double port_orbit = 50.0;
    
    // Determine port colors based on phase
    float port_r, port_g, port_b;
    if (queen->phase == 0) {
        port_r = 1.0f; port_g = 0.2f; port_b = 0.2f;  // Red
    } else if (queen->phase == 1) {
        port_r = 1.0f; port_g = 0.5f; port_b = 0.8f;  // Magenta
    } else {
        port_r = 0.8f; port_g = 0.3f; port_b = 1.0f;  // Purple
    }
    
    // Pulsing glow intensity
    double glow_intensity = 0.5 + 0.5 * sin(queen->spawn_particle_timer * 5.0);
    
    for (int i = 0; i < 6; i++) {
        double angle = 2.0 * M_PI * i / 6.0;
        double px = cos(angle) * port_orbit;
        double py = sin(angle) * port_orbit * 0.6;
        
        // Outer glow
        gl_set_color_alpha(port_r, port_g, port_b, (float)(glow_intensity * 0.5));
        gl_draw_circle(queen->x + px, queen->y + py, (float)(port_radius + 4.0), 16);
        
        // Port center (brighter)
        gl_set_color(port_r, port_g, port_b);
        gl_draw_circle(queen->x + px, queen->y + py, (float)port_radius, 16);
    }
    
    // Damage flash overlay
    if (queen->damage_flash_timer > 0) {
        gl_set_color_alpha(1.0f, 0.5f, 0.5f, 0.4f);
        gl_draw_circle(queen->x, queen->y, (float)major_axis, 32);
    }
    
    // Red pulsing core
    double core_size = 12.0 + 3.0 * sin(queen->phase_timer * 3.0);
    gl_set_color(1.0f, 0.2f, 0.2f);
    gl_draw_circle(queen->x, queen->y, (float)core_size, 16);
    
    // Health bar above boss
    double bar_width = 100.0;
    double bar_height = 8.0;
    double bar_x = queen->x - bar_width / 2.0;
    double bar_y = queen->y - 70.0;
    
    // Health bar - Background
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height);
    
    // Health fill
    double health_ratio = (double)queen->health / queen->max_health;
    gl_set_color(1.0f, 0.2f, 0.2f);
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)(bar_width * health_ratio), (float)bar_height);
    
    // Health bar border
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height, 1.0f);
    
    // Shield bar below health bar
    double shield_y = bar_y + bar_height + 2.0;
    
    // Shield bar - Background
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled((float)bar_x, (float)shield_y, (float)bar_width, (float)bar_height);
    
    // Shield fill (cyan)
    double shield_ratio = (double)queen->shield_health / queen->max_shield_health;
    gl_set_color(0.0f, 1.0f, 1.0f);
    gl_draw_rect_filled((float)bar_x, (float)shield_y, (float)(bar_width * shield_ratio), (float)bar_height);
    
    // Shield bar border
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline((float)bar_x, (float)shield_y, (float)bar_width, (float)bar_height, 1.0f);
    
    // Phase indicator text
    const char *phase_text = "";
    float text_r = 1.0f, text_g = 0.5f, text_b = 0.0f;
    
    if (queen->phase == 0) {
        phase_text = "RECRUITING";
        text_r = 1.0f; text_g = 0.5f; text_b = 0.0f;  // Orange
    } else if (queen->phase == 1) {
        phase_text = "AGGRESSIVE";
        text_r = 1.0f; text_g = 1.0f; text_b = 0.0f;  // Yellow
    } else {
        phase_text = "DESPERATE!";
        text_r = 1.0f; text_g = 0.0f; text_b = 0.0f;  // Red
    }
    
    gl_set_color(text_r, text_g, text_b);
    gl_draw_text_simple(phase_text, (int)(queen->x - 35), (int)(queen->y + 75), 11);
}

void draw_void_nexus_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    gl_set_color(0.2f, 0.2f, 0.8f);
    gl_draw_circle(boss->x, boss->y, 55.0f, 20);
    gl_set_color(0.5f, 0.5f, 1.0f);
    gl_draw_circle_outline(boss->x, boss->y, 55.0f, 2.0f, 20);
}

void draw_harbinger_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    gl_set_color(1.0f, 0.5f, 0.0f);
    gl_draw_circle(boss->x, boss->y, 45.0f, 20);
    gl_set_color(1.0f, 1.0f, 0.0f);
    gl_draw_circle_outline(boss->x, boss->y, 45.0f, 2.0f, 20);
}

void draw_star_vortex_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    gl_set_color(1.0f, 0.0f, 1.0f);
    gl_draw_circle(boss->x, boss->y, 50.0f, 20);
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_circle_outline(boss->x, boss->y, 50.0f, 2.0f, 20);
}

void draw_singularity_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    gl_set_color(0.1f, 0.1f, 0.1f);
    gl_draw_circle(boss->x, boss->y, 50.0f, 20);
    gl_set_color(0.5f, 0.0f, 1.0f);
    gl_draw_circle_outline(boss->x, boss->y, 50.0f, 3.0f, 20);
}

void draw_comet_buster_hud_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)height;   // Suppress unused parameter warning
    
    char text[256];
    
    // --- TOP LEFT SECTION ---
    // Score (with multiplier indicator)
    gl_set_color(1.0f, 1.0f, 1.0f);
    sprintf(text, "SCORE: %d (x%.1f)", game->score, game->score_multiplier);
    gl_draw_text_simple(text, 20, 30, 18);
    
    // Lives
    sprintf(text, "LIVES: %d", game->ship_lives);
    gl_draw_text_simple(text, 20, 55, 18);
    
    // Shield status
    sprintf(text, "SHIELD: %d/%d", game->shield_health, game->max_shield_health);
    if (game->shield_health <= 0) {
        gl_set_color(1.0f, 0.3f, 0.3f);  // Red when no shield
    } else if (game->shield_health == 1) {
        gl_set_color(1.0f, 0.8f, 0.0f);  // Orange when low
    } else {
        gl_set_color(0.0f, 1.0f, 1.0f);  // Cyan when healthy
    }
    gl_draw_text_simple(text, 20, 105, 18);
    gl_set_color(1.0f, 1.0f, 1.0f);  // Reset to white
    
    // --- TOP RIGHT SECTION ---
    // Wave
    gl_set_color(1.0f, 1.0f, 1.0f);
    sprintf(text, "WAVE: %d", game->current_wave);
    gl_draw_text_simple(text, width - 180, 30, 18);
    
    // Asteroids remaining
    sprintf(text, "ASTEROIDS: %d", game->comet_count);
    gl_draw_text_simple(text, width - 280, 55, 18);
    
    // Wave progress info (only show if wave is incomplete)
    if (game->wave_complete_timer > 0) {
        sprintf(text, "NEXT WAVE in %.1fs", game->wave_complete_timer);
        gl_set_color(1.0f, 1.0f, 0.0f);
        gl_draw_text_simple(text, width / 2 - 160, height / 2 - 50, 18);
        gl_set_color(1.0f, 1.0f, 1.0f);
    } else if (game->comet_count > 0) {
        int expected_count = comet_buster_get_wave_comet_count(game->current_wave);
        sprintf(text, "DESTROYED: %d/%d", expected_count - game->comet_count, expected_count);
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_text_simple(text, width - 280, 75, 12);
    }
    
    // --- FLOATING TEXT POPUPS ---
    gl_set_color(1.0f, 1.0f, 1.0f);
    for (int i = 0; i < game->floating_text_count; i++) {
        FloatingText *ft = &game->floating_texts[i];
        if (ft->active) {
            // Calculate fade (alpha) based on remaining lifetime
            float alpha = (float)(ft->lifetime / ft->max_lifetime);
            
            // Set color with fade
            gl_set_color_alpha((float)ft->color[0], (float)ft->color[1], (float)ft->color[2], alpha);
            
            // Draw text
            gl_draw_text_simple(ft->text, (int)ft->x - 30, (int)ft->y, 16);
        }
    }
    gl_set_color(1.0f, 1.0f, 1.0f);  // Reset to white
    
    // --- BOTTOM LEFT SECTION ---
    // Energy bar
    gl_set_color(1.0f, 1.0f, 1.0f);
    sprintf(text, "ENERGY: %.0f%%", game->energy_amount);
    
    // Color based on fuel level
    if (game->energy_amount < 20) {
        gl_set_color(1.0f, 0.2f, 0.2f);  // Red - critical
    } else if (game->energy_amount < 50) {
        gl_set_color(1.0f, 1.0f, 0.0f);  // Yellow - low
    } else {
        gl_set_color(0.2f, 1.0f, 0.2f);  // Green - good
    }
    
    gl_draw_text_simple(text, 20, height - 40, 14);
    
    // Draw fuel bar (visual indicator)
    double bar_width = 150;
    double bar_height = 12;
    double bar_x = 20;
    double bar_y = height - 25;
    
    // Background (dark)
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height);
    
    // Fuel level (colored)
    double fuel_percent = game->energy_amount / game->max_energy;
    if (fuel_percent > 0.5) {
        gl_set_color(0.2f, 1.0f, 0.2f);  // Green
    } else if (fuel_percent > 0.2) {
        gl_set_color(1.0f, 1.0f, 0.0f);  // Yellow
    } else {
        gl_set_color(1.0f, 0.2f, 0.2f);  // Red
    }
    gl_draw_rect_filled((float)bar_x, (float)bar_y, (float)(bar_width * fuel_percent), (float)bar_height);
    
    // Border
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline((float)bar_x, (float)bar_y, (float)bar_width, (float)bar_height, 1.0f);
    
    // --- MISSILES DISPLAY ---
    // Missile ammo display (bottom left, above Energy)
    if (game->missile_ammo > 0 || game->using_missiles) {
        gl_set_color(1.0f, 0.8f, 0.0f);  // Yellow/orange
        char missile_text[32];
        sprintf(missile_text, "MISSILES: %d", game->missile_ammo);
        
        gl_draw_text_simple(missile_text, 20, height - 110, 14);
        
        // Draw missile bar
        double missile_bar_width = 150;
        double missile_bar_height = 12;
        double missile_bar_x = 20;
        double missile_bar_y = height - 95;
        
        // Background
        gl_set_color(0.2f, 0.2f, 0.2f);
        gl_draw_rect_filled((float)missile_bar_x, (float)missile_bar_y, (float)missile_bar_width, (float)missile_bar_height);
        
        // Missile bar (assume max 100 for display, each pickup adds 20)
        double missile_percent = (game->missile_ammo > 100) ? 1.0 : (game->missile_ammo / 100.0);
        gl_set_color(1.0f, 0.8f, 0.0f);  // Yellow
        gl_draw_rect_filled((float)missile_bar_x, (float)missile_bar_y, (float)(missile_bar_width * missile_percent), (float)missile_bar_height);
        
        // Border
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_rect_outline((float)missile_bar_x, (float)missile_bar_y, (float)missile_bar_width, (float)missile_bar_height, 1.0f);
    }
    
    // --- BOMBS DISPLAY ---
    // Bomb ammo display
    if (game->bomb_ammo > 0 || game->bomb_count > 0) {
        gl_set_color(1.0f, 0.6f, 0.0f);  // Orange for bombs
        char bomb_text[32];
        sprintf(bomb_text, "BOMBS: %d", game->bomb_ammo);
        
        gl_draw_text_simple(bomb_text, 20, height - 65, 14);
        
        // Show active bombs
        if (game->bomb_count > 0) {
            gl_set_color(1.0f, 1.0f, 0.0f);  // Yellow for active
            char active_text[32];
            sprintf(active_text, "Armed: %d", game->bomb_count);
            gl_draw_text_simple(active_text, 20, height - 50, 12);
        }
        
        // Draw bomb bar
        double bomb_bar_width = 150;
        double bomb_bar_height = 12;
        double bomb_bar_x = 20;
        double bomb_bar_y = game->bomb_count > 0 ? (height - 35) : (height - 50);
        
        // Background
        gl_set_color(0.2f, 0.2f, 0.2f);
        gl_draw_rect_filled((float)bomb_bar_x, (float)bomb_bar_y, (float)bomb_bar_width, (float)bomb_bar_height);
        
        // Bomb bar
        double bomb_percent = (game->bomb_ammo > 10) ? 1.0 : (game->bomb_ammo / 10.0);
        gl_set_color(1.0f, 0.6f, 0.0f);  // Orange
        gl_draw_rect_filled((float)bomb_bar_x, (float)bomb_bar_y, (float)(bomb_bar_width * bomb_percent), (float)bomb_bar_height);
        
        // Border
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_rect_outline((float)bomb_bar_x, (float)bomb_bar_y, (float)bomb_bar_width, (float)bomb_bar_height, 1.0f);
    }
}

void draw_comet_buster_game_over_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || !game->game_over) return;
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
    gl_draw_rect_filled(0.0f, 0.0f, width, height);
    gl_set_color(1.0f, 0.0f, 0.0f);
    gl_draw_rect_outline(width / 2.0f - 50.0f, height / 2.0f - 30.0f, 100.0f, 60.0f, 2.0f);
}

void boss_explosion_draw_gl(BossExplosion *explosion, void *cr) {
    if (!explosion) return;
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active) continue;
        gl_set_color_alpha(p->color[0], p->color[1], p->color[2], p->glow_intensity);
        gl_draw_circle(p->x, p->y, 3.0f, 12);
    }
}

void comet_buster_draw_splash_screen_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || !game->splash_screen_active) return;
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.9f);
    gl_draw_rect_filled(0.0f, 0.0f, width, height);
}

void comet_buster_draw_victory_scroll_gl(CometBusterGame *game, void *cr, int width, int height) {}
void comet_buster_draw_finale_splash_gl(CometBusterGame *game, void *cr, int width, int height) {}
void comet_buster_update_victory_scroll_gl(CometBusterGame *game, double dt) {}
void comet_buster_update_finale_splash_gl(CometBusterGame *game, double dt) {}


