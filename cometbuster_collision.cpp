#include <cairo.h>
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

void comet_buster_handle_comet_collision(Comet *c1, Comet *c2, double dx, double dy, 
                                         double dist, double min_dist) {
    if (dist < 0.01) dist = 0.01;  // Avoid division by zero
    
    // Normalize collision vector
    double nx = dx / dist;  // Normal x
    double ny = dy / dist;  // Normal y
    
    // Relative velocity
    double dvx = c2->vx - c1->vx;
    double dvy = c2->vy - c1->vy;
    
    // Relative velocity in collision normal direction
    double dvn = dvx * nx + dvy * ny;
    
    // Don't collide if velocities are moving apart
    if (dvn >= 0) return;
    
    // Get masses (proportional to size - larger asteroids are "heavier")
    double m1 = c1->radius * c1->radius;  // Mass proportional to area
    double m2 = c2->radius * c2->radius;
    
    // Collision impulse (simplified elastic collision)
    // For equal elasticity, exchange velocity components
    double impulse = 2.0 * dvn / (m1 + m2);
    
    // Apply impulse to both comets
    c1->vx += impulse * m2 * nx;
    c1->vy += impulse * m2 * ny;
    
    c2->vx -= impulse * m1 * nx;
    c2->vy -= impulse * m1 * ny;
    
    // Separate comets to prevent overlap (overlap resolution)
    double overlap = min_dist - dist;
    double separate = (overlap / 2.0) + 0.01;  // Small buffer to prevent re-collision
    
    double ratio1 = m2 / (m1 + m2);  // How much c1 moves
    double ratio2 = m1 / (m1 + m2);  // How much c2 moves
    
    c1->x -= separate * ratio1 * nx;
    c1->y -= separate * ratio1 * ny;
    
    c2->x += separate * ratio2 * nx;
    c2->y += separate * ratio2 * ny;
}

bool comet_buster_check_bullet_comet(Bullet *b, Comet *c) {
    if (!b->active || !c->active) return false;
    
    double dx = b->x - c->x;
    double dy = b->y - c->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < (c->radius + 2.0);
}

bool comet_buster_check_missile_comet(Missile *m, Comet *c) {
    if (!m->active || !c->active) return false;
    
    double dx = m->x - c->x;
    double dy = m->y - c->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < (c->radius + 8.0);  // Missiles have larger collision radius
}

bool comet_buster_check_ship_comet(CometBusterGame *game, Comet *c) {
    if (!c->active) return false;
    
    double dx = game->ship_x - c->x;
    double dy = game->ship_y - c->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < (c->radius + 15.0);
}

bool comet_buster_check_bullet_enemy_ship(Bullet *b, EnemyShip *e) {
    if (!b->active || !e->active) return false;
    
    double dx = b->x - e->x;
    double dy = b->y - e->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 15.0;  // Enemy ship collision radius is 15 pixels
}

bool comet_buster_check_enemy_bullet_ship(CometBusterGame *game, Bullet *b) {
    if (!b->active) return false;
    
    double dx = game->ship_x - b->x;
    double dy = game->ship_y - b->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 15.0;  // Player ship collision radius
}

/**
 * Check if an enemy bullet hits another enemy ship
 * Returns true if bullet hit a ship (and handles the collision)
 * Returns -1 if hit the ship that fired the bullet (friendly fire - ignore)
 * Returns the index of the hit ship, or -1 if no hit or friendly fire
 */
int comet_buster_check_enemy_bullet_enemy_ship(CometBusterGame *game, Bullet *b) {
    if (!b->active) return -1;
    
    // Check collision with all enemy ships
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        
        if (!ship->active) continue;
        
        // ← KEY FIX: Don't hit the ship that fired this bullet (prevent self-damage)
        if (b->owner_ship_id == i) continue;
        
        double dx = ship->x - b->x;
        double dy = ship->y - b->y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Enemy ship collision radius is 15 pixels
        if (dist < 15.0) {
            return i;  // Return the ship index that was hit
        }
    }
    
    return -1;  // No hit
}

