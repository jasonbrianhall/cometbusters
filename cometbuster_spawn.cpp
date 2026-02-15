#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"
#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif


#ifdef ExternalSound
#include "audio_wad.h"
#endif 

#include "comet_lang.h"

void comet_buster_spawn_comet(CometBusterGame *game, int frequency_band, int screen_width, int screen_height) {
    if (!game) return;
    
    if (game->comet_count >= MAX_COMETS) {
        return;
    }
    
    int slot = game->comet_count;
    Comet *comet = &game->comets[slot];
    
    memset(comet, 0, sizeof(Comet));
    
    // Random position on screen edge
    int edge = rand() % 4;
    
    switch (edge) {
        case 0:  // Top
            comet->x = rand() % screen_width;
            comet->y = -30;
            break;
        case 1:  // Right
            comet->x = screen_width + 30;
            comet->y = rand() % screen_height;
            break;
        case 2:  // Bottom
            comet->x = rand() % screen_width;
            comet->y = screen_height + 30;
            break;
        case 3:  // Left
            comet->x = -30;
            comet->y = rand() % screen_height;
            break;
    }
    // Random initial rotation angle
    double rotation = rand() % 360;
    
    // Random rotation speed - can be positive (clockwise) or negative (counter-clockwise)
    double rotation_speed = rand() % 400;
    if (rand() % 2 == 0) {
        rotation_speed = -rotation_speed;  // 50% chance to rotate backwards
    }
    
    comet->rotation = rotation;
    comet->rotation_speed = rotation_speed;
    
    // Random velocity toward center-ish
    double target_x = screen_width / 2 + (rand() % 200 - 100);
    double target_y = screen_height / 2 + (rand() % 200 - 100);
    double dx = target_x - comet->x;
    double dy = target_y - comet->y;
    double len = sqrt(dx*dx + dy*dy);
    
    double speed = 50.0 + (rand() % 50);
    if (len > 0) {
        comet->vx = (dx / len) * speed;
        comet->vy = (dy / len) * speed;
    }
    
    // Set size based on wave
    int rnd = rand() % 100;
    
    // Mega comets most likely, then large, then medium, then small
    if (rnd < 40) {
        comet->size = COMET_MEGA;
        comet->radius = 50;
    } else if (rnd < 70) {
        comet->size = COMET_LARGE;
        comet->radius = 30;
    } else if (rnd < 90) {
        comet->size = COMET_MEDIUM;
        comet->radius = 20;
    } else {
        comet->size = COMET_SMALL;
        comet->radius = 10;
    }
    
    // Set properties
    comet->frequency_band = frequency_band;
    comet->active = true;
    comet->health = 1;
    
    // For vector asteroids, set a base rotation angle and shape variant
    comet->base_angle = (rand() % 360) * (M_PI / 180.0);
    
    // Store a shape variant based on current comet count (deterministic but varies)
    // This ensures same-sized asteroids don't all have the same shape
    // Use modulo on the integer calculation, then convert to double
    int speed_variant = ((int)comet->rotation_speed + game->comet_count * 17) % 360;
    //comet->rotation_speed = speed_variant + (comet->rotation_speed - (int)comet->rotation_speed);
    
    // Set color based on frequency
    comet_buster_get_frequency_color(frequency_band, 
                                     &comet->color[0], 
                                     &comet->color[1], 
                                     &comet->color[2]);
    
    game->comet_count++;
}

void comet_buster_spawn_random_comets(CometBusterGame *game, int count, int screen_width, int screen_height) {
    for (int i = 0; i < count; i++) {
        int band = rand() % 3;
        comet_buster_spawn_comet(game, band, screen_width, screen_height);
    }
}

void comet_buster_spawn_wave(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) return;
    
    // Reset boss flags
    game->boss.active = false;
    game->spawn_queen.active = false;
    
    // Check if this is a boss wave
    if (game->current_wave % 30 == 5) {
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, death_star_approaches_text[game->current_language], 0.8, 0.8, 0.2);
        // Regular boss on waves 5, 35, 65, 95, etc.
        comet_buster_spawn_boss(game, screen_width, screen_height);
        // Spawn some normal comets alongside the boss
        comet_buster_spawn_random_comets(game, 3, screen_width, screen_height);
    } else if (game->current_wave % 30 == 10) {
        // Spawn Queen on waves 10, 40, 70, 100, etc.
        comet_buster_spawn_spawn_queen(game, screen_width, screen_height);
        // Don't spawn normal comets - spawn queen controls the difficulty
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, spawn_queen_rises_text[game->current_language], 1.0, 0.3, 0.8);
    } else if (game->current_wave % 30 == 15) {
        // Void Nexus on waves 15, 45, 75, 105, etc.
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, void_nexus_emerges_text[game->current_language], 0.2, 0.8, 1.0);
        comet_buster_spawn_void_nexus(game, screen_width, screen_height);
        // Spawn some normal comets alongside the boss
        comet_buster_spawn_random_comets(game, 5, screen_width, screen_height);
    } else if (game->current_wave % 30 == 20) {
        // Harbinger on waves 20, 50, 80, 110, etc.
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, harbinger_descends_text[game->current_language], 1.0, 0.2, 0.2);
        comet_buster_spawn_harbinger(game, screen_width, screen_height);
        // Spawn some normal comets alongside the boss
        comet_buster_spawn_random_comets(game, 5, screen_width, screen_height);
    } else if (game->current_wave % 30 == 25) {
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, star_vortex_awakens_text[game->current_language], 0.8, 0.2, 1.0);
        // Star boss on waves 25, 55, 85, 115, etc.
        comet_buster_spawn_star_vortex(game, screen_width, screen_height);
        //comet_buster_spawn_boss(game, screen_width, screen_height);
        // Spawn some normal comets alongside the boss
        comet_buster_spawn_random_comets(game, 8, screen_width, screen_height);
    } else if (game->current_wave % 30 == 0) {
        comet_buster_spawn_floating_text(game, screen_width/2.0, 100.0, ultimate_threat_detected_text[game->current_language], 1.0, 1.0, 0.0);
        // Final boss on waves 30, 60, 90, 120, etc. - THE SINGULARITY
        comet_buster_spawn_singularity(game, screen_width, screen_height);
        // Spawn some normal comets alongside the boss
        comet_buster_spawn_random_comets(game, 10, screen_width, screen_height);
    } else {
        // Normal waves - spawn comets
        int wave_count = comet_buster_get_wave_comet_count(game->current_wave);
        
        for (int i = 0; i < wave_count; i++) {
            int band = rand() % 3;
            comet_buster_spawn_comet(game, band, screen_width, screen_height);
            
            // Apply speed multiplier based on wave
            if (game->comet_count > 0) {
                Comet *last_comet = &game->comets[game->comet_count - 1];
                double speed_mult = comet_buster_get_wave_speed_multiplier(game->current_wave);
                last_comet->vx *= speed_mult;
                last_comet->vy *= speed_mult;
            }
        }
        
        game->wave_comets = 0;  // Reset wave comet counter
    }
    
    // Spawn Juggernaut with 1/10 chance at the start of ANY wave (but not on first wave)
    if (game->current_wave > 1 && (rand() % 10 == 0)) {
        int random_edge = rand() % 8;  // Random spawn edge (0-7)
        double juggernaut_speed = 80.0;  // Slower than normal ships
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height,
                                              5,  // Type 5 = Juggernaut
                                              random_edge,
                                              juggernaut_speed,
                                              -1,  // No formation
                                              1);  // Formation size 1
    }
}

