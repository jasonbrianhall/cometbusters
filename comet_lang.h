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

static const char* phase_stabilizing_text[] = {
    "STABILIZING",     // English
    "ESTABILIZANDO",   // Spanish
    "STABILISATION",   // French
    "СТАБИЛИЗАЦИЯ"     // Russian
};

static const char* phase_fragmenting_text[] = {
    "FRAGMENTING",
    "FRAGMENTANDO",
    "FRAGMENTATION",
    "ФРАГМЕНТАЦИЯ"
};

static const char* fragment_text[] = {
    "FRAGMENT!",        // English
    "¡FRAGMENTO!",      // Spanish
    "FRAGMENT !",       // French
    "ФРАГМЕНТ!"         // Russian
};

static const char* gravitational_pull_text[] = {
    "GRAVITATIONAL PULL",        // English
    "ATRACCIÓN GRAVITATORIA",    // Spanish
    "ATTRACTION GRAVITATIONNELLE",// French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ"  // Russian
};

static const char* void_expansion_imminent_text[] = {
    "VOID EXPANSION IMMINENT",        // English
    "EXPANSIÓN DEL VACÍO INMINENTE",  // Spanish
    "EXPANSION DU VIDE IMMINENTE",    // French
    "РАСШИРЕНИЕ ПУСТОТЫ НЕИЗБЕЖНО"    // Russian
};

static const char* singularity_collapse_initiated_text[] = {
    "SINGULARITY COLLAPSE INITIATED",        // English
    "COLAPSO DE SINGULARIDAD INICIADO",      // Spanish
    "EFFONDREMENT DE LA SINGULARITÉ INITIÉ", // French
    "КОЛЛАПС СИНГУЛЯРНОСТИ ЗАПУЩЕН"          // Russian
};

static const char* singularity_collapsed_text[] = {
    "SINGULARITY COLLAPSED",          // English
    "SINGULARIDAD COLAPSADA",         // Spanish
    "SINGULARITÉ EFFONDREE",          // French
    "СИНГУЛЯРНОСТЬ РУХНУЛА"           // Russian
};

static const char* dimensional_barrier_restored_text[] = {
    "DIMENSIONAL BARRIER RESTORED",        // English
    "BARRERA DIMENSIONAL RESTAURADA",      // Spanish
    "BARRIÈRE DIMENSIONNELLE RESTAURÉE",   // French
    "ИЗМЕРЕНЧЕСКИЙ БАРЬЕР ВОССТАНОВЛЕН"    // Russian
};

static const char* cosmic_threat_eliminated_text[] = {
    "COSMIC THREAT ELIMINATED",        // English
    "AMENAZA CÓSMICA ELIMINADA",       // Spanish
    "MENACE COSMIQUE ÉLIMINÉE",        // French
    "КОСМИЧЕСКАЯ УГРОЗА УСТРАНЕНА"     // Russian
};




static const char* gravitational_field_intensifies_text[] = {
    "GRAVITATIONAL FIELD INTENSIFIES",        // English
    "EL CAMPO GRAVITATORIO SE INTENSIFICA",   // Spanish
    "LE CHAMP GRAVITATIONNEL S’INTENSIFIE",   // French
    "ГРАВИТАЦИОННОЕ ПОЛЕ УСИЛИВАЕТСЯ"         // Russian
};

static const char* phase_dispersing_text[] = {
    "DISPERSING",
    "DISPERSANDO",
    "DISPERSION",
    "РАССЕИВАНИЕ"
};

static const char* subtitle_texts[] = {
    "Press fire key to start",            // English
    "Pulsa la tecla de disparo para iniciar", // Spanish
    "Appuyez sur la touche tir pour démarrer", // French
    "Нажмите клавишу огня, чтобы начать"      // Russian
};

static const char* phase_normal_text[] = {
    "NORMAL",        // English
    "NORMAL",        // Spanish
    "NORMAL",        // French
    "НОРМАЛЬНО"      // Russian
};

static const char* phase_shielded_text[] = {
    "SHIELDED",
    "BLINDADO",
    "PROTÉGÉ",
    "ЗАЩИЩЁН"
};

static const char* phase_enraged_text[] = {
    "ENRAGED!",
    "¡ENFURECIDO!",
    "ENRAGÉ!",
    "ВБЕШЕНСТВЕ!"
};

static const char* queen_phase_recruiting_text[] = {
    "RECRUITING",        // English
    "RECLUTANDO",        // Spanish
    "RECRUTEMENT",       // French
    "ВЕРБОВКА"           // Russian
};