void comet_buster_destroy_comet(CometBusterGame *game, int comet_index, int width, int height, void *vis) {
    (void)width;
    (void)height;
    if (comet_index < 0 || comet_index >= game->comet_count) return;
    
    Comet *c = &game->comets[comet_index];
    if (!c->active) return;
    
    // Create explosion
    int particle_count = 15;
    if (c->size == COMET_MEGA) particle_count = 30;
    else if (c->size == COMET_LARGE) particle_count = 20;
    else if (c->size == COMET_SMALL) particle_count = 8;
    
    comet_buster_spawn_explosion(game, c->x, c->y, c->frequency_band, particle_count);
    
    // Play explosion sound - but NOT during splash screen
    if (vis && !game->splash_screen_active) {
        Visualizer *visualizer = (Visualizer *)vis;
#ifdef ExternalSound
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_explosion);
#endif
    }
    
    // Award points
    int points = 0;
    switch (c->size) {
        case COMET_SMALL: points = 50; break;
        case COMET_MEDIUM: points = 100; break;
        case COMET_LARGE: points = 200; break;
        case COMET_MEGA: points = 500; break;
        case COMET_SPECIAL: points = 500; break;
    }
    
    int score_add = (int)(points * game->score_multiplier);
    game->score += score_add;
    game->comets_destroyed++;
    game->consecutive_hits++;
    
    // Check for extra life bonus - every 100000 points
    int current_milestone = game->score / 100000;
    if (current_milestone > game->last_life_milestone) {
        game->ship_lives++;
        game->last_life_milestone = current_milestone;
        
        // Spawn floating text popup
        char text[32];
        snprintf(text, sizeof(text), "* +1 LIFE *");
        comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y - 30, text, 1.0, 1.0, 0.0);  // Yellow
    }
    
    // Increase multiplier
    if (game->consecutive_hits % 5 == 0) {
        double old_multiplier = game->score_multiplier;
        game->score_multiplier += 0.1;
        if (game->score_multiplier > 5.0) game->score_multiplier = 5.0;
        
        // Only show popup when crossing 0.5 increments (2.0, 2.5, 3.0, etc)
        if ((int)(game->score_multiplier * 2) != (int)(old_multiplier * 2)) {
            // Visual feedback for multiplier increase
            char mult_text[32];
            snprintf(mult_text, sizeof(mult_text), "Multiplier x%.1f", game->score_multiplier);
            comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y + 30, mult_text, 1.0, 1.0, 0.0);  // Yellow
            
            // Audio feedback for multiplier increase
#ifdef ExternalSound
            Visualizer *visualizer = (Visualizer *)vis;
            if (visualizer && visualizer->audio.sfx_energy) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_energy);
            }