void comet_buster_update_wave_progression(CometBusterGame *game) {
    if (!game || game->game_over) return;
    
    // Check if all comets are destroyed to trigger next wave
    // Only trigger if we're not already in countdown (wave_complete_timer == 0)
    // AND if boss is not active (boss must be defeated before next wave)
    
    // Special handling for Spawn Queen waves (10, 40, 70, 100, etc.)
    if (game->current_wave % 30 == 10) {
        // Spawn Queen wave - allow up to 2 comets like other waves, but queen must be dead
        if (game->comet_count <= 2 && game->wave_complete_timer == 0 && !game->spawn_queen.active) {
            SDL_Log("[Comet Busters] [WAVE] Spawn Queen wave %d complete - progressing to next wave (comets remaining: %d)\n", game->current_wave, game->comet_count);
            game->wave_complete_timer = 2.0;  // 2 second delay before next wave
        }
    } else if ((game->current_wave%5 == 0 && game->comet_count <= 2 && game->wave_complete_timer == 0 && !game->boss_active) || (game->current_wave%5 > 0 && game->comet_count <= 2 && game->wave_complete_timer == 0)) {
        // All comets destroyed (except 2) and no boss active - start countdown to next wave
        game->wave_complete_timer = 2.0;  // 2 second delay before next wave
    }
}

int comet_buster_get_wave_comet_count(int wave) {
    // Returns the number of comets to spawn for a given wave
    // Wave difficulty increases progressively
    if (wave <= 0) wave = 1;
    
    // Base count increases by 2 per wave: Wave 1=3, Wave 2=5, Wave 3=7, etc.
    // With exponential scaling for later waves
    if (wave == 1) return 3;
    else if (wave == 2) return 5;
    else if (wave == 3) return 7;
    else if (wave == 4) return 9;
    else if (wave == 5) return 11;
    else {
        // For waves 6+, use exponential growth with a cap
        int count = 11 + (wave - 5) * 3;
        return (count > 32) ? 32 : count;  // Cap at 32 to prevent too many
    }
}

double comet_buster_get_wave_speed_multiplier(int wave) {
    // Returns speed multiplier for comets in given wave
    // Speeds increase as waves progress
    if (wave <= 1) return 1.0;
    else if (wave == 2) return 1.1;
    else if (wave == 3) return 1.2;
    else if (wave == 4) return 1.35;
    else if (wave == 5) return 1.5;
    else {
        // For waves 6+, continue scaling (but cap at 2.5x)
        double multiplier = 1.5 + (wave - 5) * 0.1;
        return (multiplier > 2.5) ? 2.5 : multiplier;
    }
}

void comet_buster_spawn_bullet(CometBusterGame *game, void *vis) {
    if (!game) return;
    
    // Check if we should fire a bomb instead
    if (game->using_bombs && game->bomb_ammo > 0) {
#ifndef ANDROID
        comet_buster_drop_bomb(game, 1920, 1080, vis);  // 1920x1080 is standard game resolution
#else
        comet_buster_drop_bomb(game, 720, 480, vis);  // 720x480 is standard game resolution for Android
#endif
        return;
    }
    
    // Check if we should fire a missile instead
    if (game->using_missiles && game->missile_ammo > 0) {
        comet_buster_fire_missile(game, vis);
        return;
    }
    
    // Check if we should fire spread fire instead
    // Spread fire is disabled on hard difficulty (difficulty == 2)
    if (game->using_spread_fire && game->difficulty != 2) {
        comet_buster_spawn_spread_fire(game, vis);
        return;
    }
    
    // Otherwise, fire a normal bullet
    if (game->bullet_count >= MAX_BULLETS) {
        return;
    }
    
    int slot = game->bullet_count;
    Bullet *bullet = &game->bullets[slot];
    
    memset(bullet, 0, sizeof(Bullet));
    
    bullet->x = game->ship_x;
    bullet->y = game->ship_y;
    
    double bullet_speed = 400.0;
    bullet->vx = cos(game->ship_angle) * bullet_speed;
    bullet->vy = sin(game->ship_angle) * bullet_speed;
    
    bullet->angle = game->ship_angle;
    bullet->lifetime = 1.5;  // Bullets disappear quickly
    bullet->max_lifetime = 1.5;
    bullet->active = true;
    
    game->bullet_count++;
    
    // Muzzle flash
    game->muzzle_flash_timer = 0.1;
}

