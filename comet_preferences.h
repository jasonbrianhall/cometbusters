#ifndef COMET_PREFERENCES_H
#define COMET_PREFERENCES_H

#include "comet_lang.h"

// Preferences structure
typedef struct {
    int language;           // Language enum (LANG_ENGLISH, LANG_SPANISH, etc.)
    int music_volume;       // 0-100
    int sfx_volume;         // 0-100
} CometPreferences;

// Initialize default preferences
void preferences_init_defaults(CometPreferences *prefs);

// Load preferences from disk
bool preferences_load(CometPreferences *prefs);

// Save preferences to disk
bool preferences_save(const CometPreferences *prefs);

// Get the preferences file path
const char* preferences_get_path(void);

#endif // COMET_PREFERENCES_H
