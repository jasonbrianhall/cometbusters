#include <cairo.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cometbuster.h"
#include "visualization.h"
#include "comet_lang.h"
#ifdef ExternalSound
#include "cometbuster_splashscreen.h"
#endif

#ifdef ExternalSound
#include "audio_wad.h"
#endif 

// ============================================================================
// RENDERING - VECTOR-BASED ASTEROIDS
// ============================================================================

void draw_comet_buster(Visualizer *visualizer, cairo_t *cr) {
    if (!visualizer || !cr) return;
    
    CometBusterGame *game = &visualizer->comet_buster;
    int width = visualizer->width;
    int height = visualizer->height;
    
#ifdef ExternalSound
    // Draw splash screen if active
    if (game->splash_screen_active) {
        comet_buster_draw_splash_screen(game, cr, width, height);
        return;  // Don't draw game yet
    }
#endif
    
    // Background
    cairo_set_source_rgb(cr, 0.04, 0.06, 0.15);
    cairo_paint(cr);
    
    // Grid (extended 50 pixels to the right)
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
    
    // Draw game elements
    draw_comet_buster_comets(game, cr, width, height);
    draw_comet_buster_bullets(game, cr, width, height);
    draw_comet_buster_enemy_ships(game, cr, width, height);
    draw_comet_buster_ufos(game, cr, width, height);  // Draw UFO flying saucers
    
    // Draw boss (either Spawn Queen or regular Death Star)
    if (game->boss_active) {
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            draw_spawn_queen_boss(&game->spawn_queen, cr, width, height);
        } else if (game->boss.active) {
            if (game->current_wave % 30 == 5) {
               draw_comet_buster_boss(&game->boss, cr, width, height);      // Death Star (wave 5, 35, 65, etc)
           } else if (game->current_wave % 30 == 10) {
               draw_spawn_queen_boss(&game->spawn_queen, cr, width, height); // Spawn Queen (wave 10, 40, 70, etc)
           } else if (game->current_wave % 30 == 15) {
              draw_void_nexus_boss(game, &game->boss, cr, width, height);         // Void Nexus (wave 15, 45, 75, etc)
           } else if (game->current_wave % 30 == 20) {
              draw_harbinger_boss(game, &game->boss, cr, width, height);          // Harbinger (wave 20, 50, 80, etc)
           } else if (game->current_wave % 30 == 25) {
              draw_star_vortex_boss(&game->boss, cr, width, height);        // Star Vortex (wave 25, 55, 85, etc)
           } else if (game->current_wave % 30 == 0) {
              draw_singularity_boss(game, &game->boss, cr, width, height);        // Singularity (wave 30, 60, 90, etc)
           }
       }
    }
    
    draw_comet_buster_enemy_bullets(game, cr, width, height);
    draw_comet_buster_canisters(game, cr, width, height);
    draw_comet_buster_missile_pickups(game, cr, width, height);
    draw_comet_buster_bomb_pickups(game, cr, width, height);
    draw_comet_buster_missiles(game, cr, width, height);
    draw_comet_buster_bombs(game, cr, width, height);
    draw_comet_buster_particles(game, cr, width, height);
    draw_comet_buster_ship(game, cr, width, height);
    
    // Draw boss explosion effect
    boss_explosion_draw(&game->boss_explosion_effect, cr);
    
    // Draw HUD
    draw_comet_buster_hud(game, cr, width, height);
    
    // Draw game over
    if (game->game_over) {
        draw_comet_buster_game_over(game, cr, width, height);
    }
}

