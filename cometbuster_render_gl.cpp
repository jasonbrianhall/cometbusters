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
// OPENGL UTILITIES
// ============================================================================

// Simple 2D orthographic projection matrix setup
static void gl_setup_2d_projection(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);  // Y-axis flipped to match screen coords
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

static void gl_restore_projection(void) {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

// Set color using RGB values (0.0 to 1.0)
static void gl_set_color(float r, float g, float b) {
    glColor3f(r, g, b);
}

// Set color using RGBA values
static void gl_set_color_alpha(float r, float g, float b, float a) {
    glColor4f(r, g, b, a);
}

// Draw a line from (x1, y1) to (x2, y2)
static void gl_draw_line(float x1, float y1, float x2, float y2, float width) {
    glLineWidth(width);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

// Draw a filled rectangle
static void gl_draw_rect_filled(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Draw a rectangle outline
static void gl_draw_rect_outline(float x, float y, float width, float height, float line_width) {
    glLineWidth(line_width);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();
}

// Draw a filled circle/arc
static void gl_draw_circle(float cx, float cy, float radius, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(cx, cy);
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(cx + radius * cosf(angle), cy + radius * sinf(angle));
    }
    glEnd();
}

// Draw a circle outline
static void gl_draw_circle_outline(float cx, float cy, float radius, float line_width, int segments) {
    glLineWidth(line_width);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segments; i++) {
        float angle = 2.0f * M_PI * i / segments;
        glVertex2f(cx + radius * cosf(angle), cy + radius * sinf(angle));
    }
    glEnd();
}

// Draw a polygon (connected points)
static void gl_draw_polygon(float *points, int num_points, int filled) {
    if (filled) {
        glBegin(GL_TRIANGLE_FAN);
    } else {
        glBegin(GL_LINE_LOOP);
    }
    for (int i = 0; i < num_points; i++) {
        glVertex2f(points[i * 2], points[i * 2 + 1]);
    }
    glEnd();
}

// Push matrix for transformations
static void gl_push_transform(void) {
    glPushMatrix();
}

// Pop matrix
static void gl_pop_transform(void) {
    glPopMatrix();
}

// Translate
static void gl_translate(float x, float y) {
    glTranslatef(x, y, 0.0f);
}

// Rotate (angle in radians)
static void gl_rotate(float angle_rad) {
    glRotatef(angle_rad * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
}

// ============================================================================
// RENDERING - VECTOR-BASED ASTEROIDS (OpenGL Version)
// ============================================================================

void draw_comet_buster_gl(Visualizer *visualizer, void *gl_context) {
    if (!visualizer) return;
    
    CometBusterGame *game = &visualizer->comet_buster;
    int width = visualizer->width;
    int height = visualizer->height;
    
#ifdef ExternalSound
    // Draw splash screen if active
    if (game->splash_screen_active) {
        comet_buster_draw_splash_screen_gl(game, NULL, width, height);
        return;
    }
#endif
    
    gl_setup_2d_projection(width, height);
    
    // Background
    gl_set_color(0.04f, 0.06f, 0.15f);
    gl_draw_rect_filled(0, 0, width, height);
    
    // Grid (extended 50 pixels to the right)
    gl_set_color(0.1f, 0.15f, 0.35f);
    glLineWidth(0.5f);
    
    for (int i = 0; i <= width + 50; i += 50) {
        gl_draw_line(i, 0, i, height, 0.5f);
    }
    for (int i = 0; i <= height; i += 50) {
        gl_draw_line(0, i, width + 50, i, 0.5f);
    }
    
    // Draw game elements
    draw_comet_buster_comets_gl(game, NULL, width, height);
    draw_comet_buster_bullets_gl(game, NULL, width, height);
    draw_comet_buster_enemy_ships_gl(game, NULL, width, height);
    draw_comet_buster_ufos_gl(game, NULL, width, height);
    
    // Draw boss (either Spawn Queen or regular Death Star)
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
    
    // Draw boss explosion effect
    boss_explosion_draw_gl(&game->boss_explosion_effect, (void*)NULL);
    
    // Draw HUD
    draw_comet_buster_hud_gl(game, NULL, width, height);
    
    // Draw game over
    if (game->game_over) {
        draw_comet_buster_game_over_gl(game, NULL, width, height);
    }
    
    gl_restore_projection();
    glFlush();
}

// ✓ VECTOR-BASED ASTEROIDS (like original Asteroids arcade game)
void draw_comet_buster_comets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        
        if (!c->active) continue;
        
        gl_push_transform();
        gl_translate(c->x, c->y);
        gl_rotate(c->base_angle + c->rotation * M_PI / 180.0f);
        
        // Use comet's color if available, otherwise default
        double r = c->color[0];
        double g = c->color[1];
        double b = c->color[2];
        gl_set_color(r, g, b);
        
        // Draw comet as circle with radius
        gl_draw_circle(0.0f, 0.0f, c->radius, 16);
        
        // Outline
        gl_set_color(r * 1.2f, g * 1.2f, b * 1.2f);
        gl_draw_circle_outline(0.0f, 0.0f, c->radius, 1.5f, 16);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->bullet_count; i++) {
        Bullet *b = &game->bullets[i];
        
        if (!b->active) continue;
        
        gl_push_transform();
        gl_translate(b->x, b->y);
        gl_rotate(b->angle);
        
        gl_set_color(0.2f, 1.0f, 0.2f);
        
        float points[] = {
            5.0f, 0.0f,
            -5.0f, -2.0f,
            -5.0f, 2.0f
        };
        
        gl_draw_polygon(points, 3, 1);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_enemy_ships_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *e = &game->enemy_ships[i];
        
        if (!e->active) continue;
        
        gl_push_transform();
        gl_translate(e->x, e->y);
        gl_rotate(e->angle);
        
        // Choose color based on ship type
        float r = 1.0f, g = 0.0f, b = 0.0f;
        switch (e->ship_type) {
            case 0:  // Blue patrol
                r = 0.0f; g = 0.5f; b = 1.0f;
                break;
            case 1:  // Red aggressive
                r = 1.0f; g = 0.0f; b = 0.0f;
                break;
            case 2:  // Green hunter
                r = 0.0f; g = 1.0f; b = 0.0f;
                break;
            case 3:  // Purple sentinel
                r = 1.0f; g = 0.0f; b = 1.0f;
                break;
            case 4:  // Brown coat elite
                r = 0.6f; g = 0.3f; b = 0.0f;
                break;
            case 5:  // Juggernaut
                r = 1.0f; g = 0.8f; b = 0.0f;
                break;
            default:
                r = 0.5f; g = 0.5f; b = 0.5f;
        }
        
        gl_set_color(r, g, b);
        
        float points[] = {
            12.0f, 0.0f,
            -8.0f, -8.0f,
            -5.0f, 0.0f,
            -8.0f, 8.0f
        };
        
        gl_draw_polygon(points, 4, 1);
        
        // Outline
        gl_set_color(r * 0.7f, g * 0.7f, b * 0.7f);
        gl_draw_polygon(points, 4, 0);
        glLineWidth(1.0f);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_ufos_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *u = &game->ufos[i];
        
        if (!u->active) continue;
        
        gl_push_transform();
        gl_translate(u->x, u->y);
        
        // UFO body - flying saucer shape
        gl_set_color(1.0f, 1.0f, 0.0f);
        gl_draw_circle(0.0f, 0.0f, 15.0f, 16);
        
        // Outline
        gl_set_color(1.0f, 0.8f, 0.0f);
        gl_draw_circle_outline(0.0f, 0.0f, 15.0f, 1.5f, 16);
        
        // Dome on top
        gl_set_color_alpha(0.5f, 0.5f, 1.0f, 0.6f);
        gl_draw_circle(0.0f, -7.0f, 6.0f, 12);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_enemy_bullets_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        Bullet *b = &game->enemy_bullets[i];
        
        if (!b->active) continue;
        
        gl_push_transform();
        gl_translate(b->x, b->y);
        
        gl_set_color(1.0f, 0.5f, 0.0f);
        gl_draw_circle(0.0f, 0.0f, 2.0f, 8);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_missiles_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->missile_count; i++) {
        Missile *m = &game->missiles[i];
        
        if (!m->active) continue;
        
        gl_push_transform();
        gl_translate(m->x, m->y);
        gl_rotate(atan2f(m->vy, m->vx));
        
        // Missile body
        gl_set_color(1.0f, 1.0f, 0.5f);
        float points[] = {
            8.0f, 0.0f,
            -6.0f, -3.0f,
            -6.0f, 3.0f
        };
        gl_draw_polygon(points, 3, 1);
        
        // Flame trail
        gl_set_color_alpha(1.0f, 0.5f, 0.0f, 0.6f);
        float flame[] = {
            -6.0f, 0.0f,
            -10.0f, -2.0f,
            -10.0f, 2.0f
        };
        gl_draw_polygon(flame, 3, 1);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_bombs_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *b = &game->bombs[i];
        
        if (!b->active) continue;
        
        gl_push_transform();
        gl_translate(b->x, b->y);
        gl_rotate(b->rotation * M_PI / 180.0f);
        
        // Bomb body - sphere
        gl_set_color(0.2f, 0.2f, 0.2f);
        gl_draw_circle(0.0f, 0.0f, 8.0f, 16);
        
        // Glow
        float glow_intensity = 0.5f + 0.5f * sinf(b->lifetime);
        gl_set_color_alpha(1.0f, 0.5f, 0.0f, glow_intensity * 0.5f);
        gl_draw_circle(0.0f, 0.0f, 11.0f, 16);
        
        // Outline
        gl_set_color(1.0f, 0.5f, 0.0f);
        gl_draw_circle_outline(0.0f, 0.0f, 8.0f, 1.0f, 16);
        
        // Explosion wave visualization
        if (b->detonated && b->wave_radius > 0.0f) {
            gl_set_color_alpha(1.0f, 0.2f, 0.0f, 0.3f);
            gl_draw_circle_outline(0.0f, 0.0f, b->wave_radius, 2.0f, 32);
        }
        
        gl_pop_transform();
    }
}