void comet_buster_spawn_omnidirectional_fire(CometBusterGame *game) {
    // Fire in all 32 directions (Last Starfighter style)
    if (!game) return;
    
    // Check if we have enough fuel (costs 30 fuel per omnidirectional burst)
    double omni_energy_cost = 30.0;
    if (game->difficulty == EASY) {
        omni_energy_cost *= 0.5;  // 15 energy on Easy
    } else if (game->difficulty == MEDIUM) {
        omni_energy_cost *= 0.75;  // 22.5 energy on Medium
    }


    if (game->energy_amount < omni_energy_cost) {
        return;  // Not enough fuel
    }
    
    double bullet_speed = 400.0;
    int directions = 32;  // 32 directions in a circle
    
    for (int i = 0; i < directions; i++) {
        if (game->bullet_count >= MAX_BULLETS) break;
        
        int slot = game->bullet_count;
        Bullet *bullet = &game->bullets[slot];
        
        memset(bullet, 0, sizeof(Bullet));
        
        bullet->x = game->ship_x;
        bullet->y = game->ship_y;
        
        // Calculate angle for this direction (32 evenly spaced directions)
        double angle = (i * 360.0 / directions) * (M_PI / 180.0);
        
        bullet->vx = cos(angle) * bullet_speed;
        bullet->vy = sin(angle) * bullet_speed;
        
        bullet->angle = angle;
        bullet->lifetime = 1.5;
        bullet->max_lifetime = 1.5;
        bullet->active = true;
        
        game->bullet_count++;
    }
    
    // Consume fuel for omnidirectional fire
    game->energy_amount -= omni_energy_cost;
    if (game->energy_amount < 0) game->energy_amount = 0;
    
    // Muzzle flash
    game->muzzle_flash_timer = 0.15;
}

void comet_buster_spawn_spread_fire(CometBusterGame *game, void *vis) {
    // Fire bullets in a spread pattern in front of the ship
    // Energy cost is always 5x (1.25 energy per shot)
    // 
    // On Medium difficulty: fires only 3 bullets (but still costs 5x energy)
    //   This creates a tradeoff: less firepower, but faster energy drain
    // On Easy/Hard difficulty: fires 5 bullets (and costs 5x energy)
    //
    // Hard difficulty does NOT have spread fire available at all
    if (!game) return;
    
    double bullet_speed = 400.0;
    int num_bullets = 5;  // Default: 5 bullets
    
    // On medium difficulty, fire only 3 bullets (but same 5x energy cost)
    if (game->difficulty == 1) {  // MEDIUM difficulty
        num_bullets = 3;
    }
    
    double spread_angle = 30.0 * (M_PI / 180.0);  // 30 degree total spread (±15 degrees)
    
    // Calculate angle offset from center
    double angle_step = spread_angle / (num_bullets - 1);
    double base_angle = game->ship_angle - (spread_angle / 2.0);
    
    for (int i = 0; i < num_bullets; i++) {
        if (game->bullet_count >= MAX_BULLETS) break;
        
        int slot = game->bullet_count;
        Bullet *bullet = &game->bullets[slot];
        
        memset(bullet, 0, sizeof(Bullet));
        
        bullet->x = game->ship_x;
        bullet->y = game->ship_y;
        
        // Calculate angle for this bullet in the spread
        double bullet_angle = base_angle + (i * angle_step);
        
        bullet->vx = cos(bullet_angle) * bullet_speed;
        bullet->vy = sin(bullet_angle) * bullet_speed;
        
        bullet->angle = bullet_angle;
        bullet->lifetime = 1.5;
        bullet->max_lifetime = 1.5;
        bullet->active = true;
        
        game->bullet_count++;
    }
    
    // Muzzle flash (slightly larger for spread fire)
    game->muzzle_flash_timer = 0.12;
}

void comet_buster_spawn_explosion(CometBusterGame *game, double x, double y,
                                   int frequency_band, int particle_count) {
    for (int i = 0; i < particle_count; i++) {
        if (game->particle_count >= MAX_PARTICLES) {
            break;
        }
        
        int slot = game->particle_count;
        Particle *p = &game->particles[slot];
        
        memset(p, 0, sizeof(Particle));
        
        double angle = (2.0 * M_PI * i) / particle_count + 
                       ((rand() % 100) / 100.0) * 0.3;
        double speed = 100.0 + (rand() % 100);
        
        p->x = x;
        p->y = y;
        p->vx = cos(angle) * speed;
        p->vy = sin(angle) * speed;
        p->lifetime = 0.3 + (rand() % 20) / 100.0;
        p->max_lifetime = p->lifetime;
        p->size = 2.0 + (rand() % 4);
        p->active = true;
        
        comet_buster_get_frequency_color(frequency_band,
                                        &p->color[0],
                                        &p->color[1],
                                        &p->color[2]);
        
        game->particle_count++;
    }
}