#endif
        }
    }
    
    // Spawn child comets (at parent location, not at screen edge)
    if (c->size == COMET_LARGE) {
        for (int i = 0; i < 2; i++) {
            if (game->comet_count >= MAX_COMETS) break;
            
            int slot = game->comet_count;
            Comet *child = &game->comets[slot];
            memset(child, 0, sizeof(Comet));
            
            // Spawn at parent location
            child->x = c->x + (rand() % 20 - 10);  // Small offset
            child->y = c->y + (rand() % 20 - 10);
            
            // Scatter in random direction
            double angle = (rand() % 360) * (M_PI / 180.0);
            double speed = 100.0 + (rand() % 100);
            child->vx = cos(angle) * speed;
            child->vy = sin(angle) * speed;
            
            // Set as medium asteroid
            child->size = COMET_MEDIUM;
            child->radius = 20;
            child->frequency_band = c->frequency_band;
            child->rotation = 0;
            child->rotation_speed = 50 + (rand() % 200);
            child->active = true;
            child->health = 1;
            child->base_angle = (rand() % 360) * (M_PI / 180.0);
            
            // Color
            comet_buster_get_frequency_color(c->frequency_band, 
                                             &child->color[0], 
                                             &child->color[1], 
                                             &child->color[2]);
            
            game->comet_count++;
        }
    } else if (c->size == COMET_MEDIUM) {
        for (int i = 0; i < 2; i++) {
            if (game->comet_count >= MAX_COMETS) break;
            
            int slot = game->comet_count;
            Comet *child = &game->comets[slot];
            memset(child, 0, sizeof(Comet));
            
            // Spawn at parent location
            child->x = c->x + (rand() % 20 - 10);  // Small offset
            child->y = c->y + (rand() % 20 - 10);
            
            // Scatter in random direction
            double angle = (rand() % 360) * (M_PI / 180.0);
            double speed = 150.0 + (rand() % 100);
            child->vx = cos(angle) * speed;
            child->vy = sin(angle) * speed;
            
            // Set as small asteroid
            child->size = COMET_SMALL;
            child->radius = 10;
            child->frequency_band = c->frequency_band;
            child->rotation = 0;
            child->rotation_speed = 50 + (rand() % 200);
            child->active = true;
            child->health = 1;
            child->base_angle = (rand() % 360) * (M_PI / 180.0);
            
            // Color
            comet_buster_get_frequency_color(c->frequency_band, 
                                             &child->color[0], 
                                             &child->color[1], 
                                             &child->color[2]);
            
            game->comet_count++;
        }
    } else if (c->size == COMET_MEGA) {
        // Mega comets break into 3 large comets
        for (int i = 0; i < 3; i++) {
            if (game->comet_count >= MAX_COMETS) break;
            
            int slot = game->comet_count;
            Comet *child = &game->comets[slot];
            memset(child, 0, sizeof(Comet));
            
            // Spawn at parent location
            child->x = c->x + (rand() % 30 - 15);  // Slightly larger offset
            child->y = c->y + (rand() % 30 - 15);
            
            // Scatter in random direction
            double angle = (rand() % 360) * (M_PI / 180.0);
            double speed = 80.0 + (rand() % 80);
            child->vx = cos(angle) * speed;
            child->vy = sin(angle) * speed;
            
            // Set as large asteroid
            child->size = COMET_LARGE;
            child->radius = 30;
            child->frequency_band = c->frequency_band;
            child->rotation = 0;
            child->rotation_speed = 50 + (rand() % 200);
            child->active = true;
            child->health = 1;
            child->base_angle = (rand() % 360) * (M_PI / 180.0);
            
            // Color
            comet_buster_get_frequency_color(c->frequency_band, 
                                             &child->color[0], 
                                             &child->color[1], 
                                             &child->color[2]);
            
            game->comet_count++;
        }
    }
    // Swap with last and remove
    if (comet_index != game->comet_count - 1) {
        game->comets[comet_index] = game->comets[game->comet_count - 1];
    }
    game->comet_count--;
}

void comet_buster_destroy_enemy_ship(CometBusterGame *game, int ship_index, int width, int height, void *vis) {
    (void)width;
    (void)height;
    if (ship_index < 0 || ship_index >= game->enemy_ship_count) return;
    
    EnemyShip *ship = &game->enemy_ships[ship_index];
    if (!ship->active) return;
    
    // Create explosion
    comet_buster_spawn_explosion(game, ship->x, ship->y, 1, 12);  // Mid-frequency explosion
    
    // Play explosion sound - but NOT during splash screen
    if (vis && !game->splash_screen_active) {
        Visualizer *visualizer = (Visualizer *)vis;
#ifdef ExternalSound
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_explosion);
#endif
    }
    
    // Award points
    int points = 300;  // Enemy ships worth 300 points
    int score_add = (int)(points * game->score_multiplier);
    game->score += score_add;
    game->consecutive_hits++;
    
    // Floating text
    char text[32];
    snprintf(text, sizeof(text), "+%d", score_add);
    comet_buster_spawn_floating_text(game, ship->x, ship->y, text, 0.0, 1.0, 0.0);  // Green
    
    // Increase multiplier
    if (game->consecutive_hits % 5 == 0) {
        double old_multiplier = game->score_multiplier;
        game->score_multiplier += 0.1;
        if (game->score_multiplier > 5.0) game->score_multiplier = 5.0;
        
        // Only show popup when crossing 0.5 increments (2.0, 2.5, 3.0, etc)
        if ((int)(game->score_multiplier * 2) != (int)(old_multiplier * 2)) {
            // Visual feedback for multiplier increase
            char mult_text[32];
            snprintf(mult_text, sizeof(mult_text), "Multiplier x%.1f", game->score_multiplier);
            comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y + 30, mult_text, 1.0, 1.0, 0.0);  // Yellow
            
            // Audio feedback for multiplier increase
#ifdef ExternalSound
            Visualizer *visualizer = (Visualizer *)vis;
            if (visualizer && visualizer->audio.sfx_energy) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_energy);
            }
