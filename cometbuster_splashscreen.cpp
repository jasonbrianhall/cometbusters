#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"

// ============================================================================
// OPENING CRAWL TEXT - PROPER LINE BY LINE
// ============================================================================

static const char *OPENING_CRAWL_LINES[] = {
    "",
    "",
    "COMET BUSTER",
    "",
    "In the not so distant future in a galaxy not so far away",
    "",
    "",
    "The Kepler-442 Asteroid Field, once a",
    "treasure trove of minerals, now lies in ruin.",
    "Asteroids fracture, comets drift, factions clash.",
    "",
    "Red warships hunt without mercy.",
    "Blue patrols guard with fragile honor.",
    "Green drones strip-mine with ruthless speed.",
    "And now... the PURPLE SENTINELS arrive—",
    "enigmatic guardians with unknown intent.",
    "",
    "You fly the DESTINY—",
    "an ancient warship of unknown origin,",
    "reborn as a mining vessel,",
    "armed with rapid-fire cannons,",
    "advanced thrusters, hyper-accurate missles,",
    "and omnidirectional fire.",
    "",
    "It is fragile, yet fierce.",
    "It carries no banner, no allegiance,",
    "only the will to survive.",
    "",
    "But survival is not enough.",
    "Beyond the factions loom colossal threats:",
    "MEGA BOSS SHIPS, engines of annihilation,",
    "whose presence darkens the field itself.",
    "",
    "And deeper still, from the void,",
    "alien forces gather—",
    "a tide that consumes all in its path.",
    "",
    "Your mission: endure the chaos,",
    "outwit rival factions,",
    "and face the horrors that await.",
    "",
    "The asteroid field is no longer a mine.",
    "It is a crucible of war.",
    "",
    "Survive. Score. Ascend.",
    "",
    "",
};

#define NUM_CRAWL_LINES (sizeof(OPENING_CRAWL_LINES) / sizeof(OPENING_CRAWL_LINES[0]))

// ============================================================================
// VICTORY SCROLL TEXT - FINALE WHEN WAVE 30 IS BEATEN
// ============================================================================

static const char *VICTORY_SCROLL_LINES[] = {
    "",
    "",
    "",
    "THE KEPLER-442 INCIDENT: CONCLUDED",
    "",
    "The asteroids... they were never truly chaotic.",
    "",
    "As you clear the final waves from the field,",
    "the truth crystallizes in the wreckage of a",
    "thousand ships. This was not a mining operation.",
    "It was a crucible. A test.",
    "",
    "The three factions sent their finest.",
    "The Red Warships of the Galactic Defense Collective.",
    "The peaceful patrols of the Independent Sector Alliance.",
    "The corporate drones of the Asteroid Mining Collective.",
    "",
    "You destroyed them all.",
    "",
    "The Purple Sentinels... they were watching.",
    "Observing. Calculating. Waiting.",
    "When the final Juggernaut fell to your fire,",
    "they simply... departed.",
    "",
    "As if satisfied.",
    "",
    "The DESTINY's ancient systems hum with purpose.",
    "Weapons that should not exist. Shields that defy physics.",
    "A ship of unknown origin, reborn as a mining vessel,",
    "now standing victorious in a field of ash and silence.",
    "",
    "What are you, truly?",
    "What was the DESTINY, before?",
    "",
    "The void offers no answers.",
    "Only echoes.",
    "",
    "---",
    "",
    "You have restored order to Kepler-442.",
    "The factions will reconsider their ambitions.",
    "The Sentinels have completed their observations.",
    "The Golden Juggernauts lie dormant in the dark.",
    "",
    "But something remains.",
    "",
    "At the edge of the asteroid field,",
    "beyond the reach of conventional sensors,",
    "something ancient stirs.",
    "",
    "The DESTINY's original designers are watching.",
    "They are pleased with what you have become.",
    "They are waiting.",
    "",
    "And they are preparing.",
    "",
    "---",
    "",
    "Press any key to return to the stars...",
    "",
    ""
};

#define NUM_VICTORY_LINES (sizeof(VICTORY_SCROLL_LINES) / sizeof(VICTORY_SCROLL_LINES[0]))

// ============================================================================

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
                // Use the REAL destroy functions from collision.cpp
                // This handles everything: explosions, damage, removal
                comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                comet_buster_destroy_comet(game, j, width, height, visualizer);
                break;  // Exit since we modified array indices
            }
        }
    }
}

