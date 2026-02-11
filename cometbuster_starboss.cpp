#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"
#include "comet_lang.h"

#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#ifdef ExternalSound
#include "audio_wad.h"
#endif

// ============================================================================
// STAR VORTEX BOSS - Rotating star that travels across screen in 3 phases
// ============================================================================

// Helper function to spawn a missile from the star vortex
static void star_vortex_spawn_missile(CometBusterGame *game, double x, double y, 
                                       double vx, double vy, int owner_id) {
    if (!game || game->missile_count >= MAX_MISSILES) return;
    
    int slot = game->missile_count;
    Missile *missile = &game->missiles[slot];
    
    memset(missile, 0, sizeof(Missile));
    
    missile->x = x;
    missile->y = y;
    missile->vx = vx;
    missile->vy = vy;
    missile->angle = atan2(vy, vx);
    
    // Boss missiles target the player
    missile->target_x = game->ship_x;
    missile->target_y = game->ship_y;
    missile->target_id = -2;  // Special ID for player target
    missile->has_target = true;
    
    missile->lifetime = 8.0;
    missile->max_lifetime = 8.0;
    missile->turn_speed = 30.0;  // Medium turning speed
    missile->speed = 200.0;
    missile->active = true;
    missile->missile_type = 1;  // Red color (targeting player)
    missile->owner_ship_id = owner_id;  // Mark as fired by boss
    
    game->missile_count++;
}

// ============================================================================
// SPAWNING & INITIALIZATION
// ============================================================================

void comet_buster_spawn_star_vortex(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) return;
    
    SDL_Log("[SPAWN STAR VORTEX] Spawning rotating star boss at Wave %d\n", game->current_wave);
    
    BossShip *boss = &game->boss;
    memset(boss, 0, sizeof(BossShip));
    
    // Spawn off-screen left side, will travel to right side
    boss->x = -50.0;  // Start off-screen left
    boss->y = screen_height / 2.0;  // Vertical center
    
    // Horizontal movement across screen (left to right)
    // Speed: travel across 1920px width in about 20 seconds = 96 px/sec
    boss->vx = 96.0;  // Positive to move right
    boss->vy = 0;     // No vertical movement (just horizontal drifting)
    
    // Star vortex is medium health
    boss->health = 120;
    boss->max_health = 120;
    
    // Shield system
    boss->shield_health = 20;
    boss->max_shield_health = 20;
    boss->shield_active = true;
    
    // Firing
    boss->shoot_cooldown = 0.8;  // Start shooting after a moment
    
    // Phases: Phase 0 = normal (shooting missiles), Phase 1 = juggernauts spawn,
    //         Phase 2 = final explosion countdown
    boss->phase = 0;
    boss->phase_timer = 0;
    boss->phase_duration = 15.0;  // Each phase lasts ~15 seconds
    
    // Visual rotation - slower spinning star
    boss->rotation = 0;
    boss->rotation_speed = 180.0;  // Half rotation per second (slower)
    
    // Star vortex specific fields using existing BossShip fields
    boss->damage_flash_timer = 0;
    boss->fire_pattern_timer = 0;
    boss->burst_angle_offset = 0;
    
    boss->active = true;
    game->boss_active = true;
    
    // Spawn initial message
    comet_buster_spawn_floating_text(game, boss->x + 200, boss->y - 100, "STAR VORTEX ENGAGED!", 1.0, 0.5, 0.0);
    
    // Spawn a few comets alongside the boss
    comet_buster_spawn_random_comets(game, 2, screen_width, screen_height);
    
    SDL_Log("[SPAWN STAR VORTEX] Boss spawned! Position: (%.1f, %.1f), Health: %d\n", 
            boss->x, boss->y, boss->health);
}

// ============================================================================
// UPDATE & PHYSICS
// ============================================================================

// Forward declarations
static void star_vortex_shoot_asteroids(CometBusterGame *game);

