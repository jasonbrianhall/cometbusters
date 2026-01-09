#ifndef COMETBUSTER_H
#define COMETBUSTER_H

#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <cairo.h>
#include "cometbuster_bossexplosion.h"

// Static memory allocation constants
#define MAX_COMETS 128
#define MAX_BULLETS 128
#define MAX_PARTICLES 2048  // Increased for massive explosions
#define MAX_FLOATING_TEXT 32
#define MAX_CANISTERS 32
#define MAX_MISSILES 64
#define MAX_MISSILE_PICKUPS 16
#define MAX_BOMBS 16
#define MAX_BOMB_PICKUPS 8
#define BOMB_COUNTDOWN_TIME 3.0
#define BOMB_WAVE_MAX_RADIUS 600.0
#define BOMB_WAVE_DAMAGE 20
#define BOMB_WAVE_SPEED 1200.0
#define MAX_HIGH_SCORES 25

// PI
#ifndef M_PI
#define M_PI 3.1415926535
#endif

typedef enum {
    COMET_SMALL = 0,
    COMET_MEDIUM = 1,
    COMET_LARGE = 2,
    COMET_SPECIAL = 3,
    COMET_MEGA = 4
} CometSize;

typedef enum {
    EASY = 0,
    MEDIUM = 1,
    HARD = 2,
} CometDifficulty;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double radius;
    CometSize size;
    int frequency_band;         // 0=bass, 1=mid, 2=treble
    double rotation;            // For rotating visual (degrees)
    double rotation_speed;      // degrees per second
    double base_angle;          // Base rotation angle (radians) for vector asteroids
    double color[3];            // RGB
    bool active;
    int health;                 // For special comets
} Comet;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double angle;               // Direction
    double lifetime;            // Seconds remaining
    double max_lifetime;
    bool active;
    int owner_ship_id;
} Bullet;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double lifetime;            // Seconds remaining
    double max_lifetime;
    double size;                // Radius
    double color[3];            // RGB
    bool active;
} Particle;

typedef struct {
    double x, y;                // Position
    double lifetime;            // Seconds remaining
    double max_lifetime;
    char text[64];              // Text to display
    double color[3];            // RGB
    bool active;
} FloatingText;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity (drifts slowly)
    double lifetime;            // Seconds remaining
    double max_lifetime;        // Total lifetime (7 seconds)
    double rotation;            // Visual rotation (in degrees)
    double rotation_speed;      // Rotation speed
    bool active;
} Canister;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double angle;               // Direction
    double lifetime;            // Seconds remaining
    double max_lifetime;
    double target_x, target_y;  // Target position (for tracking)
    int target_id;              // ID of target (-1 = no target)
    bool active;
    bool has_target;            // Is tracking a target?
    double turn_speed;          // How fast missile can turn (degrees/sec)
    double speed;               // Missile speed (faster than bullets)
    int missile_type;           // 0-4 based on targeting behavior (type 0: furthest, 1: ships/boss, 2: closest comets, 3: comets ~400px, 4: comets 200-600px)
    int owner_ship_id;          // ID of ship that fired this missile (-1 if player, ship index if enemy)
} Missile;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity (drifts slowly)
    double lifetime;            // Seconds remaining
    double max_lifetime;        // Total lifetime
    double rotation;            // Visual rotation
    double rotation_speed;      // Rotation speed
    bool active;
} MissilePickup;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity (usually 0, placed statically)
    double lifetime;            // Seconds remaining until detonation
    double max_lifetime;        // 3 seconds
    double rotation;            // Visual rotation
    double rotation_speed;      // Rotation speed
    bool active;                // Is the bomb active?
    bool detonated;             // Has it exploded?
    bool damage_applied;        // Has damage been applied from this explosion?
    double wave_radius;         // Current radius of the explosion wave
    double wave_max_radius;     // Maximum wave radius (~300 pixels)
} Bomb;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity (drifts slowly)
    double lifetime;            // Seconds remaining
    double max_lifetime;        // Total lifetime (10 seconds, like missile pickups)
    double rotation;            // Visual rotation
    double rotation_speed;      // Rotation speed
    bool active;
    int bomb_count;             // How many bombs this pickup gives (usually 1)
} BombPickup;

typedef struct {
    int score;
    int wave;
    char player_name[32];       // Static string buffer
    time_t timestamp;
} HighScore;

