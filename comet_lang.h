typedef enum { 
    WLANG_ENGLISH = 0, 
    WLANG_SPANISH = 1,
    WLANG_FRENCH = 2,
    WLANG_RUSSIAN = 3, 
    WLANG_GERMAN = 4,
    WLANG_COUNT 
} WLanguage;

static const char *wlanguagename[] = {
   "English",
   "Español (Spanish)",
   "Français (French)",
   "Русский (Russian)",
   "Deutsch (German)"
};

static const char *wlanguage_intro_file[] = {
    "music/intro.mp3", // English
    "music/intro_es.mp3", // Spanish
    "music/intro_fr.mp3", // French
    "music/intro_ru.mp3", // Russian
    "music/intro.mp3", // German (needs replaced)
};  

static const char *wlanguage_finale_file[] = {
    "music/finale.mp3", // English
    "music/finale_es.mp3", // Spanish
    "music/finale_fr.mp3", // French
    "music/finale_ru.mp3", // Russian
    "music/finale.mp3", // German (needs replaced)
};  


static const char* bomb_text[] = { 
    "Bomb", // English 
    "Bomba", // Spanish
    "Bombe", // French 
    "Бомба", // Russian
    "Bombe" // German
};

static const char* life_text[] = {
    "+1 LIFE",      // English
    "+1 VIDA",      // Spanish
    "+1 VIE",       // French
    "+1 ЖИЗНЬ",     // Russian
    "+1 LEBEN"      // German
};

static const char* multiplier_text[] = {
    "Multiplier",   // English
    "Multiplicador",// Spanish
    "Multiplicateur",// French
    "Множитель",    // Russian
    "Multiplikator" // German
};

static const char* boss_destroyed_text[] = {
    "BOSS DESTROYED!",     // English
    "¡JEFE DESTRUIDO!",    // Spanish
    "BOSS DÉTRUIT!",       // French
    "БОСС УНИЧТОЖЕН!",     // Russian
    "BOSS ZERSTÖRT!"       // German
};

static const char* wave_complete_text[] = {
    "WAVE 30 COMPLETE",      // English
    "OLEADA 30 COMPLETADA",  // Spanish
    "VAGUE 30 TERMINÉE",     // French
    "ВОЛНА 30 ЗАВЕРШЕНА",    // Russian
    "WELLE 30 ABGESCHLOSSEN" // German
};

static const char* phase0_text[] = {
    "GRAVITATIONAL PULL",      // English
    "ATRACCIÓN GRAVITACIONAL", // Spanish
    "ATTRACTION GRAVITATIONNELLE", // French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ", // Russian
    "GRAVITATIONSZUG"          // German
};

static const char* phase1_text[] = {
    "STELLAR COLLAPSE",
    "COLAPSO ESTELAR",
    "EFFONDREMENT STELLAIRE",
    "ЗВЁЗДНЫЙ КОЛЛАПС",
    "STELLARER ZUSAMMENBRUCH"
};

static const char* phase2_text[] = {
    "VOID EXPANSION",
    "EXPANSIÓN DEL VACÍO",
    "EXPANSION DU VIDE",
    "РАСШИРЕНИЕ ПУСТОТЫ",
    "VAKUUMAUSDEHNUNG"
};

static const char* phase3_text[] = {
    "SINGULARITY COLLAPSE",
    "COLAPSO DE LA SINGULARIDAD",
    "EFFONDREMENT DE LA SINGULARITÉ",
    "КОЛЛАПС СИНГУЛЯРНОСТИ",
    "SINGULARITÄTSKOLLAPS"
};

static const char* phase_unknown_text[] = {
    "UNKNOWN",
    "DESCONOCIDO",
    "INCONNU",
    "НЕИЗВЕСТНО",
    "UNBEKANNT"
};

static const char* phase_dormant_text[] = {
    "DORMANT",      // English
    "DURMIENTE",    // Spanish
    "SOMMEIL",      // French
    "СПЯЩИЙ",       // Russian
    "RUHEND"        // German
};

static const char* phase_active_text[] = {
    "ACTIVE",
    "ACTIVO",
    "ACTIF",
    "АКТИВНЫЙ",
    "AKTIV"
};

static const char* phase_frenzy_text[] = {
    "FRENZY",
    "FRENESÍ",
    "FRÉNÉSIE",
    "БЕШЕНСТВО",
    "RASEREI"
};