// ✓ VECTOR-BASED ASTEROIDS (like original Asteroids arcade game)
void draw_comet_buster_comets(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        
        // Skip inactive (destroyed) comets - DO NOT RENDER THEM
        if (!c->active) continue;
        
        cairo_save(cr);
        cairo_translate(cr, c->x, c->y);
        cairo_rotate(cr, c->base_angle + c->rotation * M_PI / 180.0);
        
        // Set color and line width
        cairo_set_source_rgb(cr, c->color[0], c->color[1], c->color[2]);
        cairo_set_line_width(cr, 2.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        
        // Draw vector-based asteroid (polygon with jagged edges)
        double radius = c->radius;
        
        // Use rotation_speed as a shape variant seed (deterministic but varied)
        int shape_variant = (int)c->rotation_speed % 3;
        
        if (c->size == COMET_MEGA) {
            // Mega asteroid: giant 12+ pointed shape that appears on boss waves (wave % 5 == 0)
            // Thicker lines for emphasis
            cairo_set_line_width(cr, 3.5);
            
            if (shape_variant == 0) {
                // Complex mega variant 1
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.8, radius * 0.55},
                    {radius * 0.6, radius * 0.9},
                    {radius * 0.2, radius * 0.95},
                    {-radius * 0.4, radius * 0.85},
                    {-radius * 0.75, radius * 0.65},
                    {-radius * 0.95, radius * 0.2},
                    {-radius * 0.9, -radius * 0.35},
                    {-radius * 0.6, -radius * 0.8},
                    {-radius * 0.1, -radius * 0.95},
                    {radius * 0.5, -radius * 0.85},
                    {radius * 0.85, -radius * 0.5}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 12; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else if (shape_variant == 1) {
                // Complex mega variant 2
                double points[][2] = {
                    {radius * 0.95, radius * 0.15},
                    {radius * 0.7, radius * 0.75},
                    {radius * 0.3, radius * 0.95},
                    {-radius * 0.2, radius * 0.9},
                    {-radius * 0.65, radius * 0.75},
                    {-radius * 0.9, radius * 0.3},
                    {-radius * 0.95, -radius * 0.2},
                    {-radius * 0.75, -radius * 0.7},
                    {-radius * 0.35, -radius * 0.92},
                    {radius * 0.15, -radius * 0.95},
                    {radius * 0.65, -radius * 0.75},
                    {radius * 0.9, -radius * 0.35}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 12; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else {
                // Complex mega variant 3
                double points[][2] = {
                    {radius, -radius * 0.1},
                    {radius * 0.8, radius * 0.6},
                    {radius * 0.5, radius * 0.88},
                    {radius * 0.1, radius * 0.96},
                    {-radius * 0.35, radius * 0.88},
                    {-radius * 0.7, radius * 0.7},
                    {-radius * 0.95, radius * 0.15},
                    {-radius * 0.88, -radius * 0.4},
                    {-radius * 0.55, -radius * 0.85},
                    {-radius * 0.05, -radius * 0.96},
                    {radius * 0.6, -radius * 0.8},
                    {radius * 0.9, -radius * 0.4}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 12; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            }
        } else if (c->size == COMET_LARGE) {
            // Large asteroid: multiple shape variants using same point count but different geometry
            if (shape_variant == 0) {
                // Standard 8-pointed
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.7, radius * 0.7},
                    {0, radius},
                    {-radius * 0.6, radius * 0.8},
                    {-radius * 0.9, 0},
                    {-radius * 0.5, -radius * 0.8},
                    {0, -radius * 0.95},
                    {radius * 0.8, -radius * 0.6}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 8; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else if (shape_variant == 1) {
                // More jagged variant
                double points[][2] = {
                    {radius * 0.9, radius * 0.2},
                    {radius * 0.6, radius * 0.8},
                    {radius * 0.1, radius * 0.95},
                    {-radius * 0.7, radius * 0.7},
                    {-radius * 0.95, -0.1},
                    {-radius * 0.6, -radius * 0.8},
                    {radius * 0.2, -radius * 0.9},
                    {radius * 0.85, -radius * 0.3}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 8; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else {
                // Rounder variant
                double points[][2] = {
                    {radius, -radius * 0.2},
                    {radius * 0.75, radius * 0.6},
                    {radius * 0.2, radius * 0.9},
                    {-radius * 0.5, radius * 0.85},
                    {-radius * 0.95, radius * 0.1},
                    {-radius * 0.75, -radius * 0.65},
                    {-radius * 0.1, -radius * 0.95},
                    {radius * 0.7, -radius * 0.75}
                };
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 8; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            }
        } else if (c->size == COMET_MEDIUM) {
            // Medium asteroid: 3 shape variants with 5-6 points
            if (shape_variant == 0) {
                double points[][2] = {
                    {radius, 0},
                    {radius * 0.6, radius * 0.75},
                    {-radius * 0.5, radius * 0.8},
                    {-radius * 0.8, -radius * 0.6},
                    {radius * 0.5, -radius * 0.9}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 5; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else if (shape_variant == 1) {
                double points[][2] = {
                    {radius * 0.85, radius * 0.3},
                    {radius * 0.4, radius * 0.85},
                    {-radius * 0.7, radius * 0.6},
                    {-radius * 0.75, -radius * 0.7},
                    {radius * 0.7, -radius * 0.8}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 5; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else {
                double points[][2] = {
                    {radius * 0.95, -radius * 0.15},
                    {radius * 0.55, radius * 0.8},
                    {-radius * 0.65, radius * 0.75},
                    {-radius * 0.85, -radius * 0.5},
                    {radius * 0.6, -radius * 0.85},
                    {radius * 0.9, -radius * 0.3}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 6; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            }
        } else {
            // Small asteroid: simple variants with 3-4 points
            if (shape_variant == 0) {
                double points[][2] = {
                    {radius, 0},
                    {-radius * 0.7, radius * 0.7},
                    {-radius * 0.5, -radius * 0.8}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 3; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else if (shape_variant == 1) {
                double points[][2] = {
                    {radius * 0.9, radius * 0.2},
                    {-radius * 0.8, radius * 0.6},
                    {-radius * 0.6, -radius * 0.9}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 3; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            } else {
                double points[][2] = {
                    {radius, -radius * 0.3},
                    {-radius * 0.6, radius * 0.8},
                    {-radius * 0.7, -radius * 0.7},
                    {radius * 0.8, -radius * 0.1}
                };
                
                cairo_move_to(cr, points[0][0], points[0][1]);
                for (int j = 1; j < 4; j++) {
                    cairo_line_to(cr, points[j][0], points[j][1]);
                }
            }
        }
        
        cairo_close_path(cr);
        cairo_stroke(cr);
        
        cairo_restore(cr);
    }
}

void draw_comet_buster_bullets(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->bullet_count; i++) {
        Bullet *b = &game->bullets[i];
        
        // Draw bullet as small diamond
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_set_line_width(cr, 1.0);
        
        double size = 3.0;
        cairo_move_to(cr, b->x + size, b->y);
        cairo_line_to(cr, b->x, b->y + size);
        cairo_line_to(cr, b->x - size, b->y);
        cairo_line_to(cr, b->x, b->y - size);
        cairo_close_path(cr);
        cairo_fill(cr);
        
        // Draw short trail (classic Asteroids style)
        double trail_length = 5.0;  // Much shorter trail
        double norm_len = sqrt(b->vx*b->vx + b->vy*b->vy);
        if (norm_len > 0.1) {
            double trail_x = b->x - (b->vx / norm_len) * trail_length;
            double trail_y = b->y - (b->vy / norm_len) * trail_length;
            cairo_move_to(cr, trail_x, trail_y);
            cairo_line_to(cr, b->x, b->y);
            cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.3);
            cairo_set_line_width(cr, 0.5);
            cairo_stroke(cr);
        }
    }
}

void draw_comet_buster_enemy_ships(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        
        if (!ship->active) continue;
        
        // Draw enemy ship as a triangle (pointing in direction angle)
        cairo_save(cr);
        cairo_translate(cr, ship->x, ship->y);
        cairo_rotate(cr, ship->angle);
        
        // Choose color based on ship type
        if (ship->ship_type == 1) {
            // Aggressive red ship
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);  // Bright red
        } else if (ship->ship_type == 2) {
            // Hunter green ship
            cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);  // Bright green
        } else if (ship->ship_type == 3) {
            // Sentinel purple ship
            cairo_set_source_rgb(cr, 0.8, 0.2, 1.0);  // Bright purple
        } else if (ship->ship_type == 4) {
            // BROWN COAT ELITE BLUE SHIP - bright cyan, more saturated
            cairo_set_source_rgb(cr, 0.0, 0.9, 1.0);  // Bright cyan (more saturated than patrol blue)
        } else if (ship->ship_type == 5) {
            // JUGGERNAUT - massive golden ship
            cairo_set_source_rgb(cr, 1.0, 0.84, 0.0);  // Gold
        } else {
            // Patrol blue ship (type 0)
            cairo_set_source_rgb(cr, 0.2, 0.6, 1.0);  // Standard blue
        }
        
        cairo_set_line_width(cr, 1.5);
        
        // Draw ship body as triangle (size varies by type)
        // Juggernaut is 3x, brown coats are 1.5x, others are 1x
        double ship_size;
        if (ship->ship_type == 5) {
            ship_size = 36;  // Juggernaut: 3x normal size
        } else if (ship->ship_type == 4) {
            ship_size = 18;  // Brown coat: 1.5x normal size
        } else {
            ship_size = 12;  // All others: normal size
        }
        
        cairo_move_to(cr, ship_size, 0);              // Front point
        cairo_line_to(cr, -ship_size, -ship_size/1.5);  // Back left
        cairo_line_to(cr, -ship_size, ship_size/1.5);   // Back right
        cairo_close_path(cr);
        cairo_fill_preserve(cr);
        cairo_stroke(cr);
        
        // Draw health indicator (single bar at top of ship)
        cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);  // Green
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, ship_size - 5, -ship_size - 3);
        cairo_line_to(cr, ship_size - 5, -ship_size);
        cairo_stroke(cr);
        
        // Draw thruster/burner effect - show if there's any intensity
        if (ship->burner_intensity > 0.01) {  // Only show if actually has intensity
            draw_enemy_ship_burner(cr, ship->burner_intensity, ship_size);
        }
        
        cairo_restore(cr);
        
        // For Juggernaut, draw a detailed health bar below the ship (in screen coordinates, not rotated)
        if (ship->ship_type == 5) {
            cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);  // Dark gray background
            cairo_set_line_width(cr, 1.0);
            double bar_width = 60.0;
            double bar_height = 8.0;
            double bar_x = ship->x - bar_width/2;
            double bar_y = ship->y + 50;  // Always below the ship
            
            cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
            cairo_fill_preserve(cr);
            cairo_stroke(cr);
            
            // Health bar (green to yellow to red as health decreases)
            double health_ratio = (double)ship->health / 10.0;  // 10 is max health
            if (health_ratio < 0) health_ratio = 0;
            
            // Color changes: green -> yellow -> red
            double bar_r, bar_g, bar_b;
            if (health_ratio > 0.5) {
                // Green to yellow
                bar_r = (1.0 - health_ratio) * 2;
                bar_g = 1.0;
                bar_b = 0.0;
            } else {
                // Yellow to red
                bar_r = 1.0;
                bar_g = health_ratio * 2;
                bar_b = 0.0;
            }
            
            cairo_set_source_rgb(cr, bar_r, bar_g, bar_b);
            cairo_rectangle(cr, bar_x + 1, bar_y + 1, (bar_width - 2) * health_ratio, bar_height - 2);
            cairo_fill(cr);
        }
        
        // Draw shield circle around enemy ship if it has shields
        if (ship->shield_health > 0) {
            cairo_save(cr);
            cairo_translate(cr, ship->x, ship->y);
            
            // Shield color based on ship type
            if (ship->ship_type == 1) {
                // Red ship shield: orange/red
                cairo_set_source_rgba(cr, 1.0, 0.5, 0.0, 0.5);
            } else if (ship->ship_type == 2) {
                // Green ship shield: bright green
                cairo_set_source_rgba(cr, 0.5, 1.0, 0.5, 0.5);
            } else if (ship->ship_type == 3) {
                // Sentinel purple shield: bright purple
                cairo_set_source_rgba(cr, 0.8, 0.4, 1.0, 0.5);
            } else if (ship->ship_type == 4) {
                // BROWN COAT CYAN SHIELD - bright cyan
                cairo_set_source_rgba(cr, 0.2, 0.9, 1.0, 0.6);  // Brighter and more saturated
            } else if (ship->ship_type == 5) {
                // JUGGERNAUT GOLD SHIELD - golden glow
                cairo_set_source_rgba(cr, 1.0, 0.9, 0.2, 0.5);  // Golden
            } else {
                // Blue ship: no shield (shouldn't reach here)
                cairo_set_source_rgba(cr, 0.2, 0.6, 1.0, 0.5);
            }
            
            cairo_set_line_width(cr, 2.0);
            
            // Shield radius scales with ship size
            double shield_radius;
            if (ship->ship_type == 5) {
                shield_radius = 50;  // Larger shield for juggernaut
            } else if (ship->ship_type == 4) {
                shield_radius = 24;  // Slightly larger for brown coat
            } else {
                shield_radius = 22;  // Standard shield radius
            }
            
            cairo_arc(cr, 0, 0, shield_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
            
            // Draw shield impact flash
            if (ship->shield_impact_timer > 0) {
                double impact_x = 22 * cos(ship->shield_impact_angle);
                double impact_y = 22 * sin(ship->shield_impact_angle);
                double flash_alpha = ship->shield_impact_timer / 0.2;
                
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, flash_alpha * 0.8);
                cairo_arc(cr, impact_x, impact_y, 4, 0, 2 * M_PI);
                cairo_fill(cr);
                
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, flash_alpha * 0.4);
                cairo_set_line_width(cr, 1.0);
                double ring_radius = 6 + (1.0 - flash_alpha) * 10;
                cairo_arc(cr, impact_x, impact_y, ring_radius, 0, 2 * M_PI);
                cairo_stroke(cr);
            }
            
            cairo_restore(cr);
        }
        
        // Draw formation connection lines between Sentinel ships
        if (ship->ship_type == 3) {
            cairo_save(cr);
            
            // Find and draw lines to other sentinels in formation
            for (int j = i + 1; j < game->enemy_ship_count; j++) {
                EnemyShip *other = &game->enemy_ships[j];
                if (other->active && other->ship_type == 3 && other->formation_id == ship->formation_id) {
                    cairo_set_source_rgba(cr, 0.8, 0.4, 1.0, 0.3);  // Purple line
                    cairo_set_line_width(cr, 1.0);
                    cairo_move_to(cr, ship->x, ship->y);
                    cairo_line_to(cr, other->x, other->y);
                    cairo_stroke(cr);
                }
            }
            
            cairo_restore(cr);
        }
    }
}

void draw_comet_buster_enemy_bullets(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        Bullet *b = &game->enemy_bullets[i];
        
        // Draw enemy bullets as small cyan circles (different from player's yellow)
        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);  // Cyan - very distinct from yellow
        cairo_arc(cr, b->x, b->y, 2.5, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Draw trail
        double trail_length = 4.0;
        double norm_len = sqrt(b->vx*b->vx + b->vy*b->vy);
        if (norm_len > 0.1) {
            double trail_x = b->x - (b->vx / norm_len) * trail_length;
            double trail_y = b->y - (b->vy / norm_len) * trail_length;
            cairo_move_to(cr, trail_x, trail_y);
            cairo_line_to(cr, b->x, b->y);
            cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.3);  // Cyan trail
            cairo_set_line_width(cr, 0.5);
            cairo_stroke(cr);
        }
    }
}

void draw_comet_buster_particles(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->particle_count; i++) {
        Particle *p = &game->particles[i];
        
        double alpha = p->lifetime / p->max_lifetime;
        cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha);
        cairo_arc(cr, p->x, p->y, p->size, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
}

void draw_comet_buster_canisters(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    for (int i = 0; i < game->canister_count; i++) {
        Canister *c = &game->canisters[i];
        if (!c->active) continue;
        
        // Alpha based on remaining lifetime (fade out near end)
        double alpha = 1.0;
        if (c->lifetime < 2.0) {
            alpha = c->lifetime / 2.0;  // Fade out in last 2 seconds
        }
        
        cairo_save(cr);
        cairo_translate(cr, c->x, c->y);
        cairo_rotate(cr, c->rotation * M_PI / 180.0);
        
        // Draw canister as a proper shield shape (like in the image)
        cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, alpha);  // Cyan color
        
        double shield_width = 16.0;
        double shield_height = 20.0;
        
        // Begin shield path
        cairo_new_path(cr);
        
        // Draw shield outline
        // Top-left corner (rounded)
        cairo_move_to(cr, -shield_width * 0.5 + 3.0, -shield_height * 0.4);
        cairo_curve_to(cr, -shield_width * 0.5, -shield_height * 0.4,
                          -shield_width * 0.5, -shield_height * 0.3,
                          -shield_width * 0.5, -shield_height * 0.2);
        
        // Left edge going down
        cairo_line_to(cr, -shield_width * 0.5, shield_height * 0.2);
        
        // Bottom point (sharp)
        cairo_line_to(cr, 0, shield_height * 0.5);
        
        // Right edge coming up
        cairo_line_to(cr, shield_width * 0.5, shield_height * 0.2);
        
        // Top-right corner (rounded)
        cairo_line_to(cr, shield_width * 0.5, -shield_height * 0.2);
        cairo_curve_to(cr, shield_width * 0.5, -shield_height * 0.3,
                          shield_width * 0.5, -shield_height * 0.4,
                          shield_width * 0.5 - 3.0, -shield_height * 0.4);
        
        // Top edge (curved)
        cairo_curve_to(cr, shield_width * 0.3, -shield_height * 0.45,
                          -shield_width * 0.3, -shield_height * 0.45,
                          -shield_width * 0.5 + 3.0, -shield_height * 0.4);
        
        cairo_close_path(cr);
        
        // Fill the shield with semi-transparent color
        cairo_set_line_width(cr, 2.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        
        // Fill with darker cyan
        cairo_set_source_rgba(cr, 0.0, 0.8, 0.8, alpha * 0.3);
        cairo_fill_preserve(cr);
        
        // Outline in bright cyan
        cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, alpha);
        cairo_stroke(cr);
        
        // Draw a medical cross/plus symbol in the center (smaller than before)
        cairo_set_line_width(cr, 2.0);
        double cross_size = 6.0;
        
        // Vertical bar of cross
        cairo_move_to(cr, 0, -cross_size);
        cairo_line_to(cr, 0, cross_size);
        cairo_stroke(cr);
        
        // Horizontal bar of cross
        cairo_move_to(cr, -cross_size, 0);
        cairo_line_to(cr, cross_size, 0);
        cairo_stroke(cr);
        
        cairo_restore(cr);
    }
}


void draw_comet_buster_ship(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;    // Suppress unused parameter warning
    (void)height;   // Suppress unused parameter warning
    
    cairo_save(cr);
    cairo_translate(cr, game->ship_x, game->ship_y);
    cairo_rotate(cr, game->ship_angle);
    
    // Ship as vector triangle (like Asteroids)
    double ship_size = 12;
    
    cairo_set_line_width(cr, 2.0);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
    
    if (game->invulnerability_time > 0) {
        double alpha = sin(game->invulnerability_time * 10) * 0.5 + 0.5;
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.0, alpha);
    } else {
        cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    }
    
    // Main triangle
    cairo_move_to(cr, ship_size, 0);
    cairo_line_to(cr, -ship_size, -ship_size);
    cairo_line_to(cr, -ship_size * 0.3, 0);
    cairo_line_to(cr, -ship_size, ship_size);
    cairo_close_path(cr);
    cairo_stroke(cr);
    
    // Muzzle flash
    if (game->muzzle_flash_timer > 0) {
        double alpha = game->muzzle_flash_timer / 0.1;
        cairo_move_to(cr, ship_size, 0);
        cairo_line_to(cr, ship_size + 20, -5);
        cairo_line_to(cr, ship_size + 20, 5);
        cairo_close_path(cr);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, alpha);
        cairo_fill(cr);
    }
    
    // Draw thruster/burner effect - show if there's any intensity
    if (game->burner_intensity > 0.01) {  // Only show if actually has intensity
        // Draw big burner - scale based on speed for dramatic effect
        double speed = sqrt(game->ship_vx * game->ship_vx + game->ship_vy * game->ship_vy);
        double length_mult = 1.0 + (speed / 150.0) * 0.5;  // 1.0x to 1.5x longer at high speed (updated from 400 to 150)
        draw_ship_burner(cr, game->burner_intensity, length_mult);
    }
    
    cairo_restore(cr);
    
    // Draw shield circle around ship
    if (game->shield_health > 0) {
        cairo_save(cr);
        cairo_translate(cr, game->ship_x, game->ship_y);
        
        // Shield circle - brighter when more healthy
        double shield_alpha = (double)game->shield_health / game->max_shield_health;
        
        // Shield color: cyan when healthy, orange when low
        if (game->shield_health >= 2) {
            cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, shield_alpha * 0.6);  // Bright cyan
        } else if (game->shield_health >= 1) {
            cairo_set_source_rgba(cr, 1.0, 0.8, 0.0, shield_alpha * 0.6);  // Orange/yellow
        } else {
            cairo_set_source_rgba(cr, 1.0, 0.3, 0.3, shield_alpha * 0.6);  // Red warning
        }
        
        cairo_set_line_width(cr, 2.5);
        cairo_arc(cr, 0, 0, 28, 0, 2 * M_PI);
        cairo_stroke(cr);
        
        // Draw shield segments/pips to show health
        cairo_set_line_width(cr, 1.5);
        double segment_angle = (2 * M_PI) / game->max_shield_health;
        
        for (int i = 0; i < game->shield_health; i++) {
            double angle = (i * segment_angle) - (M_PI / 2);  // Start at top
            double x1 = 24 * cos(angle);
            double y1 = 24 * sin(angle);
            double x2 = 32 * cos(angle);
            double y2 = 32 * sin(angle);
            
            cairo_move_to(cr, x1, y1);
            cairo_line_to(cr, x2, y2);
            cairo_stroke(cr);
        }
        
        // Draw impact flash at hit point
        if (game->shield_impact_timer > 0) {
            double impact_x = 28 * cos(game->shield_impact_angle);
            double impact_y = 28 * sin(game->shield_impact_angle);
            double flash_alpha = game->shield_impact_timer / 0.2;  // Fade out over time
            
            // Bright white flash at impact point
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, flash_alpha * 0.8);
            cairo_arc(cr, impact_x, impact_y, 5, 0, 2 * M_PI);
            cairo_fill(cr);
            
            // Expanding rings at impact
            cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, flash_alpha * 0.4);
            cairo_set_line_width(cr, 1.0);
            double ring_radius = 8 + (1.0 - flash_alpha) * 12;
            cairo_arc(cr, impact_x, impact_y, ring_radius, 0, 2 * M_PI);
            cairo_stroke(cr);
        }
        
        cairo_restore(cr);
    }
}