// Draw splash screen with proper line-by-line scrolling crawl
void comet_buster_draw_splash_screen(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !game->splash_screen_active) return;
    
    // Draw background (dark space)
    cairo_set_source_rgb(cr, 0.04, 0.06, 0.15);
    cairo_paint(cr);

    // Draw grid (extended 50 pixels further to the right)
    cairo_set_source_rgb(cr, 0.1, 0.15, 0.35);
    cairo_set_line_width(cr, 0.5);
    for (int i = 0; i <= width + 50; i += 50) {  // Extend 50 pixels beyond width
        cairo_move_to(cr, i, 0);
        cairo_line_to(cr, i, height);
    }
    for (int i = 0; i <= height; i += 50) {
        cairo_move_to(cr, 0, i);
        cairo_line_to(cr, width + 50, i);  // Extend 50 pixels to the right
    }
    cairo_stroke(cr);
    
    // Use existing game functions to draw all animated objects
    draw_comet_buster_comets(game, cr, width, height);
    draw_comet_buster_enemy_ships(game, cr, width, height);
    draw_comet_buster_enemy_bullets(game, cr, width, height);
    draw_comet_buster_particles(game, cr, width, height);
    
    // Draw boss if active
    if (game->boss_active) {
        draw_comet_buster_boss(&game->boss, cr, width, height);
    }
    
    // Dim the background with overlay for text visibility
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
    cairo_paint(cr);
    
    // ===== OPENING CRAWL PHASE =====
    // Duration: approximately 0-38 seconds (much longer to account for extended fade zones)
    double scroll_speed = 1.0;  // seconds per line - SLOWER

    if (game->splash_timer < 38.0) {
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 24.0);
        
        // Yellow Star Wars style text
        cairo_set_source_rgb(cr, 1.0, 0.95, 0.0);
        
        // Calculate line height
        cairo_text_extents_t extents;
        cairo_text_extents(cr, "A", &extents);
        double line_height = extents.height * 1.8;
        
        // Each line appears and scrolls from bottom to top SLOWLY
        // Scroll speed: one line per 0.6 seconds (slower than before)
        double lines_visible = (height + 200.0) / line_height;
        
        // Calculate which lines should be visible
        double current_line_offset = (game->splash_timer / scroll_speed);
        double fractional_offset = fmod(game->splash_timer, scroll_speed) / scroll_speed;
        
        // Draw all lines that could be visible
        for (int i = 0; i < (int)lines_visible + 2; i++) {
            int line_index = (int)current_line_offset + i;
            if (line_index < 0 || line_index >= (int)NUM_CRAWL_LINES) continue;
            
            // Calculate Y position (lines scroll up from bottom to top)
            double y_pos = height - (fractional_offset * line_height) + (i * line_height) - (current_line_offset * line_height);
            
            // Calculate fade for lines entering (bottom) and leaving (top) - MUCH LONGER FADE
            double alpha = 1.0;
            if (y_pos < 200.0) {
                alpha = y_pos / 200.0;  // Fade in at top - MUCH LONGER (200px instead of 120px)
            } else if (y_pos > height - 200.0) {
                alpha = (height - y_pos) / 200.0;  // Fade out at bottom - MUCH LONGER
            }
            
            if (alpha < 0.0) alpha = 0.0;
            if (alpha > 1.0) alpha = 1.0;
            
            cairo_set_source_rgba(cr, 1.0, 0.95, 0.0, alpha);
            
            // Center the text
            cairo_text_extents(cr, OPENING_CRAWL_LINES[line_index], &extents);
            double x_pos = (width - extents.width) / 2.0;
            
            cairo_move_to(cr, x_pos, y_pos);
            cairo_show_text(cr, OPENING_CRAWL_LINES[line_index]);
        }
    }
    // ===== TITLE PHASE =====
    // After crawl ends, show big GALAXY RAIDERS title
    else if (game->splash_timer < 43.0) {
        // Fade in the title
        double phase_timer = game->splash_timer - 38.0;
        double title_alpha = phase_timer / 2.0;  // Fade in over 2 seconds
        if (title_alpha > 1.0) title_alpha = 1.0;
        
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 120.0);
        
        // Draw glowing text effect
        for (int i = 5; i > 0; i--) {
            double alpha = 0.1 * (5 - i) / 5.0 * title_alpha;
            cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, alpha);
            
            cairo_text_extents_t extents;
            cairo_text_extents(cr, "COMET BUSTERS", &extents);
            double title_x = (width - extents.width) / 2.0;
            double title_y = height / 2.0 + 20;
            
            cairo_move_to(cr, title_x, title_y);
            cairo_show_text(cr, "COMET BUSTERS");
        }
        
        // Draw bright main title text
        cairo_text_extents_t extents;
        cairo_text_extents(cr, "COMET BUSTERS", &extents);
        double title_x = (width - extents.width) / 2.0;
        double title_y = height / 2.0 + 20;
        
        cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, title_alpha);
        cairo_move_to(cr, title_x, title_y);
        cairo_show_text(cr, "COMET BUSTERS");
        
        // Draw subtitle
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 28.0);
        
        cairo_text_extents(cr, "Press fire key to start", &extents);
        double subtitle_x = (width - extents.width) / 2.0;
        double subtitle_y = title_y + 80;
        
        // Blinking text effect for subtitle
        double blink_alpha = 0.5 + 0.5 * sin(game->splash_timer * 3.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, blink_alpha * title_alpha);
        cairo_move_to(cr, subtitle_x, subtitle_y);
        cairo_show_text(cr, "Press fire key to start");
    }
    // ===== WAIT PHASE =====
    // Just show the title and wait for input
    else {
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 120.0);
        
        // Full brightness
        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, "COMET BUSTERS", &extents);
        double title_x = (width - extents.width) / 2.0;
        double title_y = height / 2.0 + 20;
        
        cairo_move_to(cr, title_x, title_y);
        cairo_show_text(cr, "COMET BUSTERS");
        
        // Draw subtitle
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 28.0);
        
        cairo_text_extents(cr, "Press fire key to start", &extents);
        double subtitle_x = (width - extents.width) / 2.0;
        double subtitle_y = title_y + 80;
        
        // Blinking text effect for subtitle
        double blink_alpha = 0.5 + 0.5 * sin(game->splash_timer * 3.0);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, blink_alpha);
        cairo_move_to(cr, subtitle_x, subtitle_y);
        cairo_show_text(cr, "Press fire key to start");
    }
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
    // audio_play_intro_music(&visualizer->audio, "music/finale.mp3");
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

