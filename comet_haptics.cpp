#include "comet_haptics.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Get haptic effect parameters tuned for PS5 DualSense controller
HapticEffect haptic_get_effect_params(HapticEffectType type) {
    HapticEffect effect = {0};
    effect.repeats = 1;
    effect.delay_ms = 0;
    
    switch (type) {
        case HAPTIC_PLAYER_SHOOT:
            // Light pop feedback for gun fire
            effect.duration_ms = 80;
            effect.left_motor = 180;      // Strong bass
            effect.right_motor = 100;     // Light treble
            effect.repeats = 1;
            break;
            
        case HAPTIC_COMET_IMPACT:
            // Medium impact when bullet hits comet
            effect.duration_ms = 120;
            effect.left_motor = 220;      // Strong bass
            effect.right_motor = 140;     // Medium treble
            effect.repeats = 1;
            break;
            
        case HAPTIC_COMET_EXPLOSION:
            // Punchy explosion effect - quick burst
            effect.duration_ms = 150;
            effect.left_motor = 255;      // Full bass
            effect.right_motor = 180;     // Strong treble
            effect.repeats = 1;
            break;
            
        case HAPTIC_BOMB_EXPLOSION:
            // Very strong directional explosion - heavy and sustained
            effect.duration_ms = 200;
            effect.left_motor = 255;      // Full bass
            effect.right_motor = 220;     // Full treble
            effect.repeats = 1;           // Single strong pulse
            break;
            
        case HAPTIC_BOSS_DAMAGE:
            // Heavy sustained rumble when hitting boss
            effect.duration_ms = 250;
            effect.left_motor = 240;      // Full bass
            effect.right_motor = 150;     // Medium-strong treble
            effect.repeats = 1;
            break;
            
        case HAPTIC_BOSS_DEFEATED:
            // Celebratory effect - crescendo pattern
            effect.duration_ms = 400;
            effect.left_motor = 180;      // Start medium
            effect.right_motor = 200;     // Build up
            effect.repeats = 2;           // Two pulses to create crescendo
            break;
            
        case HAPTIC_PLAYER_HIT:
            // Sharp painful pulse when player takes damage
            effect.duration_ms = 100;
            effect.left_motor = 255;      // Full intensity
            effect.right_motor = 200;     // Strong
            effect.repeats = 1;
            break;
            
        case HAPTIC_MISSILE_FIRE:
            // Firm burst for missile launch (heavier than regular shoot)
            effect.duration_ms = 140;
            effect.left_motor = 220;      // Strong bass
            effect.right_motor = 180;     // Strong treble
            effect.repeats = 1;
            break;
            
        case HAPTIC_CANISTER_COLLECT:
            // Light pleasant buzz for pickup
            effect.duration_ms = 80;
            effect.left_motor = 140;      // Light-medium bass
            effect.right_motor = 160;     // Medium treble (higher pitched)
            effect.repeats = 1;
            break;
            
        case HAPTIC_WAVE_COMPLETE:
            // Celebratory double pulse
            effect.duration_ms = 100;
            effect.left_motor = 200;      // Medium-strong
            effect.right_motor = 200;     // Medium-strong
            effect.repeats = 2;           // Double tap
            break;
            
        case HAPTIC_GAME_OVER:
            // Sad declining rumble
            effect.duration_ms = 300;
            effect.left_motor = 255;      // Start strong
            effect.right_motor = 200;     // Start strong
            effect.repeats = 1;           // Will fade out
            break;
            
        case HAPTIC_BUTTON_PRESS:
            // UI feedback - light click
            effect.duration_ms = 50;
            effect.left_motor = 120;      // Light
            effect.right_motor = 120;     // Light
            effect.repeats = 1;
            break;
            
        case HAPTIC_WARNING:
            // Rhythmic pulse alert (boss spawning, etc)
            effect.duration_ms = 100;
            effect.left_motor = 200;      // Medium-strong
            effect.right_motor = 120;     // Light
            effect.repeats = 3;           // Triple pulse for urgency
            break;
            
        default:
            effect.duration_ms = 100;
            effect.left_motor = 150;
            effect.right_motor = 150;
            effect.repeats = 1;
            break;
    }
    
    effect.type = type;
    return effect;
}