#endif
        }
    }
    
    // Weapon/Pickup drop chances (difficulty-based)
    int drop_roll = rand() % 100;
    int missile_chance, shield_chance, bomb_chance;
    
    if (game->difficulty == 0) {
        // Easy: 20% missile, 20% shield, 60% nothing
        missile_chance = 20;
        shield_chance = 40;
        bomb_chance = 10;
    } else if (game->difficulty == 2) {
        // Hard: 5% missile, 5% shield, 90% nothing
        missile_chance = 5;
        shield_chance = 10;
        bomb_chance = 5;
    } else {
        // Medium: 10% missile, 10% shield, 80% nothing
        missile_chance = 10;
        shield_chance = 20;
        bomb_chance = 5;
    }

    if (drop_roll < missile_chance) {
        // Chance to spawn missile pickup
        comet_buster_spawn_missile_pickup(game, ship->x, ship->y);
    } else if (drop_roll < (missile_chance + bomb_chance)) {
        // Chance to spawn bomb pickup
        comet_buster_spawn_bomb_pickup(game, ship->x, ship->y);
    } else if (drop_roll < (missile_chance + bomb_chance + shield_chance)) {
        // Chance to spawn shield canister
        comet_buster_spawn_canister(game, ship->x, ship->y);
    }
    // Remainder: drop nothing
    
    // Swap with last and remove
    if (ship_index != game->enemy_ship_count - 1) {
        game->enemy_ships[ship_index] = game->enemy_ships[game->enemy_ship_count - 1];
    }
    game->enemy_ship_count--;
}

void comet_buster_destroy_boss(CometBusterGame *game, int width, int height, void *vis) {
    (void)width;
    (void)height;
    
    if (!game || !game->boss_active) return;
    
    BossShip *boss = &game->boss;
    
    // Create large explosion
    comet_buster_spawn_explosion(game, boss->x, boss->y, 1, 60);  // HUGE explosion
    
    // Create radial neon burst explosion effect
    const char *boss_type = "death_star";  // Default
    if (game->spawn_queen.is_spawn_queen) {
        boss_type = "spawn_queen";
    } else if (game->current_wave % 30 == 15) {
        boss_type = "void_nexus";
    } else if (game->current_wave % 30 == 20) {
        boss_type = "harbinger";
    } else if (game->current_wave % 30 == 25) {
        boss_type = "star_vortex";
    } else if (game->current_wave % 30 == 0) {
        boss_type = "singularity";
    }
    boss_explosion_create(&game->boss_explosion_effect, boss->x, boss->y, boss_type);
    
    // Play explosion sound - but NOT during splash screen
    if (vis && !game->splash_screen_active) {
        Visualizer *visualizer = (Visualizer *)vis;
#ifdef ExternalSound
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_explosion);
#endif
    }
    
    // Award MASSIVE points
    int points = 5000;  // Boss worth 5000 points!
    int score_add = (int)(points * game->score_multiplier);
    game->score += score_add;
    game->consecutive_hits += 10;  // Big hit bonus
    
    // Floating text - BIG text
    char text[64];
    snprintf(text, sizeof(text), "BOSS DESTROYED! +%d", score_add);
    comet_buster_spawn_floating_text(game, boss->x, boss->y, text, 1.0, 1.0, 0.0);  // Yellow
    
    // Increase multiplier significantly
    game->score_multiplier += 1.0;
    if (game->score_multiplier > 5.0) game->score_multiplier = 5.0;
    
    // MAJOR visual feedback for boss multiplier increase - always show for boss kills
    char mult_text[64];
    snprintf(mult_text, sizeof(mult_text), "*** Multiplier x%.1f ***", game->score_multiplier);
    comet_buster_spawn_floating_text(game, boss->x, boss->y - 50, mult_text, 1.0, 0.8, 0.0);  // Gold/Orange
    
    // Audio feedback for boss multiplier increase (louder/more dramatic)
