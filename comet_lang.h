typedef enum { 
    LANG_ENGLISH = 0, 
    LANG_SPANISH = 1,
    LANG_FRENCH = 2,
    LANG_RUSSIAN = 3, 
    LANG_COUNT 
} Language;

static const char* bomb_text[] = { 
    "Bomb", // English 
    "Bomba", // Spanish
    "Bombe", // French 
    "Бомба" // Russian 
};

static const char* life_text[] = {
    "+1 LIFE",      // English
    "+1 VIDA",      // Spanish
    "+1 VIE",       // French
    "+1 ЖИЗНЬ"      // Russian
};

static const char* multiplier_text[] = {
    "Multiplier",   // English
    "Multiplicador",// Spanish
    "Multiplicateur",// French
    "Множитель"     // Russian
};

static const char* boss_destroyed_text[] = {
    "BOSS DESTROYED!",     // English
    "¡JEFE DESTRUIDO!",    // Spanish
    "BOSS DÉTRUIT!",       // French
    "БОСС УНИЧТОЖЕН!"      // Russian
};