// Special explosion for ship death - ABSOLUTELY UNMISSABLE
void comet_buster_spawn_ship_death_explosion(CometBusterGame *game, double x, double y) {
    if (!game) return;
    
    // Spawn purple/blue explosion particles
    // Core burst - 100 particles
    for (int burst = 0; burst < 100; burst++) {
        if (game->particle_count >= MAX_PARTICLES - 50) break;
        
        int slot = game->particle_count;
        Particle *p = &game->particles[slot];
        memset(p, 0, sizeof(Particle));
        
        double angle = (2.0 * M_PI * burst) / 100.0 + ((rand() % 100) / 100.0) * 0.3;
        double speed = 120.0 + (rand() % 100);  // 120-220 px/sec
        
        p->x = x;
        p->y = y;
        p->vx = cos(angle) * speed;
        p->vy = sin(angle) * speed;
        p->lifetime = 0.8 + (rand() % 20) / 100.0;  // 0.8-1.0 seconds
        p->max_lifetime = p->lifetime;
        p->size = 4.0 + (rand() % 5);  // 4-9 pixels
        p->active = true;
        
        // Purple/blue core
        p->color[0] = 0.6;
        p->color[1] = 0.3;
        p->color[2] = 1.0;
        
        game->particle_count++;
    }
    
    // Trailing debris - 70 particles
    for (int burst = 0; burst < 70; burst++) {
        if (game->particle_count >= MAX_PARTICLES - 30) break;
        
        int slot = game->particle_count;
        Particle *p = &game->particles[slot];
        memset(p, 0, sizeof(Particle));
        
        double angle = (2.0 * M_PI * burst) / 70.0 + ((rand() % 100) / 100.0) * 0.5;
        double speed = 80.0 + (rand() % 60);  // 80-140 px/sec
        
        p->x = x;
        p->y = y;
        p->vx = cos(angle) * speed;
        p->vy = sin(angle) * speed;
        p->lifetime = 1.0 + (rand() % 20) / 100.0;  // 1.0-1.2 seconds
        p->max_lifetime = p->lifetime;
        p->size = 3.0 + (rand() % 4);  // 3-7 pixels
        p->active = true;
        
        // Light blue trailing smoke
        p->color[0] = 0.4;
        p->color[1] = 0.6;
        p->color[2] = 1.0;
        
        game->particle_count++;
    }
    
    // Apply explosion damage in radius - up to 20 damage based on distance
    double explosion_radius = 250.0;  // Damage radius
    double max_damage = 20.0;
    
    // Damage comets within radius
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        if (!c->active) continue;
        
        double dx = c->x - x;
        double dy = c->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist < explosion_radius) {
            // Damage decreases with distance (inverse relationship)
            // At center (dist=0): 20 damage, at radius edge: 1 damage
            double damage = max_damage * (1.0 - (dist / explosion_radius));
            damage = (damage < 1.0) ? 1.0 : damage;  // Minimum 1 damage
            c->health -= (int)damage;
            
            if (c->health <= 0) {
                c->active = false;
                game->comets_destroyed++;
            }
        }
    }
    
    // Damage enemy ships within radius
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *e = &game->enemy_ships[i];
        if (!e->active) continue;
        
        double dx = e->x - x;
        double dy = e->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist < explosion_radius) {
            double damage = max_damage * (1.0 - (dist / explosion_radius));
            damage = (damage < 1.0) ? 1.0 : damage;
            e->health -= (int)damage;
            
            if (e->health <= 0) {
                e->active = false;
            }
        }
    }
    
    // Damage UFOs within radius
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *u = &game->ufos[i];
        if (!u->active) continue;
        
        double dx = u->x - x;
        double dy = u->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        if (dist < explosion_radius) {
            double damage = max_damage * (1.0 - (dist / explosion_radius));
            damage = (damage < 1.0) ? 1.0 : damage;
            u->health -= (int)damage;
            
            if (u->health <= 0) {
                u->active = false;
            }
        }
    }
    
    SDL_Log("[Comet Busters] [Explosion] Ship destroyed at (%.0f, %.0f) - dealt damage in %.0f pixel radius\n", 
            x, y, explosion_radius);
}

