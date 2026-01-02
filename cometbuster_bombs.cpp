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

// ============================================================================
// BOMB PICKUP SPAWNING AND PICKUP SYSTEM
// ============================================================================

void comet_buster_spawn_bomb_pickup(CometBusterGame *game, double x, double y) {
    if (!game) return;
    
    if (game->bomb_pickup_count >= MAX_BOMB_PICKUPS) {
        return;
    }
    
    int slot = game->bomb_pickup_count;
    BombPickup *pickup = &game->bomb_pickups[slot];
    
    memset(pickup, 0, sizeof(BombPickup));
    
    // Position bomb pickup at the given location
    pickup->x = x;
    pickup->y = y;
    
    // Give it a small drift velocity for visual interest
    pickup->vx = (rand() % 100 - 50) * 0.5;  // Random drift
    pickup->vy = (rand() % 100 - 50) * 0.5;
    
    // Bomb pickup lasts 10 seconds (same as missile pickups)
    pickup->lifetime = 10.0;
    pickup->max_lifetime = 10.0;
    
    // Spin animation
    pickup->rotation = 0;
    pickup->rotation_speed = 200.0 + (rand() % 160);  // 200-360 degrees per second
    
    pickup->bomb_count = 1;  // Each pickup gives 1 bomb
    pickup->active = true;
    
    game->bomb_pickup_count++;
}

void comet_buster_update_bomb_pickups(CometBusterGame *game, double dt) {
    if (!game) return;
    
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        BombPickup *p = &game->bomb_pickups[i];
        if (!p->active) continue;
        
        // Update lifetime
        p->lifetime -= dt;
        if (p->lifetime <= 0) {
            p->active = false;
            continue;
        }
        
        // Update position (slow drift)
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        
        // Update rotation
        p->rotation += p->rotation_speed * dt;
        if (p->rotation >= 360.0) {
            p->rotation -= 360.0;
        }
        
        // Apply wrap-around
        comet_buster_wrap_position(&p->x, &p->y, 1920, 1080);
    }
    
    // Remove inactive pickups
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        if (!game->bomb_pickups[i].active) {
            if (i != game->bomb_pickup_count - 1) {
                game->bomb_pickups[i] = game->bomb_pickups[game->bomb_pickup_count - 1];
            }
            game->bomb_pickup_count--;
            i--;
        }
    }
}

bool comet_buster_check_ship_bomb_pickup(CometBusterGame *game, BombPickup *p) {
    if (!game || !p || !p->active) return false;
    
    double dist = comet_buster_distance(game->ship_x, game->ship_y, p->x, p->y);
    if (dist < 25.0) {  // Pickup radius
        p->active = false;
        game->bomb_ammo += p->bomb_count;
        
        // Play pickup sound
#ifdef ExternalSound
        // Could add a specific bomb pickup sound here
#endif
        
        // Visual feedback
        char text[32];
        snprintf(text, sizeof(text), "+%d Bomb", p->bomb_count);
        comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y - 30, text, 
                                        1.0, 0.8, 0.0);  // Gold color
        
        return true;
    }
    
    return false;
}

void draw_comet_buster_bomb_pickups(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !cr) return;
    
    cairo_set_line_width(cr, 2.0);
    
    for (int i = 0; i < game->bomb_pickup_count; i++) {
        BombPickup *p = &game->bomb_pickups[i];
        if (!p->active) continue;
        
        // Convert rotation to radians
        double rotation_rad = p->rotation * (M_PI / 180.0);
        
        // Draw bomb icon - a circle with a larger outline to look like a bomb
        // Outer circle (bomb body)
        cairo_save(cr);
        cairo_translate(cr, p->x, p->y);
        cairo_rotate(cr, rotation_rad);
        
        // Color: orange/yellow for bomb
        cairo_set_source_rgb(cr, 1.0, 0.7, 0.0);
        
        // Main circle
        cairo_arc(cr, 0, 0, 12.0, 0, 2 * M_PI);
        cairo_fill(cr);
        
        // Fuse sticking out
        cairo_set_source_rgb(cr, 0.8, 0.4, 0.2);
        cairo_set_line_width(cr, 1.5);
        cairo_move_to(cr, 0, -12);
        cairo_line_to(cr, 0, -22);
        cairo_stroke(cr);
        
        // Small spark at fuse tip
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_arc(cr, 0, -22, 2.0, 0, 2 * M_PI);
        cairo_fill(cr);
        
        cairo_restore(cr);
        
        // Draw pickup radius indicator (faint circle)
        cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, 0.2);
        cairo_set_line_width(cr, 1.0);
        cairo_arc(cr, p->x, p->y, 25.0, 0, 2 * M_PI);
        cairo_stroke(cr);
    }
}

