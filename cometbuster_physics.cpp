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

void comet_buster_update_ship(CometBusterGame *game, double dt, int mouse_x, int mouse_y, int width, int height, bool mouse_active) {
    if (game->game_over || !game) return;
    
    if (game->invulnerability_time > 0) {
        game->invulnerability_time -= dt;
    }
    
    // Check if any keyboard movement keys are pressed
    bool keyboard_active = game->keyboard.key_a_pressed || 
                          game->keyboard.key_d_pressed || 
                          game->keyboard.key_w_pressed || 
                          game->keyboard.key_s_pressed;
    
    // Use keyboard if any movement keys pressed, otherwise use mouse
    if (keyboard_active) {
        // KEYBOARD-BASED ARCADE CONTROLS
        // Rotation: A=left, D=right
        double rotation_speed = 6.0;  // Radians per second
        mouse_active=false;
        if (game->keyboard.key_a_pressed) {
            game->ship_angle -= rotation_speed * dt;
        }
        if (game->keyboard.key_d_pressed) {
            game->ship_angle += rotation_speed * dt;
        }
        
        // Normalize angle to [0, 2π)
        while (game->ship_angle < 0) game->ship_angle += 2.0 * M_PI;
        while (game->ship_angle >= 2.0 * M_PI) game->ship_angle -= 2.0 * M_PI;
        
        // Thrust: W=forward, S=backward
        double thrust_accel = 500.0;
        
        if (game->keyboard.key_w_pressed) {
            double thrust_vx = cos(game->ship_angle) * thrust_accel;
            double thrust_vy = sin(game->ship_angle) * thrust_accel;
            
            game->ship_vx += thrust_vx * dt;
            game->ship_vy += thrust_vy * dt;
        }
        
        if (game->keyboard.key_s_pressed) {
            double thrust_vx = cos(game->ship_angle) * thrust_accel;
            double thrust_vy = sin(game->ship_angle) * thrust_accel;
            
            game->ship_vx -= thrust_vx * dt;
            game->ship_vy -= thrust_vy * dt;
        }
    } else if (mouse_active) {
        // MOUSE-BASED CONTROLS (Original system - use mouse to aim)
        // Rotate ship to face mouse
        double dx = mouse_x - game->ship_x;
        double dy = mouse_y - game->ship_y;
        double target_angle = atan2(dy, dx);
        
        double angle_diff = target_angle - game->ship_angle;
        while (angle_diff > M_PI) angle_diff -= 2.0 * M_PI;
        while (angle_diff < -M_PI) angle_diff += 2.0 * M_PI;
        
        double rotation_speed = 5.0;
        if (fabs(angle_diff) > rotation_speed * dt) {
            if (angle_diff > 0) {
                game->ship_angle += rotation_speed * dt;
            } else {
                game->ship_angle -= rotation_speed * dt;
            }
        } else {
            game->ship_angle = target_angle;
        }
        
        // Normal mouse-based movement: based on mouse distance
        double dx_move = mouse_x - game->ship_x;
        double dy_move = mouse_y - game->ship_y;
        double mouse_dist = sqrt(dx_move*dx_move + dy_move*dy_move);
        double max_dist = 400.0;
        
        double acceleration = 1.0;
        if (mouse_dist < 50.0) {
            acceleration = 0.1;
        } else if (mouse_dist > max_dist) {
            acceleration = 2.0;
        } else {
            acceleration = 1.0 + (mouse_dist / max_dist) * 1.5;
        }
        
        double accel_magnitude = acceleration * 200.0;
        
        if (mouse_dist > 0.1) {
            game->ship_vx += (dx_move / mouse_dist) * accel_magnitude * dt;
            game->ship_vy += (dy_move / mouse_dist) * accel_magnitude * dt;
        }
    }
    
    // BOOST: X or SPACE key (works with both control schemes)
    // Boost adds significant forward acceleration
    // Requires at least 2.0 energy to prevent boosting during recharge
    if ((game->keyboard.key_x_pressed || game->keyboard.key_space_pressed) && game->energy_amount >= 2.0) {
        game->is_boosting = true;
        
        // Apply boost forward acceleration in facing direction
        double boost_accel = 800.0;  // Boost acceleration
        double boost_vx = cos(game->ship_angle) * boost_accel;
        double boost_vy = sin(game->ship_angle) * boost_accel;
        
        game->ship_vx += boost_vx * dt;
        game->ship_vy += boost_vy * dt;
    } else if (game->mouse_right_pressed && game->energy_amount >= 2.0) {
        // Right-click boost (mouse) - accelerate in facing direction
        // Requires at least 2.0 energy to prevent boosting during recharge
        game->is_boosting = true;
        
        double boost_accel = 800.0;  // Same as keyboard boost
        double boost_vx = cos(game->ship_angle) * boost_accel;
        double boost_vy = sin(game->ship_angle) * boost_accel;
        
        game->ship_vx += boost_vx * dt;
        game->ship_vy += boost_vy * dt;
    } else {
        game->is_boosting = false;
    }
    
    // Apply friction/drag
    double friction = 0.95;
    game->ship_vx *= friction;
    game->ship_vy *= friction;
    
    // ========== GRAVITY EFFECT (Singularity Boss) ==========
    double max_speed = 400.0;
    if (game->boss_active && game->boss.active && game->current_wave % 30 == 0) {
        // Singularity boss is active - calculate and apply gravity
        BossShip *boss = &game->boss;
        
        // STEP 1: Calculate direction and distance from player to boss
        // This gives us the direction the gravity should pull toward
        double dx = boss->x - game->ship_x;  // Horizontal distance to boss
        double dy = boss->y - game->ship_y;  // Vertical distance to boss
        double dist = sqrt(dx*dx + dy*dy);   // Total distance to boss
        
        if (dist > 0) {  // Avoid division by zero
            // STEP 2: Determine gravity strength based on boss phase
            // Stronger gravity in later phases makes it harder to escape
            double gravity_strength = 100.0;  // Default gravity acceleration
            switch (boss->phase) {
                case 0: 
                    gravity_strength = 100.0;   // GRAVITATIONAL PULL - weak, learning phase
                    break;
                case 1: 
                    gravity_strength = 150.0;   // STELLAR COLLAPSE - medium, pressure building
                    break;
                case 2: 
                    gravity_strength = 200.0;   // VOID EXPANSION - heavy, very hard to move
                    break;
                case 3: 
                    gravity_strength = 300.0;   // SINGULARITY COLLAPSE - maximum, nearly irresistible
                    break;
            }
            
            // STEP 3: Calculate gravitational acceleration vector
            // Normalize the direction (dx/dist, dy/dist) and multiply by gravity strength
            // This creates a force pulling the player toward the boss center
            double grav_vx = (dx / dist) * gravity_strength;
            double grav_vy = (dy / dist) * gravity_strength;
            
            // STEP 4: Apply gravitational force to player velocity
            // The gravity accelerates the player toward the boss each frame
            game->ship_vx += grav_vx * dt;
            game->ship_vy += grav_vy * dt;
            
            // STEP 5: Reduce player maximum speed based on phase
            // This simulates the "crushing weight" of gravity
            // Players move slower as they get deeper into the fight
            double speed_penalty = 1.0;  // Multiplier for max allowed speed (1.0 = no change)
            switch (boss->phase) {
                case 0: 
                    speed_penalty = 0.80;   // Phase 0: 20% slower (-20% movement speed)
                    break;
                case 1: 
                    speed_penalty = 0.60;   // Phase 1: 40% slower (-40% movement speed)
                    break;
                case 2: 
                    speed_penalty = 0.40;   // Phase 2: 60% slower (-60% movement speed)
                    break;
                case 3: 
                    speed_penalty = 0.25;   // Phase 3: 75% slower (-75% movement speed, barely mobile)
                    break;
            }
            
            // Apply the speed penalty to the maximum speed
            max_speed *= speed_penalty;
        }
    }
    
    // Apply max velocity cap (respects the reduced max_speed from gravity)
    // This prevents gravity from accelerating the player to infinite speed
    double current_speed = sqrt(game->ship_vx * game->ship_vx + game->ship_vy * game->ship_vy);
    if (current_speed > max_speed) {
        // Clamp velocity magnitude to max_speed while preserving direction
        game->ship_vx = (game->ship_vx / current_speed) * max_speed;
        game->ship_vy = (game->ship_vy / current_speed) * max_speed;
    }
    
    // Update position
    game->ship_x += game->ship_vx * dt;
    game->ship_y += game->ship_vy * dt;
    
    // Wrap
    comet_buster_wrap_position(&game->ship_x, &game->ship_y, width, height);
}

void comet_buster_update_comets(CometBusterGame *game, double dt, int width, int height) {
    if (!game) return;
    
    for (int i = 0; i < game->comet_count; i++) {
        Comet *c = &game->comets[i];
        
        // ========== GRAVITY WELL EFFECT ==========
        // If boss is active and pulling, affect comets within void radius
        if (game->boss_active && game->boss.active && game->boss.gravity_pull_strength > 0) {
            double dx = game->boss.x - c->x;
            double dy = game->boss.y - c->y;
            double dist = sqrt(dx*dx + dy*dy);
            
            // Only apply gravity if comet is within void radius
            if (dist < game->boss.void_radius && dist > 1.0) {
                // Normalized direction toward boss
                double dir_x = dx / dist;
                double dir_y = dy / dist;
                
                // Gravitational acceleration (inverse square law, scaled)
                // Strength decreases with distance squared
                double gravity_accel = (game->boss.gravity_pull_strength * 10000.0) / (dist * dist);
                
                // Cap acceleration to prevent massive velocities near center
                double max_gravity_accel = 500.0;  // Maximum acceleration per frame
                if (gravity_accel > max_gravity_accel) {
                    gravity_accel = max_gravity_accel;
                }
                
                // Apply gravitational acceleration to comet velocity
                c->vx += dir_x * gravity_accel * dt;
                c->vy += dir_y * gravity_accel * dt;
            }
        }
        
        // Update position
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        
        // Update rotation
        c->rotation += c->rotation_speed * dt;
        while (c->rotation > 360) c->rotation -= 360;
        
        // Wrap
        comet_buster_wrap_position(&c->x, &c->y, width, height);
    }
    
    // Check comet-comet collisions
    for (int i = 0; i < game->comet_count; i++) {
        for (int j = i + 1; j < game->comet_count; j++) {
            Comet *c1 = &game->comets[i];
            Comet *c2 = &game->comets[j];
            
            if (!c1->active || !c2->active) continue;
            
            // Check collision distance
            double dx = c2->x - c1->x;
            double dy = c2->y - c1->y;
            double dist = sqrt(dx*dx + dy*dy);
            double min_dist = c1->radius + c2->radius;
            
            if (dist < min_dist) {
                // Collision detected - perform elastic collision physics
                comet_buster_handle_comet_collision(c1, c2, dx, dy, dist, min_dist);
            }
        }
    }
}

void comet_buster_update_bullets(CometBusterGame *game, double dt, int width, int height, void *vis) {
    if (!game) return;
    
    for (int i = 0; i < game->bullet_count; i++) {
        Bullet *b = &game->bullets[i];
        
        // Remove inactive bullets from the list
        if (!b->active) {
            // Swap with last bullet
            if (i != game->bullet_count - 1) {
                game->bullets[i] = game->bullets[game->bullet_count - 1];
            }
            game->bullet_count--;
            i--;
            continue;
        }
        
        // Update lifetime
        b->lifetime -= dt;
        if (b->lifetime <= 0) {
            b->active = false;
            
            // Swap with last bullet
            if (i != game->bullet_count - 1) {
                game->bullets[i] = game->bullets[game->bullet_count - 1];
            }
            game->bullet_count--;
            i--;
            continue;
        }
        
        // Update position
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        
        // Wrap
        comet_buster_wrap_position(&b->x, &b->y, width, height);
        
        // Check collision with comets
        for (int j = 0; j < game->comet_count; j++) {
            Comet *c = &game->comets[j];
            
            if (comet_buster_check_bullet_comet(b, c)) {
                b->active = false;
                comet_buster_destroy_comet(game, j, width, height, vis);
                break;
            }
        }
    }
}

void comet_buster_update_particles(CometBusterGame *game, double dt) {
    if (!game) return;
    
    for (int i = 0; i < game->particle_count; i++) {
        Particle *p = &game->particles[i];
        
        // Update lifetime
        p->lifetime -= dt;
        if (p->lifetime <= 0) {
            p->active = false;
            
            // Swap with last
            if (i != game->particle_count - 1) {
                game->particles[i] = game->particles[game->particle_count - 1];
            }
            game->particle_count--;
            i--;
            continue;
        }
        
        // Update position with gravity
        p->x += p->vx * dt;
        p->y += p->vy * dt;
        p->vy += 100.0 * dt;
    }
}

void comet_buster_update_floating_text(CometBusterGame *game, double dt) {
    if (!game) return;
    
    for (int i = 0; i < game->floating_text_count; i++) {
        FloatingText *ft = &game->floating_texts[i];
        
        // Update lifetime
        ft->lifetime -= dt;
        if (ft->lifetime <= 0) {
            ft->active = false;
            
            // Swap with last
            if (i != game->floating_text_count - 1) {
                game->floating_texts[i] = game->floating_texts[game->floating_text_count - 1];
            }
            game->floating_text_count--;
            i--;
            continue;
        }
        
        // Float upward
        ft->y -= 20.0 * dt;
    }
}