static const char* queen_phase_aggressive_text[] = {
    "AGGRESSIVE",
    "AGRESIVA",
    "AGRESSIVE",
    "АГРЕССИВНАЯ"
};

static const char* queen_phase_desperate_text[] = {
    "DESPERATE!",
    "¡DESESPERADA!",
    "DÉSESPÉRÉE!",
    "ОТЧАЯННАЯ!"
};

static const char* boss_bonus_text[] = {
    "BOSS BONUS",        // English
    "BONO DE JEFE",      // Spanish
    "BONUS DE BOSS",     // French
    "БОНУС ЗА БОССА"      // Russian
};

static const char* phase1_offensive_text[] = {
    "PHASE 1: OFFENSIVE!",          // English
    "FASE 1: OFENSIVA!",            // Spanish
    "PHASE 1 : OFFENSIVE !",        // French
    "ФАЗА 1: НАСТУПЛЕНИЕ!"          // Russian
};


static const char* phase2_detonation_text[] = {
    "PHASE 2: DETONATION!",          // English
    "FASE 2: DETONACIÓN!",           // Spanish
    "PHASE 2 : DÉTONATION !",        // French
    "ФАЗА 2: ДЕТОНАЦИЯ!"             // Russian
};

static const char* spawning_escorts_text[] = {
    "SPAWNING ESCORTS!",          // English
    "GENERANDO ESCOLTAS!",        // Spanish
    "APPARITION D'ESCORTES !",    // French
    "ПРИЗЫВ ЭСКОРТОВ!"            // Russian
};

static const char* suppressing_threats_text[] = {
    "SUPPRESSING THREATS!",          // English
    "SUPRIMIENDO AMENAZAS!",         // Spanish
    "SUPPRESSION DES MENACES !",     // French
    "ПОДАВЛЕНИЕ УГРОЗ!"              // Russian
};

static const char* core_destabilizing_text[] = {
    "CORE DESTABILIZING!",          // English
    "¡NÚCLEO DESestabilizándose!",  // Spanish
    "DÉSTABILISATION DU NŒUD !",    // French
    "ЯДРО ДЕСТАБИЛИЗИРУЕТСЯ!"        // Russian
};

static const char* star_vortex_destroyed_text[] = {
    "STAR VORTEX DESTROYED!",        // English
    "¡VÓRTICE ESTELAR DESTRUIDO!",   // Spanish
    "VORTEX STELLAIRE DÉTRUIT !",    // French
    "ЗВЁЗДНЫЙ ВИХРЬ УНИЧТОЖЕН!"      // Russian
};

static const char* score_label_text[] = {
    "SCORE:",        // English
    "PUNTOS:",       // Spanish
    "SCORE :",       // French
    "СЧЁТ:"          // Russian
};

static const char* lives_label_text[] = {
    "LIVES:",        // English
    "VIDAS:",        // Spanish
    "VIES :",        // French
    "ЖИЗНИ:"         // Russian
};

static const char* shield_label_text[] = {
    "SHIELD:",        // English
    "ESCUDO:",        // Spanish
    "BOUCLIER :",     // French
    "ЩИТ:"            // Russian
};

static const char* shield2_label_text[] = {
    "+SHIELD",        // English
    "+ESCUDO",        // Spanish
    "+BOUCLIER",     // French
    "+ЩИТ"            // Russian
};

static const char* HUD_WAVE_LABEL[LANG_COUNT] = {
    "WAVE: %d",        // EN
    "OLEADA: %d",      // ES
    "VAGUE: %d",       // FR
    "ВОЛНА: %d",       // RU
};

static const char* asteroids_label_text[] = {
    "ASTEROIDS:",      // English
    "ASTEROIDES:",     // Spanish
    "ASTÉROÏDES :",    // French
    "АСТЕРОИДЫ:"       // Russian
};

static const char* next_wave_in_text[] = {
    "NEXT WAVE in",          // English
    "PRÓXIMA OLEADA en",     // Spanish
    "PROCHAINE VAGUE dans",  // French
    "СЛЕДУЮЩАЯ ВОЛНА через"  // Russian
};

static const char* destroyed_label_text[] = {
    "DESTROYED:",        // English
    "DESTRUIDOS:",       // Spanish
    "DÉTRUITS :",        // French
    "УНИЧТОЖЕНО:"        // Russian
};

static const char* energy_label_text[] = {
    "ENERGY:",        // English
    "ENERGÍA:",       // Spanish
    "ÉNERGIE :",      // French
    "ЭНЕРГИЯ:"        // Russian
};

