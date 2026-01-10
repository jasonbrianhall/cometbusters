#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"

#ifdef ExternalSound
#include "audio_wad.h"
#endif

// ============================================================================
// MODERN OPENGL RENDERING ENGINE - Core Profile Compatible
// ============================================================================

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
    gl_state.projection = mat4_ortho(0, width, height, 0, -1, 1);
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
    Vertex *verts = malloc(segments * sizeof(Vertex));
    
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        float x = cx + radius * cosf(angle);
        float y = cy + radius * sinf(angle);
        verts[i] = (Vertex){x, y, gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    
    draw_vertices(verts, segments, GL_LINE_LOOP);
    free(verts);
}

void gl_draw_polygon(float *points, int num_points, int filled) {
    Vertex *verts = malloc(num_points * sizeof(Vertex));
    for (int i = 0; i < num_points; i++) {
        verts[i] = (Vertex){points[i*2], points[i*2+1], gl_state.color[0], gl_state.color[1], gl_state.color[2], gl_state.color[3]};
    }
    draw_vertices(verts, num_points, filled ? GL_TRIANGLE_FAN : GL_LINE_LOOP);
    free(verts);
}

void gl_push_transform(void) {}
void gl_pop_transform(void) {}
void gl_translate(float x, float y) {(void)x;(void)y;}
void gl_rotate(float angle_rad) {(void)angle_rad;}

// ============================================================================
// GAME DRAWING FUNCTIONS
// ============================================================================

void draw_comet_buster_gl(Visualizer *visualizer, void *gl_context) {
    if (!visualizer) return;
    (void)gl_context;
    
    gl_init();
    
    CometBusterGame *game = &visualizer->comet_buster;
    int width = visualizer->width;
    int height = visualizer->height;

#ifdef ExternalSound
    if (game->splash_screen_active) {
        comet_buster_draw_splash_screen_gl(game, NULL, width, height);
        return;
    }
#endif
    
    gl_setup_2d_projection(width, height);
    
    // Background
    gl_set_color(0.04f, 0.06f, 0.15f);
    gl_draw_rect_filled(0, 0, width, height);
    
    // Grid
    gl_set_color(0.1f, 0.15f, 0.35f);
    for (int i = 0; i <= width + 50; i += 50) {
        gl_draw_line(i, 0, i, height, 0.5f);
    }
    for (int i = 0; i <= height; i += 50) {
        gl_draw_line(0, i, width + 50, i, 0.5f);
    }
    
    // Game elements
    draw_comet_buster_comets_gl(game, NULL, width, height);
    draw_comet_buster_bullets_gl(game, NULL, width, height);
    draw_comet_buster_enemy_ships_gl(game, NULL, width, height);
    draw_comet_buster_ufos_gl(game, NULL, width, height);
    
    // Boss
    if (game->boss_active) {
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            draw_spawn_queen_boss_gl(&game->spawn_queen, NULL, width, height);
        } else if (game->boss.active) {
            if (game->current_wave % 30 == 5) {
                draw_comet_buster_boss_gl(&game->boss, NULL, width, height);
            } else if (game->current_wave % 30 == 10) {
                draw_spawn_queen_boss_gl(&game->spawn_queen, NULL, width, height);
            } else if (game->current_wave % 30 == 15) {
                draw_void_nexus_boss_gl(&game->boss, NULL, width, height);
            } else if (game->current_wave % 30 == 20) {
                draw_harbinger_boss_gl(&game->boss, NULL, width, height);
            } else if (game->current_wave % 30 == 25) {
                draw_star_vortex_boss_gl(&game->boss, NULL, width, height);
            } else if (game->current_wave % 30 == 0) {
                draw_singularity_boss_gl(&game->boss, NULL, width, height);
            }
        }
    }
    
    draw_comet_buster_enemy_bullets_gl(game, NULL, width, height);
    draw_comet_buster_canisters_gl(game, NULL, width, height);
    draw_comet_buster_missile_pickups_gl(game, NULL, width, height);
    draw_comet_buster_bomb_pickups_gl(game, NULL, width, height);
    draw_comet_buster_missiles_gl(game, NULL, width, height);
    draw_comet_buster_bombs_gl(game, NULL, width, height);
    draw_comet_buster_particles_gl(game, NULL, width, height);
    draw_comet_buster_ship_gl(game, NULL, width, height);
    
    boss_explosion_draw_gl(&game->boss_explosion_effect, NULL);
    draw_comet_buster_hud_gl(game, NULL, width, height);
    
    if (game->game_over) {
        draw_comet_buster_game_over_gl(game, NULL, width, height);
    }
    
    gl_restore_projection();
    glFlush();
}