void comet_buster_update_enemy_ships(CometBusterGame *game, double dt, int width, int height, Visualizer *visualizer) {
    if (!game) return;
    
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        
        if (!ship->active) continue;
        
        // Update shield impact timer
        if (ship->shield_impact_timer > 0) {
            ship->shield_impact_timer -= dt;
        }
        
        if (ship->ship_type == 1) {
            // AGGRESSIVE RED SHIP: Chase player with smooth turning
            double dx = game->ship_x - ship->x;
            double dy = game->ship_y - ship->y;
            double dist_to_player = sqrt(dx*dx + dy*dy);
            
            if (dist_to_player > 0.1) {
                // Move toward player at constant speed with smooth turning
                double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                if (base_speed < 1.0) base_speed = 100.0;  // Default speed if not set
                
                double target_vx = (dx / dist_to_player) * base_speed;
                double target_vy = (dy / dist_to_player) * base_speed;
                
                // Smooth turning: red ships turn faster when chasing
                double turn_rate = 0.20;  // Red ships are aggressive, turn quickly
                ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
                ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
                
                // Update angle to face player
                ship->angle = atan2(ship->vy, ship->vx);
            }
        } else if (ship->ship_type == 2) {
            // HUNTER GREEN SHIP: Follow sine wave, but chase player if close
            double dx = game->ship_x - ship->x;
            double dy = game->ship_y - ship->y;
            double dist_to_player = sqrt(dx*dx + dy*dy);
            
            double chase_range = 300.0;  // Switch to chasing if player within 300px
            
            if (dist_to_player < chase_range && dist_to_player > 0.1) {
                // Chase player - with smooth turning
                double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                if (base_speed < 1.0) base_speed = 90.0;
                
                double target_vx = (dx / dist_to_player) * base_speed;
                double target_vy = (dy / dist_to_player) * base_speed;
                
                // Smooth turning: blend current velocity with target direction
                double turn_rate = 0.15;  // 0.0-1.0, higher = snappier, lower = smoother
                ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
                ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
                ship->angle = atan2(ship->vy, ship->vx);
            } else {
                // Update patrol behavior
                ship->patrol_behavior_timer += dt;
                if (ship->patrol_behavior_timer >= ship->patrol_behavior_duration) {
                    // Time to change behavior
                    ship->patrol_behavior_timer = 0.0;
                    ship->patrol_behavior_duration = 2.0 + (rand() % 30) / 10.0;  // 2-5 seconds
                    
                    int behavior_roll = rand() % 100;
                    if (behavior_roll < 70) {
                        ship->patrol_behavior_type = 0;  // 70% straight movement
                    } else if (behavior_roll < 90) {
                        ship->patrol_behavior_type = 1;  // 20% circular movement
                        // Set circle center ahead of current direction
                        double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                        if (base_speed > 0.1) {
                            ship->patrol_circle_center_x = ship->x + (ship->base_vx / base_speed) * 150.0;
                            ship->patrol_circle_center_y = ship->y + (ship->base_vy / base_speed) * 150.0;
                        }
                        ship->patrol_circle_angle = 0.0;
                    } else {
                        ship->patrol_behavior_type = 2;  // 10% sudden direction change
                        // Pick a new random direction
                        double rand_angle = (rand() % 360) * (M_PI / 180.0);
                        double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                        if (base_speed < 1.0) base_speed = 90.0;
                        ship->base_vx = cos(rand_angle) * base_speed;
                        ship->base_vy = sin(rand_angle) * base_speed;
                    }
                }
                
                // Apply patrol behavior
                double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                double target_vx, target_vy;
                
                if (ship->patrol_behavior_type == 0) {
                    // Straight movement with gentle sine wave
                    if (base_speed > 0.1) {
                        double dir_x = ship->base_vx / base_speed;
                        double dir_y = ship->base_vy / base_speed;
                        double perp_x = -dir_y;
                        double perp_y = dir_x;
                        double wave_amplitude = 40.0;
                        double wave_frequency = 1.2;
                        double sine_offset = sin(ship->path_time * wave_frequency * M_PI) * wave_amplitude;
                        target_vx = dir_x * base_speed + perp_x * sine_offset;
                        target_vy = dir_y * base_speed + perp_y * sine_offset;
                    } else {
                        target_vx = ship->vx;
                        target_vy = ship->vy;
                    }
                } else if (ship->patrol_behavior_type == 1) {
                    // Circular movement - move smoothly instead of teleporting
                    ship->patrol_circle_angle += (base_speed / ship->patrol_circle_radius) * dt;
                    
                    // Calculate target position on circle
                    double target_x = ship->patrol_circle_center_x + cos(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                    double target_y = ship->patrol_circle_center_y + sin(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                    
                    // Move toward target instead of teleporting
                    double dx_circle = target_x - ship->x;
                    double dy_circle = target_y - ship->y;
                    double dist_to_target = sqrt(dx_circle*dx_circle + dy_circle*dy_circle);
                    
                    if (dist_to_target > 0.1) {
                        target_vx = (dx_circle / dist_to_target) * base_speed;
                        target_vy = (dy_circle / dist_to_target) * base_speed;
                    } else {
                        target_vx = -sin(ship->patrol_circle_angle) * base_speed;
                        target_vy = cos(ship->patrol_circle_angle) * base_speed;
                    }
                    
                    ship->angle = atan2(target_vy, target_vx);  // Face direction of movement
                } else {
                    // Evasive turns - use base_vx/base_vy for new direction
                    target_vx = ship->base_vx;
                    target_vy = ship->base_vy;
                }
                
                // Smooth turning towards target velocity
                double turn_rate = 0.12;  // Smoother for patrol behaviors
                ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
                ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
                ship->angle = atan2(ship->vy, ship->vx);
            }
        } else if (ship->ship_type == 3) {
            // SENTINEL PURPLE SHIP: Formation-based with occasional coordinated maneuvers
            
            // Update patrol behavior timer
            ship->patrol_behavior_timer += dt;
            if (ship->patrol_behavior_timer >= ship->patrol_behavior_duration) {
                ship->patrol_behavior_timer = 0.0;
                ship->patrol_behavior_duration = 3.0 + (rand() % 30) / 10.0;  // 3-6 seconds (slower changes)
                
                int behavior_roll = rand() % 100;
                if (behavior_roll < 75) {
                    ship->patrol_behavior_type = 0;  // 75% formation maintenance
                } else if (behavior_roll < 90) {
                    ship->patrol_behavior_type = 1;  // 15% coordinated circular movement
                    double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                    if (base_speed > 0.1) {
                        ship->patrol_circle_center_x = ship->x + (ship->base_vx / base_speed) * 120.0;
                        ship->patrol_circle_center_y = ship->y + (ship->base_vy / base_speed) * 120.0;
                    }
                    ship->patrol_circle_angle = 0.0;
                } else {
                    ship->patrol_behavior_type = 2;  // 10% formation-wide direction change
                    double rand_angle = (rand() % 360) * (M_PI / 180.0);
                    double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                    if (base_speed < 1.0) base_speed = 60.0;
                    ship->base_vx = cos(rand_angle) * base_speed;
                    ship->base_vy = sin(rand_angle) * base_speed;
                }
            }
            
            // Find formation center for positioning
            double formation_center_x = ship->x;
            double formation_center_y = ship->y;
            int formation_count = 0;
            
            for (int j = 0; j < game->enemy_ship_count; j++) {
                EnemyShip *other = &game->enemy_ships[j];
                if (other->active && other->ship_type == 3 && other->formation_id == ship->formation_id) {
                    formation_center_x += other->x;
                    formation_center_y += other->y;
                    formation_count++;
                }
            }
            
            if (formation_count > 0) {
                formation_center_x /= formation_count;
                formation_center_y /= formation_count;
            }
            
            double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
            if (base_speed < 1.0) base_speed = 60.0;
            
            double target_vx, target_vy;
            
            if (ship->patrol_behavior_type == 0) {
                // Standard formation movement with gentle spacing correction
                double dx_formation = formation_center_x - ship->x;
                double dy_formation = formation_center_y - ship->y;
                double dist_to_center = sqrt(dx_formation*dx_formation + dy_formation*dy_formation);
                
                double correction_factor = 0.08 * ship->formation_cohesion;
                if (dist_to_center > 100.0 && dist_to_center > 0.1) {
                    target_vx = ship->base_vx * 0.7 + (dx_formation / dist_to_center) * base_speed * correction_factor;
                    target_vy = ship->base_vy * 0.7 + (dy_formation / dist_to_center) * base_speed * correction_factor;
                } else {
                    target_vx = ship->base_vx * 0.7;
                    target_vy = ship->base_vy * 0.7;
                }
            } else if (ship->patrol_behavior_type == 1) {
                // Coordinated circular movement around formation center - move smoothly
                ship->patrol_circle_angle += (base_speed / ship->patrol_circle_radius) * dt;
                
                // Calculate target position on circle
                double target_x = ship->patrol_circle_center_x + cos(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                double target_y = ship->patrol_circle_center_y + sin(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                
                // Move toward target instead of teleporting
                double dx_circle = target_x - ship->x;
                double dy_circle = target_y - ship->y;
                double dist_to_target = sqrt(dx_circle*dx_circle + dy_circle*dy_circle);
                
                if (dist_to_target > 0.1) {
                    target_vx = (dx_circle / dist_to_target) * base_speed * 0.8;
                    target_vy = (dy_circle / dist_to_target) * base_speed * 0.8;
                } else {
                    target_vx = -sin(ship->patrol_circle_angle) * base_speed * 0.8;
                    target_vy = cos(ship->patrol_circle_angle) * base_speed * 0.8;
                }
                
                ship->angle = atan2(target_vy, target_vx);  // Face direction of movement
            } else {
                // Coordinated direction change
                target_vx = ship->base_vx * 0.8;
                target_vy = ship->base_vy * 0.8;
            }
            
            // Smooth turning towards target velocity
            double turn_rate = 0.10;  // Smooth turning for formation flights
            ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
            ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
            ship->angle = atan2(ship->vy, ship->vx);
        } else if (ship->ship_type == 4) {
            // BROWN COAT ELITE BLUE SHIP
            comet_buster_update_brown_coat_ship(game, i, dt, visualizer);
        } else if (ship->ship_type == 5) {
            // JUGGERNAUT: Always chases player, fires rapidly
            double dx = game->ship_x - ship->x;
            double dy = game->ship_y - ship->y;
            double dist_to_player = sqrt(dx*dx + dy*dy);
            
            // Always chase player (no range limit for Juggernaut)
            if (dist_to_player > 0.1) {
                double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                if (base_speed < 1.0) base_speed = 80.0;  // Juggernaut speed
                
                double target_vx = (dx / dist_to_player) * base_speed;
                double target_vy = (dy / dist_to_player) * base_speed;
                
                // Smooth but slow turning for massive ship
                double turn_rate = 0.08;  // Slower turning than other ships
                ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
                ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
                ship->angle = atan2(ship->vy, ship->vx);
            }
        } else {
            // PATROL BLUE SHIP: More dynamic patrol with occasional evasive maneuvers
            ship->patrol_behavior_timer += dt;
            if (ship->patrol_behavior_timer >= ship->patrol_behavior_duration) {
                // Time to change behavior
                ship->patrol_behavior_timer = 0.0;
                ship->patrol_behavior_duration = 2.0 + (rand() % 30) / 10.0;  // 2-5 seconds
                
                int behavior_roll = rand() % 100;
                if (behavior_roll < 60) {
                    ship->patrol_behavior_type = 0;  // 60% straight movement
                } else if (behavior_roll < 80) {
                    ship->patrol_behavior_type = 1;  // 20% circular movement
                    // Set circle center perpendicular to current direction
                    double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                    if (base_speed > 0.1) {
                        double perp_x = -ship->base_vy / base_speed;
                        double perp_y = ship->base_vx / base_speed;
                        ship->patrol_circle_center_x = ship->x + perp_x * 100.0;
                        ship->patrol_circle_center_y = ship->y + perp_y * 100.0;
                    }
                    ship->patrol_circle_angle = 0.0;
                } else {
                    ship->patrol_behavior_type = 2;  // 20% sudden direction change
                    // Pick a new random direction
                    double rand_angle = (rand() % 360) * (M_PI / 180.0);
                    double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
                    if (base_speed < 1.0) base_speed = 80.0;
                    ship->base_vx = cos(rand_angle) * base_speed;
                    ship->base_vy = sin(rand_angle) * base_speed;
                }
            }
            
            // Apply patrol behavior
            double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
            double target_vx, target_vy;
            
            if (ship->patrol_behavior_type == 0) {
                // Straight movement with sine wave oscillation
                if (base_speed > 0.1) {
                    double dir_x = ship->base_vx / base_speed;
                    double dir_y = ship->base_vy / base_speed;
                    double perp_x = -dir_y;
                    double perp_y = dir_x;
                    double wave_amplitude = 50.0;
                    double wave_frequency = 1.5;
                    double sine_offset = sin(ship->path_time * wave_frequency * M_PI) * wave_amplitude;
                    target_vx = dir_x * base_speed + perp_x * sine_offset;
                    target_vy = dir_y * base_speed + perp_y * sine_offset;
                } else {
                    target_vx = ship->vx;
                    target_vy = ship->vy;
                }
                ship->path_time += dt;
            } else if (ship->patrol_behavior_type == 1) {
                // Circular movement - move smoothly along circle, don't teleport
                ship->patrol_circle_angle += (base_speed / ship->patrol_circle_radius) * dt;
                
                // Calculate target position on circle
                double target_x = ship->patrol_circle_center_x + cos(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                double target_y = ship->patrol_circle_center_y + sin(ship->patrol_circle_angle) * ship->patrol_circle_radius;
                
                // Move toward target position instead of teleporting
                double dx_circle = target_x - ship->x;
                double dy_circle = target_y - ship->y;
                double dist_to_target = sqrt(dx_circle*dx_circle + dy_circle*dy_circle);
                
                if (dist_to_target > 0.1) {
                    target_vx = (dx_circle / dist_to_target) * base_speed;
                    target_vy = (dy_circle / dist_to_target) * base_speed;
                } else {
                    target_vx = -sin(ship->patrol_circle_angle) * base_speed;
                    target_vy = cos(ship->patrol_circle_angle) * base_speed;
                }
                
                ship->angle = atan2(target_vy, target_vx);  // Face direction of movement
            } else {
                // Evasive turns
                target_vx = ship->base_vx;
                target_vy = ship->base_vy;
            }
            
            // Smooth turning towards target velocity
            double turn_rate = 0.14;  // Slightly sharper for blue ships, less formation-locked
            ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
            ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
            ship->angle = atan2(ship->vy, ship->vx);
        }
        
        // Emergency collision avoidance (only when VERY close)
        double avoid_x = 0.0;
        double avoid_y = 0.0;
        double max_avoidance = 0.0;
        
        for (int j = 0; j < game->comet_count; j++) {
            Comet *comet = &game->comets[j];
            if (!comet->active) continue;
            
            double dx = ship->x - comet->x;
            double dy = ship->y - comet->y;
            double dist = sqrt(dx*dx + dy*dy);
            
            double collision_radius = 50.0;  // Only emergency dodge when very close
            
            if (dist < collision_radius && dist > 0.1) {
                double strength = (1.0 - (dist / collision_radius)) * 0.3;
                double norm_x = dx / dist;
                double norm_y = dy / dist;
                
                avoid_x += norm_x * strength;
                avoid_y += norm_y * strength;
                max_avoidance = (strength > max_avoidance) ? strength : max_avoidance;
            }
        }
        
        // Apply emergency avoidance only if collision imminent
        if (max_avoidance > 0.1) {
            double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
            if (base_speed < 1.0) base_speed = 100.0;
            
            double blend_factor = 0.2;
            ship->vx = ship->vx * (1.0 - blend_factor) + avoid_x * base_speed * blend_factor;
            ship->vy = ship->vy * (1.0 - blend_factor) + avoid_y * base_speed * blend_factor;
            
            // Normalize to maintain base speed
            double new_speed = sqrt(ship->vx*ship->vx + ship->vy*ship->vy);
            if (new_speed > 0.1) {
                ship->vx = (ship->vx / new_speed) * base_speed;
                ship->vy = (ship->vy / new_speed) * base_speed;
            }
        }
        
        // ========== GRAVITY WELL EFFECT ==========
        // If boss is active and pulling, affect enemy ships within void radius
        if (game->boss_active && game->boss.active && game->boss.gravity_pull_strength > 0) {
            double dx = game->boss.x - ship->x;
            double dy = game->boss.y - ship->y;
            double dist = sqrt(dx*dx + dy*dy);
            
            // Only apply gravity if ship is within void radius
            if (dist < game->boss.void_radius && dist > 1.0) {
                // Normalized direction toward boss
                double dir_x = dx / dist;
                double dir_y = dy / dist;
                
                // Gravitational acceleration (inverse square law, scaled)
                // Strength decreases with distance squared
                double gravity_accel = (game->boss.gravity_pull_strength * 10000.0) / (dist * dist);
                
                // Cap acceleration to prevent massive velocities near center
                double max_gravity_accel = 400.0;  // Slightly lower than comets
                if (gravity_accel > max_gravity_accel) {
                    gravity_accel = max_gravity_accel;
                }
                
                // Apply gravitational acceleration to ship velocity
                ship->vx += dir_x * gravity_accel * dt;
                ship->vy += dir_y * gravity_accel * dt;
            }
        }
        
        // Update position
        ship->x += ship->vx * dt;
        ship->y += ship->vy * dt;
        
        // Remove if goes off screen
        if (ship->x < -50 || ship->x > width + 50 ||
            ship->y < -50 || ship->y > height + 50) {
            ship->active = false;
            
            if (i != game->enemy_ship_count - 1) {
                game->enemy_ships[i] = game->enemy_ships[game->enemy_ship_count - 1];
            }
            game->enemy_ship_count--;
            i--;
            continue;
        }
        
        // Update shooting
        if (ship->ship_type == 1) {
            // RED SHIPS: Shoot at player
            ship->shoot_cooldown -= dt;
            if (ship->shoot_cooldown <= 0) {
                double dx = game->ship_x - ship->x;
                double dy = game->ship_y - ship->y;
                double dist = sqrt(dx*dx + dy*dy);
                
                if (dist > 0.01) {
                    double bullet_speed = 150.0;
                    double vx = (dx / dist) * bullet_speed;
                    double vy = (dy / dist) * bullet_speed;
                    
                    // Pass ship index (i) so bullet knows who fired it
                    comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                    
                    // Play alien fire sound
#ifdef ExternalSound
                    if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                        //audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                    }
#endif
                    
                    // Aggressive ships shoot more frequently
                    ship->shoot_cooldown = 0.3 + (rand() % 50) / 100.0;  // 0.3-0.8 sec (faster)
                }
            }
        } else if (ship->ship_type == 2) {
            // GREEN SHIPS: Shoot at blue ships if close (to provoke them), OR at nearest comet VERY fast, OR at player if close
            double provoke_range = 200.0;  // Range to shoot at blue ships
            double chase_range = 300.0;    // Range to start shooting at player
            double dx_player = game->ship_x - ship->x;
            double dy_player = game->ship_y - ship->y;
            double dist_to_player = sqrt(dx_player*dx_player + dy_player*dy_player);
            
            // Priority 1: Try to shoot at nearby blue ships to provoke them
            bool found_blue_ship = false;
            int nearest_blue_idx = -1;
            double nearest_blue_dist = 1e9;
            
            for (int j = 0; j < game->enemy_ship_count; j++) {
                EnemyShip *target_ship = &game->enemy_ships[j];
                if (!target_ship->active || target_ship->ship_type != 0) continue;  // Only target blue ships (type 0)
                
                double dx = target_ship->x - ship->x;
                double dy = target_ship->y - ship->y;
                double dist = sqrt(dx*dx + dy*dy);
                
                if (dist < provoke_range && dist < nearest_blue_dist) {
                    nearest_blue_dist = dist;
                    nearest_blue_idx = j;
                    found_blue_ship = true;
                }
            }
            
            if (found_blue_ship) {
                // Shoot at the blue ship to provoke it
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    EnemyShip *target = &game->enemy_ships[nearest_blue_idx];
                    double dx = target->x - ship->x;
                    double dy = target->y - ship->y;
                    double dist = sqrt(dx*dx + dy*dy);
                    
                    if (dist > 0.01) {
                        double bullet_speed = 150.0;
                        double vx = (dx / dist) * bullet_speed;
                        double vy = (dy / dist) * bullet_speed;
                        
                        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                        
                        // Play alien fire sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                        }
#endif
                        
                        // Green ships shoot VERY fast when provoking
                        ship->shoot_cooldown = 0.2 + (rand() % 25) / 100.0;  // 0.2-0.45 sec
                    }
                }
            }
            // Priority 2: Check if player is within range
            else if (dist_to_player < chase_range) {
                // Shoot at player
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    if (dist_to_player > 0.01) {
                        double bullet_speed = 150.0;
                        double vx = (dx_player / dist_to_player) * bullet_speed;
                        double vy = (dy_player / dist_to_player) * bullet_speed;
                        
                        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                        
                        // Play alien fire sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                        }
#endif
                        
                        // Green ships shoot VERY fast at player too
                        ship->shoot_cooldown = 0.15 + (rand() % 25) / 100.0;  // 0.15-0.4 sec (very fast!)
                    }
                }
            }
            // Priority 3: Shoot at nearest comet
            else if (game->comet_count > 0) {
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    // Find nearest comet
                    int nearest_comet_idx = -1;
                    double nearest_dist = 1e9;
                    
                    for (int j = 0; j < game->comet_count; j++) {
                        Comet *comet = &game->comets[j];
                        if (!comet->active) continue;
                        
                        double dx = comet->x - ship->x;
                        double dy = comet->y - ship->y;
                        double dist = sqrt(dx*dx + dy*dy);
                        
                        if (dist < nearest_dist) {
                            nearest_dist = dist;
                            nearest_comet_idx = j;
                        }
                    }
                    
                    // Shoot at nearest comet if in range
                    if (nearest_comet_idx >= 0 && nearest_dist < 600.0) {
                        Comet *target = &game->comets[nearest_comet_idx];
                        double dx = target->x - ship->x;
                        double dy = target->y - ship->y;
                        double dist = sqrt(dx*dx + dy*dy);
                        
                        if (dist > 0.01) {
                            double bullet_speed = 150.0;
                            double vx = (dx / dist) * bullet_speed;
                            double vy = (dy / dist) * bullet_speed;
                            
                            comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                            
                            // Play alien fire sound
#ifdef ExternalSound
                            if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                            }
#endif
                            
                            // Green ships shoot VERY fast
                            ship->shoot_cooldown = 0.15 + (rand() % 25) / 100.0;  // 0.15-0.4 sec (very fast!)
                        }
                    } else {
                        // Reload even if no target in range
                        ship->shoot_cooldown = 0.3;
                    }
                }
            }
        } else if (ship->ship_type == 3) {
            // PURPLE SENTINEL SHIPS: Shoot at blue ships if close (to provoke them), OR at nearest comet
            double provoke_range = 200.0;  // Range to shoot at blue ships
            
            // Priority 1: Try to shoot at nearby blue ships to provoke them
            bool found_blue_ship = false;
            int nearest_blue_idx = -1;
            double nearest_blue_dist = 1e9;
            
            for (int j = 0; j < game->enemy_ship_count; j++) {
                EnemyShip *target_ship = &game->enemy_ships[j];
                if (!target_ship->active || target_ship->ship_type != 0) continue;  // Only target blue ships (type 0)
                
                double dx = target_ship->x - ship->x;
                double dy = target_ship->y - ship->y;
                double dist = sqrt(dx*dx + dy*dy);
                
                if (dist < provoke_range && dist < nearest_blue_dist) {
                    nearest_blue_dist = dist;
                    nearest_blue_idx = j;
                    found_blue_ship = true;
                }
            }
            
            if (found_blue_ship) {
                // Shoot at the blue ship to provoke it
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    EnemyShip *target = &game->enemy_ships[nearest_blue_idx];
                    double dx = target->x - ship->x;
                    double dy = target->y - ship->y;
                    double dist = sqrt(dx*dx + dy*dy);
                    
                    if (dist > 0.01) {
                        double bullet_speed = 150.0;
                        double vx = (dx / dist) * bullet_speed;
                        double vy = (dy / dist) * bullet_speed;
                        
                        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                        
                        // Play alien fire sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                        }
#endif
                        
                        // Purple ships shoot fairly fast when provoking
                        ship->shoot_cooldown = 0.4 + (rand() % 30) / 100.0;  // 0.4-0.7 sec
                    }
                }
            }
            // Priority 2: Shoot at nearest comet
            else if (game->comet_count > 0) {
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    // Find nearest comet
                    int nearest_comet_idx = -1;
                    double nearest_dist = 1e9;
                    
                    for (int j = 0; j < game->comet_count; j++) {
                        Comet *comet = &game->comets[j];
                        if (!comet->active) continue;
                        
                        double dx = comet->x - ship->x;
                        double dy = comet->y - ship->y;
                        double dist = sqrt(dx*dx + dy*dy);
                        
                        if (dist < nearest_dist) {
                            nearest_dist = dist;
                            nearest_comet_idx = j;
                        }
                    }
                    
                    // Shoot at nearest comet if in range
                    if (nearest_comet_idx >= 0 && nearest_dist < 600.0) {
                        Comet *target = &game->comets[nearest_comet_idx];
                        double dx = target->x - ship->x;
                        double dy = target->y - ship->y;
                        double dist = sqrt(dx*dx + dy*dy);
                        
                        if (dist > 0.01) {
                            double bullet_speed = 150.0;
                            double vx = (dx / dist) * bullet_speed;
                            double vy = (dy / dist) * bullet_speed;
                            
                            comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                            
                            // Play alien fire sound
#ifdef ExternalSound
                            if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                            }
#endif
                            
                            // Purple ships shoot at moderate speed
                            ship->shoot_cooldown = 0.5 + (rand() % 30) / 100.0;  // 0.5-0.8 sec
                        }
                    } else {
                        // Reload even if no target in range
                        ship->shoot_cooldown = 0.5;
                    }
                }
            }
        } else if (ship->ship_type == 5) {
            // JUGGERNAUT: Fires heat-seeking missiles at player
            ship->shoot_cooldown -= dt;
            if (ship->shoot_cooldown <= 0) {
                double dx = game->ship_x - ship->x;
                double dy = game->ship_y - ship->y;
                double dist = sqrt(dx*dx + dy*dy);
                
                if (dist > 0.01 && game->missile_count < MAX_MISSILES) {
                    // Spawn a heat-seeking missile from the tip of the ship
                    Missile *missile = &game->missiles[game->missile_count];
                    memset(missile, 0, sizeof(Missile));
                    
                    // Spawn from the tip of the ship (36 pixels ahead, since juggernaut is 3x size)
                    double tip_offset = 36.0;
                    missile->x = ship->x + cos(ship->angle) * tip_offset;
                    missile->y = ship->y + sin(ship->angle) * tip_offset;
                    
                    double speed = 200.0;  // Restored original speed
                    missile->vx = (dx / dist) * speed;
                    missile->vy = (dy / dist) * speed;
                    missile->angle = atan2(missile->vy, missile->vx);
                    
                    // Target the player
                    missile->target_x = game->ship_x;
                    missile->target_y = game->ship_y;
                    missile->target_id = -2;  // Special ID for player target
                    missile->has_target = true;
                    
                    missile->lifetime = 8.0;
                    missile->max_lifetime = 8.0;
                    missile->turn_speed = 25.0;  // Slow turning (25 degrees/sec)
                    missile->speed = 200.0;
                    missile->active = true;
                    missile->missile_type = 1;  // Red color (targeting player)
                    missile->owner_ship_id = i;  // Store which ship fired this missile
                    
                    game->missile_count++;
                    
                    // Play missile fire sound
#ifdef ExternalSound
                    if (visualizer && visualizer->audio.sfx_missile && !game->splash_screen_active) {
                        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_missile);
                    }
#endif
                    
                    // Slow fire rate (2.5 seconds)
                    ship->shoot_cooldown = 2.5 + (rand() % 20) / 10.0;  // 2.5-4.5 sec
                }
            }
        } else {
            // BLUE SHIPS: Shoot at nearest UFO first, then nearest comet
            Comet *target_comet = NULL;
            UFO *target_ufo = NULL;
            double nearest_dist = 1e9;
            
            // Check for nearest UFO (higher priority!)
            if (game->ufo_count > 0) {
                for (int j = 0; j < game->ufo_count; j++) {
                    UFO *ufo = &game->ufos[j];
                    if (!ufo->active) continue;
                    
                    double dx = ufo->x - ship->x;
                    double dy = ufo->y - ship->y;
                    double dist = sqrt(dx*dx + dy*dy);
                    
                    if (dist < nearest_dist && dist < 500.0) {
                        nearest_dist = dist;
                        target_ufo = ufo;
                        target_comet = NULL;  // UFO takes priority
                    }
                }
            }
            
            // If no UFO in range, check for nearest comet
            if (target_ufo == NULL && game->comet_count > 0) {
                nearest_dist = 1e9;
                for (int j = 0; j < game->comet_count; j++) {
                    Comet *comet = &game->comets[j];
                    if (!comet->active) continue;
                    
                    double dx = comet->x - ship->x;
                    double dy = comet->y - ship->y;
                    double dist = sqrt(dx*dx + dy*dy);
                    
                    if (dist < nearest_dist && dist < 500.0) {
                        nearest_dist = dist;
                        target_comet = comet;
                    }
                }
            }
            
            // Fire at the target (UFO or comet)
            if (target_ufo != NULL || target_comet != NULL) {
                ship->shoot_cooldown -= dt;
                if (ship->shoot_cooldown <= 0) {
                    double dx, dy, dist;
                    
                    if (target_ufo != NULL) {
                        dx = target_ufo->x - ship->x;
                        dy = target_ufo->y - ship->y;
                        dist = sqrt(dx*dx + dy*dy);
                    } else {
                        dx = target_comet->x - ship->x;
                        dy = target_comet->y - ship->y;
                        dist = sqrt(dx*dx + dy*dy);
                    }
                    
                    if (dist > 0.01) {
                        double bullet_speed = 150.0;
                        double vx = (dx / dist) * bullet_speed;
                        double vy = (dy / dist) * bullet_speed;
                        
                        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, i);
                        
                        // Play alien fire sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_alien_fire && !game->splash_screen_active) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_alien_fire);
                        }
#endif
                        
                        // Blue ships shoot less frequently
                        ship->shoot_cooldown = 0.8 + (rand() % 100) / 100.0;  // 0.8-1.8 sec
                    }
                }
            } else {
                // Reload even if no target in range
                ship->shoot_cooldown = 0.5;
            }
        }
    }
    
    // Spawn new enemy ships randomly
    if (!game->game_over) {
        game->enemy_ship_spawn_timer -= dt;
        if (game->enemy_ship_spawn_timer <= 0) {
            if (game->enemy_ship_count < MAX_ENEMY_SHIPS) {
                comet_buster_spawn_enemy_ship(game, width, height);
            }
            
            // Difficulty-based spawn rate adjustments
            double difficulty_multiplier = 1.0;
            if (game->difficulty == 0) {
                difficulty_multiplier = 1.5;  // Easy: 1.5x slower (fewer aliens)
            } else if (game->difficulty == 2) {
                difficulty_multiplier = 0.65;  // Hard: 0.65x faster (more aliens)
            }
            // Medium (difficulty == 1): multiplier stays 1.0 (normal rate)
            
            game->enemy_ship_spawn_timer = (game->enemy_ship_spawn_rate * difficulty_multiplier) + (rand() % 300) / 100.0;
        }
    }
}

