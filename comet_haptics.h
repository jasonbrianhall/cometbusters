#ifndef SDL_HAPTICS_H
#define SDL_HAPTICS_H

#ifdef ANDROID
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_haptic.h>
#endif

#include <stdbool.h>

// Haptic effect types for different game events
typedef enum {
    HAPTIC_PLAYER_SHOOT,          // Subtle pop when firing
    HAPTIC_COMET_IMPACT,          // Medium vibration on hit
    HAPTIC_COMET_EXPLOSION,       // Strong burst explosion
    HAPTIC_BOMB_EXPLOSION,        // Very strong directional burst
    HAPTIC_BOSS_DAMAGE,           // Heavy sustained rumble
    HAPTIC_BOSS_DEFEATED,         // Strong crescendo effect
    HAPTIC_PLAYER_HIT,            // Sharp painful pulse
    HAPTIC_MISSILE_FIRE,          // Firm burst (heavier than shoot)
    HAPTIC_CANISTER_COLLECT,      // Light pleasant buzz
    HAPTIC_WAVE_COMPLETE,         // Celebratory double pulse
    HAPTIC_GAME_OVER,             // Sad declining rumble
    HAPTIC_BUTTON_PRESS,          // UI feedback click
    HAPTIC_WARNING,               // Rhythmic pulse alert
} HapticEffectType;

// Haptic effect parameters
typedef struct {
    HapticEffectType type;
    int duration_ms;               // Duration in milliseconds
    int left_motor;                // 0-255, left motor intensity (bass)
    int right_motor;               // 0-255, right motor intensity (treble)
    int repeats;                   // Number of times to repeat (1-4)
    float delay_ms;                // Delay before effect starts
} HapticEffect;

// Haptic manager state
typedef struct {
    SDL_Joystick *joystick;        // Current joystick device
    SDL_Haptic *haptic;            // Haptic device (usually same as joystick)
    bool initialized;
    bool haptic_supported;
    int num_effects;               // Number of supported simultaneous effects
    int supported_axes;            // Number of vibration axes
    int rumble_supported;          // 1 if simple rumble supported
    int periodic_supported;        // 1 if periodic effects supported
} HapticManager;

// Initialize haptic system
bool haptic_init(HapticManager *hm, SDL_Joystick *joystick);

// Cleanup
void haptic_cleanup(HapticManager *hm);

// Trigger a haptic effect
void haptic_trigger_effect(HapticManager *hm, HapticEffectType effect_type);

// Custom haptic effect with specific parameters
void haptic_trigger_custom(HapticManager *hm, int left_intensity, int right_intensity, 
                          int duration_ms, int repeats);

// Stop all haptic effects
void haptic_stop_all(HapticManager *hm);

// Update haptic manager (call each frame if needed for ongoing effects)
void haptic_update(HapticManager *hm, float dt);

// Check if haptic is available
bool haptic_is_available(HapticManager *hm);

// Get effect parameters for a given type (for customization)
HapticEffect haptic_get_effect_params(HapticEffectType type);

#endif // SDL_HAPTICS_H