void draw_comet_buster_canisters_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->canister_count; i++) {
        Canister *c = &game->canisters[i];
        
        if (!c->active) continue;
        
        gl_push_transform();
        gl_translate(c->x, c->y);
        gl_rotate(c->rotation * M_PI / 180.0f);
        
        gl_set_color(1.0f, 0.5f, 0.0f);
        gl_draw_rect_filled(-5.0f, -10.0f, 10.0f, 20.0f);
        
        gl_set_color(1.0f, 0.8f, 0.0f);
        gl_draw_rect_outline(-5.0f, -10.0f, 10.0f, 20.0f, 1.0f);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_missile_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->missile_pickup_count; i++) {
        MissilePickup *p = &game->missile_pickups[i];
        
        if (!p->active) continue;
        
        gl_push_transform();
        gl_translate(p->x, p->y);
        gl_rotate(p->rotation * M_PI / 180.0f);
        
        gl_set_color(0.0f, 1.0f, 1.0f);
        gl_draw_circle(0.0f, 0.0f, 8.0f, 16);
        
        gl_set_color(0.5f, 1.0f, 1.0f);
        gl_draw_circle_outline(0.0f, 0.0f, 8.0f, 1.5f, 16);
        
        // Inner glow
        gl_set_color_alpha(0.0f, 1.0f, 1.0f, 0.3f);
        gl_draw_circle(0.0f, 0.0f, 5.0f, 12);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_bomb_pickups_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        BombPickup *p = &game->bomb_pickups[i];
        
        if (!p->active) continue;
        
        gl_push_transform();
        gl_translate(p->x, p->y);
        gl_rotate(p->rotation * M_PI / 180.0f);
        
        gl_set_color(1.0f, 0.2f, 0.2f);
        gl_draw_circle(0.0f, 0.0f, 8.0f, 16);
        
        gl_set_color(1.0f, 0.5f, 0.5f);
        gl_draw_circle_outline(0.0f, 0.0f, 8.0f, 1.5f, 16);
        
        // Inner glow
        gl_set_color_alpha(1.0f, 0.2f, 0.2f, 0.3f);
        gl_draw_circle(0.0f, 0.0f, 5.0f, 12);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_particles_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    (void)cr;
    
    for (int i = 0; i < game->particle_count; i++) {
        Particle *p = &game->particles[i];
        
        if (!p->active) continue;
        
        gl_push_transform();
        gl_translate(p->x, p->y);
        
        float alpha = p->lifetime / p->max_lifetime;
        gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha * 0.8f);
        gl_draw_circle(0.0f, 0.0f, p->size, 6);
        
        gl_pop_transform();
    }
}