void comet_buster_spawn_enemy_ship_internal(CometBusterGame *game, int screen_width, int screen_height, 
                                            int ship_type, int edge, double speed, int formation_id, int formation_size) {
    if (!game || game->enemy_ship_count >= MAX_ENEMY_SHIPS) {
        return;
    }
    
    int slot = game->enemy_ship_count;
    EnemyShip *ship = &game->enemy_ships[slot];
    
    memset(ship, 0, sizeof(EnemyShip));
    
    double diagonal_speed = speed / sqrt(2);  // Normalize diagonal speed
    
    ship->ship_type = ship_type;
    
    // Formation fields for sentinels
    if (ship_type == 3) {
        ship->formation_id = formation_id;
        ship->formation_size = formation_size;
        ship->has_partner = (formation_size > 1);
        ship->formation_cohesion = 0.7;
    } else {
        ship->formation_id = -1;
        ship->formation_size = 1;
        ship->has_partner = false;
        ship->formation_cohesion = 0.0;
    }
    
    // Set position based on edge
    switch (edge) {
        case 0:  // From left to right
            ship->x = -20;
            ship->y = 50 + (rand() % (screen_height - 100));  // Avoid top/bottom edges
            ship->vx = speed;
            ship->vy = 0;
            ship->angle = 0;  // Facing right
            ship->base_vx = speed;
            ship->base_vy = 0;
            break;
        case 1:  // From right to left
            ship->x = screen_width + 20;
            ship->y = 50 + (rand() % (screen_height - 100));  // Avoid top/bottom edges
            ship->vx = -speed;
            ship->vy = 0;
            ship->angle = M_PI;  // Facing left
            ship->base_vx = -speed;
            ship->base_vy = 0;
            break;
        case 2:  // From top to bottom
            ship->x = 50 + (rand() % (screen_width - 100));  // Avoid left/right edges
            ship->y = -20;
            ship->vx = 0;
            ship->vy = speed;
            ship->angle = M_PI / 2;  // Facing down
            ship->base_vx = 0;
            ship->base_vy = speed;
            break;
        case 3:  // From bottom to top
            ship->x = 50 + (rand() % (screen_width - 100));  // Avoid left/right edges
            ship->y = screen_height + 20;
            ship->vx = 0;
            ship->vy = -speed;
            ship->angle = 3 * M_PI / 2;  // Facing up
            ship->base_vx = 0;
            ship->base_vy = -speed;
            break;
        case 4:  // From top-left to bottom-right (diagonal)
            ship->x = -20;
            ship->y = -20;
            ship->vx = diagonal_speed;
            ship->vy = diagonal_speed;
            ship->angle = atan2(diagonal_speed, diagonal_speed);  // 45 degrees
            ship->base_vx = diagonal_speed;
            ship->base_vy = diagonal_speed;
            break;
        case 5:  // From top-right to bottom-left (diagonal)
            ship->x = screen_width + 20;
            ship->y = -20;
            ship->vx = -diagonal_speed;
            ship->vy = diagonal_speed;
            ship->angle = atan2(diagonal_speed, -diagonal_speed);  // 135 degrees
            ship->base_vx = -diagonal_speed;
            ship->base_vy = diagonal_speed;
            break;
        case 6:  // From bottom-left to top-right (diagonal)
            ship->x = -20;
            ship->y = screen_height + 20;
            ship->vx = diagonal_speed;
            ship->vy = -diagonal_speed;
            ship->angle = atan2(-diagonal_speed, diagonal_speed);  // 315 degrees
            ship->base_vx = diagonal_speed;
            ship->base_vy = -diagonal_speed;
            break;
        case 7:  // From bottom-right to top-left (diagonal)
            ship->x = screen_width + 20;
            ship->y = screen_height + 20;
            ship->vx = -diagonal_speed;
            ship->vy = -diagonal_speed;
            ship->angle = atan2(-diagonal_speed, -diagonal_speed);  // 225 degrees
            ship->base_vx = -diagonal_speed;
            ship->base_vy = -diagonal_speed;
            break;
    }
    
    // Add slight offset for sentinel formation ships (so they don't spawn on top of each other)
    if (ship_type == 3) {
        double offset_angle = (formation_size > 1) ? (2.0 * M_PI * formation_id / formation_size) : 0;
        double offset_dist = 30.0;  // Pixels apart
        ship->x += cos(offset_angle) * offset_dist;
        ship->y += sin(offset_angle) * offset_dist;
    }
    
    // Set health based on ship type (juggernaut is special)
    if (ship_type == 5) {
        ship->health = 10;  // Juggernaut: reduced durability
    } else {
        ship->health = 1;   // All other types: 1 hit = destroyed
    }
    
    ship->shoot_cooldown = 1.0 + (rand() % 20) / 10.0;  // Shoot after 1-3 seconds
    ship->path_time = 0.0;  // Start at beginning of sine wave
    ship->active = true;
    
    // Initialize patrol behavior for blue, green, and purple ships
    // Red aggressive ships (type 1) don't use patrol behaviors
    ship->patrol_behavior_timer = 0.0;
    ship->patrol_behavior_duration = 2.0 + (rand() % 20) / 10.0;  // 2-4 seconds before behavior change
    ship->patrol_behavior_type = 0;  // Start with straight movement (0=straight, 1=circle, 2=evasive turns)
    ship->patrol_circle_radius = 80.0 + (rand() % 60);  // 80-140px radius circles
    ship->patrol_circle_angle = 0.0;
    
    // Shield system for enemy ships (varies by type)
    if (ship->ship_type == 1) {
        // Red ships (aggressive): 2 shield points
        ship->max_shield_health = 2;
        ship->shield_health = 2;
    } else if (ship->ship_type == 2) {
        // Green ships (hunter): 3 shield points
        ship->max_shield_health = 3;
        ship->shield_health = 3;
    } else if (ship->ship_type == 3) {
        // Purple ships (sentinel): 4 shield points (more durable)
        ship->max_shield_health = 4;
        ship->shield_health = 4;
    } else if (ship_type == 4) {
        // BROWN COAT ELITE BLUE SHIP
        ship->max_shield_health = 5;
        ship->shield_health = 5;
        ship->burst_fire_cooldown = 1.0;      // Start with quick first burst
        ship->burst_trigger_range = 250.0;    // Trigger range for burst
        ship->last_burst_direction = 0;       // Initial pattern rotation
        ship->proximity_detection_timer = 0.0;// Start checking immediately
        ship->burst_count_this_wave = 0;      // Track total bursts
    } else if (ship_type == 5) {
        // JUGGERNAUT - massive, heavily armored ship
        ship->max_shield_health = 5;
        ship->shield_health = 5;  // Reduced shield protection
        // Juggernaut fires FAST
        ship->shoot_cooldown = 0.5 + (rand() % 10) / 10.0;  // Shoot every 0.5-1.5 seconds
    } else {
        // Blue ships (patrol): 3 shield points
        ship->max_shield_health = 3;
        ship->shield_health = 3;
    }
    
    ship->shield_impact_timer = 0;
    ship->shield_impact_angle = 0;
    
    game->enemy_ship_count++;
}

void comet_buster_spawn_enemy_ship(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) {
        return;
    }
    
    // Random edge to spawn from (now includes diagonals)
    int edge = rand() % 8;
    double speed = 80.0 + (rand() % 40);  // 80-120 pixels per second
    
    // Calculate wave-based blue ship reduction
    // Start at 70%, decrease 2% per wave until reaching a minimum of 40%
    int blue_ship_chance = 70 - (game->current_wave * 2);
    if (blue_ship_chance < 40) {
        blue_ship_chance = 40;
    }
    
    // Difficulty-based adjustments
    if (game->difficulty == 0) {
        // Easy: more blue ships (safer), fewer aggressive ships
        blue_ship_chance += 20;  // Increase blue ships by 20%
        if (blue_ship_chance > 85) blue_ship_chance = 85;
    } else if (game->difficulty == 2) {
        // Hard: fewer blue ships, more aggressive/dangerous ships
        blue_ship_chance -= 15;  // Decrease blue ships by 15%
        if (blue_ship_chance < 25) blue_ship_chance = 25;
    }
    // Medium (difficulty == 1): no adjustment to blue_ship_chance
    
    // Calculate wave-based difficulty increase (blue reduction distributed to other ships)
    // Each wave, 2% taken from blue is distributed proportionally to other ship types
    int wave_difficulty_bonus = (70 - blue_ship_chance) / 5;  // Distribute to 5 ship types (red, green, brown, sentinel, and buffer)
    
    int red_ship_chance = 10 + wave_difficulty_bonus;
    int green_ship_chance = 10 + wave_difficulty_bonus;
    int brown_ship_chance = 3 + (wave_difficulty_bonus / 2);
    int sentinel_ship_chance = 7 + (wave_difficulty_bonus / 2);
    (void)sentinel_ship_chance;  // Used in future expansion
    
    // Randomly decide ship type:
    // 10% → 20% (increases with waves) chance of aggressive red ship (attacks player)
    // 70% → 40% (decreases with waves) chance of patrol blue ship (shoots comets)
    // 10% → 20% (increases with waves) chance of hunter green ship (shoots comets fast, chases if close)
    // 3% → 8% (increases with waves) chance of brown coat type 4 ship
    // 7% → 12% (increases with waves) chance of sentinel purple ship (defensive formation)
    
    // Check if any red ships are currently active
    bool red_ship_active = false;
    for (int i = 0; i < game->enemy_ship_count; i++) {
        if (game->enemy_ships[i].active && game->enemy_ships[i].ship_type == 1) {
            red_ship_active = true;
            break;
        }
    }
    
    int type_roll = rand() % 100;
    int threshold = 0;
    
    threshold += red_ship_chance;
    if (type_roll < threshold) {
        // Red (aggressive) - single ship
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 1, edge, speed, -1, 1);
    } else if (type_roll < threshold + blue_ship_chance) {
        // Blue (patrol) - single ship (decreases as waves increase)
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 0, edge, speed, -1, 1);
    } else if ((threshold += blue_ship_chance) + green_ship_chance > type_roll) {
        // Green (hunter) - single ship
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 2, edge, speed, -1, 1);
    } else if ((threshold += green_ship_chance) + brown_ship_chance > type_roll) {
        // Brown coat (type 4) - single ship
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 4, edge, speed, -1, 1);
    } else if (!red_ship_active && game->enemy_ship_count + 2 < MAX_ENEMY_SHIPS) {
        // Purple (sentinel) - spawn as PAIR (2-3 ships) - only if no red ships active
        // and if there's room for at least 2 more ships
        int formation_id = game->current_wave * 100 + (int)(game->enemy_ship_spawn_timer * 10);
        int formation_size = (rand() % 2) + 2;  // 2 or 3 sentinels
        
        // Spawn all sentinels in the formation at the same edge
        for (int i = 0; i < formation_size; i++) {
            comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 3, edge, speed, formation_id, formation_size);
        }
    } else {
        // Fallback to blue ship if conditions not met for sentinel
        comet_buster_spawn_enemy_ship_internal(game, screen_width, screen_height, 0, edge, speed, -1, 1);
    }
}
void comet_buster_spawn_enemy_bullet(CometBusterGame *game, double x, double y, double vx, double vy) {
    comet_buster_spawn_enemy_bullet_from_ship(game, x, y, vx, vy, -1);
}