void comet_buster_update_enemy_bullets(CometBusterGame *game, double dt, int width, int height, void *vis) {
    if (!game) return;
    
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        Bullet *b = &game->enemy_bullets[i];
        
        // Update lifetime
        b->lifetime -= dt;
        if (b->lifetime <= 0) {
            b->active = false;
            
            // Swap with last bullet
            if (i != game->enemy_bullet_count - 1) {
                game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
            }
            game->enemy_bullet_count--;
            i--;
            continue;
        }
        
        // Update position
        b->x += b->vx * dt;
        b->y += b->vy * dt;
        
        // Check collision with comets
        for (int j = 0; j < game->comet_count; j++) {
            Comet *c = &game->comets[j];
            if (!c->active) continue;
            
            if (comet_buster_check_bullet_comet(b, c)) {
                comet_buster_destroy_comet(game, j, width, height, vis);
                b->active = false;
                break;
            }
        }
        
        // Skip further checks if bullet was destroyed
        if (!b->active) {
            // Swap with last
            if (i != game->enemy_bullet_count - 1) {
                game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
            }
            game->enemy_bullet_count--;
            i--;
            continue;
        }
        
        // Check collision with boss - impact but no damage
        if (game->boss.active) {
            if (comet_buster_check_bullet_boss(b, &game->boss)) {
                // Enemy bullet hits boss and is destroyed, but boss takes no damage
                b->active = false;
                
                // Optional: spawn small particle effect at impact
                comet_buster_spawn_explosion(game, b->x, b->y, 0, 3);  // Small impact
                
                // Swap with last
                if (i != game->enemy_bullet_count - 1) {
                    game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
                }
                game->enemy_bullet_count--;
                i--;
                continue;
            }
        }
        
        // Check collision with spawn queen - impact but no damage
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            if (comet_buster_check_bullet_spawn_queen(b, &game->spawn_queen)) {
                // Enemy bullet hits spawn queen and is destroyed, but queen takes no damage
                b->active = false;
                
                // Spawn small particle effect at impact
                comet_buster_spawn_explosion(game, b->x, b->y, 0, 3);  // Small impact
                
                // Swap with last
                if (i != game->enemy_bullet_count - 1) {
                    game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
                }
                game->enemy_bullet_count--;
                i--;
                continue;
            }
        }
        
        // Remove if goes off screen
        if (b->x < -50 || b->x > width + 50 ||
            b->y < -50 || b->y > height + 50) {
            b->active = false;
            
            // Swap with last
            if (i != game->enemy_bullet_count - 1) {
                game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
            }
            game->enemy_bullet_count--;
            i--;
            continue;
        }
    }
}