static const char* continue_texts[] = {
    "RIGHT-CLICK TO CONTINUE TO WAVE 31",      // English
    "CLIC DERECHO PARA CONTINUAR A LA OLEADA 31", // Spanish
    "CLIC DROIT POUR CONTINUER VERS LA VAGUE 31", // French
    "ПРАВЫЙ КЛИК, ЧТОБЫ ПРОДОЛЖИТЬ К ВОЛНЕ 31",  // Russian
    "RECHTSKLICK ZUM FORTFAHREN ZUR WELLE 31"    // German
};

static const char* phase_stabilizing_text[] = {
    "STABILIZING",     // English
    "ESTABILIZANDO",   // Spanish
    "STABILISATION",   // French
    "СТАБИЛИЗАЦИЯ",    // Russian
    "STABILISIERUNG"   // German
};

static const char* phase_fragmenting_text[] = {
    "FRAGMENTING",
    "FRAGMENTANDO",
    "FRAGMENTATION",
    "ФРАГМЕНТАЦИЯ",
    "FRAGMENTIERUNG"
};

static const char* fragment_text[] = {
    "FRAGMENT!",        // English
    "¡FRAGMENTO!",      // Spanish
    "FRAGMENT !",       // French
    "ФРАГМЕНТ!",        // Russian
    "FRAGMENT!"         // German
};

static const char* gravitational_pull_text[] = {
    "GRAVITATIONAL PULL",        // English
    "ATRACCIÓN GRAVITATORIA",    // Spanish
    "ATTRACTION GRAVITATIONNELLE",// French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ",  // Russian
    "GRAVITATIONSZUG"            // German
};

static const char* void_expansion_imminent_text[] = {
    "VOID EXPANSION IMMINENT",        // English
    "EXPANSIÓN DEL VACÍO INMINENTE",  // Spanish
    "EXPANSION DU VIDE IMMINENTE",    // French
    "РАСШИРЕНИЕ ПУСТОТЫ НЕИЗБЕЖНО",   // Russian
    "VAKUUMAUSDEHNUNG UNMITTELBAR"    // German
};

static const char* singularity_collapse_initiated_text[] = {
    "SINGULARITY COLLAPSE INITIATED",        // English
    "COLAPSO DE SINGULARIDAD INICIADO",      // Spanish
    "EFFONDREMENT DE LA SINGULARITÉ INITIÉ", // French
    "КОЛЛАПС СИНГУЛЯРНОСТИ ЗАПУЩЕН",         // Russian
    "SINGULARITÄTSKOLLAPS EINGELEITET"       // German
};

static const char* singularity_collapsed_text[] = {
    "SINGULARITY COLLAPSED",          // English
    "SINGULARIDAD COLAPSADA",         // Spanish
    "SINGULARITÉ EFFONDRÉE",          // French
    "СИНГУЛЯРНОСТЬ РУХНУЛА",          // Russian
    "SINGULARITÄT KOLLABIERT"         // German
};

static const char* dimensional_barrier_restored_text[] = {
    "DIMENSIONAL BARRIER RESTORED",        // English
    "BARRERA DIMENSIONAL RESTAURADA",      // Spanish
    "BARRIÈRE DIMENSIONNELLE RESTAURÉE",   // French
    "МЕРНЫЙ БАРЬЕР ВОССТАНОВЛЕН",          // Russian
    "DIMENSIONALE BARRIERE WIEDERHERGESTELLT" // German
};

static const char* cosmic_threat_eliminated_text[] = {
    "COSMIC THREAT ELIMINATED",        // English
    "AMENAZA CÓSMICA ELIMINADA",       // Spanish
    "MENACE COSMIQUE ÉLIMINÉE",        // French
    "КОСМИЧЕСКАЯ УГРОЗА УСТРАНЕНА",    // Russian
    "KOSMISCHE BEDROHUNG BESEITIGT"    // German
};




static const char* gravitational_field_intensifies_text[] = {
    "GRAVITATIONAL FIELD INTENSIFIES",        // English
    "EL CAMPO GRAVITATORIO SE INTENSIFICA",   // Spanish
    "LE CHAMP GRAVITATIONNEL S'INTENSIFIE",   // French
    "ГРАВИТАЦИОННОЕ ПОЛЕ УСИЛИВАЕТСЯ",       // Russian
    "GRAVITATIONSFELD WIRD INTENSIVER"        // German
};

static const char* phase_dispersing_text[] = {
    "DISPERSING",
    "DISPERSANDO",
    "DISPERSION",
    "РАССЕИВАНИЕ",
    "ZERSTREUUNG"
};

