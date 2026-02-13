#ifndef BOSS_EXPLOSION_H
#define BOSS_EXPLOSION_H

#include <stdbool.h>
#ifdef CAIROBUILD
#include <cairo.h>
#endif

// Radial burst particle for explosion effects
typedef struct {
    double x, y;                 // Center position
    double vx, vy;               // Velocity
    double angle;                // Direction of ray (0-2PI)
    double lifetime;             // Seconds remaining
    double max_lifetime;
    double length;               // Current length of ray
    double max_length;           // Max length ray can grow
    double width;                // Line width
    double color[3];             // RGB (glow color)
    double glow_intensity;       // Brightness/alpha
    bool active;
    bool is_radial_line;         // True = radial burst line, False = normal particle
} BossExplosionParticle;

// Boss explosion state tracker
typedef struct {
    BossExplosionParticle particles[256];
    int particle_count;
    bool active;
    double timer;
} BossExplosion;

// Initialize explosion at position
void boss_explosion_create(BossExplosion *explosion, double x, double y, const char *boss_type);

// Update explosion (shrink/fade particles)
void boss_explosion_update(BossExplosion *explosion, double dt);

#ifdef CAIROBUILD
// Render explosion particles
void boss_explosion_draw(BossExplosion *explosion, cairo_t *cr);
#endif

// Check if explosion is still active
bool boss_explosion_is_active(BossExplosion *explosion);

// Clean up
void boss_explosion_reset(BossExplosion *explosion);
#endif