#ifdef ExternalSound
    Visualizer *visualizer = (Visualizer *)vis;
    if (visualizer && visualizer->audio.sfx_energy) {
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_energy);
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_energy);  // Play twice for impact
    }
#endif
    
    boss->active = false;
    game->boss_active = false;
    
    // Reset boss.active flag for future boss waves
    game->boss.active = false;
    
    // Set wave complete timer for 2 second countdown before next wave
    // Boss is dead, so wave will progress after countdown regardless of remaining comets
    game->wave_complete_timer = 2.0;
}

void comet_buster_on_ship_hit(CometBusterGame *game, Visualizer *visualizer) {
    if (game->invulnerability_time > 0) return;
    
    // Play hit sound
#ifdef ExternalSound
    if (visualizer) {
        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
    }
#endif
    
    // Priority 1: Try to use 80% energy to absorb the hit
    if (game->energy_amount >= 80.0) {
        game->energy_amount -= 80.0;
        
        // Floating text for energy absorption
        comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y - 30, 
                                         "ENERGY USED", 1.0, 1.0, 0.0);  // Yellow
        
        // Minor invulnerability for energy hit
        game->invulnerability_time = 0.5;
        return;
    }
    
    // If energy is less than 80%, it drains to zero (but still allows shield check)
    if (game->energy_amount > 0) {
        game->energy_amount = 0;
        
        // Floating text for energy drain
        comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y - 30, 
                                         "ENERGY DRAINED", 1.0, 0.5, 0.0);  // Orange
        
        // Don't return here - continue to shield check!
    }
    
    // Priority 2: Use shield if available (either after energy drain or if energy was already 0)
    if (game->shield_health > 0) {
        game->shield_health--;
        game->shield_regen_timer = 0;  // Reset regen timer
        
        // Track impact angle for visual effect (angle from ship to source of hit)
        // We don't have exact hit source, so just use a random direction
        game->shield_impact_angle = (rand() % 360) * (M_PI / 180.0);
        game->shield_impact_timer = 0.2;  // Flash for 0.2 seconds
        
        // Floating text for shield hit
        comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y - 30, 
                                         "SHIELD HIT", 0.0, 1.0, 1.0);  // Cyan
        
        // Minor invulnerability for shield hit
        game->invulnerability_time = 0.5;
        return;
    }
    
    // Priority 3: If energy is zero and shield is down, take actual damage (lose life)
    game->ship_lives--;
    game->consecutive_hits = 0;
    game->score_multiplier = 1.0;
    game->shield_regen_timer = 0;  // Reset shield regen after taking life damage
    
    // Reset shield to full when taking life damage (amount depends on difficulty)
    if (game->difficulty == 0) {
        // Easy: restore 6 shield on new life
        game->shield_health = 6;
        game->max_shield_health = 6;
    } else if (game->difficulty == 2) {
        // Hard: restore 1 shield on new life
        game->shield_health = 1;
        game->max_shield_health = 1;
    } else {
        // Medium: restore 3 shield on new life
        game->shield_health = 3;
        game->max_shield_health = 3;
    }
    game->shield_impact_timer = 0;
    
    if (game->ship_lives <= 0) {
        game->game_over = true;
        game->game_over_timer = 3.0;
        
        // Play game over sound effect
        #ifdef ExternalSound
        if (visualizer && visualizer->audio.sfx_game_over) {
            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_game_over);
        }
        #endif
        
        // Don't add high score here - let the GUI dialog handle player name entry
        // The high score will be added when player submits their name in the dialog
    } else {
        // Move ship to center (like classic Asteroids) - resolution aware
        if (visualizer && visualizer->width > 0 && visualizer->height > 0) {
            game->ship_x = visualizer->width / 2.0;
            game->ship_y = visualizer->height / 2.0;
        } else {
            game->ship_x = 400.0;  // Fallback
            game->ship_y = 300.0;
        }
        game->ship_vx = 0;
        game->ship_vy = 0;
        game->ship_speed = 0;
        
        // Invulnerability period while finding safe spot
        game->invulnerability_time = 3.0;
    }
}