static const char* subtitle_texts[] = {
    "Press fire key to start",            // English
    "Pulsa la tecla de disparo para iniciar", // Spanish
    "Appuyez sur la touche tir pour démarrer", // French
    "Нажмите клавишу огня, чтобы начать",    // Russian
    "Drücke Feuer-Taste zum Starten"         // German
};

static const char* phase_normal_text[] = {
    "NORMAL",        // English
    "NORMAL",        // Spanish
    "NORMAL",        // French
    "НОРМАЛЬНО",     // Russian
    "NORMAL"         // German
};

static const char* phase_shielded_text[] = {
    "SHIELDED",
    "BLINDADO",
    "PROTÉGÉ",
    "ЗАЩИЩЁН",
    "ABGESCHIRMT"
};

static const char* provoked_text[] = {
    "PROVOKED!",        // English
    "¡PROVOCADO!",      // Spanish
    "PROVOQUÉ !",       // French
    "ПРОВОЦИРОВАН!",    // Russian
    "PROVOZIERT!"       // German
};

static const char* energy_drained_text[] = {
    "ENERGY DRAINED!",        // English
    "¡ENERGÍA AGOTADA!",      // Spanish
    "ÉNERGIE ÉPUISÉE !",      // French
    "ЭНЕРГИЯ ИСЧЕРПАНА!",     // Russian
    "ENERGIE ENTLADEN!"       // German
};



static const char* phase_enraged_text[] = {
    "ENRAGED!",
    "¡ENFURECIDO!",
    "ENRAGÉ!",
    "РАЗЪЯРЁН!",
    "WÜTEND!"
};

static const char* queen_phase_recruiting_text[] = {
    "RECRUITING",        // English
    "RECLUTANDO",        // Spanish
    "RECRUTEMENT",       // French
    "ВЕРБОВКА",          // Russian
    "REKRUTIERUNG"       // German
};

static const char* queen_phase_aggressive_text[] = {
    "AGGRESSIVE",
    "AGRESIVA",
    "AGRESSIVE",
    "АГРЕССИВНАЯ",
    "AGGRESSIV"
};

static const char* queen_phase_desperate_text[] = {
    "DESPERATE!",
    "¡DESESPERADA!",
    "DÉSESPÉRÉE!",
    "ОТЧАЯННАЯ!",
    "VERZWEIFELT!"
};

static const char* boss_bonus_text[] = {
    "BOSS BONUS",        // English
    "BONO DE JEFE",      // Spanish
    "BONUS DE BOSS",     // French
    "БОНУС ЗА БОССА",    // Russian
    "BOSS-BONUS"         // German
};

static const char* phase1_offensive_text[] = {
    "PHASE 1: OFFENSIVE!",          // English
    "FASE 1: OFENSIVA!",            // Spanish
    "PHASE 1 : OFFENSIVE !",        // French
    "ФАЗА 1: НАСТУПЛЕНИЕ!",         // Russian
    "PHASE 1: ANGRIFF!"             // German
};


static const char* phase2_detonation_text[] = {
    "PHASE 2: DETONATION!",          // English
    "FASE 2: DETONACIÓN!",           // Spanish
    "PHASE 2 : DÉTONATION !",        // French
    "ФАЗА 2: ДЕТОНАЦИЯ!",            // Russian
    "PHASE 2: DETONATION!"           // German
};

static const char* spawning_escorts_text[] = {
    "SPAWNING ESCORTS!",          // English
    "GENERANDO ESCOLTAS!",        // Spanish
    "APPARITION D'ESCORTES !",    // French
    "ПРИЗЫВ ЭСКОРТОВ!",           // Russian
    "ESKORTE ERSCHEINEN!"         // German
};

static const char* suppressing_threats_text[] = {
    "SUPPRESSING THREATS!",          // English
    "SUPRIMIENDO AMENAZAS!",         // Spanish
    "SUPPRESSION DES MENACES !",     // French
    "ПОДАВЛЕНИЕ УГРОЗ!",             // Russian
    "UNTERDRÜCKUNG VON BEDROHUNGEN!" // German
};

static const char* core_destabilizing_text[] = {
    "CORE DESTABILIZING!",          // English
    "¡NÚCLEO DESESTABILIZÁNDOSE!",  // Spanish
    "DÉSTABILISATION DU NŒUD !",    // French
    "ЯДРО ДЕСТАБИЛИЗИРУЕТСЯ!",      // Russian
    "KERN DESTABILISIERT!"          // German
};