void comet_buster_update_shooting(CometBusterGame *game, double dt, void *vis) {
    if (!game || game->game_over) return;
    
    // Update fire cooldown
    if (game->mouse_fire_cooldown > 0) {
        game->mouse_fire_cooldown -= dt;
    }
    
    // Update omnidirectional fire cooldown
    if (game->omni_fire_cooldown > 0) {
        game->omni_fire_cooldown -= dt;
    }
    
    // Update weapon toggle cooldown
    if (game->weapon_toggle_cooldown > 0) {
        game->weapon_toggle_cooldown -= dt;
    }
        
    // If left mouse button is held down, fire continuously (costs fuel)
    if (game->mouse_left_pressed) {
        if (game->mouse_fire_cooldown <= 0) {
            // Normal fire costs 0.25 fuel per bullet
            if (game->energy_amount >= 0.25) {
                bool was_using_missiles = game->using_missiles;
                comet_buster_spawn_bullet(game, vis);
                game->energy_amount -= 0.25;  // Consume 0.25 fuel
                game->mouse_fire_cooldown = 0.05;  // ~20 bullets per second
                
                // Play fire sound (only for bullets, not missiles - missiles have their own sound)
                if (!was_using_missiles) {
#ifdef ExternalSound
                    Visualizer *visualizer = (Visualizer *)vis;
                    if (visualizer && visualizer->audio.sfx_fire) {
                        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_fire);
                    }
#endif
                }
            }
        }
    }
    
    // CTRL key fire (keyboard-based)
    if (game->keyboard.key_ctrl_pressed) {
        if (game->mouse_fire_cooldown <= 0) {
            // Normal fire costs 0.25 fuel per bullet
            if (game->energy_amount >= 0.25) {
                bool was_using_missiles = game->using_missiles;
                comet_buster_spawn_bullet(game, vis);
                game->energy_amount -= 0.25;  // Consume 0.25 fuel
                game->mouse_fire_cooldown = 0.05;  // ~20 bullets per second
                
                // Play fire sound (only for bullets, not missiles - missiles have their own sound)
                if (!was_using_missiles) {
#ifdef ExternalSound
                    Visualizer *visualizer = (Visualizer *)vis;
                    if (visualizer && visualizer->audio.sfx_fire) {
                        audio_play_sound(&visualizer->audio, visualizer->audio.sfx_fire);
                    }
#endif
                }
            }
        }
    }
    
    // Z key omnidirectional fire (32 directions)
    if (game->keyboard.key_z_pressed) {
        if (game->omni_fire_cooldown <= 0) {
            if (game->energy_amount >= 30) {
                comet_buster_spawn_omnidirectional_fire(game);  // This function handles energy drain
                game->omni_fire_cooldown = 0.3;  // Slower than normal fire
                
                // Play fire sound
#ifdef ExternalSound
                Visualizer *visualizer = (Visualizer *)vis;
                if (visualizer && visualizer->audio.sfx_fire) {
                    audio_play_sound(&visualizer->audio, visualizer->audio.sfx_fire);
                }
#endif
            }
        }
    }
    
    // Q key or scroll wheel to toggle between missiles and bullets
    bool toggle_requested = game->keyboard.key_q_pressed || game->scroll_direction != 0;
    
    if (toggle_requested) {
        if (game->weapon_toggle_cooldown <= 0) {
            if (game->missile_ammo > 0) {
                // Toggle between missiles and bullets
                game->using_missiles = !game->using_missiles;
                game->weapon_toggle_cooldown = 0.3;  // Prevent rapid toggling
                
                // Show floating text below ship indicating weapon change
                const char *weapon_name = game->using_missiles ? "Missiles" : "Bullets";
                double text_color_r, text_color_g, text_color_b;
                if (game->using_missiles) {
                    // Yellow for missiles
                    text_color_r = 1.0;
                    text_color_g = 1.0;
                    text_color_b = 0.0;
                } else {
                    // Cyan for bullets
                    text_color_r = 0.0;
                    text_color_g = 1.0;
                    text_color_b = 1.0;
                }
                comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y + 40, 
                                                weapon_name, text_color_r, text_color_g, text_color_b);
            }
        }
    }
    
    // Update weapon toggle cooldown
    if (game->mouse_middle_pressed) {
        if (game->omni_fire_cooldown <= 0) {
            if (game->energy_amount >= 30) {
                comet_buster_spawn_omnidirectional_fire(game);  // This function handles energy drain
                game->omni_fire_cooldown = 0.3;  // Slower than normal fire
                
                // Play fire sound
#ifdef ExternalSound
                Visualizer *visualizer = (Visualizer *)vis;
                if (visualizer && visualizer->audio.sfx_fire) {
                    audio_play_sound(&visualizer->audio, visualizer->audio.sfx_fire);
                }
#endif
            }
        }
    }
}

void comet_buster_update_fuel(CometBusterGame *game, double dt) {
    if (!game) return;
    
    // Update boost timer for visual effects
    if (game->boost_thrust_timer > 0) {
        game->boost_thrust_timer -= dt;
    }
    
    // Handle fuel burn and recharge
    if (game->is_boosting && game->energy_amount > 0) {
        // Burning fuel while boosting
        game->energy_amount -= game->energy_burn_rate * dt;
        if (game->energy_amount <= 0) {
            game->energy_amount = 0;
            game->is_boosting = false;
        }
    } else if (!game->mouse_left_pressed && !game->keyboard.key_ctrl_pressed) {
        // Recharging fuel when not boosting AND not firing (either mouse or keyboard)
        if (game->energy_amount < game->max_energy) {
            game->energy_amount += game->energy_recharge_rate * dt;
            if (game->energy_amount > game->max_energy) {
                game->energy_amount = game->max_energy;
            }
        }
    }
    
    // Passive missile generation when energy is at max (rate depends on difficulty)
    if (game->energy_amount >= game->max_energy) {
        game->missile_generation_timer += dt;
        
        // Determine generation interval based on difficulty
        // Easy: 5 missiles per second (0.2s interval)
        // Medium: 2.5 missiles per second (0.4s interval)
        // Hard: 1.25 missiles per second (0.8s interval)
        double generation_interval = 0.2;  // Default (easy)
        if (game->difficulty == MEDIUM) {
            generation_interval = 0.4;  // Half speed
        } else if (game->difficulty == HARD) {
            generation_interval = 0.8;  // 1/4 speed
        }
        
        while (game->missile_generation_timer >= generation_interval && game->missile_ammo < 200) {
            // Check if we're going from 0 to 1 missile
            if (game->missile_ammo == 0) {
                game->using_missiles = true;  // Auto-toggle when you get first missile
            }
            game->missile_ammo++;
            game->missile_generation_timer -= generation_interval;
        }
        
        // Stop accumulating timer if at max missiles
        if (game->difficulty == HARD && game->missile_ammo >= 25) {
             game->missile_generation_timer = 0;
        } else if (game->difficulty == MEDIUM && game->missile_ammo >= 100) {
            game->missile_generation_timer = 0;
        } else if (game->difficulty == EASY && game->missile_ammo >= 200) {
            game->missile_generation_timer = 0;
        }        
    } else {
        // Reset timer if energy drops below max
        game->missile_generation_timer = 0;
    }
}