// Initialize haptic system with improved DualSense detection
bool haptic_init(HapticManager *hm, SDL_Joystick *joystick) {
    if (!hm || !joystick) {
        SDL_Log("[Comet Busters] [ERROR] haptic_init: Invalid arguments\n");
        return false;
    }
    
    memset(hm, 0, sizeof(HapticManager));
    hm->joystick = joystick;
    
    // Try to initialize haptic device from joystick
    hm->haptic = SDL_HapticOpenFromJoystick(joystick);
    
    if (!hm->haptic) {
        SDL_Log("[Comet Busters] [HAPTIC] Standard joystick haptic failed: %s\n", 
                SDL_GetError());
        
        // WORKAROUND: For platforms where SDL_HapticOpenFromJoystick doesn't work
        // (Windows, some Linux, macOS) - try opening haptic devices directly
        SDL_Log("[Comet Busters] [HAPTIC] Attempting fallback: searching for haptic devices...\n");
        
        int num_haptics = SDL_NumHaptics();
        SDL_Log("[Comet Busters] [HAPTIC] Found %d haptic devices\n", num_haptics);
        
        if (num_haptics > 0) {
            // Try to find a device that matches the joystick name
            const char *joystick_name = SDL_JoystickName(joystick);
            SDL_Log("[Comet Busters] [HAPTIC] Looking for device matching: %s\n", joystick_name);
            
            for (int i = 0; i < num_haptics; i++) {
                const char *haptic_name = SDL_HapticName(i);
                SDL_Log("[Comet Busters] [HAPTIC] Checking device %d: %s\n", i, haptic_name);
                
                // Look for common DualSense indicators
                if (haptic_name && (
                    strstr(haptic_name, "DualSense") != NULL ||
                    strstr(haptic_name, "PlayStation") != NULL ||
                    strstr(haptic_name, "Wireless Controller") != NULL ||
                    strstr(haptic_name, "054c:05c5") != NULL ||  // DualSense USB VID:PID
                    strstr(haptic_name, "054c:09cc") != NULL)) { // DualSense Wireless VID:PID
                    
                    SDL_Log("[Comet Busters] [HAPTIC] Attempting to open: %s\n", haptic_name);
                    hm->haptic = SDL_HapticOpen(i);
                    
                    if (hm->haptic) {
                        SDL_Log("[Comet Busters] [HAPTIC] ✓ Successfully opened haptic device via fallback!\n");
                        break;
                    } else {
                        SDL_Log("[Comet Busters] [HAPTIC] Failed to open device: %s\n", SDL_GetError());
                    }
                }
            }
        }
        
        if (!hm->haptic) {
            SDL_Log("[Comet Busters] [HAPTIC] No haptic device available\n");
            hm->haptic_supported = false;
            return false;
        }
    }
    
    SDL_Log("[Comet Busters] [HAPTIC] Haptic device opened successfully\n");
    
    // Get haptic capabilities
    hm->num_effects = SDL_HapticNumEffects(hm->haptic);
    hm->supported_axes = SDL_HapticNumAxes(hm->haptic);
    hm->rumble_supported = SDL_HapticRumbleSupported(hm->haptic);
    
    SDL_Log("[Comet Busters] [HAPTIC] Supported effects: %d\n", hm->num_effects);
    SDL_Log("[Comet Busters] [HAPTIC] Vibration axes: %d\n", hm->supported_axes);
    SDL_Log("[Comet Busters] [HAPTIC] Rumble supported: %s\n", 
            hm->rumble_supported ? "YES" : "NO");
    
    // Check for periodic effects (DualSense supports this)
    unsigned int supported = SDL_HapticQuery(hm->haptic);
    
    // SDL_HAPTIC_PERIODIC may not be defined in all SDL2 versions
    #ifdef SDL_HAPTIC_PERIODIC
    hm->periodic_supported = (supported & SDL_HAPTIC_PERIODIC) ? 1 : 0;
    #else
    hm->periodic_supported = 0;
    #endif
    
    #ifdef SDL_HAPTIC_LEFTRIGHT
    if (supported & SDL_HAPTIC_LEFTRIGHT) {
        SDL_Log("[Comet Busters] [HAPTIC] Left/Right motor control: SUPPORTED\n");
    }
    #endif
    
    #ifdef SDL_HAPTIC_SINE
    if (supported & SDL_HAPTIC_SINE) {
        SDL_Log("[Comet Busters] [HAPTIC] Sine wave effects: SUPPORTED\n");
    }
    #endif
    
    // Initialize rumble
    if (hm->rumble_supported) {
        if (SDL_HapticRumbleInit(hm->haptic) == 0) {
            SDL_Log("[Comet Busters] [HAPTIC] Rumble initialized successfully\n");
            hm->haptic_supported = true;
            hm->initialized = true;
            return true;
        }
    }
    
    // If rumble not supported, try periodic effects
    if (hm->periodic_supported) {
        SDL_Log("[Comet Busters] [HAPTIC] Using periodic effects instead of rumble\n");
        hm->haptic_supported = true;
        hm->initialized = true;
        return true;
    }
    
    SDL_Log("[Comet Busters] [HAPTIC] WARNING: No compatible haptic features found\n");
    SDL_HapticClose(hm->haptic);
    hm->haptic = NULL;
    hm->haptic_supported = false;
    
    return false;
}