static const char* nexus_shattered_text[] = {
    "NEXUS SHATTERED",          // English
    "NEXO DESTRUIDO",           // Spanish
    "NEXUS BRISÉ",              // French
    "НЕКСУС РАЗРУШЕН",          // Russian
    "NEXUS ZERSCHMETTERT"       // German
};

static const char* crystalline_collapse_text[] = {
    "CRYSTALLINE COLLAPSE",         // English
    "COLAPSO CRISTALINO",           // Spanish
    "EFFONDREMENT CRISTALLIN",      // French
    "КРИСТАЛЛИЧЕСКИЙ КОЛЛАПС",      // Russian
    "KRISTALLINER ZUSAMMENBRUCH"    // German
};

static const char* perfect_destruction_text[] = {
    "PERFECT DESTRUCTION!",        // English
    "DESTRUCCIÓN PERFECTA!",       // Spanish
    "DESTRUCTION PARFAITE !",      // French
    "ИДЕАЛЬНОЕ УНИЧТОЖЕНИЕ!",      // Russian
    "PERFEKTE ZERSTÖRUNG!"         // German
};

static const char* main_menu_items[][6] = {
    {   // English
        "CONTINUE",
        "NEW GAME",
        "HIGH SCORES",
        "AUDIO",
        "LANGUAGE",
        "QUIT"
    },
    {   // Spanish
        "CONTINUAR",
        "NUEVA PARTIDA",
        "PUNTUACIONES",
        "AUDIO",
        "IDIOMA (Language)",
        "SALIR"
    },
    {   // French
        "CONTINUER",
        "NOUVELLE PARTIE",
        "MEILLEURS SCORES",
        "AUDIO",
        "LANGUE (Language)",
        "QUITTER"
    },
    {   // Russian
        "ПРОДОЛЖИТЬ",
        "НОВАЯ ИГРА",
        "РЕКОРДЫ",
        "АУДИО",
        "ЯЗЫК (Language)",
        "ВЫХОД"
    },
    {   // German
        "FORTFAHREN",
        "NEUES SPIEL",
        "HIGHSCORES",
        "AUDIO",
        "SPRACHE (Language)",
        "BEENDEN"
    }
};

static const char* hint_select_close[][1] = {
    { "Up/Down/Enter to select; ESC to close" },        // EN
    { "Arriba/Abajo/Enter para elegir; ESC para cerrar" }, // ES
    { "Haut/Bas/Entrée pour choisir; Échap pour fermer" }, // FR
    { "Вверх/Вниз/Enter — выбрать; ESC — закрыть" },    // RU
    { "Auf/Ab/Enter zum Auswählen; ESC zum Schließen" } // DE
};

static const char* label_high_scores[] = {
    "HIGH SCORES",      // EN
    "PUNTUACIONES",     // ES
    "MEILLEURS SCORES", // FR
    "РЕКОРДЫ",          // RU
    "HIGHSCORES"        // DE
};

static const char* hint_continue_back[] = {
    "ENTER to continue | ESC to go back",          // EN
    "ENTER para continuar | ESC para volver",      // ES
    "ENTRÉE pour continuer | ÉCHAP pour revenir",  // FR
    "ENTER — продолжить | ESC — назад",            // RU
    "ENTER zum Fortfahren | ESC zum Zurück"        // DE
};

static const char* label_audio_settings[] = {
    "AUDIO SETTINGS",     // EN
    "AJUSTES DE AUDIO",   // ES
    "RÉGLAGES AUDIO",     // FR
    "НАСТРОЙКИ АУДИО",    // RU
    "AUDIOEINSTELLUNGEN"  // DE
};

static const char* menu_audio_options[][2] = {
    {   // EN
        "MUSIC VOLUME",
        "SFX VOLUME"
    },
    {   // ES
        "VOLUMEN DE MÚSICA",
        "VOLUMEN DE SFX"
    },
    {   // FR
        "VOLUME MUSIQUE",
        "VOLUME EFFETS"
    },
    {   // RU
        "ГРОМКОСТЬ МУЗЫКИ",
        "ГРОМКОСТЬ ЭФФЕКТОВ"
    },
    {   // DE
        "MUSIKLAUTSTÄRKE",
        "EFFEKTLAUTSTÄRKE"
    }
};