// ============================================================================
// PROVOKE BLUE SHIPS - Convert patrol to aggressive when hit
// ============================================================================

bool comet_buster_hit_enemy_ship_provoke(CometBusterGame *game, int ship_index) {
    if (!game || ship_index < 0 || ship_index >= game->enemy_ship_count) {
        return false;
    }
    
    EnemyShip *ship = &game->enemy_ships[ship_index];
    
    // On easy difficulty, blue ships don't get provoked
    if (game->difficulty == 0) {
        return false;  // No provocation on easy
    }
    
    // If this is a patrol (blue) ship, convert it to aggressive
    if (ship->ship_type == 0) {
        ship->ship_type = 1;  // Change to aggressive ship (red)
        ship->max_shield_health = 3;  // Boost its shield
        if (ship->shield_health < 3) {
            ship->shield_health = 3;  // Restore full shield
        }
        ship->shoot_cooldown = 0.0;  // Immediately start shooting
        
        // Spawn visual indicator that ship became angry
        char anger_text[64];
        sprintf(anger_text, "PROVOKED!");
        comet_buster_spawn_floating_text(game, ship->x, ship->y, anger_text, 1.0, 0.2, 0.2);
        
        return true;  // Successfully provoked
    }
    
    return false;  // Already aggressive or different type
}

// ============================================================================
// CANISTER COLLISION - Check if ship collides with canister
// ============================================================================

bool comet_buster_check_ship_canister(CometBusterGame *game, Canister *c) {
    if (!c->active) return false;
    
    double dx = game->ship_x - c->x;
    double dy = game->ship_y - c->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 20.0;  // Canister collision radius is 20 pixels
}

// ============================================================================
// MISSILE PICKUP COLLISION - Check if ship collides with missile pickup
// ============================================================================

bool comet_buster_check_ship_missile_pickup(CometBusterGame *game, MissilePickup *p) {
    if (!p->active) return false;
    
    double dx = game->ship_x - p->x;
    double dy = game->ship_y - p->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 20.0;  // Missile pickup collision radius is 20 pixels
}

// ============================================================================
// UFO-COMET COLLISION - Check if UFO collides with asteroid/comet
// ============================================================================

bool comet_buster_check_ufo_comet(UFO *u, Comet *c) {
    if (!u->active || !c->active) return false;
    
    double dx = u->x - c->x;
    double dy = u->y - c->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    // UFO collision radius is 25 pixels (matches UFO size)
    return dist < (c->radius + 25.0);
}

// ============================================================================
// SHIP-UFO COLLISION - Check if player ship hits UFO
// ============================================================================

bool comet_buster_check_ship_ufo(CometBusterGame *game, UFO *u) {
    if (!u->active) return false;
    
    double dx = game->ship_x - u->x;
    double dy = game->ship_y - u->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < (25.0 + 15.0);  // UFO radius (25) + Ship radius (15)
}

// ============================================================================
// ENEMY BULLET-UFO COLLISION - Check if enemy bullets hit UFO
// ============================================================================

bool comet_buster_check_enemy_bullet_ufo(Bullet *b, UFO *u) {
    if (!b->active || !u->active) return false;
    
    double dx = u->x - b->x;
    double dy = u->y - b->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    return dist < 25.0;  // UFO collision radius
}