void draw_comet_buster_hud(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)height;   // Suppress unused parameter warning
    
    cairo_set_font_size(cr, 18);
    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    
    char text[256];
    
    // Score (with multiplier indicator)
    sprintf(text, "SCORE: %d (x%.1f)", game->score, game->score_multiplier);
    cairo_move_to(cr, 20, 30);
    cairo_show_text(cr, text);
    
    // Lives
    sprintf(text, "LIVES: %d", game->ship_lives);
    cairo_move_to(cr, 20, 55);
    cairo_show_text(cr, text);
    
    // Shield status
    sprintf(text, "SHIELD: %d/%d", game->shield_health, game->max_shield_health);
    if (game->shield_health <= 0) {
        cairo_set_source_rgb(cr, 1.0, 0.3, 0.3);  // Red when no shield
    } else if (game->shield_health == 1) {
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);  // Orange when low
    } else {
        cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);  // Cyan when healthy
    }
    cairo_move_to(cr, 20, 105);
    cairo_show_text(cr, text);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  // Reset to white
    
    // Wave
    sprintf(text, "WAVE: %d", game->current_wave);
    cairo_move_to(cr, width - 180, 30);
    cairo_show_text(cr, text);
    
    // Asteroids remaining
    sprintf(text, "ASTEROIDS: %d", game->comet_count);
    cairo_move_to(cr, width - 280, 55);
    cairo_show_text(cr, text);
    
    // Wave progress info (only show if wave is incomplete)
    if (game->wave_complete_timer > 0) {
        sprintf(text, "NEXT WAVE in %.1fs", game->wave_complete_timer);
        cairo_set_font_size(cr, 18);
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);
        cairo_move_to(cr, width / 2 - 160, height / 2 - 50);
        cairo_show_text(cr, text);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_font_size(cr, 18);
    } else if (game->comet_count > 0) {
        int expected_count = comet_buster_get_wave_comet_count(game->current_wave);
        sprintf(text, "DESTROYED: %d/%d", expected_count - game->comet_count, expected_count);
        cairo_set_font_size(cr, 12);
        cairo_move_to(cr, width - 280, 75);
        cairo_show_text(cr, text);
        cairo_set_font_size(cr, 18);
    }
    
    // Render floating text popups
    cairo_set_font_size(cr, 16);
    for (int i = 0; i < game->floating_text_count; i++) {
        FloatingText *ft = &game->floating_texts[i];
        if (ft->active) {
            // Calculate fade (alpha) based on remaining lifetime
            double alpha = ft->lifetime / ft->max_lifetime;
            
            // Set color with fade
            cairo_set_source_rgba(cr, ft->color[0], ft->color[1], ft->color[2], alpha);
            
            // Draw text centered at position
            cairo_move_to(cr, ft->x - 30, ft->y);
            cairo_show_text(cr, ft->text);
        }
    }
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);  // Reset to white
    
    // Fuel bar (bottom left)
    cairo_set_font_size(cr, 14);
    sprintf(text, "ENERGY: %.0f%%", game->energy_amount);
    
    // Color based on fuel level
    if (game->energy_amount < 20) {
        cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);  // Red - critical
    } else if (game->energy_amount < 50) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);  // Yellow - low
    } else {
        cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);  // Green - good
    }
    
    cairo_move_to(cr, 20, height - 40);
    cairo_show_text(cr, text);
    
    // Draw fuel bar (visual indicator)
    double bar_width = 150;
    double bar_height = 12;
    double bar_x = 20;
    double bar_y = height - 25;
    
    // Background (dark)
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Fuel level (colored)
    double fuel_percent = game->energy_amount / game->max_energy;
    if (fuel_percent > 0.5) {
        cairo_set_source_rgb(cr, 0.2, 1.0, 0.2);  // Green
    } else if (fuel_percent > 0.2) {
        cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);  // Yellow
    } else {
        cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);  // Red
    }
    cairo_rectangle(cr, bar_x, bar_y, bar_width * fuel_percent, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
        
    // Missile ammo display (bottom left, above Energy)
    if (game->missile_ammo > 0 || game->using_missiles) {
        cairo_set_font_size(cr, 14);
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);  // Yellow/orange
        char missile_text[32];
        sprintf(missile_text, "MISSILES: %d", game->missile_ammo);
        
        cairo_move_to(cr, 20, height - 110);  // Moved higher (was -70)
        cairo_show_text(cr, missile_text);
        
        // Draw missile bar
        double missile_bar_width = 150;
        double missile_bar_height = 12;
        double missile_bar_x = 20;
        double missile_bar_y = height - 95;  // Moved higher (was -55)
        
        // Background
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_rectangle(cr, missile_bar_x, missile_bar_y, missile_bar_width, missile_bar_height);
        cairo_fill(cr);
        
        // Missile bar (assume max 100 for display, each pickup adds 20)
        double missile_percent = (game->missile_ammo > 100) ? 1.0 : (game->missile_ammo / 100.0);
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.0);  // Yellow
        cairo_rectangle(cr, missile_bar_x, missile_bar_y, missile_bar_width * missile_percent, missile_bar_height);
        cairo_fill(cr);
        
        // Border
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, missile_bar_x, missile_bar_y, missile_bar_width, missile_bar_height);
        cairo_stroke(cr);
    }
    
    // Bomb ammo display
    if (game->bomb_ammo > 0 || game->bomb_count > 0) {
        cairo_set_font_size(cr, 14);
        cairo_set_source_rgb(cr, 1.0, 0.6, 0.0);  // Orange for bombs
        char bomb_text[32];
        sprintf(bomb_text, "BOMBS: %d", game->bomb_ammo);
        
        cairo_move_to(cr, 20, height - 65);  // Below missiles
        cairo_show_text(cr, bomb_text);
        
        // Show active bombs
        if (game->bomb_count > 0) {
            cairo_set_font_size(cr, 12);
            cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);  // Yellow for active
            char active_text[32];
            sprintf(active_text, "Armed: %d", game->bomb_count);
            cairo_move_to(cr, 20, height - 50);
            cairo_show_text(cr, active_text);
        }
        
        // Draw bomb bar
        double bomb_bar_width = 150;
        double bomb_bar_height = 12;
        double bomb_bar_x = 20;
        double bomb_bar_y = game->bomb_count > 0 ? (height - 35) : (height - 50);
        
        // Background
        cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
        cairo_rectangle(cr, bomb_bar_x, bomb_bar_y, bomb_bar_width, bomb_bar_height);
        cairo_fill(cr);
        
        // Bomb bar
        double bomb_percent = (game->bomb_ammo > 10) ? 1.0 : (game->bomb_ammo / 10.0);
        cairo_set_source_rgb(cr, 1.0, 0.6, 0.0);  // Orange
        cairo_rectangle(cr, bomb_bar_x, bomb_bar_y, bomb_bar_width * bomb_percent, bomb_bar_height);
        cairo_fill(cr);
        
        // Border
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, 1.0);
        cairo_rectangle(cr, bomb_bar_x, bomb_bar_y, bomb_bar_width, bomb_bar_height);
        cairo_stroke(cr);
    }
}