static const char* menu_difficulties[][3] = {
    {   // EN
        "EASY",
        "NORMAL",
        "HARD"
    },
    {   // ES
        "FÁCIL",
        "NORMAL",
        "DIFÍCIL"
    },
    {   // FR
        "FACILE",
        "NORMAL",
        "DIFFICILE"
    },
    {   // RU
        "ЛЁГКО",
        "НОРМАЛЬНО",
        "ТРУДНО"
    },
    {   // DE
        "LEICHT",
        "NORMAL",
        "SCHWER"
    }
};

static const char* hint_name_entry[] = {
    "Type your name | ENTER to save | BackSpace to delete",          // EN
    "Escribe tu nombre | ENTER para guardar | BackSpace para borrar", // ES
    "Tapez votre nom | ENTRÉE pour valider | BackSpace pour effacer", // FR
    "Введите имя | ENTER — сохранить | BackSpace — удалить",        // RU
    "Geben Sie Ihren Namen ein | ENTER zum Speichern | BackSpace zum Löschen" // DE
};

static const char* hint_max_chars[] = {
    "Max 32 characters",          // EN
    "Máx. 32 caracteres",         // ES
    "Max 32 caractères",          // FR
    "Макс. 32 символа",           // RU
    "Max 32 Zeichen"              // DE
};

static const char* fmt_score_wave[] = {
    "Score: %d | Wave: %d",          // EN
    "Puntuación: %d | Oleada: %d",   // ES
    "Score : %d | Vague : %d",       // FR
    "Счёт: %d | Волна: %d",          // RU
    "Punktzahl: %d | Welle: %d"      // DE
};

static const char* label_select_difficulty[] = {
    "SELECT DIFFICULTY",     // EN
    "SELECCIONA DIFICULTAD", // ES
    "SÉLECTIONNEZ LA DIFFICULTÉ", // FR
    "ВЫБЕРИТЕ СЛОЖНОСТЬ",    // RU
    "SCHWIERIGKEITSGRAD WÄHLEN" // DE
};

static const char* hint_adjust_menu[] = {
    "UP/DOWN to select | LEFT/RIGHT to adjust | ESC to go back",              // EN
    "ARRIBA/ABAJO para elegir | IZQ/DER para ajustar | ESC para volver",     // ES
    "HAUT/BAS pour choisir | GAUCHE/DROITE pour régler | ÉCHAP pour revenir",// FR
    "ВВЕРХ/ВНИЗ — выбрать | ВЛЕВО/ВПРАВО — изменить | ESC — назад",         // RU
    "OBEN/UNTEN zum Auswählen | LINKS/RECHTS zum Anpassen | ESC zum Zurück" // DE
};

static const char* label_new_high_score[] = {
    "NEW HIGH SCORE!",      // EN
    "¡NUEVA PUNTUACIÓN!",   // ES
    "NOUVEAU RECORD !",     // FR
    "НОВЫЙ РЕКОРД!",        // RU
    "NEUER HIGHSCORE!"      // DE
};

static const char* label_enter_name[] = {
    "Enter Your Name:",        // EN
    "Ingresa tu nombre:",      // ES
    "Entrez votre nom :",      // FR
    "Введите имя:",            // RU
    "Geben Sie Ihren Namen ein:" // DE
};

static const char* label_paused[] = {
    "PAUSED",      // EN
    "PAUSA",       // ES
    "PAUSE",       // FR
    "ПАУЗА",       // RU
    "PAUSE"        // DE
};

static const char* label_game_paused[] = {
    "The game is paused",        // EN
    "El juego está en pausa",    // ES
    "Le jeu est en pause",       // FR
    "Игра на паузе",             // RU
    "Das Spiel ist pausiert"     // DE
};

static const char* fmt_pause_stats[] = {
    "Wave %d | Score %d | Lives %d",        // EN
    "Oleada %d | Puntuación %d | Vidas %d", // ES
    "Vague %d | Score %d | Vies %d",        // FR
    "Волна %d | Счёт %d | Жизни %d",       // RU
    "Welle %d | Punktzahl %d | Leben %d"    // DE
};

static const char* hint_resume_p[] = {
    "Press P to Resume",        // EN
    "Pulsa P para reanudar",    // ES
    "Appuyez sur P pour reprendre", // FR
    "Нажмите P, чтобы продолжить",  // RU
    "Drücke P zum Fortfahren"   // DE
};

static const char* hint_esc_menu[] = {
    "ESC for Menu",          // EN
    "ESC para Menú",         // ES
    "ÉCHAP pour le menu",    // FR
    "ESC — меню",            // RU
    "ESC für Menü"           // DE
};