void update_comet_buster(Visualizer *visualizer, double dt) {
    if (!visualizer) return;
    
    CometBusterGame *game = &visualizer->comet_buster;

#ifdef ExternalSound

    // Update joystick hardware state and sync to visualizer fields
    joystick_manager_update(&visualizer->joystick_manager);
    update_visualizer_joystick(visualizer);

    // Handle splash screen
    if (game->splash_screen_active) {
        comet_buster_update_splash_screen(game, dt, visualizer->width, visualizer->height, visualizer);
        
        // Check for input to start game
        if (comet_buster_splash_screen_input_detected(visualizer)) {
            comet_buster_exit_splash_screen(game);
            fprintf(stdout, "[SPLASH] Splash screen exited, game starting\n");
        }
        return;  // Don't update game yet
    }
#endif

    
    int mouse_x = visualizer->mouse_x;
    int mouse_y = visualizer->mouse_y;
    int width = visualizer->width;
    int height = visualizer->height;
    
    // Initialize ship position on first run (resolution-aware)
    static bool first_run = true;
    if (first_run && width > 0 && height > 0) {
        game->ship_x = width / 2.0;
        game->ship_y = height / 2.0;
        first_run = false;
    }
    
    game->mouse_left_pressed = visualizer->mouse_left_pressed;
    game->mouse_right_pressed = visualizer->mouse_right_pressed;
    game->mouse_middle_pressed = visualizer->mouse_middle_pressed;
    game->scroll_direction = visualizer->scroll_direction;  // Transfer scroll wheel input
    
    if (game->scroll_direction != 0) {
        fprintf(stdout, "[GAME] Scroll transferred: direction=%d\n", game->scroll_direction);
    }
    
#ifdef ExternalSound
    // Copy arcade-style keyboard input state from visualizer
    game->keyboard.key_a_pressed = visualizer->key_a_pressed;
    game->keyboard.key_d_pressed = visualizer->key_d_pressed;
    game->keyboard.key_w_pressed = visualizer->key_w_pressed;
    game->keyboard.key_s_pressed = visualizer->key_s_pressed;
    game->keyboard.key_z_pressed = visualizer->key_z_pressed;
    game->keyboard.key_x_pressed = visualizer->key_x_pressed;
    game->keyboard.key_space_pressed = visualizer->key_space_pressed;
    game->keyboard.key_ctrl_pressed = visualizer->key_ctrl_pressed;
    game->keyboard.key_q_pressed = visualizer->key_q_pressed;  // Weapon toggle
    
    // ========== JOYSTICK INPUT ==========
    // Get joystick state
    JoystickState *active_joystick = joystick_manager_get_active(&visualizer->joystick_manager);
    bool joystick_connected = (active_joystick && active_joystick->connected);
    
    // Check if ANY joystick input is active (buttons, sticks, triggers)
    bool joystick_any_input = false;
    if (joystick_connected) {
        // Check stick movement
        if (fabs(active_joystick->axis_x) > 0.3 || fabs(active_joystick->axis_y) > 0.3) {
            joystick_any_input = true;
        }
        // Check triggers
        if (active_joystick->axis_lt > 0.1 || active_joystick->axis_rt > 0.1) {
            joystick_any_input = true;
        }
        // Check any button
        if (active_joystick->button_a || active_joystick->button_b || 
            active_joystick->button_x || active_joystick->button_y ||
            active_joystick->button_lb || active_joystick->button_rb ||
            active_joystick->button_start || active_joystick->button_back ||
            active_joystick->button_left_stick || active_joystick->button_right_stick) {
            joystick_any_input = true;
        }
    }
  
    bool joy_active=false;  
    if (joystick_connected && joystick_any_input) {
         joy_active=true;
        // Left stick movement
         if (active_joystick->axis_x < -0.5) {
            game->keyboard.key_a_pressed = true;  // Turn left
        }
        if (active_joystick->axis_x > 0.5) {
            game->keyboard.key_d_pressed = true;  // Turn right
        }
        if (active_joystick->axis_y > 0.5) {
            game->keyboard.key_w_pressed = true;  // Forward thrust
        }
        if (active_joystick->axis_y < -0.5) {
            game->keyboard.key_s_pressed = true;  // Backward thrust
        }
        
        // Firing (triggers and B button)
        if (active_joystick->axis_lt > 0.3 || 
            active_joystick->axis_rt > 0.3 || 
            active_joystick->button_b) {
            game->keyboard.key_ctrl_pressed = true;  // Fire
        }
        
        // Special abilities (X/LB for boost)
        if (active_joystick->button_x || active_joystick->button_lb) {
            game->keyboard.key_space_pressed = true;  // Boost
        }
        
        // Omnidirectional fire (Y/RB)
        if (active_joystick->button_y || active_joystick->button_rb) {
            game->keyboard.key_z_pressed = true;  // Omnidirectional fire
        }
    }
    // ====================================
    
    // If ANY keyboard movement key is pressed, disable mouse control immediately
    bool keyboard_active = visualizer->key_a_pressed || visualizer->key_d_pressed || 
                          visualizer->key_w_pressed || visualizer->key_s_pressed;
    
    // Disable mouse if ANY joystick input is active
    bool mouse_active = visualizer->mouse_just_moved && !keyboard_active && !joy_active;
    // Update game state
    comet_buster_update_ship(game, dt, mouse_x, mouse_y, width, height, mouse_active);
#else
    comet_buster_update_ship(game, dt, mouse_x, mouse_y, width, height, true);
#endif

    comet_buster_update_comets(game, dt, width, height);
    comet_buster_update_shooting(game, dt, visualizer);  // Uses mouse_left_pressed state
    comet_buster_update_bullets(game, dt, width, height, visualizer);
    comet_buster_update_particles(game, dt);
    comet_buster_update_floating_text(game, dt);  // Update floating text popups
    comet_buster_update_canisters(game, dt);  // Update shield canisters
    comet_buster_update_missile_pickups(game, dt);  // Update missile pickups
    comet_buster_update_missiles(game, dt, width, height);  // Update missiles
    comet_buster_update_fuel(game, dt);  // Update fuel system
    comet_buster_update_burner_effects(game, dt);  // Update thruster/burner effects
    
    // Update shield regeneration
    if (game->shield_health < game->max_shield_health) {
        game->shield_regen_timer += dt;
        if (game->shield_regen_timer >= game->shield_regen_delay) {
            // Shield regeneration active
            double regen_amount = game->shield_regen_rate * dt;
            game->shield_health += regen_amount;
            if (game->shield_health > game->max_shield_health) {
                game->shield_health = game->max_shield_health;
            }
        }
    }
    
    // Update shield impact timer
    if (game->shield_impact_timer > 0) {
        game->shield_impact_timer -= dt;
    }
    
    comet_buster_update_enemy_ships(game, dt, width, height, visualizer);  // Update enemy ships
    comet_buster_update_enemy_bullets(game, dt, width, height, visualizer);  // Update enemy bullets
    comet_buster_update_ufos(game, dt, width, height, visualizer);  // Update UFO flying saucers

    // Check missiles hitting enemy ships
    for (int i = 0; i < game->missile_count; i++) {
        if (!game->missiles[i].active) continue;
        
        Missile *missile = &game->missiles[i];
        
        for (int j = 0; j < game->enemy_ship_count; j++) {
            EnemyShip *ship = &game->enemy_ships[j];
            if (!ship->active) continue;
            
            // Don't let missiles hit the ship that fired them
            if (missile->owner_ship_id == j) continue;
            
            double dx = ship->x - missile->x;
            double dy = ship->y - missile->y;
            double dist = sqrt(dx*dx + dy*dy);
            
            if (dist < 15.0) {
                comet_buster_spawn_explosion(game, missile->x, missile->y, 1, 8);
                missile->active = false;
                
                // Check if this is a blue (patrol) ship that hasn't been provoked yet
                bool was_provoked = comet_buster_hit_enemy_ship_provoke(game, j);
                
                if (!was_provoked) {
                    // Not a blue ship, or already provoked - normal damage system
                    // Missiles do 3 damage to shields first
                    if (ship->shield_health > 0) {
                        ship->shield_health -= 3;
                        if (ship->shield_health < 0) {
                            ship->shield_health = 0;
                        }
                    } else {
                        // Shields depleted - damage health instead
                        ship->health -= 2;
                    }
                    
                    // Only destroy if health reaches 0
                    if (ship->health <= 0) {
                        comet_buster_destroy_enemy_ship(game, j, width, height, visualizer);
                    }
                }
                // If it was provoked, the missile just triggers the provocation but doesn't damage it
                
                break;
            }
        }
    }
    
    // Check missiles hitting comets
    for (int i = 0; i < game->missile_count; i++) {
        if (!game->missiles[i].active) continue;
        
        Missile *missile = &game->missiles[i];
        
        for (int j = 0; j < game->comet_count; j++) {
            Comet *comet = &game->comets[j];
            if (!comet->active) continue;
            
            if (comet_buster_check_missile_comet(missile, comet)) {
                comet_buster_destroy_comet(game, j, width, height, visualizer);
                comet_buster_spawn_explosion(game, missile->x, missile->y, 1, 6);
                missile->active = false;
                break;
            }
        }
    }
    
    // Update boss if active
    if (game->boss_active && game->spawn_queen.active && game->spawn_queen.is_spawn_queen && game->current_wave % 30 == 10) {
        comet_buster_update_spawn_queen(game, dt, width, height);
    } else if (game->boss_active && game->boss.active) {
        // Route to correct boss based on wave
        if (game->current_wave % 30 == 5) {
            comet_buster_update_boss(game, dt, width, height);        // Death Star (wave 5, 35, 65, etc)
        } else if (game->current_wave % 30 == 15) {
            comet_buster_update_void_nexus(game, dt, width, height);  // Void Nexus (wave 15, 45, 75, etc)
        } else if (game->current_wave % 30 == 20) {
            comet_buster_update_harbinger(game, dt, width, height);   // Harbinger (wave 20, 50, 80, etc)
        } else if (game->current_wave % 30 == 25) {
            comet_buster_update_star_vortex(game, dt, width, height); // Star Vortex (wave 25, 55, 85, etc)
        } else if (game->current_wave % 30 == 0) {
            comet_buster_update_singularity(game, dt, width, height); // Singularity (wave 30, 60, 90, etc)
        }
    } 
    
    // Spawn boss on waves 5, 10, 15, 20, etc. (every 5 waves starting at wave 5)
    // But only if the boss hasn't already been defeated this wave (game->wave_complete_timer == 0)
    /*if ((game->current_wave % 15 == 5) && !game->boss_active && game->comet_count == 0 && !game->boss.active && game->wave_complete_timer == 0) {
        fprintf(stdout, "[UPDATE] Conditions met to spawn boss: Wave=%d, BossActive=%d, CometCount=%d\n",
                game->current_wave, game->boss_active, game->comet_count);
        comet_buster_spawn_boss(game, width, height);
    }*/
    
    // Update fuel system
    comet_buster_update_fuel(game, dt);
    
    // Handle wave completion and progression (only if not already counting down)
    // BUT: Don't progress if boss is active (boss must be defeated first)
    if (game->wave_complete_timer <= 0 && !game->boss_active) {
        comet_buster_update_wave_progression(game);
        
        // NEW: Play wave complete sound when timer just started
        if (game->wave_complete_timer > 0) {
#ifdef ExternalSound
            if (visualizer && visualizer->audio.sfx_wave_complete) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_wave_complete);
                fprintf(stdout, "[AUDIO] Playing wave complete sound\n");
            }
#endif
        }
    }
    
    // Handle wave complete timer (delay before next wave spawns)
    // If boss was just killed, spawn next wave after timer regardless of remaining comets
    if (game->wave_complete_timer > 0 && !game->boss_active) {
        game->wave_complete_timer -= dt;
        if (game->wave_complete_timer <= 0) {
            // Timer expired - spawn the next wave
            game->current_wave++;  // Increment wave
            comet_buster_spawn_wave(game, width, height);
            game->wave_complete_timer = 0;
        }
    }
    
    // Check ship-comet collisions
    for (int i = 0; i < game->comet_count; i++) {
        if (comet_buster_check_ship_comet(game, &game->comets[i])) {
            // Destroy the comet when ship hits it
            comet_buster_destroy_comet(game, i, width, height, visualizer);
            // Damage the ship
            comet_buster_on_ship_hit(game, visualizer);
            break;  // Exit loop since we just modified comet_count
        }
    }
    
    // Check ship-UFO collisions (UFO damages ship but doesn't get destroyed)
    for (int i = 0; i < game->ufo_count; i++) {
        if (comet_buster_check_ship_ufo(game, &game->ufos[i])) {
            // UFO damages ship
            comet_buster_on_ship_hit(game, visualizer);
            break;  // Only one collision per frame
        }
    }
    
    // Check ship-canister collisions
    for (int i = 0; i < game->canister_count; i++) {
        if (comet_buster_check_ship_canister(game, &game->canisters[i])) {
            // Collect canister - gain extra shield
            game->shield_health++;
            if (game->shield_health > game->max_shield_health) {
                game->shield_health = game->max_shield_health + 1;  // Can go one over max
            }
            
            // Grant brief invulnerability period when picking up shield
            game->invulnerability_time = 0.3;
            
            // Floating text
            comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y, 
                                           "+SHIELD", 0.0, 1.0, 1.0);  // Cyan
            
            // Play a positive sound effect (reuse shield sound or other happy sound)
#ifdef ExternalSound
            if (visualizer && visualizer->audio.sfx_hit) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
            }