// ============================================================================
// MISSILE PICKUP RENDERING
// ============================================================================

void draw_comet_buster_missile_pickups(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->missile_pickup_count; i++) {
        MissilePickup *pickup = &game->missile_pickups[i];
        if (!pickup->active) continue;
        
        double alpha = 1.0;
        if (pickup->lifetime < 2.0) {
            alpha = pickup->lifetime / 2.0;
        }
        
        cairo_save(cr);
        cairo_translate(cr, pickup->x, pickup->y);
        cairo_rotate(cr, pickup->rotation * M_PI / 180.0);
        
        cairo_set_line_width(cr, 2.5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        
        double size = 10.0;
        
        cairo_set_source_rgba(cr, 1.0, 0.65, 0.0, alpha);
        
        cairo_move_to(cr, -size, -size);
        cairo_line_to(cr, size, size);
        cairo_stroke(cr);
        
        cairo_move_to(cr, size, -size);
        cairo_line_to(cr, -size, size);
        cairo_stroke(cr);
        
        cairo_arc(cr, 0, 0, size + 4.0, 0, 2.0 * M_PI);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
        
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, alpha);
        cairo_arc(cr, 0, 0, 2.5, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        cairo_restore(cr);
    }
}

// ============================================================================
// MISSILE RENDERING
// ============================================================================

void draw_comet_buster_missiles(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    
    // Color scheme for each missile type
    // Type 0: CYAN - Target furthest comet
    // Type 1: RED - Target ships and boss
    // Type 2: GREEN - Target closest comets
    // Type 3: MAGENTA - Target comets ~400px away
    // Type 4: ORANGE - Target comets 200-600px away
    
    for (int i = 0; i < game->missile_count; i++) {
        Missile *missile = &game->missiles[i];
        if (!missile->active) continue;
        
        double alpha = 1.0;
        if (missile->lifetime < 0.5) {
            alpha = missile->lifetime / 0.5;
        }
        
        cairo_save(cr);
        cairo_translate(cr, missile->x, missile->y);
        cairo_rotate(cr, missile->angle);  // missile->angle is in radians!
        
        cairo_set_line_width(cr, 1.5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        
        // Determine fill color based on missile type
        double fill_r = 1.0, fill_g = 1.0, fill_b = 0.2;  // Default yellow
        double stroke_r = 1.0, stroke_g = 0.8, stroke_b = 0.0;  // Default orange
        
        switch (missile->missile_type) {
            case 0:  // Furthest comet - CYAN
                fill_r = 0.2; fill_g = 1.0; fill_b = 1.0;
                stroke_r = 0.0; stroke_g = 0.8; stroke_b = 1.0;
                break;
            case 1:  // Ships and boss - RED
                fill_r = 1.0; fill_g = 0.2; fill_b = 0.2;
                stroke_r = 1.0; stroke_g = 0.0; stroke_b = 0.0;
                break;
            case 2:  // Closest comets - GREEN
                fill_r = 0.2; fill_g = 1.0; fill_b = 0.2;
                stroke_r = 0.0; stroke_g = 0.8; stroke_b = 0.0;
                break;
            case 3:  // Comets ~400px away - MAGENTA
                fill_r = 1.0; fill_g = 0.2; fill_b = 1.0;
                stroke_r = 0.8; stroke_g = 0.0; stroke_b = 0.8;
                break;
            case 4:  // Comets 200-600px away - ORANGE
                fill_r = 1.0; fill_g = 0.6; fill_b = 0.0;
                stroke_r = 1.0; stroke_g = 0.4; stroke_b = 0.0;
                break;
        }
        
        // Draw missile body
        cairo_set_source_rgba(cr, fill_r, fill_g, fill_b, alpha);
        cairo_move_to(cr, 8.0, 0);
        cairo_line_to(cr, -4.0, 3.0);
        cairo_line_to(cr, -2.0, 0);
        cairo_line_to(cr, -4.0, -3.0);
        cairo_close_path(cr);
        cairo_fill(cr);
        
        // Draw missile outline
        cairo_set_source_rgba(cr, stroke_r, stroke_g, stroke_b, alpha);
        cairo_move_to(cr, 8.0, 0);
        cairo_line_to(cr, -4.0, 3.0);
        cairo_line_to(cr, -2.0, 0);
        cairo_line_to(cr, -4.0, -3.0);
        cairo_close_path(cr);
        cairo_stroke(cr);
        
        cairo_restore(cr);
    }
}


void draw_comet_buster_game_over(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game || !game->game_over) return;
    
    // Overlay
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.6);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
    
    // Title
    cairo_set_source_rgb(cr, 1.0, 0.3, 0.3);
    cairo_set_font_size(cr, 48);
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_move_to(cr, width / 2 - 150, height / 2 - 80);
    cairo_show_text(cr, "GAME OVER!");
    
    // Score
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_font_size(cr, 24);
    char text[256];
    sprintf(text, "FINAL SCORE: %d", game->score);
    cairo_move_to(cr, width / 2 - 120, height / 2);
    cairo_show_text(cr, text);
    
    // Wave
    sprintf(text, "WAVE REACHED: %d", game->current_wave);
    cairo_move_to(cr, width / 2 - 100, height / 2 + 40);
    cairo_show_text(cr, text);
    
    // Restart prompt - pulsing text
    double pulse = sin(game->game_over_timer * 3) * 0.5 + 0.5;
    cairo_set_source_rgba(cr, 0.0, 1.0, 0.5, pulse);
    cairo_set_font_size(cr, 18);
    cairo_move_to(cr, width / 2 - 100, height / 2 + 100);
    cairo_show_text(cr, "RIGHT CLICK to restart");
}