static const char* label_cheat_menu[] = {
    "CHEAT MENU",        // EN
    "MENÚ DE TRUCOS",    // ES
    "MENU DES CODES",    // FR
    "МЕНЮ ЧИТОВ",        // RU
    "CHEAT-MENÜ"         // DE
};

static const char* fmt_cheat_wave[] = {
    "Wave: %d/30 (LEFT/RIGHT to adjust)",          // EN
    "Oleada: %d/30 (IZQ/DER para ajustar)",        // ES
    "Vague : %d/30 (GAUCHE/DROITE pour régler)",   // FR
    "Волна: %d/30 (ВЛЕВО/ВПРАВО изменить)",        // RU
    "Welle: %d/30 (LINKS/RECHTS zum Anpassen)"     // DE
};

static const char* fmt_cheat_lives[] = {
    "Lives: %d/20 (LEFT/RIGHT to adjust)",          // EN
    "Vidas: %d/20 (IZQ/DER para ajustar)",          // ES
    "Vies : %d/20 (GAUCHE/DROITE pour régler)",     // FR
    "Жизни: %d/20 (ВЛЕВО/ВПРАВО изменить)",         // RU
    "Leben: %d/20 (LINKS/RECHTS zum Anpassen)"      // DE
};

static const char* fmt_cheat_missiles[] = {
    "Missiles: %d/99 (LEFT/RIGHT to adjust)",          // EN
    "Misiles: %d/99 (IZQ/DER para ajustar)",           // ES
    "Missiles : %d/99 (GAUCHE/DROITE pour régler)",    // FR
    "Ракеты: %d/99 (ВЛЕВО/ВПРАВО изменить)",           // RU
    "Raketen: %d/99 (LINKS/RECHTS zum Anpassen)"       // DE
};

static const char* fmt_cheat_bombs[] = {
    "Bombs: %d/99 (LEFT/RIGHT to adjust)",          // EN
    "Bombas: %d/99 (IZQ/DER para ajustar)",         // ES
    "Bombes : %d/99 (GAUCHE/DROITE pour régler)",   // FR
    "Бомбы: %d/99 (ВЛЕВО/ВПРАВО изменить)",         // RU
    "Bomben: %d/99 (LINKS/RECHTS zum Anpassen)"     // DE
};

static const char* fmt_cheat_difficulty[][3] = {
    {   // EN
        "Difficulty: Easy (LEFT/RIGHT to adjust)",
        "Difficulty: Medium (LEFT/RIGHT to adjust)",
        "Difficulty: Hard (LEFT/RIGHT to adjust)"
    },
    {   // ES
        "Dificultad: Fácil (IZQ/DER para ajustar)",
        "Dificultad: Media (IZQ/DER para ajustar)",
        "Dificultad: Difícil (IZQ/DER para ajustar)"
    },
    {   // FR
        "Difficulté : Facile (GAUCHE/DROITE pour régler)",
        "Difficulté : Moyenne (GAUCHE/DROITE pour régler)",
        "Difficulté : Difficile (GAUCHE/DROITE pour régler)"
    },
    {   // RU
        "Сложность: Лёгкая (ВЛЕВО/ВПРАВО изменить)",
        "Сложность: Средняя (ВЛЕВО/ВПРАВО изменить)",
        "Сложность: Тяжёлая (ВЛЕВО/ВПРАВО изменить)"
    },
    {   // DE
        "Schwierigkeitsgrad: Leicht (LINKS/RECHTS zum Anpassen)",
        "Schwierigkeitsgrad: Mittel (LINKS/RECHTS zum Anpassen)",
        "Schwierigkeitsgrad: Schwer (LINKS/RECHTS zum Anpassen)"
    }
};

static const char* label_apply[] = {
    "APPLY",        // EN
    "APLICAR",      // ES
    "APPLIQUER",    // FR
    "ПРИМЕНИТЬ",    // RU
    "ANWENDEN"      // DE
};

static const char* label_cancel[] = {
    "CANCEL",       // EN
    "CANCELAR",     // ES
    "ANNULER",      // FR
    "ОТМЕНА",       // RU
    "ABBRECHEN"     // DE
};

static const char* label_select_language[] = {
    "SELECT LANGUAGE",        // EN
    "SELECCIONA IDIOMA",      // ES
    "SÉLECTIONNEZ LA LANGUE", // FR
    "ВЫБЕРИТЕ ЯЗЫК",          // RU
    "SPRACHE WÄHLEN"          // DE
};