void comet_buster_draw_victory_scroll(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !game->splash_screen_active || game->game_won == false) return;
    
    double timer = game->splash_timer;
    
    // Draw semi-transparent black background
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.95);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Setup text
    cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);  // Yellow text like Star Wars
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 24);
    
    // Calculate scroll
    double line_duration = 0.8;  // Seconds per line
    int start_line = (int)(timer / line_duration);
    double line_alpha = 1.0 - fmod(timer, line_duration) / (line_duration * 0.3);
    line_alpha = (line_alpha < 0) ? 0 : (line_alpha > 1) ? 1 : line_alpha;
    
    // Draw visible lines
    int y_pos = height / 3;
    for (int i = 0; i < 8 && start_line + i < NUM_VICTORY_LINES; i++) {
        double fade = 1.0;
        
        // First line fades in
        if (i == 0) {
            fade = line_alpha;
        }
        // Last visible line fades out
        if (i == 7) {
            fade = 1.0 - (double)i / 10.0;
        }
        
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, fade);
        
        // Center text horizontally
        cairo_text_extents_t extents;
        cairo_text_extents(cr, VICTORY_SCROLL_LINES[start_line + i], &extents);
        double x_pos = (width - extents.width) / 2.0;
        
        cairo_move_to(cr, x_pos, y_pos + i * 40);
        cairo_show_text(cr, VICTORY_SCROLL_LINES[start_line + i]);
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
    
    // Auto-advance text lines every 1.5 seconds
    if (game->finale_scroll_timer >= 1.5) {
        game->finale_scroll_line_index++;
        game->finale_scroll_timer = 0.0;
        
        // When we reach the end, wait for input
        if (game->finale_scroll_line_index >= NUM_VICTORY_LINES) {
            game->finale_waiting_for_input = true;
            game->finale_scroll_line_index = NUM_VICTORY_LINES - 1;
        }
    }
}

void comet_buster_draw_finale_splash(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !game->finale_splash_active) return;
    
    // Semi-transparent dark overlay
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.75);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Title
    cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 36);
    cairo_text_extents_t extents;
    
    const char *title = "WAVE 30 COMPLETE";
    cairo_text_extents(cr, title, &extents);
    cairo_move_to(cr, width/2.0 - extents.width/2.0, 80);
    cairo_show_text(cr, title);
    
    // Victory scroll text
    cairo_set_source_rgb(cr, 0.2, 0.8, 1.0);
    cairo_set_font_size(cr, 15);
    
    double y_pos = 150;
    for (int i = 0; i <= game->finale_scroll_line_index && i < NUM_VICTORY_LINES; i++) {
        cairo_text_extents(cr, VICTORY_SCROLL_LINES[i], &extents);
        cairo_move_to(cr, width/2.0 - extents.width/2.0, y_pos);
        cairo_show_text(cr, VICTORY_SCROLL_LINES[i]);
        y_pos += 22;
    }
    
    // Show "RIGHT-CLICK TO CONTINUE" when done scrolling
    if (game->finale_waiting_for_input) {
        double pulse = 0.5 + 0.5 * sin(game->finale_splash_timer * 2.5);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, pulse);
        
        cairo_set_font_size(cr, 16);
        const char *continue_text = "RIGHT-CLICK TO CONTINUE TO WAVE 31";
        cairo_text_extents(cr, continue_text, &extents);
        cairo_move_to(cr, width/2.0 - extents.width/2.0, height - 50);
        cairo_show_text(cr, continue_text);
    }
}