void draw_comet_buster_ship_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || game->ship_lives <= 0) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(game->ship_x, game->ship_y);
    gl_rotate(game->ship_angle);
    
    // Ship body (triangle)
    gl_set_color(0.0f, 1.0f, 0.5f);
    float points[] = {
        12.0f, 0.0f,
        -10.0f, -8.0f,
        -5.0f, 0.0f,
        -10.0f, 8.0f
    };
    gl_draw_polygon(points, 4, 1);
    
    // Outline
    gl_set_color(0.5f, 1.0f, 0.8f);
    gl_draw_polygon(points, 4, 0);
    glLineWidth(1.5f);
    
    // Engine flame (when moving)
    if (game->ship_speed > 0.1f) {
        gl_set_color_alpha(1.0f, 0.5f, 0.0f, 0.7f * (game->ship_speed / 5.0f));
        float flame[] = {
            -5.0f, -3.0f,
            -5.0f, 3.0f,
            -10.0f - game->ship_speed * 2.0f, 0.0f
        };
        gl_draw_polygon(flame, 3, 1);
    }
    
    // Shield visualization
    if (game->shield_health > 0) {
        float shield_alpha = 0.3f * (game->shield_health / (float)game->max_shield_health);
        gl_set_color_alpha(0.0f, 0.5f, 1.0f, shield_alpha);
        gl_draw_circle_outline(0.0f, 0.0f, 25.0f, 2.0f, 24);
    }
    
    gl_pop_transform();
}