#endif
            
            // Remove canister
            game->canisters[i].active = false;
            break;  // Exit loop
        }
    }
    
    // Check ship-missile pickup collisions
    for (int i = 0; i < game->missile_pickup_count; i++) {
        if (comet_buster_check_ship_missile_pickup(game, &game->missile_pickups[i])) {
            // Auto-toggle to missiles if you had 0 before
            bool had_zero_missiles = (game->missile_ammo == 0);
            
            game->missile_ammo += 20;  // Add 20 missiles (cumulative, not reset)
            
            // Cap at 100 missiles maximum
            if (game->missile_ammo > 100) {
                game->missile_ammo = 100;
            }
            
            // Only auto-toggle if you just went from 0 to having missiles
            if (had_zero_missiles) {
                game->using_missiles = true;
            }
            
            // Grant brief invulnerability period when picking up missiles
            game->invulnerability_time = 0.3;
            
            comet_buster_spawn_floating_text(game, game->ship_x, game->ship_y, 
                                           "+20 MISSILES", 1.0, 0.8, 0.0);
            
#ifdef ExternalSound
            if (visualizer && visualizer->audio.sfx_hit) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
            }
#endif
            
            game->missile_pickups[i].active = false;
            break;
        }
    }
    
    // Check bullet-enemy ship collisions
    for (int i = 0; i < game->enemy_ship_count; i++) {
        for (int j = 0; j < game->bullet_count; j++) {
            if (comet_buster_check_bullet_enemy_ship(&game->bullets[j], &game->enemy_ships[i])) {
                EnemyShip *enemy = &game->enemy_ships[i];
                
                // Check if this is a blue (patrol) ship that hasn't been provoked yet
                bool was_provoked = comet_buster_hit_enemy_ship_provoke(game, i);
                
                if (!was_provoked) {
                    // Not a blue ship, or already provoked - normal damage system
                    // Check if enemy has shield
                    if (enemy->shield_health > 0) {
                        // Shield absorbs the bullet
                        enemy->shield_health--;
                        enemy->shield_impact_angle = atan2(enemy->y - game->bullets[j].y, 
                                                            enemy->x - game->bullets[j].x);
                        enemy->shield_impact_timer = 0.2;
                        
                        // Play alien hit sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                    } else {
                        // Shields are gone - damage health instead
                        enemy->health--;
                        
                        // Play alien hit sound
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                        
                        // Only destroy if health reaches 0
                        if (enemy->health <= 0) {
                            comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                        }
                    }
                }
                // If it was provoked, the bullet just triggers the provocation but doesn't destroy it
                
                game->bullets[j].active = false;  // Bullet is consumed either way
                break;
            }
        }
    }
    
    // Check bullet-UFO collisions
    for (int i = 0; i < game->ufo_count; i++) {
        for (int j = 0; j < game->bullet_count; j++) {
            if (comet_buster_check_bullet_ufo(&game->bullets[j], &game->ufos[i])) {
                UFO *ufo = &game->ufos[i];
                
                // UFO takes damage
                ufo->health--;
                ufo->damage_flash_timer = 0.1;
                
                // Play hit sound
#ifdef ExternalSound
                if (visualizer && visualizer->audio.sfx_hit) {
                    audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                }
#endif
                
                // Destroy if health reaches 0
                if (ufo->health <= 0) {
                    comet_buster_destroy_ufo(game, i, width, height, visualizer);
                }
                
                game->bullets[j].active = false;  // Bullet is consumed
                break;
            }
        }
    }
    
    // Check missile-UFO collisions (UFOs are valid missile targets!)
    for (int i = 0; i < game->ufo_count; i++) {
        for (int j = 0; j < game->missile_count; j++) {
            if (comet_buster_check_missile_ufo(&game->missiles[j], &game->ufos[i])) {
                UFO *ufo = &game->ufos[i];
                
                // UFO takes damage (missiles do more damage than bullets)
                ufo->health -= 2;  // 2 damage per missile vs 1 per bullet
                ufo->damage_flash_timer = 0.15;
                
                // Play hit sound
#ifdef ExternalSound
                if (visualizer && visualizer->audio.sfx_hit) {
                    audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                }
#endif
                
                // Destroy if health reaches 0
                if (ufo->health <= 0) {
                    comet_buster_destroy_ufo(game, i, width, height, visualizer);
                }
                
                game->missiles[j].active = false;  // Missile is consumed
                break;
            }
        }
    }
    
    // Check enemy bullets hitting enemy ships (friendly fire)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *target_ship = &game->enemy_ships[i];
        if (!target_ship->active) continue;
        
        for (int j = 0; j < game->enemy_bullet_count; j++) {
            Bullet *bullet = &game->enemy_bullets[j];
            if (!bullet->active) continue;
            
            // CRITICAL: Skip if bullet came from this same ship
            if (bullet->owner_ship_id == i) continue;
            
            double dx = target_ship->x - bullet->x;
            double dy = target_ship->y - bullet->y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = 15.0;  // Enemy ship collision radius
            
            if (dist < collision_dist) {
                // Try to provoke blue ships first
                bool was_provoked = comet_buster_hit_enemy_ship_provoke(game, i);
                
                if (!was_provoked) {
                    // Bullet destroys on impact
                    bullet->active = false;
                    
                    // Ship takes damage from friendly fire (from OTHER ships)
                    if (target_ship->shield_health > 0) {
                        target_ship->shield_health--;
                        target_ship->shield_impact_angle = atan2(target_ship->y - bullet->y,
                                                                  target_ship->x - bullet->x);
                        target_ship->shield_impact_timer = 0.2;
                    } else {
                        // No shield - ship is destroyed by friendly fire
                        comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                        
                        // Award player points for friendly fire destruction
                        game->score += (int)(150 * game->score_multiplier);
                        break;  // Exit inner loop since we destroyed the ship
                    }
                    break;  // Bullet hit this ship, move to next bullet
                } else {
                    // Blue ship was provoked, bullet destroys on impact
                    bullet->active = false;
                    break;
                }
            }
        }
    }
    
    // Check enemy bullet-ship collisions
    for (int i = 0; i < game->enemy_bullet_count; i++) {
        if (comet_buster_check_enemy_bullet_ship(game, &game->enemy_bullets[i])) {
            fprintf(stdout, "[COLLISION] Enemy bullet hit player ship! Bullet removed.\n");
            comet_buster_on_ship_hit(game, visualizer);
            // Bullet disappears on impact with player ship
            game->enemy_bullets[i].active = false;
            // Swap with last and remove
            if (i != game->enemy_bullet_count - 1) {
                game->enemy_bullets[i] = game->enemy_bullets[game->enemy_bullet_count - 1];
            }
            game->enemy_bullet_count--;
            i--;
            continue;
        }
    }
    
    // Check enemy bullet-UFO collisions (enemy ships can damage UFOs!)
    for (int i = 0; i < game->ufo_count; i++) {
        for (int j = 0; j < game->enemy_bullet_count; j++) {
            if (comet_buster_check_enemy_bullet_ufo(&game->enemy_bullets[j], &game->ufos[i])) {
                UFO *ufo = &game->ufos[i];
                
                // UFO takes damage from enemy bullets
                ufo->health--;
                ufo->damage_flash_timer = 0.1;
                
                // Destroy if health reaches 0
                if (ufo->health <= 0) {
                    comet_buster_destroy_ufo(game, i, width, height, visualizer);
                }
                
                game->enemy_bullets[j].active = false;  // Bullet is consumed
                break;
            }
        }
    }
    
    // Check enemy missiles hitting player
    for (int i = 0; i < game->missile_count; i++) {
        if (!game->missiles[i].active) continue;
        
        Missile *missile = &game->missiles[i];
        
        // Only check if it's targeting the player (from enemy)
        if (missile->target_id != -2) continue;
        
        double dx = game->ship_x - missile->x;
        double dy = game->ship_y - missile->y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Collision radius for player ship
        if (dist < 15.0) {
            fprintf(stdout, "[COLLISION] Enemy missile hit player ship!\n");
            
            // Missiles do same damage as bullets (1 to shield/health)
            comet_buster_on_ship_hit(game, visualizer);
            
            // Missile disappears on impact
            missile->active = false;
            continue;
        }
    }
    
    // Check enemy ship-enemy ship collisions (ships destroy each other on contact)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship1 = &game->enemy_ships[i];
        if (!ship1->active) continue;
        
        for (int j = i + 1; j < game->enemy_ship_count; j++) {
            EnemyShip *ship2 = &game->enemy_ships[j];
            if (!ship2->active) continue;
            
            double dx = ship2->x - ship1->x;
            double dy = ship2->y - ship1->y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = 15.0 + 15.0;  // Both have ~15px radius
            
            if (dist < collision_dist) {
                // Both ships are destroyed
                ship1->active = false;
                ship2->active = false;
                
                // Spawn explosion at midpoint
                double ex = (ship1->x + ship2->x) / 2.0;
                double ey = (ship1->y + ship2->y) / 2.0;
                comet_buster_spawn_explosion(game, ex, ey, 1, 15);  // Mid-frequency explosion
                
                // Award points for mutual destruction
                game->score += (int)(250 * game->score_multiplier);
                
                break;  // Exit inner loop after collision
            }
        }
    }
    
    // Check enemy ship-player ship collisions (both take damage)
    // NOTE: Singularity satellites (blue orbiting balls) are visual only - they don't cause damage
    // The satellites are drawn around the Singularity boss but are not separate collision objects
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *enemy_ship = &game->enemy_ships[i];
        if (!enemy_ship->active) continue;
        
        double dx = game->ship_x - enemy_ship->x;
        double dy = game->ship_y - enemy_ship->y;
        double dist = sqrt(dx*dx + dy*dy);
        double collision_dist = 15.0 + 15.0;  // Both ships have ~15px radius
        
        if (dist < collision_dist) {
            // Damage enemy ship - shields first, then health
            if (enemy_ship->shield_health > 0) {
                enemy_ship->shield_health -= 3;
                if (enemy_ship->shield_health < 0) {
                    enemy_ship->shield_health = 0;
                }
            } else {
                enemy_ship->health--;
                if (enemy_ship->health <= 0) {
                    comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                }
            }
            // Player ship takes damage
            comet_buster_on_ship_hit(game, visualizer);
            break;  // Exit loop since we just modified enemy_ship_count
        }
    }
    
    // Check enemy ship-comet collisions (ships take damage from comets)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        for (int j = 0; j < game->comet_count; j++) {
            EnemyShip *ship = &game->enemy_ships[i];
            Comet *comet = &game->comets[j];
            if (!ship->active || !comet->active) continue;
            
            double dx = ship->x - comet->x;
            double dy = ship->y - comet->y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = 30.0 + comet->radius;  // Ship radius ~ 30px
            
            if (dist < collision_dist) {
                // Check if ship has shield
                if (ship->shield_health > 0) {
                    // Shield absorbs the comet impact
                    ship->shield_health--;
                    ship->shield_impact_angle = atan2(ship->y - comet->y, 
                                                       ship->x - comet->x);
                    ship->shield_impact_timer = 0.2;
                } else {
                    // Shields are down - damage health instead
                    ship->health--;
                    
                    // Only destroy if health reaches 0
                    if (ship->health <= 0) {
                        comet_buster_destroy_enemy_ship(game, i, width, height, visualizer);
                    }
                }
                
                // Comet is destroyed either way
                comet_buster_destroy_comet(game, j, width, height, visualizer);
                break;
            }
        }
    }
    
    // Check player bullets hitting boss (either Spawn Queen or regular boss)
    if (game->boss_active) {
        // Check boss-comet collisions first (comets can damage boss)
        for (int j = 0; j < game->comet_count; j++) {
            Comet *comet = &game->comets[j];
            if (!comet->active) continue;
            
            // Check against regular boss
            if (game->boss.active) {
                BossShip *boss = &game->boss;
                
                // Skip comet damage during Phase 2 (countdown - boss is invulnerable)
                if (boss->phase == 2) continue;
                
                double dx = boss->x - comet->x;
                double dy = boss->y - comet->y;
                double dist = sqrt(dx*dx + dy*dy);
                double collision_dist = 50.0 + comet->radius;  // Boss is big (~50px)
                
                if (dist < collision_dist) {
                    // Boss takes damage from comet collision
                    // Damage scales with comet size
                    int comet_damage = 2;  // Base damage
                    switch (comet->size) {
                        case COMET_SMALL:   comet_damage = 1; break;
                        case COMET_MEDIUM:  comet_damage = 2; break;
                        case COMET_LARGE:   comet_damage = 3; break;
                        case COMET_MEGA:    comet_damage = 4; break;
                        case COMET_SPECIAL: comet_damage = 4; break;
                        default:            comet_damage = 2; break;
                    }
                    
                    if (boss->shield_active && boss->shield_health > 0) {
                        // Shield absorbs damage from smaller comets, reduces damage from larger ones
                        boss->shield_health--;  // Shield always takes 1 hit
                        boss->shield_impact_angle = atan2(boss->y - comet->y,
                                                           boss->x - comet->x);
                        boss->shield_impact_timer = 0.2;
                        
                        // Large comets penetrate shields
                        if (comet->size >= COMET_LARGE) {
                            boss->health -= 1;  // Penetrating damage
                        }
                    } else {
                        // Shield down - full damage scaled by comet size
                        boss->health -= comet_damage;
                    }
                    
                    boss->damage_flash_timer = 0.1;
                    
                    // On splash screen, don't apply collision knockback to boss
                    // The comet is still destroyed normally, but boss doesn't move backwards
                    if (!game->splash_screen_active) {
                        // Apply collision impulse to boss to push it back (normal gameplay)
                        double nx = (boss->x - comet->x) / dist;
                        double ny = (boss->y - comet->y) / dist;
                        boss->vx += nx * comet->radius;  // Push boss away from comet
                        boss->vy += ny * comet->radius;
                    }
                    
                    // Comet is destroyed
                    comet_buster_destroy_comet(game, j, width, height, visualizer);
                    
                    if (boss->health <= 0) {
                        comet_buster_destroy_boss(game, width, height, visualizer);
                    }
                    break;
                }
            }
            // Also check Spawn Queen - NOTE: Spawn Queen is IMMUNE to asteroid damage!
            // She can only be damaged by player bullets
            else if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
                SpawnQueenBoss *queen = &game->spawn_queen;
                double dx = queen->x - comet->x;
                double dy = queen->y - comet->y;
                double dist = sqrt(dx*dx + dy*dy);
                double collision_dist = 60.0 + comet->radius;  // Queen is larger (~60px)
                
                // Spawn Queen is IMMUNE to asteroid damage - asteroids just pass through
                // IMPORTANT: The Spawn Queen (Mothership) cannot be damaged by asteroids,
                // only by direct player gunfire. This is by design - asteroids are her weapon!
                if (dist < collision_dist) {
                    // Simply destroy the comet, but do NOT damage the queen
                    comet_buster_destroy_comet(game, j, width, height, visualizer);
                    break;
                }
            }
        }
        
        // Check Spawn Queen collision first
        if (game->spawn_queen.active && game->spawn_queen.is_spawn_queen) {
            for (int j = 0; j < game->bullet_count; j++) {
                if (comet_buster_check_bullet_spawn_queen(&game->bullets[j], &game->spawn_queen)) {
                    game->bullets[j].active = false;  // Consume bullet
                    game->spawn_queen.damage_flash_timer = 0.1;
                    game->consecutive_hits++;
                    
                    // Shield reduces damage
                    if (game->spawn_queen.shield_health > 0) {
                        game->spawn_queen.shield_health--;
                    } else {
                        // No shield - direct damage
                        game->spawn_queen.health--;
                    }
                    
                    fprintf(stdout, "[COLLISION] Spawn Queen hit! Health: %d, Shield: %d\n", 
                            game->spawn_queen.health, game->spawn_queen.shield_health);
                    
                    if (game->spawn_queen.health <= 0) {
                        comet_buster_destroy_spawn_queen(game, width, height, visualizer);
                    }
                    break;
                }
            }
            
            // Check Spawn Queen-player ship collision
            double dx = game->ship_x - game->spawn_queen.x;
            double dy = game->ship_y - game->spawn_queen.y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = 20.0 + 50.0;  // Player ~20px, Queen ~50px
            
            if (dist < collision_dist) {
                comet_buster_on_ship_hit(game, visualizer);
                
                // Push player away from Spawn Queen to prevent clipping inside
                if (dist > 0.1) {
                    double nx = dx / dist;  // Normal vector
                    double ny = dy / dist;
                    
                    // Push player to safe distance
                    double push_distance = collision_dist + 5.0;  // Extra buffer
                    game->ship_x = game->spawn_queen.x + nx * push_distance;
                    game->ship_y = game->spawn_queen.y + ny * push_distance;
                    
                    // Also apply knockback velocity
                    game->ship_vx = nx * 200.0;  // Push away at 200 px/s
                    game->ship_vy = ny * 200.0;
                }
            }
        }
        // Regular Death Star boss collision
        else if (game->boss.active) {
            for (int j = 0; j < game->bullet_count; j++) {
                if (comet_buster_check_bullet_boss(&game->bullets[j], &game->boss)) {
                    game->bullets[j].active = false;  // Consume bullet
                    
                    // Shield reduces damage but doesn't block it
                    bool shield_active = (game->boss.shield_active && game->boss.shield_health > 0);
                    
                    if (shield_active) {
                        // Shield takes damage and reduces boss damage
                        game->boss.shield_health--;
                        game->boss.shield_impact_timer = 0.2;
                        game->boss.shield_impact_angle = atan2(game->boss.y - game->bullets[j].y,
                                                               game->boss.x - game->bullets[j].x);
                        
                        // Boss still takes reduced damage (50% damage gets through shield)
                        game->boss.health--;  // Still damage the boss
                        game->boss.damage_flash_timer = 0.1;
                        game->consecutive_hits++;
                        
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                    } else {
                        // No shield - full damage
                        game->boss.health -= 2;  // Double damage when no shield
                        if(game->boss.health <0) { game->boss.health = 0; }

                        game->boss.damage_flash_timer = 0.1;
                        game->consecutive_hits++;
                        
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                    }
                    
                    // Don't destroy boss here - let starboss update handle phase transitions
                    break;
                }
            }
            
            // Check missile-boss collisions
            for (int j = 0; j < game->missile_count; j++) {
                // Skip missiles fired by the boss itself (owner_ship_id == -3)
                // Player missiles (-1) and enemy missiles (0+) should hit the boss
                if (game->missiles[j].owner_ship_id == -3) continue;
                
                if (comet_buster_check_missile_boss(&game->missiles[j], &game->boss)) {
                    game->missiles[j].active = false;
                    
                    bool shield_active = (game->boss.shield_active && game->boss.shield_health > 0);
                    
                    if (shield_active) {
                        game->boss.shield_health -= 2;
                        if (game->boss.shield_health < 0) { game->boss.shield_health = 0; }
                        game->boss.shield_impact_timer = 0.2;
                        game->boss.shield_impact_angle = atan2(game->boss.y - game->missiles[j].y,
                                                               game->boss.x - game->missiles[j].x);
                        game->boss.damage_flash_timer = 0.1;
                        game->consecutive_hits++;
                        
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                    } else {
                        game->boss.health -= 4;
                        if(game->boss.health <0) { game->boss.health = 0; }
                        game->boss.damage_flash_timer = 0.1;
                        game->consecutive_hits++;
                        
#ifdef ExternalSound
                        if (visualizer && visualizer->audio.sfx_hit) {
                            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_hit);
                        }
#endif
                    }
                    
                    // Don't destroy boss here - let starboss update handle phase transitions
                    break;
                }
            }
            
            // Check boss-player ship collisions
            double dx = game->ship_x - game->boss.x;
            double dy = game->ship_y - game->boss.y;
            double dist = sqrt(dx*dx + dy*dy);
            double collision_dist = 20.0 + 35.0;  // Player ~20px, Boss ~35px
            
            if (dist < collision_dist) {
                comet_buster_on_ship_hit(game, visualizer);
                
                // Push player away from boss to prevent clipping inside
                if (dist > 0.1) {
                    double nx = dx / dist;  // Normal vector
                    double ny = dy / dist;
                    
                    // Push player to safe distance
                    double push_distance = collision_dist + 5.0;  // Extra buffer
                    game->ship_x = game->boss.x + nx * push_distance;
                    game->ship_y = game->boss.y + ny * push_distance;
                    
                    // Also apply knockback velocity
                    game->ship_vx = nx * 200.0;  // Push away at 200 px/s
                    game->ship_vy = ny * 200.0;
                }
            }
        }
    }
    
    // Update timers
    game->muzzle_flash_timer -= dt;
    game->difficulty_timer -= dt;
    if (game->game_over) {
        game->game_over_timer -= dt;
        
        // Handle right-click restart
        if (game->mouse_right_pressed) {
            comet_buster_reset_game(game);
        }
    }
}