void comet_buster_update_star_vortex(CometBusterGame *game, double dt, int width, int height) {
    if (!game || !game->boss_active) return;
    
    BossShip *boss = &game->boss;
    if (!boss->active) {
        game->boss_active = false;
        return;
    }
    
    // Phase management timer
    boss->phase_timer += dt;
    
    // Check if boss health reached zero in ANY phase before Phase 2 - transition to Phase 2
    if (boss->health <= 0 && boss->phase < 2) {
        SDL_Log("[STAR VORTEX] Boss health reached zero! Entering Phase 2 (10 Second Countdown)\n");
        comet_buster_spawn_floating_text( game, boss->x, boss->y - 80, phase2_detonation_text[game->current_language], 1.0, 0.5, 0.0 );
        boss->phase = 2;
        boss->phase_timer = 0;
        boss->burst_angle_offset = 0;
        boss->health = 1;  // Make immortal during countdown
    }
    
    // During Phase 2 (countdown), boss becomes immortal
    if (boss->phase == 2) {
        boss->health = 1;  // Keep health at 1 so it never dies early
    }
    // During Phase 1, keep health at minimum 3 to preserve remaining structure
    else if (boss->phase == 1 && boss->health > 0 && boss->health < 3) {
        boss->health = 3;  // Clamp to 3 (but don't interfere with death trigger above)
    }
    
    // ==========================================================================
    // INTELLIGENT MOVEMENT
    // ==========================================================================
    
    if (boss->phase == 2) {
        // Phase 2: Move toward screen center for dramatic effect
        double target_x = width / 2.0;
        double move_speed = 50.0;
        
        if (boss->x < target_x - 5) {
            boss->x += move_speed * dt;
        } else if (boss->x > target_x + 5) {
            boss->x -= move_speed * dt;
        }
    } else {
        // Phases 0-1: Intelligent movement with asteroid avoidance
        double desired_x = boss->x + boss->vx * dt;
        double desired_y = boss->y + boss->vy * dt;
        double avoidance_x = 0;
        double avoidance_y = 0;
        double avoidance_strength = 300.0;
        
        // Scan for nearby asteroids and avoid them
        for (int i = 0; i < game->comet_count; i++) {
            Comet *comet = &game->comets[i];
            if (!comet->active) continue;
            
            double dx = comet->x - boss->x;
            double dy = comet->y - boss->y;
            double dist = sqrt(dx*dx + dy*dy);
            
            // Avoid asteroids within 150 pixels
            if (dist < 150.0 && dist > 0.1) {
                // Move away from this asteroid
                double avoid_dx = -dx / dist;
                double avoid_dy = -dy / dist;
                double threat_level = 1.0 - (dist / 150.0);  // 0 to 1
                
                avoidance_x += avoid_dx * avoidance_strength * threat_level * dt;
                avoidance_y += avoid_dy * avoidance_strength * threat_level * dt;
            }
        }
        
        // Apply movement with avoidance
        boss->x += boss->vx * dt + avoidance_x;
        boss->y += boss->vy * dt + avoidance_y;
    }
    
    // Wrap around horizontally (wrap sooner to prevent rendering at edge)
    if (boss->x > width + 50) {
        boss->x = -50;
    }
    if (boss->x < -50) {
        boss->x = width + 50;
    }
    
    // Wrap around vertically (wrap sooner to prevent rendering at edge)
    if (boss->y > height + 50) {
        boss->y = -50;
    }
    if (boss->y < -50) {
        boss->y = height + 50;
    }
    
    // Visual rotation
    boss->rotation += boss->rotation_speed * dt;
    if (boss->rotation >= 360.0) {
        boss->rotation -= 360.0;
    }
    
    // Update damage flash
    if (boss->damage_flash_timer > 0) {
        boss->damage_flash_timer -= dt;
    }
    
    // ==========================================================================
    // PHASE 0: Normal - Shooting missiles at player
    // ==========================================================================
    if (boss->phase == 0) {
        // Auto-advance to Phase 1 after phase duration
        if (boss->phase_timer >= boss->phase_duration) {
            SDL_Log("[STAR VORTEX] Phase 0 complete. Entering Phase 1 (Juggernaut spawn)\n");
            comet_buster_spawn_floating_text(game, boss->x, boss->y - 80, phase1_offensive_text[game->current_language], 1.0, 1.0, 0.0);
            boss->phase = 1;
            boss->phase_timer = 0;
            boss->burst_angle_offset = 0;
        }
        
        // Missile firing pattern
        boss->shoot_cooldown -= dt;
        if (boss->shoot_cooldown <= 0) {
            star_vortex_fire_missiles(game);
            boss->shoot_cooldown = 1.2;
        }
    }
    
    // ==========================================================================
    // PHASE 1: Juggernaut Spawn - shoot at asteroids and player
    // ==========================================================================
    else if (boss->phase == 1) {
        // Spawn juggernauts periodically
        boss->fire_pattern_timer += dt;
        if (boss->fire_pattern_timer >= 3.0) {
            comet_buster_spawn_floating_text( game, boss->x, boss->y - 80, spawning_escorts_text[game->current_language], 1.0, 0.5, 1.0 ); 
            star_vortex_spawn_juggernauts(game, width, height);
            boss->fire_pattern_timer = 0;
        }
        
        // Shoot missiles at player
        boss->shoot_cooldown -= dt;
        if (boss->shoot_cooldown <= 0) {
            star_vortex_fire_missiles(game);
            boss->shoot_cooldown = 2.5;
        }
        
        // Shoot bullets at nearby asteroids
        boss->burst_angle_offset += dt;
        if (boss->burst_angle_offset >= 0.4) {  // Fire bullets every 0.4 seconds
            comet_buster_spawn_floating_text( game, boss->x, boss->y - 80, suppressing_threats_text[game->current_language], 0.0, 1.0, 1.0 );
            star_vortex_shoot_asteroids(game);
            boss->burst_angle_offset = 0;
        }
    }
    
    // ==========================================================================
    // PHASE 2: Final Countdown - Visual 10 second countdown then massive explosion
    // ==========================================================================
    else if (boss->phase == 2) {
        // Display countdown (10, 9, 8, ...)
        int countdown = 10 - (int)boss->phase_timer;
        
        // Show initial phase 2 message when countdown is 10 (first second only)
        if (countdown == 10) {
            comet_buster_spawn_floating_text( game, boss->x, boss->y + 80, core_destabilizing_text[game->current_language], 1.0, 0.0, 0.0 );
        }
        
        // Make boss immortal during countdown
        boss->health = 100;  // Keep health high
        boss->shield_health = 100;  // Keep shield strong
        boss->shield_active = true;  // Ensure shield stays active
        
        if (countdown >= 0 && countdown <= 10) {
            // Spin faster as countdown progresses
            boss->rotation_speed = 360.0 + (countdown * 30.0);  // Accelerate
            
            // Fire missiles more rapidly during countdown
            boss->shoot_cooldown -= dt;
            if (boss->shoot_cooldown <= 0) {
                star_vortex_fire_missiles(game);
                boss->shoot_cooldown = 0.8;  // Rapid fire (but slower than before)
            }
        }
        
        // Explosion happens at countdown end (phase_timer >= 10)
        if (boss->phase_timer >= 10.0) {
            SDL_Log("[STAR VORTEX] Final explosion!\n");
            star_vortex_final_explosion(game);
            boss->active = false;
            game->boss_active = false;
            
            // Major victory display
            comet_buster_spawn_floating_text(game, boss->x, boss->y, star_vortex_destroyed_text[game->current_language], 1.0, 1.0, 0.0);

            // Massive explosion effect
            comet_buster_spawn_explosion(game, boss->x, boss->y, 1, 100);
            
            // Score bonus
            int bonus = 50000;
            if (!game->game_over) {
                 game->score += bonus;
                char bonus_text[64];
                 snprintf(bonus_text, sizeof(bonus_text), "+%d %s", bonus, boss_bonus_text[game->current_language]); 
                 comet_buster_spawn_floating_text(game, boss->x, boss->y - 40, bonus_text, 1.0, 1.0, 0.2);
            }  
            return;
        }
        
        // Show countdown text every second
        if (countdown >= 0 && countdown < 10) {
            // Only show the integer change once per second
            if (boss->phase_timer - countdown < dt) {
                char countdown_text[16];
                snprintf(countdown_text, sizeof(countdown_text), "%d", countdown);
                comet_buster_spawn_floating_text(game, boss->x, boss->y - 50, countdown_text, 1.0, 0.5, 0.0);
            }
        }
    }
}