void draw_comet_buster_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(boss->x, boss->y);
    gl_rotate(boss->angle * M_PI / 180.0f);
    
    // Death Star - sphere with lines
    gl_set_color(0.3f, 0.3f, 0.3f);
    gl_draw_circle(0.0f, 0.0f, 40.0f, 24);
    
    // Outline
    gl_set_color(0.6f, 0.6f, 0.6f);
    gl_draw_circle_outline(0.0f, 0.0f, 40.0f, 2.0f, 24);
    
    // Grid pattern
    gl_set_color(0.5f, 0.5f, 0.5f);
    glLineWidth(1.0f);
    for (int i = -2; i <= 2; i++) {
        gl_draw_line(-35.0f, i * 15.0f, 35.0f, i * 15.0f, 1.0f);
        gl_draw_line(i * 15.0f, -35.0f, i * 15.0f, 35.0f, 1.0f);
    }
    
    // Damage flash
    if (boss->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, boss->damage_flash_timer);
        gl_draw_circle(0.0f, 0.0f, 45.0f, 24);
    }
    
    // Shield visualization
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, 50.0f, 3.0f, 24);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_spawn_queen_boss_gl(SpawnQueenBoss *queen, void *cr, int width, int height) {
    if (!queen || !queen->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(queen->x, queen->y);
    gl_rotate(queen->rotation * M_PI / 180.0f);
    
    // Spawn Queen - organic shape
    gl_set_color(0.5f, 0.1f, 0.5f);
    gl_draw_circle(0.0f, 0.0f, 35.0f, 20);
    
    // Tentacles (4 appendages)
    gl_set_color(0.6f, 0.2f, 0.6f);
    for (int i = 0; i < 4; i++) {
        float angle = (2.0f * M_PI * i) / 4.0f;
        float tx = 35.0f * cosf(angle);
        float ty = 35.0f * sinf(angle);
        gl_draw_line(0.0f, 0.0f, tx, ty, 2.0f);
        gl_draw_circle(tx, ty, 8.0f, 12);
    }
    
    // Outline
    gl_set_color(0.8f, 0.4f, 0.8f);
    gl_draw_circle_outline(0.0f, 0.0f, 35.0f, 2.0f, 20);
    
    // Damage flash
    if (queen->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, queen->damage_flash_timer);
        gl_draw_circle(0.0f, 0.0f, 40.0f, 20);
    }
    
    // Shield
    if (queen->shield_health > 0) {
        double shield_ratio = (double)queen->shield_health / queen->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, 45.0f, 3.0f, 20);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = queen->x - bar_width / 2.0;
    double bar_y = queen->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)queen->health / queen->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_void_nexus_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(boss->x, boss->y);
    gl_rotate(boss->angle * M_PI / 180.0f);
    
    // Void Nexus - dark swirling vortex
    gl_set_color(0.2f, 0.0f, 0.3f);
    gl_draw_circle(0.0f, 0.0f, 38.0f, 24);
    
    // Spiral effect with multiple rings
    for (int ring = 1; ring <= 3; ring++) {
        float radius = 15.0f * ring;
        gl_set_color(0.1f * ring, 0.0f, 0.2f * ring);
        gl_draw_circle_outline(0.0f, 0.0f, radius, 1.0f, 12);
    }
    
    // Outline
    gl_set_color(0.4f, 0.0f, 0.6f);
    gl_draw_circle_outline(0.0f, 0.0f, 38.0f, 2.0f, 24);
    
    // Damage flash
    if (boss->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, boss->damage_flash_timer);
        gl_draw_circle(0.0f, 0.0f, 43.0f, 24);
    }
    
    // Shield
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, 48.0f, 3.0f, 24);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_harbinger_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(boss->x, boss->y);
    gl_rotate(boss->angle * M_PI / 180.0f);
    
    // Harbinger - angular, menacing shape
    gl_set_color(0.5f, 0.0f, 0.0f);
    
    float points[] = {
        40.0f, 0.0f,
        -30.0f, -30.0f,
        -20.0f, -10.0f,
        -20.0f, 10.0f,
        -30.0f, 30.0f
    };
    gl_draw_polygon(points, 5, 1);
    
    // Outline
    gl_set_color(0.8f, 0.2f, 0.2f);
    gl_draw_polygon(points, 5, 0);
    glLineWidth(2.0f);
    
    // Damage flash
    if (boss->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, boss->damage_flash_timer);
        for (int i = 0; i < 5; i++) {
            int next = (i + 1) % 5;
            gl_draw_line(points[i * 2], points[i * 2 + 1], 
                        points[next * 2], points[next * 2 + 1], 2.0f);
        }
    }
    
    // Shield
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, 50.0f, 3.0f, 24);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_star_vortex_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(boss->x, boss->y);
    gl_rotate(boss->rotation * M_PI / 180.0f);
    
    // Star Vortex - 6-pointed star
    int num_points = 6;
    double outer_radius = 50.0;
    double inner_radius = 25.0;
    
    // Star color (shifts based on phase)
    double r = 1.0, g = 0.6, b = 0.0;
    if (boss->phase == 1) {
        r = 1.0; g = 0.3; b = 0.3;
    } else if (boss->phase == 2) {
        r = 1.0; g = 1.0; b = 0.0;
    }
    
    // Draw star points
    float points[24];
    int point_count = 0;
    for (int i = 0; i < num_points * 2; i++) {
        double angle = (i * M_PI / num_points);
        double radius = (i % 2 == 0) ? outer_radius : inner_radius;
        points[point_count++] = radius * cosf(angle);
        points[point_count++] = radius * sinf(angle);
    }
    
    gl_set_color(r, g, b);
    gl_draw_polygon(points, num_points * 2, 1);
    
    // Outline
    gl_set_color(r * 0.5f, g * 0.5f, b * 0.5f);
    gl_draw_polygon(points, num_points * 2, 0);
    glLineWidth(2.0f);
    
    // Damage flash
    if (boss->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, boss->damage_flash_timer);
        gl_draw_circle(0.0f, 0.0f, outer_radius * 1.2f, 16);
    }
    
    // Shield
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, outer_radius + 10.0f, 3.0f, 16);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_singularity_boss_gl(BossShip *boss, void *cr, int width, int height) {
    if (!boss || !boss->active) return;
    (void)width;
    (void)height;
    (void)cr;
    
    gl_push_transform();
    gl_translate(boss->x, boss->y);
    gl_rotate(boss->rotation * M_PI / 180.0f);
    
    // Singularity - concentric rings getting darker toward center
    for (int ring = 5; ring >= 1; ring--) {
        float radius = 45.0f - (ring - 1) * 8.0f;
        float intensity = (float)ring / 5.0f;
        gl_set_color(intensity * 0.3f, 0.0f, intensity * 0.5f);
        gl_draw_circle(0.0f, 0.0f, radius, 20);
    }
    
    // Outer outline
    gl_set_color(0.6f, 0.0f, 1.0f);
    gl_draw_circle_outline(0.0f, 0.0f, 45.0f, 2.0f, 20);
    
    // Damage flash
    if (boss->damage_flash_timer > 0.0f) {
        gl_set_color_alpha(1.0f, 1.0f, 1.0f, boss->damage_flash_timer);
        gl_draw_circle(0.0f, 0.0f, 50.0f, 20);
    }
    
    // Shield
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        gl_set_color_alpha(0.2f, 0.8f, 1.0f, shield_ratio * 0.6f);
        gl_draw_circle_outline(0.0f, 0.0f, 55.0f, 3.0f, 20);
    }
    
    gl_pop_transform();
    
    // Health bar
    double bar_width = 80.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 70.0;
    
    gl_set_color(0.2f, 0.2f, 0.2f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width, bar_height);
    
    double health_ratio = (double)boss->health / boss->max_health;
    gl_set_color(0.0f, 1.0f, 0.0f);
    gl_draw_rect_filled(bar_x, bar_y, bar_width * health_ratio, bar_height);
    
    gl_set_color(1.0f, 1.0f, 1.0f);
    gl_draw_rect_outline(bar_x, bar_y, bar_width, bar_height, 1.0f);
}