void draw_comet_buster_comets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        if (!c->active) continue;
        gl_set_color(c->color[0], c->color[1], c->color[2]);
        gl_draw_circle(c->x, c->y, c->radius, 16);
        gl_set_color(c->color[0] * 1.2f, c->color[1] * 1.2f, c->color[2] * 1.2f);
        gl_draw_circle_outline(c->x, c->y, c->radius, 1.5f, 16);
    }
}

void draw_comet_buster_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->bullet_count; i++) {
        Bullet *b = &game->bullets[i];
        if (!b->active) continue;
        gl_set_color(0.5f, 1.0f, 0.5f);
        gl_draw_circle(b->x, b->y, 2.0f, 8);
    }
}

void draw_comet_buster_enemy_ships_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        if (!ship->active) continue;
        gl_set_color(1.0f, 0.0f, 0.0f);
        float points[] = {0.0f, -15.0f, -12.0f, 12.0f, 12.0f, 12.0f};
        gl_draw_polygon(points, 3, 1);
    }
}

void draw_comet_buster_ufos_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *ufo = &game->ufos[i];
        if (!ufo->active) continue;
        gl_set_color(1.0f, 1.0f, 0.0f);
        gl_draw_circle(ufo->x, ufo->y, 20.0f, 16);
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_circle_outline(ufo->x, ufo->y, 20.0f, 1.5f, 16);
    }
}

void draw_comet_buster_enemy_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        Bullet *b = &game->enemy_bullets[i];
        if (!b->active) continue;
        gl_set_color(1.0f, 0.5f, 0.5f);
        gl_draw_circle(b->x, b->y, 2.0f, 8);
    }
}

void draw_comet_buster_missiles_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->missile_count; i++) {
        Missile *m = &game->missiles[i];
        if (!m->active) continue;
        gl_set_color(0.0f, 1.0f, 1.0f);
        float points[] = {0.0f, -5.0f, -3.0f, 5.0f, 3.0f, 5.0f};
        gl_draw_polygon(points, 3, 1);
    }
}

void draw_comet_buster_canisters_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->canister_count; i++) {
        Canister *can = &game->canisters[i];
        if (!can->active) continue;
        gl_set_color(1.0f, 1.0f, 0.0f);
        gl_draw_rect_filled(can->x - 5, can->y - 5, 10.0f, 10.0f);
    }
}

void draw_comet_buster_missile_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->missile_pickup_count; i++) {
        MissilePickup *pickup = &game->missile_pickups[i];
        if (!pickup->active) continue;
        gl_set_color(0.5f, 0.5f, 1.0f);
        gl_draw_circle(pickup->x, pickup->y, 8.0f, 12);
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_circle_outline(pickup->x, pickup->y, 8.0f, 1.0f, 12);
    }
}

void draw_comet_buster_bomb_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        BombPickup *pickup = &game->bomb_pickups[i];
        if (!pickup->active) continue;
        gl_set_color(1.0f, 0.0f, 1.0f);
        gl_draw_rect_filled(pickup->x - 8, pickup->y - 8, 16.0f, 16.0f);
        gl_set_color(1.0f, 1.0f, 1.0f);
        gl_draw_rect_outline(pickup->x - 8, pickup->y - 8, 16.0f, 16.0f, 1.0f);
    }
}