static const char* weapon_name_bullets[] = {
    "BULLETS",          // EN
    "BALAS",            // ES
    "BALLES",           // FR
    "ПУЛИ",             // RU
    "KUGELN"            // DE
};

static const char* weapon_name_missiles[] = {
    "MISSILES",         // EN
    "MISILES",          // ES
    "MISSILES",         // FR
    "РАКЕТЫ",           // RU
    "RAKETEN"           // DE
};

static const char* weapon_name_bombs[] = {
    "BOMBS",            // EN
    "BOMBAS",           // ES
    "BOMBES",           // FR
    "БОМБЫ",            // RU
    "BOMBEN"            // DE
};

static const char* weapon_name_spread_fire[] = {
    "SPREAD FIRE",      // EN
    "FUEGO DISPERSO",   // ES
    "TIR DISPERSÉ",     // FR
    "РАССЕЯННЫЙ ОГОНЬ", // RU
    "STREUFEUER"        // DE
};

static const char* shield2_label_text[] = {
    "SHIELD",           // EN
    "ESCUDO",           // ES
    "BOUCLIER",         // FR
    "ЩИТОК",            // RU
    "SCHILD"            // DE
};

static const char* missiles2_label_text[] = {
    "MISSILES",         // EN
    "MISILES",          // ES
    "MISSILES",         // FR
    "РАКЕТЫ",           // RU
    "RAKETEN"           // DE
};

static const char* energy_used_text[] = {
    "ENERGY USED",      // EN
    "ENERGÍA USADA",    // ES
    "ÉNERGIE UTILISÉE", // FR
    "ЭНЕРГИЯ ИСПОЛЬЗОВАНА", // RU
    "ENERGIE VERBRAUCHT"    // DE
};

static const char* shield_hit_text[] = {
    "SHIELD HIT",       // EN
    "ESCUDO IMPACTADO", // ES
    "BOUCLIER TOUCHÉ",  // FR
    "ЩИТОК ПОРАЖЕН",    // RU
    "SCHILD GETROFFEN"  // DE
};

static const char* mothership_down_text[] = {
    "MOTHERSHIP DOWN",      // EN
    "NAVE MADRE DERRIBADA", // ES
    "VAISSEAU-MÈRE ABATTU", // FR
    "МАТЕРИНСКИЙ КОРАБЛЬ СБИТ", // RU
    "MUTTERSCHIFF ABGESCHOSSEN" // DE
};

static const char* multiplier_bonus_text[] = {
    "MULTIPLIER BONUS",     // EN
    "BONIFICACIÓN MULTIPLICADOR", // ES
    "BONUS MULTIPLICATEUR", // FR
    "БОНУС МНОЖИТЕЛЯ",      // RU
    "MULTIPLIKATOR-BONUS"   // DE
};

static const char* perfect_victory_text[] = {
    "PERFECT VICTORY!",     // EN
    "¡VICTORIA PERFECTA!",  // ES
    "VICTOIRE PARFAITE !",  // FR
    "ИДЕАЛЬНАЯ ПОБЕДА!",    // RU
    "PERFEKTER SIEG!"       // DE
};

static const char* score_label_text[] = {
    "SCORE",            // EN
    "PUNTUACIÓN",       // ES
    "SCORE",            // FR
    "СЧЁТ",             // RU
    "PUNKTZAHL"         // DE
};

static const char* lives_label_text[] = {
    "LIVES",            // EN
    "VIDAS",            // ES
    "VIES",             // FR
    "ЖИЗНИ",            // RU
    "LEBEN"             // DE
};

static const char* shield_label_text[] = {
    "SHIELD",           // EN
    "ESCUDO",           // ES
    "BOUCLIER",         // FR
    "ЩИТОК",            // RU
    "SCHILD"            // DE
};

static const char* HUD_WAVE_LABEL[] = {
    "WAVE %d",          // EN
    "OLEADA %d",        // ES
    "VAGUE %d",         // FR
    "ВОЛНА %d",         // RU
    "WELLE %d"          // DE
};

static const char* next_wave_in_text[] = {
    "NEXT WAVE IN",     // EN
    "PRÓXIMA OLEADA EN", // ES
    "PROCHAINE VAGUE DANS", // FR
    "СЛЕДУЮЩАЯ ВОЛНА ЧЕРЕЗ", // RU
    "NÄCHSTE WELLE IN"  // DE
};

static const char* destroyed_label_text[] = {
    "DESTROYED",        // EN
    "DESTRUIDOS",       // ES
    "DÉTRUITS",         // FR
    "УНИЧТОЖЕНО",       // RU
    "ZERSTÖRT"          // DE
};