// ============================================================================
// BOMB DROPPING AND MANAGEMENT
// ============================================================================

void comet_buster_drop_bomb(CometBusterGame *game, int width, int height, void *vis) {
    if (!game || game->bomb_ammo <= 0) return;
    
    // Check cooldown (can't drop bombs faster than once per 0.2 seconds)
    if (game->bomb_drop_cooldown > 0) return;
    
    // Create a new bomb at player ship location
    if (game->bomb_count >= MAX_BOMBS) {
        return;  // Can't add more bombs
    }
    
    int slot = game->bomb_count;
    Bomb *bomb = &game->bombs[slot];
    
    memset(bomb, 0, sizeof(Bomb));
    
    // Place bomb at ship position with slight offset
    bomb->x = game->ship_x;
    bomb->y = game->ship_y;
    bomb->vx = 0;
    bomb->vy = 0;
    
    // Countdown for 3 seconds before explosion
    bomb->lifetime = BOMB_COUNTDOWN_TIME;
    bomb->max_lifetime = BOMB_COUNTDOWN_TIME;
    
    // Visual rotation
    bomb->rotation = 0;
    bomb->rotation_speed = 200.0;
    
    bomb->active = true;
    bomb->detonated = false;
    bomb->damage_applied = false;
    bomb->wave_radius = 0;
    bomb->wave_max_radius = BOMB_WAVE_MAX_RADIUS;
    
    game->bomb_count++;
    game->bomb_ammo--;
    
    // Set cooldown
    game->bomb_drop_cooldown = 0.2;
    
    // Play sound
#ifdef ExternalSound
    Visualizer *visualizer = (Visualizer *)vis;
    if (visualizer && visualizer->audio.sfx_explosion) {
        // Could use a beep or different sound for bomb placement
    }
#endif
    
    // Visual feedback
    comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y + 40, 
                                    "BOMB!", 1.0, 1.0, 1.0);  // White
}