void draw_comet_buster_bombs_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *bomb = &game->bombs[i];
        if (!bomb->active) continue;
        if (!bomb->detonated) {
            gl_set_color(1.0f, 0.5f, 0.0f);
            gl_draw_circle(bomb->x, bomb->y, 10.0f, 12);
            gl_set_color(1.0f, 1.0f, 1.0f);
            gl_draw_circle_outline(bomb->x, bomb->y, 10.0f, 1.5f, 12);
        } else {
            gl_set_color_alpha(1.0f, 0.5f, 0.0f, 0.5f);
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
    gl_set_color(0.0f, 1.0f, 0.0f);
    float points[] = {0.0f, -15.0f, -10.0f, 15.0f, 10.0f, 15.0f};
    gl_draw_polygon(points, 3, 1);
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_polygon(points, 3, 0);
    
    if (game->burner_intensity > 0) {
        gl_set_color(1.0f, 0.5f, 0.0f);
        float thrust_points[] = {-5.0f, 15.0f, 5.0f, 15.0f, 0.0f, 15.0f + 20.0f * game->burner_intensity};
        gl_draw_polygon(thrust_points, 3, 1);
    }
    
    if (game->shield_health > 0) {
        double shield_alpha = (double)game->shield_health / game->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_alpha * 0.5f);
        gl_draw_circle_outline(game->ship_x, game->ship_y, 25.0f, 2.0f, 16);
    }
}

void draw_comet_buster_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    gl_set_color(1.0f, 0.0f, 0.0f);
    gl_draw_circle(boss->x, boss->y, 50.0f, 20);
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_circle_outline(boss->x, boss->y, 50.0f, 2.0f, 20);
}

void draw_spawn_queen_boss_gl(SpawnQueenBoss *queen, void *cr, int width, int height) {
    if (!queen || !queen->active) return;
    gl_set_color(0.5f, 0.0f, 1.0f);
    gl_draw_circle(queen->x, queen->y, 60.0f, 20);
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_circle_outline(queen->x, queen->y, 60.0f, 2.0f, 20);
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
    gl_set_color(0.2f, 0.8f, 1.0f);
    gl_draw_rect_outline(10.0f, 10.0f, width - 20.0f, 30.0f, 1.0f);
    for (int i = 0; i < game->ship_lives; i++) {
        gl_set_color(0.0f, 1.0f, 0.5f);
        gl_draw_circle(20.0f + i * 20.0f, height - 30.0f, 5.0f, 12);
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

// ============================================================================
// OPENGL CONTEXT CALLBACKS
// ============================================================================

gboolean on_realize(GtkGLArea *area, gpointer data) {
    (void)data;
    gtk_gl_area_make_current(area);
    
    static gboolean glew_initialized = FALSE;
    if (!glew_initialized) {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        fprintf(stderr, "[GL] glewInit: %s\n", glewGetErrorString(err));
        glew_initialized = TRUE;
    }
    
    glClearColor(0.04f, 0.06f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    
    fprintf(stderr, "[GL] GL Context initialized\n");
    return TRUE;
}

gboolean on_render(GtkGLArea *area, GdkGLContext *context, gpointer data) {
    (void)context;
    Visualizer *vis = (Visualizer *)data;
    if (!vis) return FALSE;
    
    gtk_gl_area_make_current(area);
    
    int window_width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int window_height = gtk_widget_get_allocated_height(GTK_WIDGET(area));
    
    if (window_width < 10 || window_height < 10) {
        gtk_widget_queue_draw(GTK_WIDGET(area));
        return TRUE;
    }
    
    vis->width = 1920;
    vis->height = 1080;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, window_width, window_height);
    
    draw_comet_buster_gl(vis, NULL);
    
    // Update game logic
    double current_time = g_get_monotonic_time() / 1000000.0;
    static double last_time = 0.0;
    double dt = (last_time > 0) ? (current_time - last_time) : 0.016;
    last_time = current_time;
    if (dt > 0.1) dt = 0.016;
    
    update_comet_buster(vis, dt);
    
    glFlush();
    gtk_widget_queue_draw(GTK_WIDGET(area));
    
    return TRUE;
}