// ============================================================================
// BURNER/THRUSTER EFFECT RENDERING
// ============================================================================

void draw_ship_burner(cairo_t *cr, double burner_intensity, double length_multiplier) {
    if (burner_intensity <= 0.0) return;
    
    // Flicker effect for more dynamic look
    double flicker_variation = 0.7 + 0.3 * sin(burner_intensity * 20);
    double effective_intensity = burner_intensity * flicker_variation;
    
    // Main burner flame - back of ship (negative X direction)
    double flame_length = 30.0 * effective_intensity * length_multiplier;
    double flame_width = 8.0 * effective_intensity;
    
    // Gradient from yellow/orange to red
    cairo_pattern_t *gradient = cairo_pattern_create_linear(-flame_length, 0, 0, 0);
    
    // Yellow core at the engine
    cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 1.0, 0.0, effective_intensity * 0.8);
    // Orange middle
    cairo_pattern_add_color_stop_rgba(gradient, 0.5, 1.0, 0.7, 0.0, effective_intensity * 0.5);
    // Red tail (fading out)
    cairo_pattern_add_color_stop_rgba(gradient, 1.0, 1.0, 0.2, 0.0, 0.0);
    
    cairo_set_source(cr, gradient);
    
    // Draw main burner as a tapered flame
    cairo_move_to(cr, 0, -flame_width);
    cairo_line_to(cr, -flame_length, -flame_width * 0.3);
    cairo_line_to(cr, -flame_length, flame_width * 0.3);
    cairo_line_to(cr, 0, flame_width);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    cairo_pattern_destroy(gradient);
    
    // Secondary smaller burst effect (more turbulent look)
    if (effective_intensity > 0.3) {
        double secondary_length = flame_length * 0.7;
        double secondary_width = flame_width * 0.5;
        
        cairo_pattern_t *gradient2 = cairo_pattern_create_linear(-secondary_length, 0, 0, 0);
        cairo_pattern_add_color_stop_rgba(gradient2, 0.0, 1.0, 0.5, 0.0, effective_intensity * 0.6);
        cairo_pattern_add_color_stop_rgba(gradient2, 1.0, 1.0, 0.0, 0.0, 0.0);
        
        cairo_set_source(cr, gradient2);
        
        // Offset flame for turbulence
        double offset = 3.0 * sin(burner_intensity * 25);
        cairo_move_to(cr, 0, -secondary_width + offset);
        cairo_line_to(cr, -secondary_length, -secondary_width * 0.2 + offset * 0.5);
        cairo_line_to(cr, -secondary_length, secondary_width * 0.2 + offset * 0.5);
        cairo_line_to(cr, 0, secondary_width + offset);
        cairo_close_path(cr);
        cairo_fill(cr);
        
        cairo_pattern_destroy(gradient2);
    }
}

void draw_enemy_ship_burner(cairo_t *cr, double burner_intensity, double ship_size) {
    if (burner_intensity <= 0.0) return;
    
    // Burner length scales with ship size
    double length_multiplier = ship_size / 12.0;
    
    // Flicker for variation
    double flicker = 0.6 + 0.4 * sin(burner_intensity * 18);
    double effective_intensity = burner_intensity * flicker;
    
    double flame_length = 25.0 * effective_intensity * length_multiplier;
    double flame_width = 6.0 * effective_intensity;
    
    // Red/orange gradient for enemy ships
    cairo_pattern_t *gradient = cairo_pattern_create_linear(-flame_length, 0, 0, 0);
    cairo_pattern_add_color_stop_rgba(gradient, 0.0, 1.0, 0.8, 0.0, effective_intensity * 0.7);
    cairo_pattern_add_color_stop_rgba(gradient, 0.6, 1.0, 0.4, 0.0, effective_intensity * 0.3);
    cairo_pattern_add_color_stop_rgba(gradient, 1.0, 0.8, 0.0, 0.0, 0.0);
    
    cairo_set_source(cr, gradient);
    
    cairo_move_to(cr, 0, -flame_width);
    cairo_line_to(cr, -flame_length, -flame_width * 0.2);
    cairo_line_to(cr, -flame_length, flame_width * 0.2);
    cairo_line_to(cr, 0, flame_width);
    cairo_close_path(cr);
    cairo_fill(cr);
    
    cairo_pattern_destroy(gradient);
}

// ============================================================================
// UFO (FLYING SAUCER) RENDERING
// ============================================================================

void draw_comet_buster_ufos(CometBusterGame *game, cairo_t *cr, int width, int height) {
    if (!game) return;
    (void)width;
    (void)height;
    
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *ufo = &game->ufos[i];
        if (!ufo->active) continue;
        
        cairo_save(cr);
        cairo_translate(cr, ufo->x, ufo->y);
        
        // Set color based on damage state
        if (ufo->damage_flash_timer > 0) {
            // Flash white when hit
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        } else {
            cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);  // Bright cyan
        }
        
        cairo_set_line_width(cr, 1.5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);
        
        // BIGGER DESIGN - Much larger UFO!
        
        // Main curved dome/top (smooth arc) - BIGGER!
        double dome_width = 40.0;  // Was 20.0
        double dome_height = 20.0;  // Was 10.0
        
        // Draw top dome - smooth curved arc
        cairo_arc(cr, 0, 0, dome_width/2, M_PI, 2*M_PI);  // Top semicircle
        cairo_stroke(cr);
        
        // Bottom of dome (flat line where portholes attach)
        cairo_move_to(cr, -dome_width/2, 0);
        cairo_line_to(cr, dome_width/2, 0);
        cairo_stroke(cr);
        
        // THREE PORTHOLES on bottom (classic design) - BIGGER!
        double porthole_radius = 3.5;  // Was 2.0
        double porthole_spacing = 14.0;  // Was 8.0
        
        // Left porthole
        cairo_arc(cr, -porthole_spacing, dome_height/2, porthole_radius, 0, 2*M_PI);
        cairo_stroke(cr);
        
        // Center porthole
        cairo_arc(cr, 0, dome_height/2, porthole_radius, 0, 2*M_PI);
        cairo_stroke(cr);
        
        // Right porthole
        cairo_arc(cr, porthole_spacing, dome_height/2, porthole_radius, 0, 2*M_PI);
        cairo_stroke(cr);
        
        // Small bottom fins/extensions (like original Asteroids) - BIGGER!
        double fin_width = 5.0;  // Was 3.0
        double fin_height = 6.0;  // Was 3.0
        
        // Left fin
        cairo_rectangle(cr, -porthole_spacing - fin_width/2, dome_height/2, fin_width, fin_height);
        cairo_stroke(cr);
        
        // Right fin
        cairo_rectangle(cr, porthole_spacing - fin_width/2, dome_height/2, fin_width, fin_height);
        cairo_stroke(cr);
        
        // Draw thruster/burner effect (coming from bottom rear)
        if (ufo->burner_intensity > 0.01) {
            draw_enemy_ship_burner(cr, ufo->burner_intensity * 0.6, 16.0);  // Also bigger
        }
        
        // Draw health indicator (small dots) - BIGGER!
        if (ufo->health < ufo->max_health) {
            for (int h = 0; h < ufo->max_health; h++) {
                if (h < ufo->health) {
                    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);  // Cyan for healthy
                } else {
                    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);  // Dark for missing
                }
                
                double dot_x = -10 + h * 10;
                cairo_arc(cr, dot_x, -dome_height - 8, 2.0, 0, 2*M_PI);  // Bigger dots
                cairo_fill(cr);
            }
        }
        
        cairo_restore(cr);
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