void comet_buster_spawn_enemy_bullet_from_ship(CometBusterGame *game, double x, double y, 
                                               double vx, double vy, int owner_ship_id) {
    if (!game || game->enemy_bullet_count >= MAX_ENEMY_BULLETS) {
        return;
    }
    
    int slot = game->enemy_bullet_count;
    Bullet *bullet = &game->enemy_bullets[slot];
    
    // Initialize ALL fields to prevent garbage data
    memset(bullet, 0, sizeof(Bullet));
    
    bullet->x = x;
    bullet->y = y;
    bullet->vx = vx;
    bullet->vy = vy;
    bullet->angle = atan2(vy, vx);
    bullet->lifetime = 10.0;
    bullet->max_lifetime = 10.0;
    bullet->active = true;
    bullet->owner_ship_id = owner_ship_id;  // EXPLICITLY set the owner
    
    game->enemy_bullet_count++;
}


void comet_buster_spawn_boss(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) return;
    
    SDL_Log("[Comet Busters] [SPAWN BOSS] Attempting to spawn boss at Wave %d\n", game->current_wave);
    
    BossShip *boss = &game->boss;
    memset(boss, 0, sizeof(BossShip));
    
    // Spawn boss off-screen at the top so it scrolls in
    boss->x = screen_width / 2.0;
    boss->y = -80.0;  // Start above the screen
    boss->vx = 40.0 + (rand() % 40);  // Slow horizontal movement
    boss->vy = 100.0;  // Scroll down at 100 pixels per second
    boss->angle = 0;
    
    // Boss health - tripled for epic final battle
    boss->health = 180;
    boss->max_health = 180;
    
    // Shield system - also increased proportionally
    boss->shield_health = 30;
    boss->max_shield_health = 30;
    boss->shield_active = true;
    
    // Shooting
    boss->shoot_cooldown = 0;
    
    // Phases
    boss->phase = 0;  // Start in normal phase
    boss->phase_timer = 0;
    boss->phase_duration = 5.0;  // 5 seconds per phase
    
    // Visual
    boss->rotation = 0;
    boss->rotation_speed = 45.0;  // degrees per second
    boss->damage_flash_timer = 0;
    
    boss->active = true;
    game->boss_active = true;
    
    // Spawn some normal comets alongside the boss
    comet_buster_spawn_random_comets(game, 3, screen_width, screen_height);
    
    SDL_Log("[Comet Busters] [SPAWN BOSS] Boss spawned! Position: (%.1f, %.1f), Active: %d, Health: %d\n", 
            boss->x, boss->y, boss->active, boss->health);
}

/*void comet_buster_spawn_star_boss(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) return;
    
    BossShip *boss = &game->boss;
    memset(boss, 0, sizeof(BossShip));
    
    // Center of screen
    boss->x = screen_width / 2.0;
    boss->y = screen_height / 2.0;
    
    boss->vx = 0;
    boss->vy = 0;
    
    boss->health = 60;  // 60 health for star boss
    boss->max_health = 60;
    
    boss->shield_health = 15;
    boss->max_shield_health = 15;
    boss->shield_active = true;
    
    boss->width = 40;
    boss->height = 40;
    
    // Star boss specific
    boss->boss_type = 3;  // Type 3 = star boss
    boss->wave_phase = 1;  // Start at phase 1
    boss->phase_timer = 0;
    boss->explosion_timer = 0;
    
    // Rapid rotation
    boss->rotation = 0;
    boss->rotation_speed = 180.0;  // Fast rotation
    boss->damage_flash_timer = 0;
    
    boss->active = true;
    game->boss_active = true;
    
    SDL_Log("[Comet Busters] [SPAWN STAR BOSS] Rotating star boss spawned! Position: (%.1f, %.1f)\n", 
            boss->x, boss->y);
}*/

void comet_buster_spawn_floating_text(CometBusterGame *game, double x, double y, const char *text, double r, double g, double b) {
    if (!game || game->floating_text_count >= MAX_FLOATING_TEXT) {
        return;
    }
    
    int slot = game->floating_text_count;
    FloatingText *ft = &game->floating_texts[slot];
    
    memset(ft, 0, sizeof(FloatingText));
    
    ft->x = x;
    ft->y = y;
    ft->lifetime = 2.0;  // Display for 2 seconds
    ft->max_lifetime = 2.0;
    ft->color[0] = r;
    ft->color[1] = g;
    ft->color[2] = b;
    ft->active = true;
    
    strncpy(ft->text, text, sizeof(ft->text) - 1);
    ft->text[sizeof(ft->text) - 1] = '\0';
    
    game->floating_text_count++;
}

