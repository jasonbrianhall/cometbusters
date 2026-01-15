#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"
#include "cometbuster_splashscreen.h"

void comet_buster_init_splash_screen(CometBusterGame *game, int width, int height) {
    if (!game) return;
    
    game->splash_screen_active = true;
    game->splash_timer = 0.0;
    game->enemy_ship_spawn_timer = 0.5;  // Spawn ships every 0.5 seconds during splash
    
    // Directly spawn lots of comets for impressive splash screen visuals
    comet_buster_spawn_random_comets(game, 32, width, height);
    
    // Spawn a boss to make the splash screen look dramatic and dynamic
    //comet_buster_spawn_boss(game, width, height);
    
    // Spawn MORE initial enemy ships for action-packed intro (6 instead of 3)
    for (int i = 0; i < 6; i++) {
        comet_buster_spawn_enemy_ship(game, width, height);
    }
    
    // 50% chance to spawn 1-2 Juggernauts at start for dramatic intro splash
    if ((rand() % 100) < 50) {
        int num_juggernauts = 1 + (rand() % 2);  // 1 or 2 juggernauts
        for (int j = 0; j < num_juggernauts; j++) {
            int random_edge = rand() % 8;  // Edges 0-7
            double juggernaut_speed = 70.0;
            comet_buster_spawn_enemy_ship_internal(game, width, height, 5, random_edge, juggernaut_speed, 0, 1);
        }
        fprintf(stdout, "[SPLASH] %d JUGGERNAUT(S) SPAWNED for intro drama!\n", num_juggernauts);
    }
    
    fprintf(stdout, "[SPLASH] Splash screen initialized:\n");
    fprintf(stdout, "  - %d comets\n", game->comet_count);
    fprintf(stdout, "  - %d enemy ships\n", game->enemy_ship_count);
}

// Update splash screen - now includes enemy ship and boss animation
void comet_buster_update_splash_screen(CometBusterGame *game, double dt, int width, int height, Visualizer *visualizer) {
    if (!game || !game->splash_screen_active) return;
    
    game->splash_timer += dt;
    
    // Use actual game physics engine for comets
    comet_buster_update_comets(game, dt, width, height);
    
    // Also update enemy ships so they move and animate on the splash screen
    comet_buster_update_enemy_ships(game, dt, width, height, visualizer);
    
    // Update enemy bullets fired by ships
    comet_buster_update_enemy_bullets(game, dt, width, height, visualizer);
    
    // Update boss if active
    if (game->boss_active) {
        comet_buster_update_boss(game, dt, width, height);
    }
    
    // Update particles
    comet_buster_update_particles(game, dt);
    
    // Continuously spawn new ships during splash screen for action-packed intro
    game->enemy_ship_spawn_timer -= dt;
    if (game->enemy_ship_spawn_timer <= 0) {
        // Spawn a new ship every 0.5 seconds
        game->enemy_ship_spawn_timer = 0.5;
        
        // 25% chance to spawn a Juggernaut instead of regular ship
        if ((rand() % 100) < 25 && game->enemy_ship_count < MAX_ENEMY_SHIPS) {
            int random_edge = rand() % 8;
            double juggernaut_speed = 70.0;
            comet_buster_spawn_enemy_ship_internal(game, width, height, 5, random_edge, juggernaut_speed, 0, 1);
            fprintf(stdout, "[SPLASH] JUGGERNAUT spawned!\n");
        } else if (game->enemy_ship_count < MAX_ENEMY_SHIPS) {
            // Regular ship spawn
            comet_buster_spawn_enemy_ship(game, width, height);
        }
    }
    
    // Check enemy bullet - enemy ship collisions
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        if (!game->enemy_bullets[i].active) continue;
        
        Bullet *bullet = &game->enemy_bullets[i];
        int hit_ship = comet_buster_check_enemy_bullet_enemy_ship(game, bullet);
        
        if (hit_ship >= 0) {
            // Enemy bullet hit another enemy ship
            bullet->active = false;
            
            EnemyShip *ship = &game->enemy_ships[hit_ship];
            if (ship->shield_health > 0) {
                ship->shield_health--;
                ship->shield_impact_angle = atan2(ship->y - bullet->y, ship->x - bullet->x);
                ship->shield_impact_timer = 0.2;
            } else {
                ship->health--;
                if (ship->health <= 0) {
                    comet_buster_destroy_enemy_ship(game, hit_ship, width, height, visualizer);
                }
            }
        }
    }
    
    // Check ship-to-ship collisions (enemy ships bumping into each other)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        if (!game->enemy_ships[i].active) continue;
        
        EnemyShip *ship1 = &game->enemy_ships[i];
        
        // Get ship1 collision radius based on type
        double ship1_radius = 12.0;
        if (ship1->ship_type == 5) {
            ship1_radius = 36.0;
        } else if (ship1->ship_type == 4) {
            ship1_radius = 18.0;
        }
        
        for (int j = i + 1; j < game->enemy_ship_count; j++) {
            if (!game->enemy_ships[j].active) continue;
            
            EnemyShip *ship2 = &game->enemy_ships[j];
            
            // Get ship2 collision radius
            double ship2_radius = 12.0;
            if (ship2->ship_type == 5) {
                ship2_radius = 36.0;
            } else if (ship2->ship_type == 4) {
                ship2_radius = 18.0;
            }
            
            double dx = ship1->x - ship2->x;
            double dy = ship1->y - ship2->y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = ship1_radius + ship2_radius;
            
            if (dist < collision_dist && dist > 0.001) {
                // Ships collided - both take damage
                if (ship1->shield_health > 0) {
                    ship1->shield_health--;
                } else {
                    ship1->health--;
                }
                
                if (ship2->shield_health > 0) {
                    ship2->shield_health--;
                } else {
                    ship2->health--;
                }
                
                // Check if either ship should be destroyed
                if (ship1->health <= 0) {
                    comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                }
                if (ship2->health <= 0) {
                    comet_buster_destroy_enemy_ship(game, j, width, height, visualizer);
                }
            }
        }
    }
    
    // Now do collision detection using the REAL collision functions from collision.cpp
    // Check enemy ship - comet collisions (same as main game)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        if (!game->enemy_ships[i].active) continue;
        
        for (int j = 0; j < game->comet_count; j++) {
            if (!game->comets[j].active) continue;
            
            EnemyShip *ship = &game->enemy_ships[i];
            Comet *comet = &game->comets[j];
            
            // Get ship collision radius based on actual ship size
            double ship_radius = 12.0;  // Default size
            if (ship->ship_type == 5) {
                ship_radius = 36.0;  // Juggernaut: 3x size
            } else if (ship->ship_type == 4) {
                ship_radius = 18.0;  // Elite blue (brown coat): 1.5x size
            }
            
            double dx = ship->x - comet->x;
            double dy = ship->y - comet->y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = ship_radius + comet->radius;  // Use actual ship size
            
            if (dist < collision_dist) {
                // Check if ship has shield protection
                if (ship->shield_health > 0) {
                    // Shield absorbs the comet impact
                    ship->shield_health--;
                    ship->shield_impact_angle = atan2(ship->y - comet->y, ship->x - comet->x);
                    ship->shield_impact_timer = 0.2;
                } else {
                    // Shields are down - destroy the ship
                    comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                }
                
                // Asteroid is always destroyed
                comet_buster_destroy_comet(game, j, width, height, visualizer);
                break;  // Exit since we modified array indices
            }
        }
    }
    
    // Cleanup pass: compact comet array by removing inactive comets
    // This prevents array from filling with dead comets and allows proper destruction/breakup animations
    int write_index = 0;
    for (int i = 0; i < game->comet_count; i++) {
        if (game->comets[i].active) {
            // Move active comet to write position
            if (i != write_index) {
                game->comets[write_index] = game->comets[i];
            }
            write_index++;
        }
    }
    game->comet_count = write_index;
}