void comet_buster_update_brown_coat_ship(CometBusterGame *game, int ship_index, double dt, Visualizer *visualizer) {
    if (!game || ship_index < 0 || ship_index >= game->enemy_ship_count) return;
    
    EnemyShip *ship = &game->enemy_ships[ship_index];
    if (!ship->active || ship->ship_type != 4) return;
    
    // Update shield impact timer
    if (ship->shield_impact_timer > 0) {
        ship->shield_impact_timer -= dt;
    }
    
    // Brown Coats are aggressive chasers with fast fire rate
    double dx = game->ship_x - ship->x;
    double dy = game->ship_y - ship->y;
    double dist_to_player = sqrt(dx*dx + dy*dy);
    
    // Chase player aggressively (always in pursuit)
    
    if (dist_to_player > 0.1) {
        // Move toward player with smooth turning
        double base_speed = sqrt(ship->base_vx*ship->base_vx + ship->base_vy*ship->base_vy);
        if (base_speed < 1.0) base_speed = 120.0;  // Faster than regular ships
        
        double target_vx = (dx / dist_to_player) * base_speed;
        double target_vy = (dy / dist_to_player) * base_speed;
        
        // Very responsive turning for tactical maneuvering
        double turn_rate = 0.25;  // Faster than all other types
        ship->vx = ship->vx * (1.0 - turn_rate) + target_vx * turn_rate;
        ship->vy = ship->vy * (1.0 - turn_rate) + target_vy * turn_rate;
        
        // Update angle to face target
        ship->angle = atan2(ship->vy, ship->vx);
    }
    
    // Proximity detection for burst attack
    ship->proximity_detection_timer += dt;
    if (ship->proximity_detection_timer >= 0.3) {  // Check every 0.3 seconds
        ship->proximity_detection_timer = 0.0;
        
        // Check if player OR comet is nearby for burst trigger
        bool trigger_burst = false;
        
        // Check player distance
        if (dist_to_player < 250.0) {
            trigger_burst = true;
        }
        
        // Check for nearby comets
        if (!trigger_burst && game->comet_count > 0) {
            for (int j = 0; j < game->comet_count; j++) {
                Comet *comet = &game->comets[j];
                if (!comet->active) continue;
                
                double dx_comet = comet->x - ship->x;
                double dy_comet = comet->y - ship->y;
                double dist_to_comet = sqrt(dx_comet*dx_comet + dy_comet*dy_comet);
                
                if (dist_to_comet < 280.0) {
                    trigger_burst = true;
                    break;
                }
            }
        }
        
        // Fire burst if triggered and cooldown ready
        if (trigger_burst && ship->burst_fire_cooldown <= 0) {
            comet_buster_brown_coat_fire_burst(game, ship_index);
            // Burst cooldown: fires every 2-3 seconds
            ship->burst_fire_cooldown = 2.0 + (rand() % 20) / 10.0;
        }
    }
    
    // Update burst cooldown
    if (ship->burst_fire_cooldown > 0) {
        ship->burst_fire_cooldown -= dt;
    }
    
    // Standard rapid fire (3x faster than other ships)
    ship->shoot_cooldown -= dt;
    if (ship->shoot_cooldown <= 0) {
        comet_buster_brown_coat_standard_fire(game, ship_index, visualizer);
        // Fire rate: every 0.1 seconds (3 shots per 0.3 seconds of other ships)
        ship->shoot_cooldown = 0.1 + (rand() % 10) / 100.0;  // 0.1-0.2 sec
    }
}

// Standard single-target fire for Brown Coats
void comet_buster_brown_coat_standard_fire(CometBusterGame *game, int ship_index, Visualizer *visualizer) {
    if (!game || ship_index < 0 || ship_index >= game->enemy_ship_count) return;
    
    EnemyShip *ship = &game->enemy_ships[ship_index];
    if (!ship->active) return;
    
    double dx = game->ship_x - ship->x;
    double dy = game->ship_y - ship->y;
    double dist = sqrt(dx*dx + dy*dy);
    
    if (dist > 0.01) {
        double bullet_speed = 200.0;
        double vx = (dx / dist) * bullet_speed;
        double vy = (dy / dist) * bullet_speed;
        
        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, ship_index);
        
        // Sound effect (same as other aggressive ships)
        #ifdef ExternalSound
            if (visualizer && visualizer->audio.sfx_fire) {
                audio_play_sound(&visualizer->audio, visualizer->audio.sfx_fire);
            }
        #endif
    }
}

// Omnidirectional burst attack (8 directions)
void comet_buster_brown_coat_fire_burst(CometBusterGame *game, int ship_index) {
    if (!game || ship_index < 0 || ship_index >= game->enemy_ship_count) return;
    
    EnemyShip *ship = &game->enemy_ships[ship_index];
    if (!ship->active) return;
    
    // Fire 8 bullets in omnidirectional pattern
    int num_directions = 8;
    double angle_step = 2.0 * M_PI / num_directions;
    double bullet_speed = 250.0;  // Faster
    
    // Rotate pattern each burst for visual variety
    ship->last_burst_direction = (ship->last_burst_direction + 1) % 4;
    double pattern_offset = ship->last_burst_direction * (M_PI / 4.0);
    
    for (int i = 0; i < num_directions; i++) {
        double angle = pattern_offset + (i * angle_step);
        double vx = cos(angle) * bullet_speed;
        double vy = sin(angle) * bullet_speed;
        
        comet_buster_spawn_enemy_bullet_from_ship(game, ship->x, ship->y, vx, vy, ship_index);
    }
    
    ship->burst_count_this_wave++;
    
    // Sound effect for burst (more dramatic than standard fire)
    #ifdef ExternalSound
    // Burst sound would play here
    #endif
}

// ============================================================================
// CANISTER UPDATE
// ============================================================================

void comet_buster_update_canisters(CometBusterGame *game, double dt) {
    if (!game) return;
    
    for (int i = 0; i < game->canister_count; i++) {
        Canister *c = &game->canisters[i];
        
        if (!c->active) continue;
        
        // Update lifetime
        c->lifetime -= dt;
        if (c->lifetime <= 0) {
            c->active = false;
            continue;
        }
        
        // Update position (drift effect)
        c->x += c->vx * dt;
        c->y += c->vy * dt;
        
        // Update rotation for visual effect
        c->rotation += c->rotation_speed * dt;
        if (c->rotation >= 360.0) {
            c->rotation -= 360.0;
        }
    }
    
    // Remove inactive canisters by swapping with last
    int i = 0;
    while (i < game->canister_count) {
        if (!game->canisters[i].active) {
            // Swap with last
            if (i != game->canister_count - 1) {
                game->canisters[i] = game->canisters[game->canister_count - 1];
            }
            game->canister_count--;
        } else {
            i++;
        }
    }
}

// ============================================================================
// MISSILE SYSTEM FUNCTIONS
// ============================================================================

// Find best target for missile (Priority tiers: Boss > Ships > Comets, with distance as a factor)
// Uses weighted distance: lower score = better target
// Boss: distance * 1.0 (highest priority)
// Ship: distance * 3.0 (medium priority)
// Comet: distance * 10.0 (lowest priority, needs to be very close to win)
struct MissileTarget {
    double score;      // Weighted distance (lower is better)
    int type;          // 1=boss, 2=ship, 3=comet, 0=none
    int index;         // Index in respective array
};

MissileTarget comet_buster_find_best_missile_target(CometBusterGame *game, double x, double y) {
    MissileTarget best = {999999.0, 0, -1};
    
    if (!game) return best;
    
    double ship_angle = game->ship_angle;  // Get ship's facing direction
    
    // Helper function to calculate directional bonus
    // Returns value between 0 (dead ahead) and 1 (to the side)
    auto get_angle_penalty = [](double ship_angle, double target_dx, double target_dy) -> double {
        // Calculate angle to target
        double target_angle = atan2(target_dy, target_dx);
        
        // Calculate difference between ship angle and target angle
        double angle_diff = target_angle - ship_angle;
        
        // Normalize to -PI to PI
        while (angle_diff > M_PI) angle_diff -= 2.0 * M_PI;
        while (angle_diff < -M_PI) angle_diff += 2.0 * M_PI;
        
        // Get absolute angle difference (0 = directly ahead, PI/2 = to the side)
        double abs_diff = fabs(angle_diff);
        
        // Convert to penalty: 0 at center, increases toward sides
        // At 0 degrees (dead ahead): 0.0 bonus
        // At 90 degrees (perpendicular): 1.0 penalty
        // At 180 degrees (behind): 1.0 penalty
        return (abs_diff / M_PI);  // Maps 0->PI to 0->1
    };
    
    // Check boss (highest priority - lowest weight multiplier)
    if (game->boss_active && game->boss.active) {
        double dx = game->boss.x - x;
        double dy = game->boss.y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 1.0 + (angle_penalty * 50.0);  // Add directional bonus
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 1;
            best.index = 0;
        }
    }
    
    // Check enemy ships (medium priority)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        if (!ship->active) continue;
    
    
       // SKIP BLUE SHIPS (type 0) - they're allies, not threats
        if (ship->ship_type == 0) continue;

        double dx = ship->x - x;
        double dy = ship->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 3.0 + (angle_penalty * 50.0);  // Add directional bonus
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 2;
            best.index = i;
        }
    }
    
    // Check comets (lowest priority)
    for (int i = 0; i < game->comet_count; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - x;
        double dy = comet->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 10.0 + (angle_penalty * 50.0);  // Add directional bonus
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 3;
            best.index = i;
        }
    }
    
    // Check UFOs (high priority - they're shooting at you!)
    for (int i = 0; i < game->ufo_count; i++) {
        UFO *ufo = &game->ufos[i];
        if (!ufo->active) continue;
        
        double dx = ufo->x - x;
        double dy = ufo->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 2.0 + (angle_penalty * 50.0);  // High priority - lower multiplier
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 4;  // UFO target type
            best.index = i;
        }
    }
    
    return best;
}

// Find best target for ANTI-ASTEROID missiles (Priority: Comets > Ships > Boss)
// Useful for clearing asteroid fields while still hitting ships if needed
MissileTarget comet_buster_find_best_anti_asteroid_target(CometBusterGame *game, double x, double y) {
    MissileTarget best = {999999.0, 0, -1};
    
    if (!game) return best;
    
    double ship_angle = game->ship_angle;
    
    auto get_angle_penalty = [](double ship_angle, double target_dx, double target_dy) -> double {
        double target_angle = atan2(target_dy, target_dx);
        double angle_diff = target_angle - ship_angle;
        
        while (angle_diff > M_PI) angle_diff -= 2.0 * M_PI;
        while (angle_diff < -M_PI) angle_diff += 2.0 * M_PI;
        
        double abs_diff = fabs(angle_diff);
        return (abs_diff / M_PI);
    };
    
    // Check comets FIRST (highest priority for anti-asteroid)
    for (int i = 0; i < game->comet_count; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - x;
        double dy = comet->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 1.0 + (angle_penalty * 50.0);  // Comet weight = 1.0 (high priority)
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 3;
            best.index = i;
        }
    }
    
    // Check enemy ships (medium priority)
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        if (!ship->active) continue;

       // SKIP BLUE SHIPS (type 0) - they're allies, not threats
        if (ship->ship_type == 0) continue;


        
        double dx = ship->x - x;
        double dy = ship->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 3.0 + (angle_penalty * 50.0);  // Ship weight = 3.0
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 2;
            best.index = i;
        }
    }
    
    // Check boss (lowest priority for anti-asteroid)
    if (game->boss_active && game->boss.active) {
        double dx = game->boss.x - x;
        double dy = game->boss.y - y;
        double dist = sqrt(dx*dx + dy*dy);
        double angle_penalty = get_angle_penalty(ship_angle, dx, dy);
        double weighted_dist = dist * 10.0 + (angle_penalty * 50.0);  // Boss weight = 10.0 (low priority)
        
        if (weighted_dist < best.score) {
            best.score = weighted_dist;
            best.type = 1;
            best.index = 0;
        }
    }
    
    return best;
}

// Find furthest comet within range (for special third missile type)
MissileTarget comet_buster_find_furthest_comet_in_range(CometBusterGame *game, double x, double y) {
    MissileTarget best = {-1.0, 0, -1};  // Use -1 for furthest (we want largest distance)
    
    if (!game) return best;
    
    double max_range = 800.0;  // Max range to consider comets
    
    // Find furthest active comet within range
    for (int i = 0; i < game->comet_count; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - x;
        double dy = comet->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Only consider comets within max range
        if (dist > max_range) continue;
        
        // Update if this is further than our current best (or it's the first one)
        if (dist > best.score) {
            best.score = dist;
            best.type = 3;
            best.index = i;
        }
    }
    
    return best;
}

