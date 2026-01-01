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
              draw_void_nexus_boss(&game->boss, cr, width, height);         // Void Nexus (wave 15, 45, 75, etc)
           } else if (game->current_wave % 30 == 20) {
              draw_harbinger_boss(&game->boss, cr, width, height);          // Harbinger (wave 20, 50, 80, etc)
           } else if (game->current_wave % 30 == 25) {
              draw_star_vortex_boss(&game->boss, cr, width, height);        // Star Vortex (wave 25, 55, 85, etc)
           } else if (game->current_wave % 30 == 0) {
              draw_singularity_boss(&game->boss, cr, width, height);        // Singularity (wave 30, 60, 90, etc)
           }
       }
    }
    
    draw_comet_buster_enemy_bullets(game, cr, width, height);
    draw_comet_buster_canisters(game, cr, width, height);
    draw_comet_buster_missile_pickups(game, cr, width, height);
    draw_comet_buster_missiles(game, cr, width, height);
    draw_comet_buster_particles(game, cr, width, height);
    draw_comet_buster_ship(game, cr, width, height);
    
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
    cairo_set_font_size(cr, 24);
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
    
    // Boost indicator
    if (game->is_boosting && game->boost_thrust_timer > 0) {
        cairo_set_font_size(cr, 16);
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 0.8);
        cairo_move_to(cr, bar_x + bar_width + 20, height - 25);
        cairo_show_text(cr, "⚡ BOOST ⚡");
    }
    
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