void draw_comet_buster_hud_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game) return;
    (void)cr;
    
    gl_set_color(0.2f, 0.8f, 1.0f);
    
    // Score bar at top
    gl_draw_rect_outline(10.0f, 10.0f, width - 20.0f, 30.0f, 1.0f);
    
    // Lives indicator (simple circles)
    for (int i = 0; i < game->ship_lives; i++) {
        gl_set_color(0.0f, 1.0f, 0.5f);
        gl_draw_circle(20.0f + i * 20.0f, height - 30.0f, 5.0f, 12);
    }
}

void draw_comet_buster_game_over_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || !game->game_over) return;
    (void)cr;
    
    // Semi-transparent overlay
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.7f);
    gl_draw_rect_filled(0.0f, 0.0f, width, height);
    
    // Game Over text would go here (requires text rendering)
    gl_set_color(1.0f, 0.0f, 0.0f);
    gl_draw_rect_outline(width / 2.0f - 50.0f, height / 2.0f - 30.0f, 100.0f, 60.0f, 2.0f);
}

void boss_explosion_draw_gl(BossExplosion *explosion, void *cr) {
    if (!explosion) return;
    (void)cr;
    
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active) continue;
        
        float alpha = p->glow_intensity;
        
        if (p->is_radial_line) {
            // Draw radial line with glow effect
            // Calculate end point of line
            double end_x = p->x + cos(p->angle) * p->length;
            double end_y = p->y + sin(p->angle) * p->length;
            
            // Draw thick glow line (outer glow)
            gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha * 0.3f);
            gl_draw_line(p->x, p->y, end_x, end_y, p->width * 4.0f);
            
            // Draw bright inner line
            gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha);
            gl_draw_line(p->x, p->y, end_x, end_y, p->width);
            
        } else {
            // Draw glow particle as circle with halo
            
            // Outer glow (larger, more transparent)
            gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha * 0.2f);
            gl_draw_circle(p->x, p->y, 8.0f, 16);
            
            // Inner bright core
            gl_set_color_alpha(p->color[0], p->color[1], p->color[2], alpha * 0.8f);
            gl_draw_circle(p->x, p->y, 3.0f, 12);
        }
    }
}