// Keyboard input state for arcade-style controls
typedef struct {
    bool key_a_pressed;         // A key - turn left
    bool key_d_pressed;         // D key - turn right
    bool key_w_pressed;         // W key - forward thrust
    bool key_s_pressed;         // S key - backward thrust
    bool key_z_pressed;         // Z key - omnidirectional fire
    bool key_x_pressed;         // X key - boost
    bool key_space_pressed;     // SPACE key - boost
    bool key_ctrl_pressed;      // CTRL key - fire
    bool key_q_pressed;         // Q key - toggle missiles/bullets
} KeyboardInput;

typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double angle;               // Direction facing
    int health;                 // 1 hit = destroyed
    double shoot_cooldown;      // Time until next shot
    double path_time;           // Time along sine wave path (for wave motion)
    double base_vx, base_vy;    // Original velocity direction (for sine calculation)
    int ship_type;              // 0 = patrol (blue), 1 = aggressive (red), 2 = hunter (green), 
                                // 3 = sentinel (purple), 4 = brown coat (elite blue), 5 = juggernaut (massive gold)
    bool active;
    
    // Shield system for enemy ships
    int shield_health;          // Current shield points
    int max_shield_health;      // Maximum shield points (varies by ship type)
    double shield_impact_timer; // Visual impact effect timer
    double shield_impact_angle; // Angle of shield impact
    
    // Sentinel formation system
    int formation_id;           // Groups sentinels that spawned together (-1 if not sentinel)
    int formation_size;         // How many sentinels in this formation (1, 2, or 3)
    bool has_partner;           // Is the paired sentinel still alive?
    double formation_cohesion;  // How tightly they stay together (0.0-1.0)
    
    // Patrol behavior system (for blue/green/purple ships)
    double patrol_behavior_timer;   // Timer for current patrol behavior
    double patrol_behavior_duration;// How long to maintain current behavior
    int patrol_behavior_type;       // 0=straight, 1=circle, 2=evasive turns
    double patrol_circle_center_x;  // Center of circle when doing circular behavior
    double patrol_circle_center_y;
    double patrol_circle_radius;    // Radius of circular path
    double patrol_circle_angle;     // Current angle in circle (radians)
    
    // BROWN COAT SPECIFIC FIELDS (NEW)
    double burst_fire_cooldown;     // Cooldown until next omnidirectional burst
    double burst_trigger_range;     // Distance at which to trigger burst (200-300 px)
    int last_burst_direction;       // Last burst angle offset (for visual variety)
    double proximity_detection_timer; // Timer to check for nearby targets
    int burst_count_this_wave;      // Track how many bursts fired this encounter
    
    // THRUSTER/BURNER EFFECT FIELDS
    double burner_flicker_timer;    // For flickering flame effect
    double burner_intensity;        // How bright/large the burner is (0.0-1.0)
    
} EnemyShip;

#define MAX_ENEMY_SHIPS 4
#define MAX_ENEMY_BULLETS 64

// UFO (Flying Saucer) structure - appears randomly across screen like Asteroids
typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity (usually straight across screen)
    double angle;               // Direction facing
    int health;                 // UFO health (3-5 hits to destroy)
    int max_health;
    double shoot_cooldown;      // Time until next shot
    bool active;                // Is the UFO alive?
    double lifetime;            // How long it's been on screen (despawns after crossing)
    int direction;              // 1 = right, -1 = left (horizontal movement)
    double entry_height;        // Y position it enters at (random between top/middle/bottom)
    double shoot_timer;         // Timer for firing pattern
    
    // Visual effects
    double burner_intensity;    // For thruster flames
    double burner_flicker_timer;
    double damage_flash_timer;  // Flash when hit
    
    // Audio effects
    double sound_timer;         // Timer for periodic UFO sound effect
    
} UFO;

#define MAX_UFOS 2  // Only 1-2 UFOs on screen at a time