void comet_buster_update_bombs(CometBusterGame *game, double dt, int width, int height, void *vis) {
    if (!game) return;
    
    // Update bomb drop cooldown
    if (game->bomb_drop_cooldown > 0) {
        game->bomb_drop_cooldown -= dt;
    }
    
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *bomb = &game->bombs[i];
        if (!bomb->active) continue;
        
        // Update position (if moving)
        bomb->x += bomb->vx * dt;
        bomb->y += bomb->vy * dt;
        
        // Update rotation
        bomb->rotation += bomb->rotation_speed * dt;
        if (bomb->rotation >= 360.0) {
            bomb->rotation -= 360.0;
        }
        
        if (!bomb->detonated) {
            // Countdown to detonation
            bomb->lifetime -= dt;
            
            if (bomb->lifetime <= 0) {
                // DETONATION!
                bomb->detonated = true;
                bomb->wave_radius = 0;
                bomb->lifetime = 0.5;  // Wave lasts 0.5 seconds
                
                // Play explosion sound
#ifdef ExternalSound
                Visualizer *visualizer = (Visualizer *)vis;
                if (visualizer && visualizer->audio.sfx_explosion) {
                    audio_play_sound(&visualizer->audio, visualizer->audio.sfx_explosion);
                }
#endif
                
                // Create particles at bomb location
                comet_buster_spawn_explosion(game, bomb->x, bomb->y, 1, 20);
            }
        } else {
            // Expanding wave after detonation
            bomb->wave_radius += BOMB_WAVE_SPEED * dt;
            bomb->lifetime -= dt;
            
            if (bomb->lifetime <= 0) {
                // Wave is done, deactivate bomb
                bomb->active = false;
            }
        }
    }
    
    // Collision detection for detonating bombs
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *bomb = &game->bombs[i];
        if (!bomb->active || !bomb->detonated || bomb->damage_applied) continue;  // Skip if damage already applied
        
        // Apply damage only once when bomb first detonates
        bool damage_this_frame = false;
        
        // Check bomb wave vs comets
        for (int j = 0; j < game->comet_count; j++) {
            Comet *comet = &game->comets[j];
            if (!comet->active) continue;
            
            if (comet_buster_check_bomb_wave_comet(bomb, comet)) {
                // Damage comet
                comet->health -= BOMB_WAVE_DAMAGE;
                if (comet->health <= 0) {
                    comet_buster_destroy_comet(game, j, width, height, NULL);
                    j--;
                }
                damage_this_frame = true;
            }
        }
        
        // Check bomb wave vs enemy ships
        for (int j = 0; j < game->enemy_ship_count; j++) {
            EnemyShip *ship = &game->enemy_ships[j];
            if (!ship->active) continue;
            
            if (comet_buster_check_bomb_wave_enemy_ship(bomb, ship)) {
                // Damage enemy ship
                ship->health -= BOMB_WAVE_DAMAGE;
                if (ship->health <= 0) {
                    comet_buster_destroy_enemy_ship(game, j, width, height, NULL);
                    j--;
                }
                damage_this_frame = true;
            }
        }
        
        // Check bomb wave vs UFOs
        for (int j = 0; j < game->ufo_count; j++) {
            UFO *ufo = &game->ufos[j];
            if (!ufo->active) continue;
            
            if (comet_buster_check_bomb_wave_ufo(bomb, ufo)) {
                // Damage UFO
                ufo->health -= BOMB_WAVE_DAMAGE;
                if (ufo->health <= 0) {
                    comet_buster_destroy_ufo(game, j, width, height, NULL);
                    j--;
                }
                damage_this_frame = true;
            }
        }
        
        // Check bomb wave vs boss
        if (game->boss_active && game->boss.active) {
            if (comet_buster_check_bomb_wave_boss(bomb, &game->boss)) {
                // Damage boss: shield first, then health
                int damage_remaining = BOMB_WAVE_DAMAGE;
                
                // Apply to shield first
                if (game->boss.shield_health > 0) {
                    if (game->boss.shield_health >= damage_remaining) {
                        game->boss.shield_health -= damage_remaining;
                        damage_remaining = 0;
                    } else {
                        damage_remaining -= game->boss.shield_health;
                        game->boss.shield_health = 0;
                    }
                }
                
                // Apply remaining damage to health
                if (damage_remaining > 0) {
                    game->boss.health -= damage_remaining;
                }
                
                // Check if boss is dead
                if (game->boss.health <= 0) {
                    game->boss.active = false;
                    game->boss_active = false;
                    // Boss will be cleaned up by normal game logic
                }
                
                damage_this_frame = true;
            }
        }
        
        // Check bomb wave vs spawn queen boss
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            // Simple distance check for spawn queen
            double dist = comet_buster_distance(bomb->x, bomb->y, game->spawn_queen.x, game->spawn_queen.y);
            if (dist <= bomb->wave_radius + 60.0) {  // Spawn queen is larger
                // Damage spawn queen: shield first, then health
                int damage_remaining = BOMB_WAVE_DAMAGE;
                
                // Apply to shield first
                if (game->spawn_queen.shield_health > 0) {
                    if (game->spawn_queen.shield_health >= damage_remaining) {
                        game->spawn_queen.shield_health -= damage_remaining;
                        damage_remaining = 0;
                    } else {
                        damage_remaining -= game->spawn_queen.shield_health;
                        game->spawn_queen.shield_health = 0;
                    }
                }
                
                // Apply remaining damage to health
                if (damage_remaining > 0) {
                    game->spawn_queen.health -= damage_remaining;
                }
                
                // Check if spawn queen is dead
                if (game->spawn_queen.health <= 0) {
                    game->spawn_queen.active = false;
                    // Spawn queen will be cleaned up by normal game logic
                }
                
                damage_this_frame = true;
            }
        }
        
        // Check bomb wave vs enemy bullets (destroy them)
        for (int j = 0; j < game->enemy_bullet_count; j++) {
            Bullet *bullet = &game->enemy_bullets[j];
            if (!bullet->active) continue;
            
            if (comet_buster_check_bomb_wave_bullet(bomb, bullet)) {
                bullet->active = false;
            }
        }
        
        // Mark damage as applied if we hit anything
        if (damage_this_frame) {
            bomb->damage_applied = true;
        }
    }
    
    // Remove inactive bombs
    for (int i = 0; i < game->bomb_count; i++) {
        if (!game->bombs[i].active) {
            if (i != game->bomb_count - 1) {
                game->bombs[i] = game->bombs[game->bomb_count - 1];
            }
            game->bomb_count--;
            i--;
        }
    }
}