// Check if splash screen should exit (any key pressed)
bool comet_buster_splash_screen_input_detected(Visualizer *visualizer) {
    if (!visualizer) return false;
    
    // Any keyboard key
    if (visualizer->key_a_pressed || visualizer->key_d_pressed ||
        visualizer->key_w_pressed || visualizer->key_s_pressed ||
        visualizer->key_z_pressed || visualizer->key_x_pressed ||
        visualizer->key_space_pressed || visualizer->key_ctrl_pressed) {
        return true;
    }
    
    // Any joystick button
    JoystickState *js = joystick_manager_get_active(&visualizer->joystick_manager);
    if (js && js->connected) {
        if (js->button_a || js->button_b || js->button_x || js->button_y ||
            js->button_start || js->button_back) {
            return true;
        }
    }
    
    // Any mouse click
    if (visualizer->mouse_left_pressed || visualizer->mouse_right_pressed || 
        visualizer->mouse_middle_pressed) {
        return true;
    }
    
    return false;
}

// Exit splash screen and start game
void comet_buster_exit_splash_screen(CometBusterGame *game) {
    if (!game) return;
    
    fprintf(stdout, "[SPLASH] Exiting splash screen, starting game\n");
    
    game->splash_screen_active = false;
    game->splash_timer = 0.0;
    
    // Clear all objects to start fresh game
    game->comet_count = 0;
    game->bullet_count = 0;
    game->particle_count = 0;
    game->floating_text_count = 0;
    game->canister_count = 0;
    game->missile_count = 0;
    game->missile_pickup_count = 0;
    // NOTE: high_score_count is NOT reset - high scores persist from disk load
    game->enemy_ship_count = 0;
    game->enemy_bullet_count = 0;
    
    game->boss_active = false;
    game->boss.active = false;
    game->spawn_queen.active = false;
    game->spawn_queen.is_spawn_queen = false;
    game->boss_spawn_timer = 0;
    game->last_boss_wave = 0;
    
    game->ship_x = 400.0;
    game->ship_y = 300.0;
    game->ship_vx = 0;
    game->ship_vy = 0;
    game->ship_angle = 0;
    game->ship_speed = 0;
//    game->ship_lives = 3;
    game->invulnerability_time = 0;
    
/*    game->shield_health = 3;
    game->max_shield_health = 3; */
    game->shield_regen_timer = 0;
    game->shield_regen_delay = 3.0;
    game->shield_regen_rate = 0.5;
    game->shield_impact_angle = 0;
    game->shield_impact_timer = 0;
    
    game->score = 0;
    game->comets_destroyed = 0;
    game->score_multiplier = 1.0;
    game->consecutive_hits = 0;
    game->current_wave = 1;
    game->wave_comets = 0;
    game->last_life_milestone = 0;
    game->game_over = false;
    game->game_won = false;
    
    game->spawn_timer = 1.0;
    game->base_spawn_rate = 1.0;
    game->beat_fire_cooldown = 0;
    game->last_beat_time = -1;
    game->difficulty_timer = 0;
    game->enemy_ship_spawn_timer = 5.0;
    game->enemy_ship_spawn_rate = 8.0;
    
    game->mouse_left_pressed = false;
    game->mouse_fire_cooldown = 0;
    game->mouse_right_pressed = false;
    game->mouse_middle_pressed = false;
    game->omni_fire_cooldown = 0;
    
    game->keyboard.key_a_pressed = false;
    game->keyboard.key_d_pressed = false;
    game->keyboard.key_w_pressed = false;
    game->keyboard.key_s_pressed = false;
    game->keyboard.key_z_pressed = false;
    game->keyboard.key_x_pressed = false;
    game->keyboard.key_space_pressed = false;
    game->keyboard.key_ctrl_pressed = false;
    
    game->energy_amount = 100.0;
    game->max_energy = 100.0;
    game->energy_burn_rate = 25.0;
    game->energy_recharge_rate = 10.0;
    game->boost_multiplier = 2.5;
    game->is_boosting = false;
    game->boost_thrust_timer = 0.0;
    
    comet_buster_spawn_wave(game, 1920, 1080);
}