// ============================================================================
// COLLISION HANDLING WITH COMETS
// ============================================================================

bool star_vortex_handle_comet_collision(CometBusterGame *game, Comet *comet, 
                                        double collision_dx, double collision_dy) {
    if (!game || !game->boss_active || !comet) return false;
    
    BossShip *boss = &game->boss;
    if (!boss->active) return false;
    
    // Calculate distance between boss and comet
    double dist = sqrt(collision_dx * collision_dx + collision_dy * collision_dy);
    if (dist < 0.01) dist = 0.01;
    
    // Normalize collision vector
    double norm_dx = collision_dx / dist;
    double norm_dy = collision_dy / dist;
    
    // Comet's momentum (mass and velocity)
    double comet_momentum = 1.0;  // Comets are unit mass
    double comet_speed = sqrt(comet->vx * comet->vx + comet->vy * comet->vy);
    
    // Boss impact: change boss velocity based on comet's velocity and direction
    // The boss gets "bounced" by the comet
    double impact_strength = comet_speed * 0.5;  // Comets can push boss at 50% of their speed
    
    // Add velocity in the direction of collision
    boss->vx += norm_dx * impact_strength;
    boss->vy += norm_dy * impact_strength;
    
    // Cap boss speed to reasonable limits
    double boss_speed = sqrt(boss->vx * boss->vx + boss->vy * boss->vy);
    if (boss_speed > 150.0) {  // Max speed
        boss->vx = (boss->vx / boss_speed) * 150.0;
        boss->vy = (boss->vy / boss_speed) * 150.0;
    }
    
    // Shield takes normal damage (comet doesn't get special treatment)
    // The collision already damages the comet normally in the main collision handler
    
    // Visual feedback: show impact angle
    boss->shield_impact_angle = atan2(norm_dy, norm_dx);
    boss->shield_impact_timer = 0.2;  // Brief flash effect
    
    SDL_Log("[STAR VORTEX] Comet collision! Boss velocity: (%.1f, %.1f)\n", boss->vx, boss->vy);
    
    return true;  // Collision was handled
}

