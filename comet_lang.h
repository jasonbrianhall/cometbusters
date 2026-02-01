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

static const char* wave_complete_text[] = {
    "WAVE 30 COMPLETE",      // English
    "OLEADA 30 COMPLETADA",  // Spanish
    "VAGUE 30 TERMINÉE",     // French
    "ВОЛНА 30 ЗАВЕРШЕНА"     // Russian
};

static const char* phase0_text[] = {
    "GRAVITATIONAL PULL",      // English
    "ATRACCIÓN GRAVITACIONAL", // Spanish
    "ATTRACTION GRAVITATIONNELLE", // French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ" // Russian
};

static const char* phase1_text[] = {
    "STELLAR COLLAPSE",
    "COLAPSO ESTELAR",
    "EFFONDREMENT STELLAIRE",
    "ЗВЁЗДНЫЙ КОЛЛАПС"
};

static const char* phase2_text[] = {
    "VOID EXPANSION",
    "EXPANSIÓN DEL VACÍO",
    "EXPANSION DU VIDE",
    "РАСШИРЕНИЕ ПУСТОТЫ"
};

static const char* phase3_text[] = {
    "SINGULARITY COLLAPSE",
    "COLAPSO DE LA SINGULARIDAD",
    "EFFONDREMENT DE LA SINGULARITÉ",
    "КОЛЛАПС СИНГУЛЯРНОСТИ"
};

static const char* phase_unknown_text[] = {
    "UNKNOWN",
    "DESCONOCIDO",
    "INCONNU",
    "НЕИЗВЕСТНО"
};

static const char* phase_dormant_text[] = {
    "DORMANT",      // English
    "DURMIENTE",    // Spanish
    "SOMMEIL",      // French
    "СПЯЩИЙ"        // Russian
};

static const char* phase_active_text[] = {
    "ACTIVE",
    "ACTIVO",
    "ACTIF",
    "АКТИВНЫЙ"
};

static const char* phase_frenzy_text[] = {
    "FRENZY",
    "FRENESÍ",
    "FRÉNÉSIE",
    "БЕШЕНСТВО"
};

static const char* continue_texts[] = {
    "RIGHT-CLICK TO CONTINUE TO WAVE 31",      // English
    "CLIC DERECHO PARA CONTINUAR A LA OLEADA 31", // Spanish
    "CLIC DROIT POUR CONTINUER VERS LA VAGUE 31", // French
    "ПРАВЫЙ КЛИК, ЧТОБЫ ПРОДОЛЖИТЬ К ВОЛНЕ 31"    // Russian
};