void draw_singularity_boss(CometBusterGame *game, BossShip *boss, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!boss || !boss->active) return;
    
    cairo_save(cr);
    cairo_translate(cr, boss->x, boss->y);
    cairo_rotate(cr, boss->rotation * M_PI / 180.0);
    
    // Determine phase for visual effects
    int phase = boss->phase;
    // double screen_darkness = 0.1 + phase * 0.15;  // Increases with phase (for future use)
    
    // Main black hole body (grows larger in later phases)
    double core_radius = 60.0;
    if (phase >= 2) core_radius = 80.0;
    
    // Dark center
    cairo_set_source_rgb(cr, 0.1, 0.05, 0.15);
    cairo_arc(cr, 0, 0, core_radius, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // Outer event horizon glow (cyan, pulsing)
    double glow_intensity = 0.2 + 0.3 * sin(boss->rotation * M_PI / 180.0 * 0.1);
    cairo_set_source_rgba(cr, 0.2, 0.8, 1.0, glow_intensity);
    cairo_set_line_width(cr, 3.0);
    cairo_arc(cr, 0, 0, core_radius + 15, 0, 2.0 * M_PI);
    cairo_stroke(cr);
    
    // Inner event horizon rings
    for (int i = 1; i <= 3; i++) {
        cairo_set_source_rgba(cr, 0.2, 0.8, 1.0, 0.3 - i * 0.08);
        cairo_set_line_width(cr, 1.5);
        cairo_arc(cr, 0, 0, core_radius - (i * 8), 0, 2.0 * M_PI);
        cairo_stroke(cr);
    }
    
    // Damage flash
    if (boss->damage_flash_timer > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, boss->damage_flash_timer);
        cairo_arc(cr, 0, 0, core_radius + 20, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    // Draw orbiting satellites
    // NOTE: These are visual elements only - they do NOT cause collision damage
    // They orbit the Singularity boss and are purely for visual feedback of the boss's status
    for (int i = 0; i < boss->fragment_count; i++) {
        double angle = (i * 360.0 / boss->fragment_count + boss->rotation) * M_PI / 180.0;
        double orbit_radius = 120.0;
        double sat_x = cos(angle) * orbit_radius;
        double sat_y = sin(angle) * orbit_radius;
        
        // Satellite glow
        cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.5);
        cairo_arc(cr, sat_x, sat_y, 12.0, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Satellite core
        cairo_set_source_rgb(cr, 0.3, 1.0, 1.0);
        cairo_arc(cr, sat_x, sat_y, 8.0, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    cairo_restore(cr);
    
    // Draw health bar
    double bar_width = 140.0;
    double bar_height = 12.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 80.0;
    
    // Background
    cairo_set_source_rgb(cr, 0.1, 0.05, 0.1);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health bar (cyan gradient)
    double health_percent = (double)boss->health / boss->max_health;
    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_percent, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 0.4, 0.4, 0.4);
    cairo_set_line_width(cr, 1.5);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // Phase indicator
    cairo_set_source_rgb(cr, 0.2, 0.8, 1.0);
    cairo_set_font_size(cr, 11);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    
    const char *phase_text = "";

    switch (boss->phase) {
        case 0: phase_text = phase0_text[game->current_language]; break;
        case 1: phase_text = phase1_text[game->current_language]; break;
        case 2: phase_text = phase2_text[game->current_language]; break;
        case 3: phase_text = phase3_text[game->current_language]; break;
        default: phase_text = phase_unknown_text[game->current_language]; break;
    }
    
    cairo_move_to(cr, boss->x - 70, boss->y + 70);
    cairo_show_text(cr, phase_text);
}

void draw_harbinger_boss(CometBusterGame *game, BossShip *boss, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!boss || !boss->active) return;
    
    cairo_save(cr);
    cairo_translate(cr, boss->x, boss->y);
    cairo_rotate(cr, boss->rotation * M_PI / 180.0);
    
    // Main body - dark cosmic entity
    double core_size = 35.0;
    
    // Outer aura (pulsing based on phase)
    double aura_pulse = 0.3 + 0.4 * sin(boss->rotation * M_PI / 180.0 * 0.05);
    if (boss->phase == 2) {
        aura_pulse = 0.7;  // Brighter in frenzy
    }
    
    cairo_set_source_rgba(cr, 0.4 + aura_pulse * 0.3, 0.0, 0.8 + aura_pulse * 0.2, 0.6);
    cairo_arc(cr, 0, 0, core_size + 15, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // Core body - dark purple with angular points
    cairo_set_source_rgb(cr, 0.3, 0.0, 0.7);
    int points = 6;
    for (int i = 0; i < points; i++) {
        double angle = (i * 2.0 * M_PI / points);
        double x = cos(angle) * core_size;
        double y = sin(angle) * core_size;
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 1.0, 0.3, 1.0);  // Magenta outline
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);
    
    // Inner core
    cairo_set_source_rgb(cr, 1.0, 1.0, 0.0);  // Yellow center
    cairo_arc(cr, 0, 0, 8.0, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // Draw laser charging indicator
    if (boss->phase == 1) {
        // Rotating indicator lines
        double laser_angle = boss->laser_angle * M_PI / 180.0;
        cairo_set_source_rgb(cr, 1.0, 0.5, 1.0);  // Light magenta
        cairo_set_line_width(cr, 2.0);
        
        for (int i = 0; i < 4; i++) {
            double angle = laser_angle + (i * M_PI / 2.0);
            double x = cos(angle) * 40.0;
            double y = sin(angle) * 40.0;
            cairo_move_to(cr, 0, 0);
            cairo_line_to(cr, x, y);
            cairo_stroke(cr);
        }
    }
    
    // Gravity well effect (visual ripples in phase 2)
    if (boss->phase == 2 && boss->gravity_well_strength > 0) {
        cairo_set_source_rgba(cr, 0.0, 1.0, 0.8, 0.3);
        cairo_set_line_width(cr, 1.5);
        double ripple_size = 20.0 + 10.0 * sin(boss->rotation * M_PI / 180.0 * 0.1);
        cairo_arc(cr, 0, 0, ripple_size, 0, 2.0 * M_PI);
        cairo_stroke(cr);
    }
    
    // Damage flash
    if (boss->damage_flash_timer > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.2, 0.2, 0.6);
        cairo_arc(cr, 0, 0, core_size, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    cairo_restore(cr);
    
    // Draw health bar
    double bar_width = 120.0;
    double bar_height = 10.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 60.0;
    
    // Background
    cairo_set_source_rgb(cr, 0.2, 0.0, 0.1);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health bar (gradient purple to magenta)
    double health_percent = (double)boss->health / boss->max_health;
    if (health_percent < 0) health_percent = 0;
    
    // Color changes as health decreases
    if (health_percent > 0.5) {
        cairo_set_source_rgb(cr, 1.0, 0.2, 0.8);  // Magenta
    } else if (health_percent > 0.25) {
        cairo_set_source_rgb(cr, 1.0, 0.5, 0.3);  // Orange
    } else {
        cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);  // Red
    }
    
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_percent, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 1.0, 0.3, 1.0);
    cairo_set_line_width(cr, 1.5);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // Phase indicator
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_font_size(cr, 12);
    const char *phase_text = "";

    switch (boss->phase) {
        case 0: phase_text = phase_dormant_text[game->current_language]; break;
        case 1: phase_text = phase_active_text[game->current_language]; break;
        case 2: phase_text = phase_frenzy_text[game->current_language]; break;
    }
    cairo_move_to(cr, boss->x - 25, boss->y + 65);
    cairo_show_text(cr, phase_text);
}

// ============================================================================
// VOID NEXUS RENDERING
// ============================================================================

void draw_void_nexus_boss(CometBusterGame *game, BossShip *boss, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!boss || !boss->active) return;
    
    cairo_save(cr);
    
    if (boss->fragment_count == 0) {
        // Draw main body - crystalline octagon with rotating rings
        cairo_translate(cr, boss->x, boss->y);
        cairo_rotate(cr, boss->rotation * M_PI / 180.0);
        
        // Outer energy ring (pulsing)
        double pulse = 0.5 + 0.3 * sin(boss->rotation * M_PI / 180.0 * 0.1);
        cairo_set_source_rgba(cr, 0.2, 0.8, 1.0, pulse);  // Cyan glow
        cairo_arc(cr, 0, 0, 40.0 + pulse * 5.0, 0, 2.0 * M_PI);
        cairo_stroke_preserve(cr);
        cairo_fill(cr);
        
        // Main crystalline body (octagon)
        cairo_set_source_rgb(cr, 0.3, 0.7, 1.0);  // Bright cyan
        double oct_radius = 30.0;
        for (int i = 0; i < 8; i++) {
            double angle = (i * 2.0 * M_PI / 8.0) + (boss->rotation * M_PI / 180.0);
            double x = cos(angle) * oct_radius;
            double y = sin(angle) * oct_radius;
            
            if (i == 0) {
                cairo_move_to(cr, x, y);
            } else {
                cairo_line_to(cr, x, y);
            }
        }
        cairo_close_path(cr);
        cairo_fill_preserve(cr);
        cairo_stroke(cr);
        
        // Core nucleus (bright white)
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_arc(cr, 0, 0, 6.0, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Damage flash
        if (boss->damage_flash_timer > 0) {
            cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.6);
            cairo_arc(cr, 0, 0, 35.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        
    } else {
        // Draw fragments - smaller crystals
        for (int i = 0; i < boss->fragment_count; i++) {
            cairo_translate(cr, boss->fragment_positions[i][0], boss->fragment_positions[i][1]);
            
            double frag_pulse = 0.3 + 0.2 * sin((boss->rotation + i * 45) * M_PI / 180.0 * 0.1);
            
            // Fragment glow
            cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, frag_pulse);
            cairo_arc(cr, 0, 0, 25.0 + frag_pulse * 3.0, 0, 2.0 * M_PI);
            cairo_stroke_preserve(cr);
            
            // Fragment body (hexagon)
            cairo_set_source_rgb(cr, 0.2, 0.9, 1.0);
            double hex_radius = 20.0;
            for (int j = 0; j < 6; j++) {
                double angle = (j * 2.0 * M_PI / 6.0);
                double x = cos(angle) * hex_radius;
                double y = sin(angle) * hex_radius;
                
                if (j == 0) {
                    cairo_move_to(cr, x, y);
                } else {
                    cairo_line_to(cr, x, y);
                }
            }
            cairo_close_path(cr);
            cairo_fill_preserve(cr);
            cairo_stroke(cr);
            
            // Fragment core
            cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_arc(cr, 0, 0, 4.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
            
            // Fragment health indicator
            cairo_set_source_rgb(cr, 1.0, 0.0, 0.0);
            cairo_set_font_size(cr, 8);
            char health_text[8];
            snprintf(health_text, sizeof(health_text), "%d", boss->fragment_health[i]);
            cairo_move_to(cr, -5, 3);
            cairo_show_text(cr, health_text);
            
            cairo_translate(cr, -boss->fragment_positions[i][0], -boss->fragment_positions[i][1]);
        }
    }
    
    cairo_restore(cr);
    
    // Draw health bar
    double bar_width = 100.0;
    double bar_height = 8.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 55.0;
    
    // Background
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health bar (cyan)
    double health_percent = (double)boss->health / boss->max_health;
    if (health_percent < 0) health_percent = 0;
    
    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_percent, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // Phase indicator dialog (below health bar)
    cairo_set_source_rgb(cr, 0.2, 0.8, 1.0);  // Cyan color
    cairo_set_font_size(cr, 12);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    
    const char *phase_text = "";

    switch (boss->phase) {
        case 0: phase_text = phase_stabilizing_text[game->current_language]; break;
        case 1: phase_text = phase_fragmenting_text[game->current_language]; break;
        case 2: phase_text = phase_dispersing_text[game->current_language]; break;
        default: phase_text = phase_unknown_text[game->current_language]; break;
    }
    
    cairo_move_to(cr, boss->x - 40, boss->y + 50);
    cairo_show_text(cr, phase_text);
}

// ============================================================================
// RENDERING
// ============================================================================

void draw_spawn_queen_boss(SpawnQueenBoss *queen, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!queen || !queen->active) return;
    
    cairo_save(cr);
    cairo_translate(cr, queen->x, queen->y);
    cairo_rotate(cr, queen->rotation * M_PI / 180.0);
    
    // Main elliptical body - iridescent magenta
    double major_axis = 70.0;
    double minor_axis = 45.0;
    
    // Body fill
    cairo_set_source_rgb(cr, 0.7, 0.3, 0.8);  // Magenta
    cairo_scale(cr, major_axis, minor_axis);
    cairo_arc(cr, 0, 0, 1.0, 0, 2.0 * M_PI);
    cairo_restore(cr);
    cairo_fill(cr);
    cairo_save(cr);
    
    // Outer metallic ring
    cairo_translate(cr, queen->x, queen->y);
    cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.7);
    cairo_set_line_width(cr, 2.5);
    cairo_scale(cr, major_axis, minor_axis);
    cairo_arc(cr, 0, 0, 1.0, 0, 2.0 * M_PI);
    cairo_restore(cr);
    cairo_stroke(cr);
    
    cairo_save(cr);
    cairo_translate(cr, queen->x, queen->y);
    
    // Spawn ports - 6 around equator
    double port_radius = 6.0;
    double port_orbit = 50.0;
    
    // Determine port colors based on phase
    double port_r, port_g, port_b;
    if (queen->phase == 0) {
        // Phase 0: Red ships - ports glow red
        port_r = 1.0; port_g = 0.2; port_b = 0.2;
    } else if (queen->phase == 1) {
        // Phase 1: Mix - ports glow yellow
        port_r = 1.0; port_g = 0.5; port_b = 0.8;
    } else {
        // Phase 2: Sentinel ships - ports glow magenta
        port_r = 0.8; port_g = 0.3; port_b = 1.0;
    }
    
    // Pulsing glow intensity
    double glow_intensity = 0.5 + 0.5 * sin(queen->spawn_particle_timer * 5.0);
    
    for (int i = 0; i < 6; i++) {
        double angle = 2.0 * M_PI * i / 6.0;
        double px = cos(angle) * port_orbit;
        double py = sin(angle) * port_orbit * 0.6;
        
        // Outer glow
        cairo_set_source_rgba(cr, port_r, port_g, port_b, glow_intensity * 0.5);
        cairo_arc(cr, px, py, port_radius + 4.0, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Port center
        cairo_set_source_rgb(cr, port_r, port_g, port_b);
        cairo_arc(cr, px, py, port_radius, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    cairo_restore(cr);
    
    // Damage flash overlay
    if (queen->damage_flash_timer > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.4);
        cairo_arc(cr, queen->x, queen->y, major_axis, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    // Red core (pulsing)
    double core_size = 12.0 + 3.0 * sin(queen->phase_timer * 3.0);
    cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);
    cairo_arc(cr, queen->x, queen->y, core_size, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // --- HEALTH BAR ---
    double bar_width = 100.0;
    double bar_height = 8.0;
    double bar_x = queen->x - bar_width / 2.0;
    double bar_y = queen->y - 70.0;
    
    // Background
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health fill
    double health_ratio = (double)queen->health / queen->max_health;
    cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);  // Red
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_ratio, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // --- SHIELD BAR ---
    double shield_y = bar_y + bar_height + 2.0;
    
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_rectangle(cr, bar_x, shield_y, bar_width, bar_height);
    cairo_fill(cr);
    
    double shield_ratio = (double)queen->shield_health / queen->max_shield_health;
    cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);  // Cyan
    cairo_rectangle(cr, bar_x, shield_y, bar_width * shield_ratio, bar_height);
    cairo_fill(cr);
    
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, bar_x, shield_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // --- PHASE INDICATOR ---
    const char *phase_text;
    double text_r, text_g, text_b;
    
    if (queen->phase == 0) {
        phase_text = "RECRUITING";
        text_r = 1.0; text_g = 0.5; text_b = 0.0;
    } else if (queen->phase == 1) {
        phase_text = "AGGRESSIVE";
        text_r = 1.0; text_g = 1.0; text_b = 0.0;
    } else {
        phase_text = "DESPERATE!";
        text_r = 1.0; text_g = 0.0; text_b = 0.0;
    }
    
    cairo_set_source_rgb(cr, text_r, text_g, text_b);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 11);
    cairo_move_to(cr, queen->x - 35, queen->y + 75);
    cairo_show_text(cr, phase_text);
}

void draw_comet_buster_boss(BossShip *boss, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!boss) {
        fprintf(stderr, "[DRAW BOSS] ERROR: boss pointer is NULL\n");
        return;
    }
    
    if (!boss->active) {
        // Boss not active - this is normal when no boss spawned
        return;
    }
        
    // Draw the boss (death star) as a large circle with rotating patterns
    cairo_save(cr);
    cairo_translate(cr, boss->x, boss->y);
    cairo_rotate(cr, boss->rotation * M_PI / 180.0);  // Convert degrees to radians
    
    // Main body - large circle
    double body_radius = 35.0;
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.4);  // Dark gray
    cairo_arc(cr, 0, 0, body_radius, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // Highlight if taking damage
    if (boss->damage_flash_timer > 0) {
        cairo_set_source_rgba(cr, 1.0, 0.5, 0.5, 0.7);  // Red flash
        cairo_arc(cr, 0, 0, body_radius, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    // Outer ring - metallic look
    cairo_set_source_rgba(cr, 0.6, 0.6, 0.7, 0.8);
    cairo_set_line_width(cr, 2.5);
    cairo_arc(cr, 0, 0, body_radius, 0, 2.0 * M_PI);
    cairo_stroke(cr);
    
    // Draw rotating pattern on the boss
    cairo_set_line_width(cr, 1.5);
    for (int i = 0; i < 8; i++) {
        double angle = (i * 2.0 * M_PI / 8.0);
        double x1 = cos(angle) * 20.0;
        double y1 = sin(angle) * 20.0;
        double x2 = cos(angle) * 30.0;
        double y2 = sin(angle) * 30.0;
        
        cairo_set_source_rgb(cr, 0.8, 0.8, 0.9);
        cairo_move_to(cr, x1, y1);
        cairo_line_to(cr, x2, y2);
        cairo_stroke(cr);
    }
    
    // Core/center glow (pulsing)
    double core_radius = 8.0;
    cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);  // Red core
    cairo_arc(cr, 0, 0, core_radius, 0, 2.0 * M_PI);
    cairo_fill(cr);
    
    // Inner glow
    cairo_set_source_rgba(cr, 1.0, 0.3, 0.3, 0.6);
    cairo_arc(cr, 0, 0, core_radius + 3.0, 0, 2.0 * M_PI);
    cairo_stroke(cr);
    
    cairo_restore(cr);
    
    // Draw health bar above boss (Red)
    double bar_width = 80.0;
    double bar_height = 6.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 50.0;
    
    // Background (gray)
    cairo_set_source_rgb(cr, 0.3, 0.3, 0.3);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health fill
    double health_ratio = (double)boss->health / boss->max_health;
    cairo_set_source_rgb(cr, 1.0, 0.2, 0.2);  // Red
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_ratio, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // Draw shield if active
    if (boss->shield_active && boss->shield_health > 0) {
        // Shield circle (larger than body)
        double shield_radius = 50.0;
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        
        cairo_save(cr);
        cairo_translate(cr, boss->x, boss->y);
        
        // Outer shield glow (pulsing)
        cairo_set_source_rgba(cr, 0.0, 0.8, 1.0, 0.3 + 0.1 * sin(boss->shield_impact_timer * 10.0));
        cairo_arc(cr, 0, 0, shield_radius, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Shield outline
        cairo_set_source_rgba(cr, 0.0, 1.0, 1.0, 0.8);
        cairo_set_line_width(cr, 2.0);
        cairo_arc(cr, 0, 0, shield_radius, 0, 2.0 * M_PI);
        cairo_stroke(cr);
        
        // Shield segments
        int num_segments = 12;
        for (int i = 0; i < num_segments; i++) {
            if (i < (num_segments * shield_ratio)) {
                double angle = (i * 2.0 * M_PI / num_segments);
                double x1 = cos(angle) * (shield_radius - 3.0);
                double y1 = sin(angle) * (shield_radius - 3.0);
                double x2 = cos(angle) * (shield_radius + 3.0);
                double y2 = sin(angle) * (shield_radius + 3.0);
                
                cairo_set_source_rgb(cr, 0.0, 1.0, 1.0);
                cairo_set_line_width(cr, 1.5);
                cairo_move_to(cr, x1, y1);
                cairo_line_to(cr, x2, y2);
                cairo_stroke(cr);
            }
        }
        
        cairo_restore(cr);
    }
    
    // Draw phase indicator (top left of boss)
    double phase_x = boss->x - 25;
    double phase_y = boss->y - 25;
    
    const char *phase_text;
    double phase_r, phase_g, phase_b;
    
    if (boss->phase == 0) {
        phase_text = "NORMAL";
        phase_r = 1.0; phase_g = 1.0; phase_b = 0.5;  // Yellow
    } else if (boss->phase == 1) {
        phase_text = "SHIELDED";
        phase_r = 0.0; phase_g = 1.0; phase_b = 1.0;  // Cyan
    } else {
        phase_text = "ENRAGED!";
        phase_r = 1.0; phase_g = 0.2; phase_b = 0.2;  // Red
    }
    
    cairo_set_source_rgb(cr, phase_r, phase_g, phase_b);
    cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 10);
    cairo_move_to(cr, phase_x, phase_y);
    cairo_show_text(cr, phase_text);
}

// Render explosion
void boss_explosion_draw(BossExplosion *explosion, cairo_t *cr) {
    if (!explosion || !cr) return;
    
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active) continue;
        
        double alpha = p->glow_intensity;
        
        cairo_save(cr);
        
        if (p->is_radial_line) {
            // Draw radial line with glow effect
            
            // Calculate end point of line
            double end_x = p->x + cos(p->angle) * p->length;
            double end_y = p->y + sin(p->angle) * p->length;
            
            // Draw thick glow line (outer glow)
            cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha * 0.3);
            cairo_set_line_width(cr, p->width * 4.0);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_move_to(cr, p->x, p->y);
            cairo_line_to(cr, end_x, end_y);
            cairo_stroke(cr);
            
            // Draw bright inner line
            cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha);
            cairo_set_line_width(cr, p->width);
            cairo_move_to(cr, p->x, p->y);
            cairo_line_to(cr, end_x, end_y);
            cairo_stroke(cr);
            
        } else {
            // Draw glow particle as circle with halo
            
            // Outer glow (larger, more transparent)
            cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha * 0.2);
            cairo_arc(cr, p->x, p->y, 8.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
            
            // Inner bright core
            cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha * 0.8);
            cairo_arc(cr, p->x, p->y, 3.0, 0, 2.0 * M_PI);
            cairo_fill(cr);
        }
        
        cairo_restore(cr);
    }
}