// Placeholder for splash screen in OpenGL
void comet_buster_draw_splash_screen_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || !game->splash_screen_active) return;
    (void)cr;
    
    gl_push_transform();
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.9f);
    gl_draw_rect_filled(0.0f, 0.0f, width, height);
    
    // Title text would go here (requires text rendering)
    gl_set_color(1.0f, 1.0f, 0.0f);
    gl_draw_rect_outline(width / 2.0f - 100.0f, height / 2.0f - 50.0f, 200.0f, 100.0f, 2.0f);
    
    gl_pop_transform();
}

// Placeholder functions that are called but only have OpenGL stubs
void comet_buster_draw_victory_scroll_gl(CometBusterGame *game, void *cr, int width, int height) {
    (void)game;
    (void)cr;
    (void)width;
    (void)height;
    // Text rendering would go here
}

void comet_buster_draw_finale_splash_gl(CometBusterGame *game, void *cr, int width, int height) {
    if (!game || !game->finale_splash_active) return;
    (void)cr;
    
    gl_set_color_alpha(0.0f, 0.0f, 0.0f, 0.75f);
    gl_draw_rect_filled(0.0f, 0.0f, width, height);
    
    gl_set_color(1.0f, 1.0f, 0.0f);
    gl_draw_rect_outline(width / 2.0f - 100.0f, height / 2.0f - 50.0f, 200.0f, 100.0f, 2.0f);
}