static const char* missiles_label_text[] = {
    "MISSILES:",        // English
    "MISILES:",         // Spanish
    "MISSILES :",       // French
    "РАКЕТЫ:"           // Russian
};

static const char* missiles2_label_text[] = {
    "+20 MISSILES",        // English
    "+20 MISILES",         // Spanish
    "+20 MISSILES",       // French
    "+20 РАКЕТЫ"           // Russian
};

static const char* bombs_label_text[] = {
    "BOMBS:",        // English
    "BOMBAS:",       // Spanish
    "BOMBES :",      // French
    "БОМБЫ:"         // Russian
};

static const char* weapon_name_bombs[] = {
    "Bombs",     // English
    "Bombas",    // Spanish
    "Bombes",    // French
    "Бомбы"      // Russian
};

static const char* weapon_name_spread_fire[] = {
    "Spread Fire",        // English
    "Disparo Abierto",    // Spanish (literally “open/spread shot”)
    "Tir Éventail",       // French (“fan‑shaped shot” — standard shmup phrasing)
    "Рассеянный Огонь"    // Russian (“scattered fire” — perfect for this weapon type)
};

static const char* perfect_victory_text[] = {
    "PERFECT VICTORY!",        // English
    "¡VICTORIA PERFECTA!",     // Spanish
    "VICTOIRE PARFAITE !",     // French
    "ИДЕАЛЬНАЯ ПОБЕДА!"        // Russian
};


static const char* weapon_name_bullets[] = {
    "Bullets",      // English
    "Balas",        // Spanish
    "Balles",       // French
    "Пули"          // Russian
};

static const char* weapon_name_missiles[] = {
    "Missiles",     // English
    "Misiles",      // Spanish
    "Missiles",     // French
    "Ракеты"        // Russian
};

static const char* armed_label_text[] = {
    "Armed:",        // English
    "Armado:",       // Spanish
    "Armé :",        // French
    "Заряжено:"      // Russian
};

static const char* final_score_label_text[] = {
    "FINAL SCORE:",        // English
    "PUNTUACIÓN FINAL:",   // Spanish
    "SCORE FINAL :",       // French
    "ФИНАЛЬНЫЙ СЧЁТ:"      // Russian
};

static const char* wave_reached_label_text[] = {
    "WAVE REACHED:",        // English
    "OLEADA ALCANZADA:",    // Spanish
    "VAGUE ATTEINTE :",     // French
    "ДОСТИГНУТАЯ ВОЛНА:"    // Russian
};

static const char* shield_hit_text[] = {
    "SHIELD HIT",          // English
    "GOLPE AL ESCUDO",     // Spanish
    "BOUCLIER TOUCHÉ",     // French
    "ЩИТ ПОВРЕЖДЁН"        // Russian
};

static const char* energy_used_text[] = {
    "ENERGY USED",        // English
    "ENERGÍA USADA",      // Spanish
    "ÉNERGIE UTILISÉE",   // French
    "ИСПОЛЬЗОВАНА ЭНЕРГИЯ"// Russian
};

static const char* mothership_down_text[] = {
    "MOTHERSHIP DOWN",        // English
    "NAVE MADRE CAÍDA",       // Spanish
    "VAISSEAU-MÈRE ABATTU",   // French
    "МАТЕРИНСКИЙ КОРАБЛЬ УНИЧТОЖЕН"  // Russian
};

static const char* multiplier_bonus_text[] = {
    "MULTIPLIER BONUS!",        // English
    "BONO DE MULTIPLICADOR!",   // Spanish
    "BONUS DE MULTIPLICATEUR !",// French
    "БОНУС МНОЖИТЕЛЯ!"          // Russian
};

static const char* nexus_shattered_text[] = {
    "NEXUS SHATTERED",          // English
    "NEXO DESTRUIDO",           // Spanish
    "NEXUS BRISÉ",              // French
    "НЕКСУС РАЗРУШЕН"           // Russian
};

static const char* crystalline_collapse_text[] = {
    "CRYSTALLINE COLLAPSE",         // English
    "COLAPSO CRISTALINO",           // Spanish
    "EFFONDREMENT CRISTALLIN",      // French
    "КРИСТАЛЛИЧЕСКИЙ КОЛЛАПС"       // Russian
};

static const char* perfect_destruction_text[] = {
    "PERFECT DESTRUCTION!",        // English
    "DESTRUCCIÓN PERFECTA!",       // Spanish
    "DESTRUCTION PARFAITE !",      // French
    "ИДЕАЛЬНОЕ УНИЧТОЖЕНИЕ!"       // Russian
};