// ============================================================================
// ASTEROID SHOOTING
// ============================================================================

static void star_vortex_shoot_asteroids(CometBusterGame *game) {
    if (!game || !game->boss_active) return;
    
    BossShip *boss = &game->boss;
    if (!boss->active) return;
    
    // Find nearest asteroids and shoot at them
    int targets_shot = 0;
    int max_targets = 2;  // Shoot at up to 2 asteroids per volley
    
    for (int i = 0; i < game->comet_count && targets_shot < max_targets; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - boss->x;
        double dy = comet->y - boss->y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Only shoot at asteroids within 400 pixels
        if (dist < 400.0 && dist > 10.0) {
            double angle = atan2(dy, dx);
            double bullet_speed = 250.0;
            double vx = cos(angle) * bullet_speed;
            double vy = sin(angle) * bullet_speed;
            
            // Spawn bullet from star point (same as missiles)
            double star_point_angle = (boss->rotation * M_PI / 180.0) + (targets_shot * M_PI / 3.0);
            double spawn_x = boss->x + cos(star_point_angle) * 50.0;
            double spawn_y = boss->y + sin(star_point_angle) * 50.0;
            
            // Spawn as enemy bullet with boss as owner
            comet_buster_spawn_enemy_bullet_from_ship(game, spawn_x, spawn_y, vx, vy, -3);
            targets_shot++;
        }
    }
}

// ============================================================================
// FIRING PATTERNS
// ============================================================================