static const char* energy_label_text[] = {
    "ENERGY",           // EN
    "ENERGÍA",          // ES
    "ÉNERGIE",          // FR
    "ЭНЕРГИЯ",          // RU
    "ENERGIE"           // DE
};

static const char* missiles_label_text[] = {
    "MISSILES",         // EN
    "MISILES",          // ES
    "MISSILES",         // FR
    "РАКЕТЫ",           // RU
    "RAKETEN"           // DE
};

static const char* bombs_label_text[] = {
    "BOMBS",            // EN
    "BOMBAS",           // ES
    "BOMBES",           // FR
    "БОМБЫ",            // RU
    "BOMBEN"            // DE
};

static const char* armed_label_text[] = {
    "ARMED",            // EN
    "ARMADAS",          // ES
    "ARMÉES",           // FR
    "ВООРУЖЕННЫЕ",      // RU
    "BEWAFFNET"         // DE
};

static const char* final_score_label_text[] = {
    "FINAL SCORE",      // EN
    "PUNTUACIÓN FINAL", // ES
    "SCORE FINAL",      // FR
    "ИТОГОВЫЙ СЧЁТ",    // RU
    "ENDSCORE"          // DE
};

static const char* wave_reached_label_text[] = {
    "WAVE REACHED",     // EN
    "OLEADA ALCANZADA", // ES
    "VAGUE ATTEINTE",   // FR
    "ВОЛНА ДОСТИГНУТА", // RU
    "WELLE ERREICHT"    // DE
};

static const char* death_star_approaches_text[] = {
    "DEATH STAR APPROACHES",        // EN
    "ESTRELLA DE LA MUERTE SE APROXIMA", // ES
    "L'ÉTOILE DE LA MORT S'APPROCHE",    // FR
    "ЗВЕЗДА СМЕРТИ ПРИБЛИЖАЕТСЯ",       // RU
    "TODESSTERN NÄHERT SICH"            // DE
};

static const char* spawn_queen_rises_text[] = {
    "QUEEN RISES",          // EN
    "LA REINA SE LEVANTA",  // ES
    "LA REINE S'ÉLÈVE",     // FR
    "КОРОЛЕВА ВОССТАЕТ",    // RU
    "KÖNIGIN ERHEBT SICH"   // DE
};

static const char* void_nexus_emerges_text[] = {
    "VOID NEXUS EMERGES",           // EN
    "NEXO DEL VACÍO EMERGE",        // ES
    "LE NEXUS DU VIDE ÉMERGE",      // FR
    "ПУСТОТНЫЙ НЕКСУС ПОЯВЛЯЕТСЯ",  // RU
    "LEERER NEXUS ERSCHEINT"        // DE
};

static const char* harbinger_descends_text[] = {
    "HARBINGER DESCENDS",            // EN
    "EL HERALDO DESCIENDE",          // ES
    "LE PRÉCURSEUR DESCEND",         // FR
    "ВЕСТНИК НИСХОДИТ",              // RU
    "VORBOTE HERABSTEIGT"            // DE
};

static const char* star_vortex_awakens_text[] = {
    "STAR VORTEX AWAKENS",           // EN
    "EL VÓRTICE ESTELAR DESPIERTA",  // ES
    "LE VORTEX STELLAIRE S'ÉVEILLE", // FR
    "ЗВЁЗДНЫЙ ВИХРЬ ПРОБУЖДАЕТСЯ",   // RU
    "STERNWIRBEL ERWACHT"            // DE
};

static const char* star_vortex_destroyed_text[] = {
    "STAR VORTEX DESTROYED",         // EN
    "VÓRTICE ESTELAR DESTRUIDO",     // ES
    "VORTEX STELLAIRE DÉTRUIT",      // FR
    "ЗВЁЗДНЫЙ ВИХРЬ УНИЧТОЖЕН",      // RU
    "STERNWIRBEL ZERSTÖRT"           // DE
};

static const char* ultimate_threat_detected_text[] = {
    "ULTIMATE THREAT DETECTED",       // EN
    "AMENAZA DEFINITIVA DETECTADA",   // ES
    "MENACE ULTIME DÉTECTÉE",         // FR
    "СМЕРТЕЛЬНАЯ УГРОЗА ОБНАРУЖЕНА",  // RU
    "ULTIMATIVE BEDROHUNG ERKANNT"    // DE
};