// Boss (Death Star) structure
typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    double angle;               // Direction facing
    int health;                 // Boss health (large number like 50-100)
    int max_health;             // Maximum health
    double shoot_cooldown;      // Time until next shot
    int phase;                  // 0 = normal, 1 = shield up, 2 = enraged
    double phase_timer;         // Time in current phase
    double phase_duration;      // How long until phase changes
    
    // Shield system
    int shield_health;          // Shield durability (10-20 points)
    int max_shield_health;      // Maximum shield durability
    bool shield_active;         // Is shield currently up?
    double shield_impact_timer; // Visual effect
    double shield_impact_angle; // Where shield was hit
    
    // Firing pattern
    double fire_pattern_timer;  // For special firing patterns
    int fire_pattern;           // Different firing modes
    
    // Visual effects
    double rotation;            // Visual rotation
    double rotation_speed;
    double damage_flash_timer;  // Flash when taking damage
    
    bool active;                // Is the boss alive?

    int fragment_count;           // Number of active fragments (0 = main, 1+ = split)
    double fragment_positions[4][2];  // X,Y positions of up to 4 fragments
    int fragment_health[4];       // Health of each fragment
    bool is_fragment;             // Is this a fragment or main body?
    int fragment_id;              // Which fragment number (0-3)
    double fragment_reunite_timer; // Time until fragments try to reunite
    double reunite_speed;         // How fast they move back together
    double last_damage_time;      // For tracking damage frequency
    int burst_angle_offset;       // For rotating firing pattern
    double nexus_ship_spawn_timer;
    
    double laser_angle;             // Current angle of orbital laser
    double laser_rotation_speed;    // How fast laser rotates
    double gravity_well_strength;   // Force to pull bullets toward boss
    bool laser_active;              // Is laser currently firing?
    double laser_charge_timer;      // Charge time before laser fires
    int bomb_count;                 // Number of active bouncing bombs
    int bomb_spawned_this_phase;    // Track bombs created in current phase
    double beam_angle_offset;       // For variety in beam attacks
    
    // GRAVITY WELL SYSTEM (SINGULARITY BOSS)
    double void_radius;              // Radius of gravitational pull (expands in later phases)
    double gravity_pull_strength;    // How strong the gravitational pull is per phase
} BossShip;

// Spawn Queen (Mothership) structure - spawns Red and Sentinel ships on waves 10, 20, 30, etc.
typedef struct {
    double x, y;                // Position
    double vx, vy;              // Velocity
    int health;                 // Queen health (scales with wave)
    int max_health;
    int shield_health;          // Shield (15 points, regenerates in phase 1)
    int max_shield_health;
    
    // Spawn mechanics
    double spawn_timer;         // Timer until next ship spawn
    double spawn_cooldown;      // Seconds between ship spawns
    
    // Attack patterns
    int phase;                  // 0 = recruitment, 1 = aggression, 2 = desperation
    double phase_timer;         // Time in current phase
    double attack_timer;        // Timer until next attack
    double attack_cooldown;     // Seconds between attacks
    
    // Movement
    double movement_timer;      // For sine wave motion
    double base_movement_speed; // How fast to move horizontally
    
    // Visual
    double rotation;            // Spinning animation
    double rotation_speed;      // Degrees per second
    double damage_flash_timer;  // Flash when taking damage
    double spawn_particle_timer;// For port glow effects
    
    // Status
    bool active;                // Is queen alive?
    bool is_spawn_queen;        // Flag: true for queen, false for regular boss
} SpawnQueenBoss;