// Find comet at preferred distance (400px away) - spreads missiles to mid-range targets
MissileTarget comet_buster_find_comet_at_preferred_distance(CometBusterGame *game, double x, double y) {
    MissileTarget best = {999999.0, 0, -1};
    
    if (!game) return best;
    
    double preferred_dist = 400.0;
    double tolerance = 150.0;  // Allow ±150px from preferred distance
    double min_range = preferred_dist - tolerance;
    double max_range = preferred_dist + tolerance;
    
    // Find comet closest to preferred distance
    for (int i = 0; i < game->comet_count; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - x;
        double dy = comet->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Only consider comets in preferred range
        if (dist < min_range || dist > max_range) continue;
        
        // Distance from preferred distance (lower is better)
        double distance_error = fabs(dist - preferred_dist);
        
        if (distance_error < best.score) {
            best.score = distance_error;
            best.type = 3;
            best.index = i;
        }
    }
    
    // If no comet in preferred range, find any comet
    if (best.index == -1) {
        for (int i = 0; i < game->comet_count; i++) {
            Comet *comet = &game->comets[i];
            if (!comet->active) continue;
            
            double dx = comet->x - x;
            double dy = comet->y - y;
            double dist = sqrt(dx*dx + dy*dy);
            double distance_error = fabs(dist - preferred_dist);
            
            if (distance_error < best.score) {
                best.score = distance_error;
                best.type = 3;
                best.index = i;
            }
        }
    }
    
    return best;
}

// Find comet in preferred distance range (200-600px) - wide scatter pattern
MissileTarget comet_buster_find_comet_in_distance_range(CometBusterGame *game, double x, double y) {
    MissileTarget best = {999999.0, 0, -1};
    
    if (!game) return best;
    
    double min_range = 200.0;
    double max_range = 600.0;
    
    // Find closest comet within the preferred range
    for (int i = 0; i < game->comet_count; i++) {
        Comet *comet = &game->comets[i];
        if (!comet->active) continue;
        
        double dx = comet->x - x;
        double dy = comet->y - y;
        double dist = sqrt(dx*dx + dy*dy);
        
        // Only consider comets in range
        if (dist < min_range || dist > max_range) continue;
        
        if (dist < best.score) {
            best.score = dist;
            best.type = 3;
            best.index = i;
        }
    }
    
    // If no comet in preferred range, fall back to any comet
    if (best.index == -1) {
        for (int i = 0; i < game->comet_count; i++) {
            Comet *comet = &game->comets[i];
            if (!comet->active) continue;
            
            double dx = comet->x - x;
            double dy = comet->y - y;
            double dist = sqrt(dx*dx + dy*dy);
            
            if (dist < best.score) {
                best.score = dist;
                best.type = 3;
                best.index = i;
            }
        }
    }
    
    return best;
}

// Fire a heat-seeking missile from the ship
// Missiles cycle through five targeting types based on fire order:
//   % 5 == 1: Ship/Boss priority (closest enemy ship or boss)
//   % 5 == 2: Comet priority (closest comet)
//   % 5 == 3: Preferred distance (comets ~400px away)
//   % 5 == 4: Distance range (comets 200-600px away)
//   % 5 == 0: Furthest comet within range

void comet_buster_fire_missile(CometBusterGame *game, void *vis) {
    if (!game || game->missile_count >= MAX_MISSILES) return;
    if (game->missile_ammo <= 0) return;
    
    int slot = game->missile_count;
    Missile *missile = &game->missiles[slot];
    
    memset(missile, 0, sizeof(Missile));
    
    // Spawn at ship position
    missile->x = game->ship_x;
    missile->y = game->ship_y;
    
    // Initial velocity in ship's facing direction
    missile->angle = game->ship_angle;
    double speed = 250.0;
    missile->vx = cos(game->ship_angle) * speed;
    missile->vy = sin(game->ship_angle) * speed;
    
    // Determine targeting based on missile count (modulo 5)
    MissileTarget target;
    int mod_type = game->missile_count % 5;
    missile->missile_type = mod_type;  // Store the type for rendering
    
    if (mod_type == 1) {
        // Missiles 1, 6, 11, 16... - target ships and boss (priority: Boss > Ships > Comets)
        target = comet_buster_find_best_missile_target(game, game->ship_x, game->ship_y);
    } else if (mod_type == 2) {
        // Missiles 2, 7, 12, 17... - target closest comets (priority: Comets > Ships > Boss)
        target = comet_buster_find_best_anti_asteroid_target(game, game->ship_x, game->ship_y);
    } else if (mod_type == 3) {
        // Missiles 3, 8, 13, 18... - target comets ~400px away
        target = comet_buster_find_comet_at_preferred_distance(game, game->ship_x, game->ship_y);
    } else if (mod_type == 4) {
        // Missiles 4, 9, 14, 19... - target comets 200-600px away
        target = comet_buster_find_comet_in_distance_range(game, game->ship_x, game->ship_y);
    } else {
        // Missiles 0, 5, 10, 15... - target furthest comet within range
        target = comet_buster_find_furthest_comet_in_range(game, game->ship_x, game->ship_y);
    }
    
    if (target.type == 1) {
        // Boss target
        missile->target_x = game->boss.x;
        missile->target_y = game->boss.y;
        missile->target_id = -1;  // -1 means boss
        missile->has_target = true;
    } else if (target.type == 2) {
        // Enemy ship target
        EnemyShip *ship = &game->enemy_ships[target.index];
        missile->target_x = ship->x;
        missile->target_y = ship->y;
        missile->target_id = target.index;
        missile->has_target = true;
    } else if (target.type == 3) {
        // Comet target
        Comet *comet = &game->comets[target.index];
        missile->target_x = comet->x;
        missile->target_y = comet->y;
        missile->target_id = target.index + 1000;  // +1000 to distinguish from ships
        missile->has_target = true;
    } else if (target.type == 4) {
        // UFO target
        UFO *ufo = &game->ufos[target.index];
        missile->target_x = ufo->x;
        missile->target_y = ufo->y;
        missile->target_id = target.index + 2000;  // +2000 to distinguish from ships and comets
        missile->has_target = true;
    } else {
        // No target
        missile->has_target = false;
    }
    
    missile->lifetime = 5.0;
    missile->max_lifetime = 5.0;
    missile->turn_speed = 120.0;
    missile->speed = 250.0;
    missile->active = true;
    missile->owner_ship_id = -1;  // Player fired this missile
    
    game->missile_count++;
    game->missile_ammo--;
    
    // Play missile fire sound
#ifdef ExternalSound
    if (vis) {
        Visualizer *visualizer = (Visualizer *)vis;
        if (visualizer && visualizer->audio.sfx_missile) {
            audio_play_sound(&visualizer->audio, visualizer->audio.sfx_missile);
        }
    }
#endif
    
    if (game->missile_ammo <= 0) {
        game->using_missiles = false;
        game->missile_ammo = 0;
    }
}

// Update all missiles
void comet_buster_update_missiles(CometBusterGame *game, double dt, int width, int height) {
    if (!game) return;
    
    for (int i = 0; i < game->missile_count; i++) {
        Missile *missile = &game->missiles[i];
        if (!missile->active) continue;
        
        missile->lifetime -= dt;
        if (missile->lifetime <= 0) {
            missile->active = false;
            continue;
        }
        
        // Track target
        if (missile->has_target) {
            bool target_valid = false;
            double target_x = 0, target_y = 0;
            
            if (missile->target_id == -2) {
                // Player target (from Juggernaut)
                target_x = game->ship_x;
                target_y = game->ship_y;
                target_valid = true;
            } else if (missile->target_id == -1) {
                // Boss target
                if (game->boss_active && game->boss.active) {
                    target_x = game->boss.x;
                    target_y = game->boss.y;
                    target_valid = true;
                }
            } else if (missile->target_id < 1000) {
                // Enemy ship target
                if (missile->target_id >= 0 && missile->target_id < game->enemy_ship_count) {
                    EnemyShip *ship = &game->enemy_ships[missile->target_id];
                    if (ship->active) {
                        target_x = ship->x;
                        target_y = ship->y;
                        target_valid = true;
                    }
                }
            } else {
                // Comet target (target_id - 1000 = comet index)
                int comet_index = missile->target_id - 1000;
                if (comet_index >= 0 && comet_index < game->comet_count) {
                    Comet *comet = &game->comets[comet_index];
                    if (comet->active) {
                        target_x = comet->x;
                        target_y = comet->y;
                        target_valid = true;
                    }
                }
            }
            
            if (target_valid) {
                // Update target position and turn toward it
                double dx = target_x - missile->x;
                double dy = target_y - missile->y;
                double target_angle = atan2(dy, dx);  // atan2 returns radians!
                
                double current = missile->angle;  // In radians
                double target = target_angle;     // In radians
                
                // Find shortest angular distance (in radians)
                double diff = target - current;
                while (diff > M_PI) diff -= 2.0 * M_PI;
                while (diff < -M_PI) diff += 2.0 * M_PI;
                
                // Limit turn speed (convert turn_speed from degrees/sec to radians/sec)
                double max_turn = (missile->turn_speed * M_PI / 180.0) * dt;
                if (diff > max_turn) diff = max_turn;
                if (diff < -max_turn) diff = -max_turn;
                
                missile->angle += diff;
                // Normalize to 0 to 2π
                while (missile->angle < 0) missile->angle += 2.0 * M_PI;
                while (missile->angle >= 2.0 * M_PI) missile->angle -= 2.0 * M_PI;
            } else {
                // Current target is dead, find new target
                MissileTarget new_target = comet_buster_find_best_missile_target(game, missile->x, missile->y);
                
                if (new_target.type == 1) {
                    missile->target_id = -1;
                    missile->has_target = true;
                } else if (new_target.type == 2) {
                    missile->target_id = new_target.index;
                    missile->has_target = true;
                } else if (new_target.type == 3) {
                    missile->target_id = new_target.index + 1000;
                    missile->has_target = true;
                } else {
                    missile->has_target = false;
                }
            }
        } else {
            // No target, find one
            MissileTarget target = comet_buster_find_best_missile_target(game, missile->x, missile->y);
            
            if (target.type == 1) {
                missile->target_id = -1;
                missile->has_target = true;
            } else if (target.type == 2) {
                missile->target_id = target.index;
                missile->has_target = true;
            } else if (target.type == 3) {
                missile->target_id = target.index + 1000;
                missile->has_target = true;
            }
        }
        
        missile->vx = cos(missile->angle) * missile->speed;  // missile->angle is in radians!
        missile->vy = sin(missile->angle) * missile->speed;
        
        missile->x += missile->vx * dt;
        missile->y += missile->vy * dt;
        
        if (missile->x < 0) missile->x += width;
        if (missile->x > width) missile->x -= width;
        if (missile->y < 0) missile->y += height;
        if (missile->y > height) missile->y -= height;
    }
    
    for (int i = game->missile_count - 1; i >= 0; i--) {
        if (!game->missiles[i].active) {
            if (i != game->missile_count - 1) {
                game->missiles[i] = game->missiles[game->missile_count - 1];
            }
            game->missile_count--;
        }
    }
}

// Update missile pickups
void comet_buster_update_missile_pickups(CometBusterGame *game, double dt) {
    if (!game) return;
    
    for (int i = 0; i < game->missile_pickup_count; i++) {
        MissilePickup *pickup = &game->missile_pickups[i];
        if (!pickup->active) continue;
        
        pickup->x += pickup->vx * dt;
        pickup->y += pickup->vy * dt;
        
        pickup->lifetime -= dt;
        if (pickup->lifetime <= 0) {
            pickup->active = false;
            continue;
        }
        
        pickup->rotation += pickup->rotation_speed * dt;
        while (pickup->rotation >= 360.0) {
            pickup->rotation -= 360.0;
        }
    }
    
    for (int i = game->missile_pickup_count - 1; i >= 0; i--) {
        if (!game->missile_pickups[i].active) {
            if (i != game->missile_pickup_count - 1) {
                game->missile_pickups[i] = game->missile_pickups[game->missile_pickup_count - 1];
            }
            game->missile_pickup_count--;
        }
    }
}

// ============================================================================
// BURNER/THRUSTER EFFECT SYSTEM
// ============================================================================

void comet_buster_update_burner_effects(CometBusterGame *game, double dt) {
    if (!game) return;
    
    // Update player ship burner
    {
        double speed = sqrt(game->ship_vx * game->ship_vx + game->ship_vy * game->ship_vy);
        
        // Burner intensity based on speed - MUCH LOWER thresholds for visible burners
        // At 50 px/sec = full intensity, so any reasonable speed shows good flames
        double max_speed = 150.0;  // Full intensity at 150 px/sec (was 400)
        double target_intensity = (speed > 0) ? fmin(speed / max_speed, 1.0) : 0.0;
        
        // Smooth transition to target intensity
        if (target_intensity > game->burner_intensity) {
            game->burner_intensity = fmin(target_intensity, game->burner_intensity + dt * 3.0);
        } else {
            game->burner_intensity = fmax(target_intensity, game->burner_intensity - dt * 2.5);
        }
        
        // Flicker timer for flame effect
        game->burner_flicker_timer += dt * 15.0;  // Fast flicker
        if (game->burner_flicker_timer > 1.0) {
            game->burner_flicker_timer = 0.0;
        }
    }
    
    // Update enemy ship burners
    for (int i = 0; i < game->enemy_ship_count; i++) {
        EnemyShip *ship = &game->enemy_ships[i];
        if (!ship->active) continue;
        
        double speed = sqrt(ship->vx * ship->vx + ship->vy * ship->vy);
        
        // Burner intensity based on speed - MUCH LOWER thresholds
        // At 30 px/sec = full intensity
        double max_speed = 100.0;  // Full intensity at 100 px/sec (was 300)
        double target_intensity = (speed > 0) ? fmin(speed / max_speed, 1.0) : 0.0;
        
        // Smooth transition to target intensity
        if (target_intensity > ship->burner_intensity) {
            ship->burner_intensity = fmin(target_intensity, ship->burner_intensity + dt * 3.0);
        } else {
            ship->burner_intensity = fmax(target_intensity, ship->burner_intensity - dt * 2.5);
        }
        
        // Flicker timer for flame effect
        ship->burner_flicker_timer += dt * 12.0;  // Slightly different timing than player
        if (ship->burner_flicker_timer > 1.0) {
            ship->burner_flicker_timer = 0.0;
        }
    }
}