void comet_buster_spawn_canister(CometBusterGame *game, double x, double y) {
    if (!game) return;
    
    if (game->canister_count >= MAX_CANISTERS) {
        return;
    }
    
    int slot = game->canister_count;
    Canister *canister = &game->canisters[slot];
    
    memset(canister, 0, sizeof(Canister));
    
    // Position canister at the given location
    canister->x = x;
    canister->y = y;
    
    // Give it a small drift velocity for visual interest
    canister->vx = (rand() % 100 - 50) * 0.5;  // Random drift -25 to 25 pixels/sec
    canister->vy = (rand() % 100 - 50) * 0.5;
    
    // Canister lasts 7 seconds
    canister->lifetime = 7.0;
    canister->max_lifetime = 7.0;
    
    // Spin animation
    canister->rotation = 0;
    canister->rotation_speed = 180.0 + (rand() % 180);  // 180-360 degrees per second
    
    canister->active = true;
    
    game->canister_count++;
}

void comet_buster_spawn_missile_pickup(CometBusterGame *game, double x, double y) {
    if (!game) return;
    
    if (game->missile_pickup_count >= MAX_MISSILE_PICKUPS) {
        return;
    }
    
    int slot = game->missile_pickup_count;
    MissilePickup *pickup = &game->missile_pickups[slot];
    
    memset(pickup, 0, sizeof(MissilePickup));
    
    // Position missile pickup at the given location
    pickup->x = x;
    pickup->y = y;
    
    // Give it a small drift velocity for visual interest
    pickup->vx = (rand() % 100 - 50) * 0.5;  // Random drift
    pickup->vy = (rand() % 100 - 50) * 0.5;
    
    // Missile pickup lasts 10 seconds (longer than canister)
    pickup->lifetime = 10.0;
    pickup->max_lifetime = 10.0;
    
    // Spin animation
    pickup->rotation = 0;
    pickup->rotation_speed = 200.0 + (rand() % 160);  // 200-360 degrees per second (faster than canister)
    
    pickup->active = true;
    
    game->missile_pickup_count++;
}

// ============================================================================
// UFO (FLYING SAUCER) SYSTEM - Classic Asteroids-style encounters
// ============================================================================

void comet_buster_spawn_ufo(CometBusterGame *game, int screen_width, int screen_height) {
    if (!game) return;
    if (game->ufo_count >= MAX_UFOS) return;
    
    int slot = game->ufo_count;
    UFO *ufo = &game->ufos[slot];
    
    memset(ufo, 0, sizeof(UFO));
    
    // Random entry side (0 = from left, 1 = from right)
    int entry_side = rand() % 2;
    
    // UFO moves horizontally across screen at SLOWER speed
    // Original Asteroids UFO: ~60 pixels/sec
    if (entry_side == 0) {
        // Enter from left
        ufo->x = -50;
        ufo->vx = 60.0 + (rand() % 40);  // 60-100 px/sec (much slower!)
        ufo->direction = 1;
    } else {
        // Enter from right
        ufo->x = screen_width + 50;
        ufo->vx = -(60.0 + (rand() % 40));
        ufo->direction = -1;
    }
    
    // Random height (avoid edges) - but this becomes the CENTER height
    int height_zone = rand() % 3;
    if (height_zone == 0) {
        ufo->entry_height = screen_height * 0.25;  // Upper third
    } else if (height_zone == 1) {
        ufo->entry_height = screen_height * 0.5;   // Middle
    } else {
        ufo->entry_height = screen_height * 0.75;  // Lower third
    }
    
    ufo->y = ufo->entry_height;
    ufo->vy = 0.0;  // Will be calculated via sine wave
    
    // UFO properties
    ufo->health = 3;
    ufo->max_health = 3;
    ufo->angle = (ufo->direction > 0) ? 0 : M_PI;  // Face direction of travel
    ufo->active = true;
    ufo->lifetime = 0.0;
    
    // Firing - shoots occasionally at player (slower than before)
    ufo->shoot_cooldown = 2.0 + (rand() % 10) * 0.1;  // 2.0-3.0 seconds between shots
    ufo->shoot_timer = ufo->shoot_cooldown;
    
    // Burner effects
    ufo->burner_intensity = 0.0;
    ufo->burner_flicker_timer = 0.0;
    ufo->damage_flash_timer = 0.0;
    
    // Audio effects - UFO sound plays periodically
    ufo->sound_timer = 0.5;  // Start sound after 0.5 seconds
    
    game->ufo_count++;
}