void draw_comet_buster_bombs(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !cr) return;
    
    for (int i = 0; i < game->bomb_count; i++) {
        Bomb *bomb = &game->bombs[i];
        if (!bomb->active) continue;
        
        cairo_save(cr);
        cairo_translate(cr, bomb->x, bomb->y);
        
        if (!bomb->detonated) {
            // Draw inactive bomb - rotating animation
            double rotation_rad = bomb->rotation * (M_PI / 180.0);
            cairo_rotate(cr, rotation_rad);
            
            // Pulsing effect based on time left
            double pulse = 1.0 - (bomb->lifetime / bomb->max_lifetime) * 0.3;  // Pulses as countdown happens
            
            // Main bomb body - orange circle
            cairo_set_source_rgb(cr, 1.0 * pulse, 0.6, 0.0);
            cairo_arc(cr, 0, 0, 15.0, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Outline
            cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);
            cairo_set_line_width(cr, 2.0);
            cairo_arc(cr, 0, 0, 15.0, 0, 2 * M_PI);
            cairo_stroke(cr);
            
            // Fuse
            cairo_set_source_rgb(cr, 0.7, 0.3, 0.1);
            cairo_set_line_width(cr, 2.0);
            cairo_move_to(cr, 0, -15);
            cairo_line_to(cr, 0, -28);
            cairo_stroke(cr);
            
            // Spark at fuse
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.2);
            cairo_arc(cr, 0, -28, 3.0, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Countdown text
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_set_font_size(cr, 12.0);
            int countdown = (int)(bomb->lifetime + 1);
            if (countdown < 1) countdown = 1;
            
            char text[8];
            snprintf(text, sizeof(text), "%d", countdown);
            
            cairo_text_extents_t extents;
            cairo_text_extents(cr, text, &extents);
            cairo_move_to(cr, -extents.width / 2, extents.height / 2);
            cairo_show_text(cr, text);
            
        } else {
            // Draw explosion wave
            // Expanding circle with pulsing opacity
            double wave_progress = bomb->wave_radius / bomb->wave_max_radius;
            double opacity = 1.0 - wave_progress;  // Fades out as it expands
            
            // Draw wave circle
            cairo_set_source_rgba(cr, 1.0, 0.7, 0.0, opacity * 0.8);  // Orange
            cairo_set_line_width(cr, 3.0 * (1.0 - wave_progress));  // Gets thinner
            cairo_arc(cr, 0, 0, bomb->wave_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
            
            // Inner shock ring
            if (bomb->wave_radius > 20) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 0.5, opacity * 0.6);
                cairo_set_line_width(cr, 2.0);
                cairo_arc(cr, 0, 0, bomb->wave_radius - 20, 0, 2 * M_PI);
                cairo_stroke(cr);
            }
            
            // Fill inner area with gradient effect
            cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, opacity * 0.15);
            cairo_arc(cr, 0, 0, bomb->wave_radius, 0, 2 * M_PI);
            cairo_fill(cr);
        }
        
        cairo_restore(cr);
    }
}

// ============================================================================
// BOMB COLLISION DETECTION
// ============================================================================

bool comet_buster_check_bomb_wave_comet(Bomb *bomb, Comet *comet) {
    if (!bomb || !comet || !bomb->detonated) return false;
    
    double dist = comet_buster_distance(bomb->x, bomb->y, comet->x, comet->y);
    
    // Check if comet is within the current wave radius
    if (dist <= bomb->wave_radius && dist >= bomb->wave_radius - 30.0) {
        return true;
    }
    
    // Also check if comet center is within wave
    if (dist <= bomb->wave_radius) {
        return true;
    }
    
    return false;
}

bool comet_buster_check_bomb_wave_enemy_ship(Bomb *bomb, EnemyShip *ship) {
    if (!bomb || !ship || !bomb->detonated) return false;
    
    double dist = comet_buster_distance(bomb->x, bomb->y, ship->x, ship->y);
    
    // Check if ship is within wave (accounting for ship size)
    if (dist <= bomb->wave_radius + 20.0) {
        return true;
    }
    
    return false;
}

bool comet_buster_check_bomb_wave_ufo(Bomb *bomb, UFO *ufo) {
    if (!bomb || !ufo || !bomb->detonated) return false;
    
    double dist = comet_buster_distance(bomb->x, bomb->y, ufo->x, ufo->y);
    
    // Check if UFO is within wave (accounting for UFO size)
    if (dist <= bomb->wave_radius + 25.0) {
        return true;
    }
    
    return false;
}

bool comet_buster_check_bomb_wave_boss(Bomb *bomb, BossShip *boss) {
    if (!bomb || !boss || !bomb->detonated) return false;
    
    double dist = comet_buster_distance(bomb->x, bomb->y, boss->x, boss->y);
    
    // Check if boss is within wave (accounting for boss size)
    if (dist <= bomb->wave_radius + 50.0) {
        return true;
    }
    
    return false;
}

bool comet_buster_check_bomb_wave_bullet(Bomb *bomb, Bullet *bullet) {
    if (!bomb || !bullet || !bomb->detonated) return false;
    
    double dist = comet_buster_distance(bomb->x, bomb->y, bullet->x, bullet->y);
    
    // Check if bullet is within wave
    if (dist <= bomb->wave_radius) {
        return true;
    }
    
    return false;
}
