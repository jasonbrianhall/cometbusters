#include "cometbuster_bossexplosion.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef M_PI
#define M_PI 3.1415926535
#endif

// Helper: Create a radial burst line particle
static void create_radial_line(BossExplosion *explosion, double cx, double cy, 
                               double angle, double length, double speed,
                               double r, double g, double b) {
    if (explosion->particle_count >= 256) return;
    
    BossExplosionParticle *p = &explosion->particles[explosion->particle_count];
    explosion->particle_count++;
    
    p->x = cx;
    p->y = cy;
    p->angle = angle;
    p->lifetime = 0.6;      // Fast decay - 0.6 seconds
    p->max_lifetime = 0.6;
    p->length = 0;          // Start at zero, grows outward
    p->max_length = length; // Final length
    p->width = 3.0;         // Line width
    p->color[0] = r;
    p->color[1] = g;
    p->color[2] = b;
    p->glow_intensity = 1.0;
    p->active = true;
    p->is_radial_line = true;
    
    // Velocity for the particle system (slight spread)
    p->vx = cos(angle) * speed;
    p->vy = sin(angle) * speed;
}

// Helper: Create a glow particle (trail/secondary effect)
static void create_glow_particle(BossExplosion *explosion, double x, double y,
                                 double vx, double vy, double lifetime,
                                 double r, double g, double b) {
    if (explosion->particle_count >= 256) return;
    
    BossExplosionParticle *p = &explosion->particles[explosion->particle_count];
    explosion->particle_count++;
    
    p->x = x;
    p->y = y;
    p->vx = vx;
    p->vy = vy;
    p->lifetime = lifetime;
    p->max_lifetime = lifetime;
    p->length = 0;
    p->max_length = 0;
    p->width = 2.0;
    p->color[0] = r;
    p->color[1] = g;
    p->color[2] = b;
    p->glow_intensity = 1.0;
    p->active = true;
    p->is_radial_line = false;
}

// Create explosion with boss-specific colors
void boss_explosion_create(BossExplosion *explosion, double x, double y, const char *boss_type) {
    if (!explosion) return;
    
    memset(explosion, 0, sizeof(BossExplosion));
    explosion->active = true;
    explosion->timer = 0;
    
    // Determine color scheme based on boss type
    double r = 0.3, g = 0.8, b = 1.0;  // Default: cyan (Death Star)
    
    if (boss_type) {
        if (strcmp(boss_type, "spawn_queen") == 0) {
            r = 1.0; g = 0.2; b = 0.8;  // Magenta
        } else if (strcmp(boss_type, "void_nexus") == 0) {
            r = 0.5; g = 0.0; b = 1.0;  // Purple
        } else if (strcmp(boss_type, "harbinger") == 0) {
            r = 1.0; g = 0.4; b = 0.0;  // Orange
        } else if (strcmp(boss_type, "star_vortex") == 0) {
            r = 1.0; g = 1.0; b = 0.0;  // Yellow
        } else if (strcmp(boss_type, "singularity") == 0) {
            r = 1.0; g = 0.0; b = 0.0;  // Red
        }
    }
    
    // Create radial burst - 32 rays spreading outward
    int num_rays = 32;
    double ray_length = 600.0;  // 3x larger radius (was 200.0)
    double ray_speed = 300.0;
    
    for (int i = 0; i < num_rays; i++) {
        double angle = (2.0 * M_PI * i) / num_rays;
        create_radial_line(explosion, x, y, angle, ray_length, ray_speed, r, g, b);
    }
    
    // Add some glow particles for secondary effect (trailing particles)
    int num_glows = 64;
    for (int i = 0; i < num_glows; i++) {
        double angle = (2.0 * M_PI * rand()) / RAND_MAX;
        double speed = 100.0 + (rand() % 200);
        double vx = cos(angle) * speed;
        double vy = sin(angle) * speed;
        
        // Slight color variation for more dynamic look
        double color_var = 0.3 + (rand() % 70) / 100.0;  // 0.3 to 1.0
        create_glow_particle(explosion, x, y, vx, vy, 0.4,
                           r * color_var, g * color_var, b * color_var);
    }
    
    fprintf(stdout, "[*] Boss explosion created at (%.0f, %.0f)\n", x, y);
}

// Update explosion particles
void boss_explosion_update(BossExplosion *explosion, double dt) {
    if (!explosion || !explosion->active) return;
    
    explosion->timer += dt;
    bool has_active = false;
    
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active) continue;
        
        p->lifetime -= dt;
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }
        
        has_active = true;
        
        if (p->is_radial_line) {
            // Radial line grows outward
            double progress = 1.0 - (p->lifetime / p->max_lifetime);
            p->length = p->max_length * progress;
            
            // Glow fades in second half
            if (progress > 0.5) {
                p->glow_intensity = 1.0 - (progress - 0.5) * 2.0;
            }
        } else {
            // Normal particle - update position and fade
            p->x += p->vx * dt;
            p->y += p->vy * dt;
            p->glow_intensity = p->lifetime / p->max_lifetime;
            
            // Apply slight gravity/slow down
            p->vx *= 0.95;
            p->vy *= 0.95;
        }
    }
    
    explosion->active = has_active;
}

// Check if active
bool boss_explosion_is_active(BossExplosion *explosion) {
    if (!explosion) return false;
    return explosion->active;
}

// Reset
void boss_explosion_reset(BossExplosion *explosion) {
    if (!explosion) return;
    memset(explosion, 0, sizeof(BossExplosion));
}