typedef struct {
    // Ship state
    double ship_x, ship_y;
    double ship_vx, ship_vy;
    double ship_angle;          // Radians
    double ship_speed;          // Current velocity magnitude
    double ship_rotation_angle; // Target angle toward mouse
    int ship_lives;
    double invulnerability_time; // Seconds of invincibility after being hit
    
    // THRUSTER/BURNER EFFECT FIELDS
    double burner_flicker_timer;    // For flickering flame effect
    double burner_intensity;        // How bright/large the burner is (0.0-1.0)
    
    // Shield system
    int shield_health;          // Current shield points (0 means no shield)
    int max_shield_health;      // Maximum shield capacity
    double shield_regen_timer;  // Timer for shield regeneration
    double shield_regen_delay;  // Delay before shield starts regenerating after hit
    double shield_regen_rate;   // Health points per second when regenerating
    double shield_impact_angle; // Angle where shield was hit (for visual effect)
    double shield_impact_timer; // Timer for impact flash effect
    
    // Game state
    int score;
    int comets_destroyed;
    double score_multiplier;    // Current multiplier (1.0 - 5.0+)
    int consecutive_hits;       // Hits without being damage for multiplier
    int current_wave;
    int wave_comets;            // Comets destroyed this wave
    int last_life_milestone;    // Last score milestone where extra life was granted (5000, 10000, etc.)
    bool game_over;
    bool game_won;              // Optional: wave complete
    int difficulty;             // 0=Easy, 1=Medium, 2=Hard
    
    // Arrays
    Comet comets[MAX_COMETS];
    int comet_count;
    Bullet bullets[MAX_BULLETS];
    int bullet_count;
    Particle particles[MAX_PARTICLES];
    int particle_count;
    FloatingText floating_texts[MAX_FLOATING_TEXT];
    int floating_text_count;
    Canister canisters[MAX_CANISTERS];
    int canister_count;
    
    Missile missiles[MAX_MISSILES];
    int missile_count;
    
    MissilePickup missile_pickups[MAX_MISSILE_PICKUPS];
    int missile_pickup_count;
    
    // Weapon system
    int missile_ammo;                // How many missiles player has (max 100)
    bool using_missiles;             // Currently in missile mode?
    bool using_bombs;                // Currently in bomb mode?
    bool using_spread_fire;          // Currently in spread fire mode?
    double spread_fire_cooldown;     // Cooldown between spread fire shots (5x normal cooldown)
    double missile_generation_timer; // Accumulates missiles when energy is full (5 per second)
    
    // Bomb system
    Bomb bombs[MAX_BOMBS];
    int bomb_count;
    int bomb_ammo;                      // How many bombs player has
    
    BombPickup bomb_pickups[MAX_BOMB_PICKUPS];
    int bomb_pickup_count;
    
    double bomb_drop_cooldown;          // Cooldown between dropping bombs

    
    EnemyShip enemy_ships[MAX_ENEMY_SHIPS];
    int enemy_ship_count;
    Bullet enemy_bullets[MAX_ENEMY_BULLETS];
    int enemy_bullet_count;
    
    // UFO (Flying Saucers) - Random encounters like original Asteroids
    UFO ufos[MAX_UFOS];
    int ufo_count;
    double ufo_spawn_timer;     // Timer for next UFO spawn
    double ufo_spawn_rate;      // Seconds between UFO spawns (20-40 seconds)
    
    // Boss (Death Star) - appears in wave 5+
    BossShip boss;
    SpawnQueenBoss spawn_queen;  // Mothership boss - appears on waves 10, 20, 30, etc.
    bool boss_active;
    double boss_spawn_timer;
    int last_boss_wave;             // Track which wave had the boss (only spawn every 5 waves)
    
    // Timing & difficulty
    double spawn_timer;         // Seconds until next spawn
    double base_spawn_rate;     // Seconds between spawns
    double beat_fire_cooldown;  // Cooldown on beat fire
    double last_beat_time;
    bool auto_fire_enabled;
    double difficulty_timer;    // Tracks when to increase difficulty
    double enemy_ship_spawn_timer;  // Time until next enemy ship spawn
    double enemy_ship_spawn_rate;   // Base rate for enemy ship spawning
    
    // Audio data (updated each frame from visualizer)
    double frequency_bands[3];  // Bass, Mid, Treble [0.0-1.0]
    double volume_level;        // Current volume [0.0-1.0]
    
    // UI & timers
    bool show_game_over;
    double game_over_timer;
    double wave_complete_timer;
    
    // Muzzle flash animation
    double muzzle_flash_timer;
    
    // Mouse input for shooting
    bool mouse_left_pressed;       // Left mouse button state
    double mouse_fire_cooldown;    // Cooldown between shots
    
    // Advanced thrusters (right-click)
    bool mouse_right_pressed;      // Right mouse button state
    bool mouse_middle_pressed;     // Middle mouse button state (omnidirectional fire)
    double omni_fire_cooldown;     // Cooldown for omnidirectional fire
    double weapon_toggle_cooldown; // Cooldown for weapon switching (Q key)
    int scroll_direction;          // Scroll wheel input: 1 for up, -1 for down, 0 for none
    double energy_amount;          // Current energy [0.0 - 100.0]
    double max_energy;             // Maximum energy capacity
    double energy_burn_rate;        // Energy burned per second at max thrust
    double energy_recharge_rate;    // Energy recharged per second when not boosting
    double boost_multiplier;       // Speed multiplier when boosting (e.g., 2.0x)
    bool is_boosting;              // Currently using advanced thrusters
    double boost_thrust_timer;     // Visual effect timer for boosting
    
    // High scores (static array)
    HighScore high_scores[MAX_HIGH_SCORES];
    int high_score_count;
    
    // Keyboard input (WASD movement)
    KeyboardInput keyboard;
    
    // Splash screen state
   bool splash_screen_active;
   double splash_timer;
   int splash_comet_count;
   Comet splash_comets[50];
   int splash_alien_count;
   struct {
       double x, y;
       double vx, vy;
       int type;
       double rotation;
   } splash_aliens[20];

    // Finale splash screen (Wave 30 victory)
    bool finale_splash_active;
    bool finale_splash_boss_paused;
    double finale_splash_timer;
    int finale_scroll_line_index;
    double finale_scroll_timer;
    bool finale_waiting_for_input;
    
    // Boss destruction explosion effect
    BossExplosion boss_explosion_effect;
    
} CometBusterGame;