void draw_star_vortex_boss(BossShip *boss, cairo_t *cr, int width, int height) {
    (void)width;
    (void)height;
    
    if (!boss || !boss->active) return;
    
    cairo_save(cr);
    cairo_translate(cr, boss->x, boss->y);
    cairo_rotate(cr, boss->rotation * M_PI / 180.0);
    
    // Draw a 6-pointed star
    int num_points = 6;
    double outer_radius = 50.0;  // Increased from 30.0
    double inner_radius = 25.0;  // Increased from 15.0
    
    // Star color (shifts based on phase)
    double r = 1.0, g = 0.6, b = 0.0;  // Orange by default
    if (boss->phase == 1) {
        r = 1.0; g = 0.3; b = 0.3;  // Red for Phase 1
    } else if (boss->phase == 2) {
        r = 1.0; g = 1.0; b = 0.0;  // Yellow for Phase 2
    }
    
    // Draw the star shape
    cairo_new_path(cr);
    for (int i = 0; i < num_points * 2; i++) {
        double angle = (i * M_PI / num_points);
        double radius = (i % 2 == 0) ? outer_radius : inner_radius;
        double x = cos(angle) * radius;
        double y = sin(angle) * radius;
        
        if (i == 0) {
            cairo_move_to(cr, x, y);
        } else {
            cairo_line_to(cr, x, y);
        }
    }
    cairo_close_path(cr);
    
    // Fill star
    cairo_set_source_rgb(cr, r, g, b);
    cairo_fill_preserve(cr);
    
    // Outline
    cairo_set_source_rgb(cr, r * 0.5, g * 0.5, b * 0.5);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);
    
    // Highlight/damage flash
    if (boss->damage_flash_timer > 0) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, boss->damage_flash_timer);
        cairo_arc(cr, 0, 0, outer_radius * 1.2, 0, 2.0 * M_PI);
        cairo_fill(cr);
    }
    
    // Shield visualization (if active)
    if (boss->shield_active && boss->shield_health > 0) {
        double shield_ratio = (double)boss->shield_health / boss->max_shield_health;
        cairo_set_source_rgba(cr, 0.2, 0.8, 1.0, shield_ratio * 0.6);
        cairo_set_line_width(cr, 3.0);
        cairo_arc(cr, 0, 0, outer_radius + 10.0, 0, 2.0 * M_PI);
        cairo_stroke(cr);
    }
    
    // Draw health indicator text
    cairo_restore(cr);
    
    // Health bar (above boss)
    double bar_width = 60.0;
    double bar_height = 8.0;
    double bar_x = boss->x - bar_width / 2.0;
    double bar_y = boss->y - 50.0;
    
    // Background
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 0.8);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_fill(cr);
    
    // Health fill
    double health_ratio = (double)boss->health / boss->max_health;
    cairo_set_source_rgb(cr, 0.0, 1.0, 0.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width * health_ratio, bar_height);
    cairo_fill(cr);
    
    // Border
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, bar_x, bar_y, bar_width, bar_height);
    cairo_stroke(cr);
    
    // Draw countdown timer during Phase 2
    if (boss->phase == 2) {
        int countdown = 10 - (int)boss->phase_timer;
        if (countdown < 0) countdown = 0;
        if (countdown > 10) countdown = 10;
        
        cairo_select_font_face(cr, "monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(cr, 48.0);
        
        // Countdown color changes: red for most of it, yellow at end
        double r = 1.0, g = 0.0, b = 0.0;  // Red by default
        if (countdown <= 3) {
            r = 1.0; g = 1.0; b = 0.0;  // Yellow for final 3 seconds
        }
        cairo_set_source_rgb(cr, r, g, b);
        
        char countdown_text[16];
        snprintf(countdown_text, sizeof(countdown_text), "%d", countdown);
        
        cairo_text_extents_t extents;
        cairo_text_extents(cr, countdown_text, &extents);
        
        // Center the countdown text above the boss
        double text_x = boss->x - extents.width / 2.0;
        double text_y = boss->y - 80.0;
        
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, countdown_text);
        
        // Add glow effect
        cairo_set_source_rgba(cr, r, g, b, 0.3);
        cairo_set_line_width(cr, 3.0);
        cairo_move_to(cr, text_x, text_y);
        cairo_show_text(cr, countdown_text);
        cairo_stroke(cr);
    }
}

// Not part of zenamp
#ifdef ExternalSound
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
    
    const char *title = wave_complete_text[game->current_language];
    cairo_text_extents(cr, title, &extents);
    cairo_move_to(cr, width/2.0 - extents.width/2.0, 80);
    cairo_show_text(cr, title);
    
    // Victory scroll text - scrolling effect (show 24 lines at a time)
    cairo_set_source_rgb(cr, 0.2, 0.8, 1.0);
    cairo_set_font_size(cr, 15);
    
    int visible_lines = 40;  // Show 24 lines at a time
    int start_line = (game->finale_scroll_line_index > visible_lines) ? 
                     (game->finale_scroll_line_index - visible_lines) : 0;
    
    double y_pos = 150;
    for (int i = start_line; i <= game->finale_scroll_line_index && i < NUM_VICTORY_LINES; i++) {
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
        const char *continue_text = continue_texts[game->current_language];
        cairo_text_extents(cr, continue_text, &extents);
        cairo_move_to(cr, width/2.0 - extents.width/2.0, height - 50);
        cairo_show_text(cr, continue_text);
    }
}
#endif