// Cleanup
void haptic_cleanup(HapticManager *hm) {
    if (!hm) return;
    
    if (hm->haptic) {
        haptic_stop_all(hm);
        SDL_HapticClose(hm->haptic);
        hm->haptic = NULL;
    }
    
    memset(hm, 0, sizeof(HapticManager));
}

// Stop all haptic effects
void haptic_stop_all(HapticManager *hm) {
    if (!hm || !hm->haptic || !hm->haptic_supported) return;
    
    SDL_HapticStopAll(hm->haptic);
}

// Trigger a haptic effect by type
void haptic_trigger_effect(HapticManager *hm, HapticEffectType effect_type) {
    if (!hm || !hm->haptic || !hm->haptic_supported) return;
    
    HapticEffect effect = haptic_get_effect_params(effect_type);
    haptic_trigger_custom(hm, effect.left_motor, effect.right_motor, 
                         effect.duration_ms, effect.repeats);
}

// Custom haptic effect with specific left/right motor control
void haptic_trigger_custom(HapticManager *hm, int left_intensity, int right_intensity, 
                          int duration_ms, int repeats) {
    if (!hm || !hm->haptic || !hm->haptic_supported) return;
    
    // Clamp values to valid range
    if (left_intensity < 0) left_intensity = 0;
    if (left_intensity > 255) left_intensity = 255;
    if (right_intensity < 0) right_intensity = 0;
    if (right_intensity > 255) right_intensity = 255;
    if (duration_ms < 10) duration_ms = 10;
    if (duration_ms > 5000) duration_ms = 5000;
    if (repeats < 1) repeats = 1;
    if (repeats > 10) repeats = 10;
    
    // Convert 0-255 to 0.0-1.0 for SDL
    float left = left_intensity / 255.0f;
    float right = right_intensity / 255.0f;
    
    // Play rumble effect
    if (hm->rumble_supported) {
        // SDL_HapticRumblePlay plays a rumble effect
        // For dual-motor control, we use the average strength
        // Note: True dual-motor control would require periodic effects
        float strength = (left + right) / 2.0f;
        float duration = duration_ms / 1000.0f;
        
        if (SDL_HapticRumblePlay(hm->haptic, strength, (uint32_t)(duration * 1000)) != 0) {
            SDL_Log("[Comet Busters] [HAPTIC] Warning: Rumble play failed: %s\n", 
                    SDL_GetError());
        }
    }
}

// Update haptic manager (for timing-based effects in the future)
void haptic_update(HapticManager *hm, float dt) {
    if (!hm || !hm->haptic) return;
    // Reserved for future timing-based effects
}

// Check if haptic is available
bool haptic_is_available(HapticManager *hm) {
    if (!hm) return false;
    return hm->haptic_supported && hm->initialized;
}