// ============================================================================
// VICTORY SCROLL FUNCTIONS
// ============================================================================

void comet_buster_show_victory_scroll(CometBusterGame *game) {
    if (!game) return;
    
    fprintf(stdout, "[VICTORY] Showing victory scroll\n");
    
    game->splash_screen_active = true;  // Reuse splash screen rendering system
    game->splash_timer = 0.0;
    
#ifdef ExternalSound
    // TODO: Play finale music
    //audio_play_intro_music(&visualizer->audio, "music/finale.mp3");
#endif
}

void comet_buster_update_victory_scroll(CometBusterGame *game, double dt) {
    if (!game || !game->splash_screen_active) return;
    
    game->splash_timer += dt;
    
    // Auto-advance if scroll completes (optional)
    double line_duration = 0.8;
    double total_duration = NUM_VICTORY_LINES * line_duration + 3.0;
    
    if (game->splash_timer > total_duration) {
        // Victory scroll complete - could auto-reset here if desired
    }
}

bool comet_buster_victory_scroll_input_detected(CometBusterGame *game, Visualizer *visualizer) {
    if (!game || !game->splash_screen_active || !game->game_won) return false;
    if (game->splash_timer < 2.0) return false;  // Minimum display time
    
    // Check keyboard
    if (game->keyboard.key_a_pressed || game->keyboard.key_d_pressed || 
        game->keyboard.key_w_pressed || game->keyboard.key_s_pressed ||
        game->keyboard.key_space_pressed) {
        return true;
    }
    
    // Check mouse
    if (game->mouse_left_pressed) {
        return true;
    }
    
    return false;
}

void comet_buster_exit_victory_scroll(CometBusterGame *game) {
    if (!game) return;
    
    fprintf(stdout, "[VICTORY] Exiting victory scroll\n");
    
    game->splash_screen_active = false;
    game->game_won = false;
    game->splash_timer = 0.0;
    
    // Reset game to return to menu
    comet_buster_reset_game(game);
}

// ============================================================================
// WAVE 30 FINALE SPLASH SCREEN
// ============================================================================

void comet_buster_update_finale_splash(CometBusterGame *game, double dt) {
    if (!game || !game->finale_splash_active) return;
    
    game->finale_splash_timer += dt;
    game->finale_scroll_timer += dt;
    
    // Auto-advance text lines every 0.6 seconds (faster scroll)
    if (game->finale_scroll_timer >= 0.6) {
        game->finale_scroll_line_index++;
        game->finale_scroll_timer = 0.0;
        
        // When we reach the end, wait for input
        if (game->finale_scroll_line_index >= NUM_VICTORY_LINES) {
            game->finale_waiting_for_input = true;
            game->finale_scroll_line_index = NUM_VICTORY_LINES - 1;
        }
    }
}