void comet_buster_update_victory_scroll_gl(CometBusterGame *game, double dt) {
    (void)game;
    (void)dt;
}

void comet_buster_update_finale_splash_gl(CometBusterGame *game, double dt) {
    if (!game || !game->finale_splash_active) return;
    
    game->finale_splash_timer += dt;
    game->finale_scroll_timer += dt;
    
    if (game->finale_scroll_timer >= 0.6) {
        game->finale_scroll_line_index++;
        game->finale_scroll_timer = 0.0;
        
        if (game->finale_scroll_line_index >= 100) {  // Placeholder count
            game->finale_waiting_for_input = true;
            game->finale_scroll_line_index = 99;
        }
    }
}

// OpenGL initialization - called when GL context is created
static gboolean on_realize(GtkGLArea *area, gpointer data) {
    gtk_gl_area_make_current(area);
    
    // Initialize GLEW (only once)
    static gboolean glew_initialized = FALSE;
    if (!glew_initialized) {
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            g_warning("GLEW initialization failed: %s", glewGetErrorString(err));
            return FALSE;
        }
        glew_initialized = TRUE;
    }
    
    // Set OpenGL state
    glClearColor(0.04f, 0.06f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    
    return TRUE;
}

// OpenGL render callback - called every frame
static gboolean on_render(GtkGLArea *area, GdkGLContext *context, gpointer data) {
    Visualizer *vis = (Visualizer *)data;
    if (!vis) return FALSE;
    
    // Get window dimensions
    int width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int height = gtk_widget_get_allocated_height(GTK_WIDGET(area));
    
    // Don't render if window is too small
    if (width <= 1 || height <= 1) {
        gtk_widget_queue_draw(GTK_WIDGET(area));
        return TRUE;
    }
    
    // Clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, width, height);
    
    // Set up 2D projection for debugging
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // DEBUG: Draw a simple colored rectangle to verify OpenGL works
    glColor3f(1.0f, 0.5f, 0.0f);  // Orange
    glBegin(GL_QUADS);
    glVertex2f(50, 50);
    glVertex2f(200, 50);
    glVertex2f(200, 200);
    glVertex2f(50, 200);
    glEnd();
    
    // Restore matrices
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    // Fixed game resolution
    int game_width = 1920;
    int game_height = 1080;
    vis->width = game_width;
    vis->height = game_height;
    
    // Draw the game using OpenGL
    draw_comet_buster_gl(vis, NULL);
    
    // Update game logic
    double current_time = g_get_monotonic_time() / 1000000.0;
    static double last_time = 0.0;
    double dt = (last_time > 0) ? (current_time - last_time) : 0.016;
    last_time = current_time;
    
    // Cap dt to prevent large jumps
    if (dt > 0.1) dt = 0.016;
    
    // Update game state
    update_comet_buster(vis, dt);
    
    // Request continuous redraw
    gtk_widget_queue_draw(GTK_WIDGET(area));
    
    return TRUE;
}