void star_vortex_fire_missiles(CometBusterGame *game) {
    if (!game || !game->boss_active) return;
    
    BossShip *boss = &game->boss;
    if (!boss->active) return;
    
    // Fire 2-3 missiles in a spread pattern toward the player
    double dx = game->ship_x - boss->x;
    double dy = game->ship_y - boss->y;
    double angle_to_player = atan2(dy, dx);
    
    int num_missiles = 2;
    if (boss->phase == 2) num_missiles = 3;  // More missiles during final phase
    
    double angle_spread = 45.0 * M_PI / 180.0;  // 45 degree spread
    double start_angle = angle_to_player - angle_spread / 2.0;
    
    double missile_speed = 200.0;
    double star_radius = 50.0;  // Matches the outer radius of the star
    
    for (int i = 0; i < num_missiles; i++) {
        // Calculate missile angle
        double angle = start_angle + (angle_spread / (num_missiles - 1)) * i;
        double vx = cos(angle) * missile_speed;
        double vy = sin(angle) * missile_speed;
        
        // Spawn from star point (alternate between outer points)
        // 6-pointed star = 60 degree spacing between points
        // Rotate based on star's current rotation so missiles come from different points
        double star_point_angle = (boss->rotation * M_PI / 180.0) + (i * M_PI / 3.0);  // 60 degrees apart
        double spawn_x = boss->x + cos(star_point_angle) * star_radius;
        double spawn_y = boss->y + sin(star_point_angle) * star_radius;
        
        star_vortex_spawn_missile(game, spawn_x, spawn_y, vx, vy, -3);  // -3 = star vortex boss
    }
}

void star_vortex_spawn_juggernauts(CometBusterGame *game, int width, int height) {
    if (!game) return;
    
    SDL_Log("[STAR VORTEX] Spawning juggernaut fleet from corners\n");
    
    // Spawn from four corners: top-left, top-right, bottom-left, bottom-right
    int corners[4][2] = {
        {50, 50},              // Top-left
        {width - 50, 50},      // Top-right
        {50, height - 50},     // Bottom-left
        {width - 50, height - 50}  // Bottom-right
    };
    
    for (int i = 0; i < 4; i++) {
        // Spawn a juggernaut (type 5 enemy ship) at each corner
        // Directed toward screen center
        double target_x = width / 2.0;
        double target_y = height / 2.0;
        
        double dx = target_x - corners[i][0];
        double dy = target_y - corners[i][1];
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist > 0.01) {
            double speed = 80.0;  // Juggernaut speed
            int edge = 0;  // We're specifying position directly
            
            // Use internal spawn function if available, or spawn directly
            comet_buster_spawn_enemy_ship_internal(game, width, height, 5, edge, speed, -1, 0);
            
            // Manually position the last spawned enemy ship
            if (game->enemy_ship_count > 0) {
                EnemyShip *ship = &game->enemy_ships[game->enemy_ship_count - 1];
                ship->x = corners[i][0];
                ship->y = corners[i][1];
                ship->vx = (dx / dist) * speed;
                ship->vy = (dy / dist) * speed;
                ship->angle = atan2(dy, dx);
            }
        }
    }
}

void star_vortex_final_explosion(CometBusterGame *game) {
    if (!game || !game->boss_active) return;
    
    BossShip *boss = &game->boss;
    if (!boss->active) return;
    
    double explosion_x = boss->x;
    double explosion_y = boss->y;
    
    SDL_Log("[STAR VORTEX] Final explosion at (%.1f, %.1f)\n", explosion_x, explosion_y);
    
    double missile_speed = 250.0;
    double bullet_speed = 200.0;
    
    // Fire 8 missiles in all directions (45 degree intervals)
    for (int i = 0; i < 8; i++) {
        double angle = (i * 2.0 * M_PI / 8.0);  // 45 degree intervals
        double vx = cos(angle) * missile_speed;
        double vy = sin(angle) * missile_speed;
        
        star_vortex_spawn_missile(game, explosion_x, explosion_y, vx, vy, -3);
    }
    
    // Fire 8 bullets in all directions (45 degree intervals, offset by 22.5 degrees)
    for (int i = 0; i < 8; i++) {
        double angle = (i * 2.0 * M_PI / 8.0) + (M_PI / 8.0);  // Offset 22.5 degrees
        double vx = cos(angle) * bullet_speed;
        double vy = sin(angle) * bullet_speed;
        
        comet_buster_spawn_enemy_bullet(game, explosion_x, explosion_y, vx, vy);
    }
}