// Initialization and cleanup
void comet_buster_cleanup(CometBusterGame *game);
void comet_buster_reset_game(CometBusterGame *game);
void comet_buster_reset_game_with_splash(CometBusterGame *game, bool show_splash, int difficulty);

// Update sub-systems
void comet_buster_update_ship(CometBusterGame *game, double dt, int mouse_x, int mouse_y, int width, int height, bool mouse_active);
void comet_buster_update_comets(CometBusterGame *game, double dt, int width, int height);
void comet_buster_update_shooting(CometBusterGame *game, double dt);  // New: click-to-shoot
void comet_buster_update_bullets(CometBusterGame *game, double dt, int width, int height, void *vis);
void comet_buster_update_particles(CometBusterGame *game, double dt);
void comet_buster_update_floating_text(CometBusterGame *game, double dt);
void comet_buster_update_fuel(CometBusterGame *game, double dt);  // Advanced thrusters fuel system
void comet_buster_update_enemy_bullets(CometBusterGame *game, double dt, int width, int height, void *vis);
void comet_buster_update_burner_effects(CometBusterGame *game, double dt);  // Burner/thruster effects

// Spawning
void comet_buster_spawn_comet(CometBusterGame *game, int frequency_band, int screen_width, int screen_height);
void comet_buster_spawn_random_comets(CometBusterGame *game, int count, int screen_width, int screen_height);
void comet_buster_spawn_wave(CometBusterGame *game, int screen_width, int screen_height);
int comet_buster_get_wave_comet_count(int wave);
double comet_buster_get_wave_speed_multiplier(int wave);
void comet_buster_spawn_bullet(CometBusterGame *game, void *vis);
void comet_buster_spawn_omnidirectional_fire(CometBusterGame *game);
void comet_buster_spawn_spread_fire(CometBusterGame *game, void *vis);
void comet_buster_spawn_explosion(CometBusterGame *game, double x, double y, int frequency_band, int particle_count);
void comet_buster_spawn_ship_death_explosion(CometBusterGame *game, double x, double y);
void comet_buster_spawn_floating_text(CometBusterGame *game, double x, double y, const char *text, double r, double g, double b);
void comet_buster_spawn_enemy_ship(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_spawn_enemy_bullet(CometBusterGame *game, double x, double y, double vx, double vy);
void comet_buster_spawn_enemy_bullet_from_ship(CometBusterGame *game, double x, double y, double vx, double vy, int owner_ship_id);

// Canister functions
void comet_buster_spawn_canister(CometBusterGame *game, double x, double y);
void comet_buster_update_canisters(CometBusterGame *game, double dt);
void draw_comet_buster_canisters(CometBusterGame *game, cairo_t *cr, int width, int height);
bool comet_buster_check_ship_canister(CometBusterGame *game, Canister *c);

// Missile functions
void comet_buster_spawn_missile_pickup(CometBusterGame *game, double x, double y);
void comet_buster_update_missile_pickups(CometBusterGame *game, double dt);
void draw_comet_buster_missile_pickups(CometBusterGame *game, cairo_t *cr, int width, int height);
bool comet_buster_check_ship_missile_pickup(CometBusterGame *game, MissilePickup *p);
void comet_buster_fire_missile(CometBusterGame *game, void *vis);
void comet_buster_update_missiles(CometBusterGame *game, double dt, int width, int height);
void draw_comet_buster_missiles(CometBusterGame *game, cairo_t *cr, int width, int height);
EnemyShip* comet_buster_find_nearest_enemy(CometBusterGame *game, double x, double y);

// Bomb functions
void comet_buster_spawn_bomb_pickup(CometBusterGame *game, double x, double y);
void comet_buster_update_bomb_pickups(CometBusterGame *game, double dt);
void draw_comet_buster_bomb_pickups(CometBusterGame *game, cairo_t *cr, int width, int height);
bool comet_buster_check_ship_bomb_pickup(CometBusterGame *game, BombPickup *p);
void comet_buster_drop_bomb(CometBusterGame *game, int width, int height, void *vis);
void comet_buster_update_bombs(CometBusterGame *game, double dt, int width, int height, void *vis);
void draw_comet_buster_bombs(CometBusterGame *game, cairo_t *cr, int width, int height);
bool comet_buster_check_bomb_wave_comet(Bomb *bomb, Comet *comet);
bool comet_buster_check_bomb_wave_enemy_ship(Bomb *bomb, EnemyShip *ship);
bool comet_buster_check_bomb_wave_ufo(Bomb *bomb, UFO *ufo);
bool comet_buster_check_bomb_wave_boss(Bomb *bomb, BossShip *boss);
bool comet_buster_check_bomb_wave_bullet(Bomb *bomb, Bullet *bullet);


// Boss functions
void comet_buster_spawn_boss(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_update_boss(CometBusterGame *game, double dt, int width, int height);
void comet_buster_boss_fire(CometBusterGame *game);
bool comet_buster_check_bullet_boss(Bullet *b, BossShip *boss);
void comet_buster_destroy_boss(CometBusterGame *game, int width, int height, void *vis);
void draw_comet_buster_boss(BossShip *boss, cairo_t *cr, int width, int height);

// Spawn Queen boss functions
void comet_buster_spawn_spawn_queen(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_update_spawn_queen(CometBusterGame *game, double dt, int width, int height);
void comet_buster_spawn_queen_fire(CometBusterGame *game);
void comet_buster_spawn_queen_spawn_ships(CometBusterGame *game, int screen_width, int screen_height);
bool comet_buster_check_bullet_spawn_queen(Bullet *b, SpawnQueenBoss *queen);
void comet_buster_destroy_spawn_queen(CometBusterGame *game, int width, int height, void *vis);
void draw_spawn_queen_boss(SpawnQueenBoss *queen, cairo_t *cr, int width, int height);

bool comet_buster_check_bullet_comet(Bullet *b, Comet *c);
bool comet_buster_check_missile_comet(Missile *m, Comet *c);
bool comet_buster_check_ship_comet(CometBusterGame *game, Comet *c);
bool comet_buster_check_missile_boss(Missile *m, BossShip *boss);
void comet_buster_handle_comet_collision(Comet *c1, Comet *c2, double dx, double dy, 
                                         double dist, double min_dist);
void comet_buster_destroy_comet(CometBusterGame *game, int comet_index, int width, int height, void *vis);
bool comet_buster_check_bullet_enemy_ship(Bullet *b, EnemyShip *e);
bool comet_buster_check_enemy_bullet_ship(CometBusterGame *game, Bullet *b);
int comet_buster_check_enemy_bullet_enemy_ship(CometBusterGame *game, Bullet *b);
void comet_buster_destroy_enemy_ship(CometBusterGame *game, int ship_index, int width, int height, void *vis);
bool comet_buster_hit_enemy_ship_provoke(CometBusterGame *game, int ship_index);  // New: provoke blue ships

// Audio integration
void comet_buster_fire_on_beat(CometBusterGame *game);
bool comet_buster_detect_beat(void *vis);

// Difficulty management
void comet_buster_increase_difficulty(CometBusterGame *game);
void comet_buster_update_wave_progression(CometBusterGame *game);

// Rendering
void draw_comet_buster_ship(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_comets(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_bullets(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_particles(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_hud(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_game_over(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_enemy_ships(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_comet_buster_enemy_bullets(CometBusterGame *game, cairo_t *cr, int width, int height);
void draw_ship_burner(cairo_t *cr, double burner_intensity, double length_multiplier);
void draw_enemy_ship_burner(cairo_t *cr, double burner_intensity, double ship_size);

// Helper functions
void comet_buster_wrap_position(double *x, double *y, int width, int height);
double comet_buster_distance(double x1, double y1, double x2, double y2);
void comet_buster_get_frequency_color(int frequency_band, double *r, double *g, double *b);

// High score management - implemented in comet_main.cpp
bool comet_buster_is_high_score(CometBusterGame *game, int score);

// Enemy
void comet_buster_spawn_enemy_ship_internal(CometBusterGame *game, int screen_width, int screen_height, 
                                            int ship_type, int edge, double speed, int formation_id, int formation_size);

// UFO (Flying Saucer) functions
void comet_buster_spawn_ufo(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_ufo_fire(CometBusterGame *game);
void draw_comet_buster_ufos(CometBusterGame *game, cairo_t *cr, int width, int height);
bool comet_buster_check_bullet_ufo(Bullet *b, UFO *u);
bool comet_buster_check_missile_ufo(Missile *m, UFO *u);  // Missile targeting
bool comet_buster_check_ufo_comet(UFO *u, Comet *c);    // UFO-asteroid collision
bool comet_buster_check_ship_ufo(CometBusterGame *game, UFO *u);  // Ship-UFO collision
bool comet_buster_check_enemy_bullet_ufo(Bullet *b, UFO *u);  // Enemy bullet-UFO collision

void comet_buster_destroy_ufo(CometBusterGame *game, int ufo_index, int width, int height, void *vis);

// Splash
void comet_buster_init_splash_screen(CometBusterGame *game, int width, int height);
void comet_buster_exit_splash_screen(CometBusterGame *game);
void comet_buster_draw_splash_screen(CometBusterGame *game, cairo_t *cr, int width, int height);
void comet_buster_update_finale_splash(CometBusterGame *game, double dt);
void comet_buster_draw_finale_splash(CometBusterGame *game, cairo_t *cr, int width, int height);                                 

void comet_buster_spawn_void_nexus(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_update_void_nexus(CometBusterGame *game, double dt, int width, int height);
void draw_void_nexus_boss(BossShip *boss, cairo_t *cr, int width, int height);
void void_nexus_fire(CometBusterGame *game);
void void_nexus_fragment_fire(CometBusterGame *game, int fragment_id);
void void_nexus_split_into_fragments(CometBusterGame *game, int num_fragments);
void comet_buster_damage_void_nexus(CometBusterGame *game, int damage, int fragment_id);
void void_nexus_spawn_ship_wave(CometBusterGame *game, int screen_width, int screen_height);

void comet_buster_brown_coat_fire_burst(CometBusterGame *game, int ship_index);
bool comet_buster_is_target_nearby(CometBusterGame *game, double ship_x, double ship_y, double range);
void harbinger_spawn_bomb(CometBusterGame *game, double x, double y);
void comet_buster_spawn_harbinger(CometBusterGame *game, int screen_width, int screen_height);
void draw_harbinger_boss(BossShip *boss, cairo_t *cr, int width, int height);
void comet_buster_update_harbinger(CometBusterGame *game, double dt, int width, int height);
bool comet_buster_hit_void_nexus_fragment(Bullet *b, BossShip *boss, int *fragment_hit);

// Star Vortex boss functions
void comet_buster_spawn_star_vortex(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_update_star_vortex(CometBusterGame *game, double dt, int width, int height);
bool star_vortex_handle_comet_collision(CometBusterGame *game, Comet *comet, 
                                        double collision_dx, double collision_dy);
void star_vortex_fire_missiles(CometBusterGame *game);
void star_vortex_spawn_juggernauts(CometBusterGame *game, int width, int height);
void star_vortex_final_explosion(CometBusterGame *game);
void draw_star_vortex_boss(BossShip *boss, cairo_t *cr, int width, int height);

// Singularity (Ultimate) boss functions
void comet_buster_spawn_singularity(CometBusterGame *game, int screen_width, int screen_height);
void comet_buster_update_singularity(CometBusterGame *game, double dt, int width, int height);
void draw_singularity_boss(BossShip *boss, cairo_t *cr, int width, int height);

void comet_buster_update_victory_scroll(CometBusterGame *game, double dt);
void comet_buster_draw_victory_scroll(CometBusterGame *game, cairo_t *cr, int width, int height);
void comet_buster_exit_victory_scroll(CometBusterGame *game);
#endif // COMETBUSTER_H