void comet_buster_update_ufos(CometBusterGame *game, double dt, int width, int height, Visualizer *visualizer) {
    if (!game) return;
    
    // Update UFO spawn timer
    if (game->ufo_count < MAX_UFOS) {
        game->ufo_spawn_timer -= dt;
        if (game->ufo_spawn_timer <= 0) {
            comet_buster_spawn_ufo(game, width, height);
            game->ufo_spawn_timer = game->ufo_spawn_rate + (rand() % 20 - 10);  // Add variance
        }
    }
    
    // Update all UFOs
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *ufo = &game->ufos[i];
        if (!ufo->active) continue;
        
        // Move UFO horizontally
        ufo->x += ufo->vx * dt;
        
        // Add sinusoidal (wavy) vertical movement - REDUCED oscillation
        // Travels further horizontally with gentler up-down motion
        double wave_amplitude = 20.0;  // Reduced from 40.0 (gentler waves)
        double wave_frequency = 0.5;   // Reduced from 2.0 (travels further before oscillating)
        
        // Calculate Y position with sine wave - travels much further before bobbing
        double sine_offset = sin(ufo->lifetime * wave_frequency * M_PI) * wave_amplitude;
        ufo->y = ufo->entry_height + sine_offset;
        
        ufo->lifetime += dt;
        
        // Check collision with asteroids - UFO is destroyed on impact!
        for (int j = 0; j < game->comet_count; j++) {
            Comet *comet = &game->comets[j];
            if (!comet->active) continue;
            
            if (comet_buster_check_ufo_comet(ufo, comet)) {
                // UFO hits asteroid and is destroyed!
                comet_buster_destroy_ufo(game, i, width, height, visualizer);
                break;  // Exit asteroid loop
            }
        }
        
        // Skip rest of update if UFO was destroyed by asteroid
        if (!ufo->active) continue;
        
        // Despawn if it goes off screen
        if ((ufo->vx > 0 && ufo->x > width + 100) ||
            (ufo->vx < 0 && ufo->x < -100)) {
            ufo->active = false;
            continue;
        }
        
        // Update burner effects
        {
            double speed = fabs(ufo->vx);
            double max_speed = 100.0;  // Full intensity at 100 px/sec
            double target_intensity = (speed > 0) ? fmin(speed / max_speed, 1.0) : 0.0;
            
            if (target_intensity > ufo->burner_intensity) {
                ufo->burner_intensity = fmin(target_intensity, ufo->burner_intensity + dt * 3.0);
            } else {
                ufo->burner_intensity = fmax(target_intensity, ufo->burner_intensity - dt * 2.5);
            }
            
            ufo->burner_flicker_timer += dt * 12.0;
            if (ufo->burner_flicker_timer > 1.0) {
                ufo->burner_flicker_timer = 0.0;
            }
        }
        
        // Update damage flash
        if (ufo->damage_flash_timer > 0) {
            ufo->damage_flash_timer -= dt;
        }
        
        // Play UFO sound effect periodically (not too annoying)
        ufo->sound_timer -= dt;
        if (ufo->sound_timer <= 0) {
#ifdef ExternalSound
            if (visualizer && visualizer->audio.sfx_ufo && !game->splash_screen_active) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_ufo);
            }
#endif
            ufo->sound_timer = 0.2;  // Repeat every 0.3 seconds
        }
        
        // UFO fires at player occasionally
        ufo->shoot_timer -= dt;
        if (ufo->shoot_timer <= 0) {
            comet_buster_ufo_fire(game);
            ufo->shoot_timer = ufo->shoot_cooldown;
        }
    }
    
    // Remove dead UFOs
    for (int i = game->ufo_count - 1; i >= 0; i--) {
        if (!game->ufos[i].active) {
            if (i != game->ufo_count - 1) {
                game->ufos[i] = game->ufos[game->ufo_count - 1];
            }
            game->ufo_count--;
        }
    }
}

void comet_buster_ufo_fire(CometBusterGame *game) {
    if (!game || game->ufo_count == 0) return;
    
    // Fire from a random UFO
    UFO *ufo = &game->ufos[0];
    if (!ufo->active) return;
    
    // Fire toward approximate player position with some spread
    double dx = game->ship_x - ufo->x;
    double dy = game->ship_y - ufo->y;
    
    // Add some inaccuracy (UFOs aren't perfect shots)
    double spread = 0.3;  // Radians of spread
    double angle_to_player = atan2(dy, dx);
    double shot_angle = angle_to_player + (rand() % 100 - 50) * 0.01 * spread;
    
    // Bullet velocity
    double bullet_speed = 200.0;
    double bvx = cos(shot_angle) * bullet_speed;
    double bvy = sin(shot_angle) * bullet_speed;
    
    // Fire from center of UFO
    comet_buster_spawn_enemy_bullet_from_ship(game, ufo->x, ufo->y, bvx, bvy, -2);  // -2 = UFO owner ID
}

bool comet_buster_check_bullet_ufo(Bullet *b, UFO *u) {
    if (!b || !u || !b->active || !u->active) return false;
    
    double dx = u->x - b->x;
    double dy = u->y - b->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 25.0;  // UFO collision radius (bigger UFO = bigger hitbox)
}

bool comet_buster_check_missile_ufo(Missile *m, UFO *u) {
    if (!m || !u || !m->active || !u->active) return false;
    
    double dx = u->x - m->x;
    double dy = u->y - m->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 30.0;  // Missile hitbox is slightly bigger than bullets
}

void comet_buster_destroy_ufo(CometBusterGame *game, int ufo_index, int width, int height, void *vis) {
    if (!game || ufo_index < 0 || ufo_index >= game->ufo_count) return;
    
    UFO *ufo = &game->ufos[ufo_index];
    if (!ufo->active) return;
    
    // Award points - UFOs are worth 250 points (bonus!) with multiplier applied
    int base_points = 250;
    int score_add=0;
    if (!game->game_over) {
        score_add = (int)(base_points * game->score_multiplier);
        game->score += score_add;
    }
    // Explosion
    comet_buster_spawn_explosion(game, ufo->x, ufo->y, 1, 15);  // Mid-frequency explosion
    
    // Play explosion sound
    if (vis && !game->splash_screen_active) {
        Visualizer *visualizer = (Visualizer *)vis;
#ifdef ExternalSound
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_explosion);
#endif
    }
    
    // Create floating text with actual score value
    char score_text[32];
    snprintf(score_text, sizeof(score_text), "+%d", score_add);
    comet_buster_spawn_floating_text(game, ufo->x, ufo->y, score_text, 1.0, 0.8, 0.0);
    
    // Weapon/Pickup drop chances for UFOs
    int drop_roll = rand() % 100;
    if (game->splash_screen_active)
    {
        drop_roll=100000;
    }
    if (drop_roll < 10) {
        // 10% chance to spawn bomb pickup from UFO
        comet_buster_spawn_bomb_pickup(game, ufo->x, ufo->y);
    } else if (drop_roll < 20) {
        // 10% chance to spawn missile pickup
        comet_buster_spawn_missile_pickup(game, ufo->x, ufo->y);
    } else if (drop_roll < 30) {
        // 10% chance to spawn shield canister
        comet_buster_spawn_canister(game, ufo->x, ufo->y);
    }
    // 70% chance to drop nothing

    
    ufo->active = false;
}
