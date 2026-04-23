#ifndef comet_lang
#define comet_lang

typedef enum { 
    WLANG_ENGLISH = 0, 
    WLANG_SPANISH = 1,
    WLANG_FRENCH = 2,
    WLANG_RUSSIAN = 3, 
    WLANG_GERMAN = 4,
    WLANG_CHINESE = 5,
    WLANG_COUNT 
} WLanguage;

static const char *wlanguagename[] = {
   "English",
   "Español (Spanish)",
   "Français (French)",
   "Русский (Russian)",
   "Deutsch (German)",
   "中文 (Chinese)"
};

#define WNUM_LANGUAGES (sizeof(wlanguagename) / sizeof(wlanguagename[0]))

static const char *wlanguage_intro_file[] = {
    "music/intro.mp3",    // English
    "music/intro_es.mp3", // Spanish
    "music/intro_fr.mp3", // French
    "music/intro_ru.mp3", // Russian
    "music/intro_de.mp3", // German
    "music/intro_zh.mp3", // Chinese
};  

static const char *wlanguage_finale_file[] = {
    "music/finale.mp3",    // English
    "music/finale_es.mp3", // Spanish
    "music/finale_fr.mp3", // French
    "music/finale_ru.mp3", // Russian
    "music/finale_de.mp3", // German
    "music/finale_zh.mp3", // Chinese
};  


static const char* bomb_text[] = { 
    "Bomb",   // English 
    "Bomba",  // Spanish
    "Bombe",  // French 
    "Бомба",  // Russian
    "Bombe",  // German
    "炸弹"    // Chinese 
};

static const char* life_text[] = {
    "+1 LIFE",   // English
    "+1 VIDA",   // Spanish
    "+1 VIE",    // French
    "+1 ЖИЗНЬ",  // Russian
    "+1 LEBEN",  // German
    "+1 生命"    // Chinese
};

static const char* multiplier_text[] = {
    "Multiplier",    // English
    "Multiplicador", // Spanish
    "Multiplicateur",// French
    "Множитель",     // Russian
    "Multiplikator", // German
    "倍增器"         // Chinese
};

static const char* boss_destroyed_text[] = {
    "BOSS DESTROYED!",   // English
    "¡JEFE DESTRUIDO!",  // Spanish
    "BOSS DÉTRUIT!",     // French
    "БОСС УНИЧТОЖЕН!",   // Russian
    "BOSS ZERSTÖRT!",    // German
    "首领已摧毁！"        // Chinese
};

static const char* wave_complete_text[] = {
    "WAVE 30 COMPLETE",      // English
    "OLEADA 30 COMPLETADA",  // Spanish
    "VAGUE 30 TERMINÉE",     // French
    "ВОЛНА 30 ЗАВЕРШЕНА",    // Russian
    "WELLE 30 ABGESCHLOSSEN",// German
    "第30波完成"              // Chinese
};

static const char* phase0_text[] = {
    "GRAVITATIONAL PULL",          // English
    "ATRACCIÓN GRAVITACIONAL",     // Spanish
    "ATTRACTION GRAVITATIONNELLE", // French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ",   // Russian
    "GRAVITATIONSZUG",             // German
    "引力拉扯"                      // Chinese
};

static const char* phase1_text[] = {
    "STELLAR COLLAPSE",
    "COLAPSO ESTELAR",
    "EFFONDREMENT STELLAIRE",
    "ЗВЁЗДНЫЙ КОЛЛАПС",
    "STELLARER ZUSAMMENBRUCH",
    "恒星坍缩"
};

static const char* phase2_text[] = {
    "VOID EXPANSION",
    "EXPANSIÓN DEL VACÍO",
    "EXPANSION DU VIDE",
    "РАСШИРЕНИЕ ПУСТОТЫ",
    "VAKUUMAUSDEHNUNG",
    "虚空扩张"
};

static const char* phase3_text[] = {
    "SINGULARITY COLLAPSE",
    "COLAPSO DE LA SINGULARIDAD",
    "EFFONDREMENT DE LA SINGULARITÉ",
    "КОЛЛАПС СИНГУЛЯРНОСТИ",
    "SINGULARITÄTSKOLLAPS",
    "奇点坍缩"
};

static const char* phase_unknown_text[] = {
    "UNKNOWN",
    "DESCONOCIDO",
    "INCONNU",
    "НЕИЗВЕСТНО",
    "UNBEKANNT",
    "未知"
};

static const char* phase_dormant_text[] = {
    "DORMANT",    // English
    "DURMIENTE",  // Spanish
    "SOMMEIL",    // French
    "СПЯЩИЙ",     // Russian
    "RUHEND",     // German
    "休眠"         // Chinese
};

static const char* phase_active_text[] = {
    "ACTIVE",
    "ACTIVO",
    "ACTIF",
    "АКТИВНЫЙ",
    "AKTIV",
    "活跃"
};

static const char* phase_frenzy_text[] = {
    "FRENZY",
    "FRENESÍ",
    "FRÉNÉSIE",
    "БЕШЕНСТВО",
    "RASEREI",
    "狂暴"
};

static const char* continue_texts[] = {
    "RIGHT-CLICK TO CONTINUE TO WAVE 31",         // English
    "CLIC DERECHO PARA CONTINUAR A LA OLEADA 31", // Spanish
    "CLIC DROIT POUR CONTINUER VERS LA VAGUE 31", // French
    "ПРАВЫЙ КЛИК, ЧТОБЫ ПРОДОЛЖИТЬ К ВОЛНЕ 31",   // Russian
    "RECHTSKLICK ZUM FORTFAHREN ZUR WELLE 31",    // German
    "右键继续至第31波"                               // Chinese
};

static const char* phase_stabilizing_text[] = {
    "STABILIZING",   // English
    "ESTABILIZANDO", // Spanish
    "STABILISATION", // French
    "СТАБИЛИЗАЦИЯ",  // Russian
    "STABILISIERUNG",// German
    "稳定中"          // Chinese
};

static const char* phase_fragmenting_text[] = {
    "FRAGMENTING",
    "FRAGMENTANDO",
    "FRAGMENTATION",
    "ФРАГМЕНТАЦИЯ",
    "FRAGMENTIERUNG",
    "碎裂中"
};

static const char* fragment_text[] = {
    "FRAGMENT!",    // English
    "¡FRAGMENTO!",  // Spanish
    "FRAGMENT !",   // French
    "ФРАГМЕНТ!",    // Russian
    "FRAGMENT!",    // German
    "碎片！"         // Chinese
};

static const char* gravitational_pull_text[] = {
    "GRAVITATIONAL PULL",          // English
    "ATRACCIÓN GRAVITATORIA",      // Spanish
    "ATTRACTION GRAVITATIONNELLE", // French
    "ГРАВИТАЦИОННОЕ ПРИТЯЖЕНИЕ",   // Russian
    "GRAVITATIONSZUG",             // German
    "引力拉扯"                      // Chinese
};

static const char* void_expansion_imminent_text[] = {
    "VOID EXPANSION IMMINENT",       // English
    "EXPANSIÓN DEL VACÍO INMINENTE", // Spanish
    "EXPANSION DU VIDE IMMINENTE",   // French
    "РАСШИРЕНИЕ ПУСТОТЫ НЕИЗБЕЖНО",  // Russian
    "VAKUUMAUSDEHNUNG UNMITTELBAR",  // German
    "虚空扩张即将来临"                 // Chinese
};

static const char* singularity_collapse_initiated_text[] = {
    "SINGULARITY COLLAPSE INITIATED",        // English
    "COLAPSO DE SINGULARIDAD INICIADO",      // Spanish
    "EFFONDREMENT DE LA SINGULARITÉ INITIÉ", // French
    "КОЛЛАПС СИНГУЛЯРНОСТИ ЗАПУЩЕН",         // Russian
    "SINGULARITÄTSKOLLAPS EINGELEITET",      // German
    "奇点坍缩已启动"                           // Chinese
};

static const char* singularity_collapsed_text[] = {
    "SINGULARITY COLLAPSED",   // English
    "SINGULARIDAD COLAPSADA",  // Spanish
    "SINGULARITÉ EFFONDRÉE",   // French
    "СИНГУЛЯРНОСТЬ РУХНУЛА",   // Russian
    "SINGULARITÄT KOLLABIERT", // German
    "奇点已坍缩"                // Chinese
};

static const char* dimensional_barrier_restored_text[] = {
    "DIMENSIONAL BARRIER RESTORED",           // English
    "BARRERA DIMENSIONAL RESTAURADA",         // Spanish
    "BARRIÈRE DIMENSIONNELLE RESTAURÉE",      // French
    "МЕРНЫЙ БАРЬЕР ВОССТАНОВЛЕН",             // Russian
    "DIMENSIONALE BARRIERE WIEDERHERGESTELLT",// German
    "次元屏障已恢复"                            // Chinese
};

static const char* cosmic_threat_eliminated_text[] = {
    "COSMIC THREAT ELIMINATED",    // English
    "AMENAZA CÓSMICA ELIMINADA",   // Spanish
    "MENACE COSMIQUE ÉLIMINÉE",    // French
    "КОСМИЧЕСКАЯ УГРОЗА УСТРАНЕНА",// Russian
    "KOSMISCHE BEDROHUNG BESEITIGT",// German
    "宇宙威胁已消除"                 // Chinese
};

static const char* gravitational_field_intensifies_text[] = {
    "GRAVITATIONAL FIELD INTENSIFIES",      // English
    "EL CAMPO GRAVITATORIO SE INTENSIFICA", // Spanish
    "LE CHAMP GRAVITATIONNEL S'INTENSIFIE", // French
    "ГРАВИТАЦИОННОЕ ПОЛЕ УСИЛИВАЕТСЯ",      // Russian
    "GRAVITATIONSFELD WIRD INTENSIVER",     // German
    "引力场正在增强"                          // Chinese
};

static const char* phase_dispersing_text[] = {
    "DISPERSING",  // English
    "DISPERSANDO", // Spanish
    "DISPERSION",  // French
    "РАССЕИВАНИЕ", // Russian 
    "ZERSTREUUNG", // German
    "消散中"        // Chinese
};

static const char* subtitle_texts[] = {
    "Press fire key to start",               // English
    "Pulsa la tecla de disparo para iniciar",// Spanish
    "Appuyez sur la touche tir pour démarrer",// French
    "Нажмите клавишу огня, чтобы начать",    // Russian
    "Drücke Feuer-Taste zum Starten",        // German
    "按开火键开始"                             // Chinese
};

static const char* phase_normal_text[] = {
    "NORMAL",    // English
    "NORMAL",    // Spanish
    "NORMAL",    // French
    "НОРМАЛЬНО", // Russian
    "NORMAL",    // German
    "正常"        // Chinese
};

static const char* phase_shielded_text[] = {
    "SHIELDED",
    "BLINDADO",
    "PROTÉGÉ",
    "ЗАЩИЩЁН",
    "ABGESCHIRMT",
    "已护盾"
};

static const char* provoked_text[] = {
    "PROVOKED!",      // English
    "¡PROVOCADO!",    // Spanish
    "PROVOQUÉ !",     // French
    "ПРОВОЦИРОВАН!",  // Russian
    "PROVOZIERT!",    // German
    "已激怒！"         // Chinese
};

static const char* energy_drained_text[] = {
    "ENERGY DRAINED!",  // English
    "¡ENERGÍA AGOTADA!",// Spanish
    "ÉNERGIE ÉPUISÉE !", // French
    "ЭНЕРГИЯ ИСЧЕРПАНА!",// Russian
    "ENERGIE ENTLADEN!", // German
    "能量耗尽！"          // Chinese
};

static const char* phase_enraged_text[] = {
    "ENRAGED!",
    "¡ENFURECIDO!",
    "ENRAGÉ!",
    "РАЗЪЯРЁН!",
    "WÜTEND!",
    "狂怒！"
};

static const char* queen_phase_recruiting_text[] = {
    "RECRUITING",  // English
    "RECLUTANDO",  // Spanish
    "RECRUTEMENT", // French
    "ВЕРБОВКА",    // Russian
    "REKRUTIERUNG",// German
    "招募中"        // Chinese
};

static const char* queen_phase_aggressive_text[] = {
    "AGGRESSIVE",
    "AGRESIVA",
    "AGRESSIVE",
    "АГРЕССИВНАЯ",
    "AGGRESSIV",
    "侵略性"
};

static const char* queen_phase_desperate_text[] = {
    "DESPERATE!",
    "¡DESESPERADA!",
    "DÉSESPÉRÉE!",
    "ОТЧАЯННАЯ!",
    "VERZWEIFELT!",
    "绝望！"
};

static const char* boss_bonus_text[] = {
    "BOSS BONUS",     // English
    "BONO DE JEFE",   // Spanish
    "BONUS DE BOSS",  // French
    "БОНУС ЗА БОССА", // Russian
    "BOSS-BONUS",     // German
    "首领奖励"         // Chinese
};

static const char* phase1_offensive_text[] = {
    "PHASE 1: OFFENSIVE!",   // English
    "FASE 1: OFENSIVA!",     // Spanish
    "PHASE 1 : OFFENSIVE !", // French
    "ФАЗА 1: НАСТУПЛЕНИЕ!",  // Russian
    "PHASE 1: ANGRIFF!",     // German
    "第1阶段：进攻！"          // Chinese
};

static const char* phase2_detonation_text[] = {
    "PHASE 2: DETONATION!",   // English
    "FASE 2: DETONACIÓN!",    // Spanish
    "PHASE 2 : DÉTONATION !", // French
    "ФАЗА 2: ДЕТОНАЦИЯ!",     // Russian
    "PHASE 2: DETONATION!",   // German
    "第2阶段：引爆！"           // Chinese
};

static const char* spawning_escorts_text[] = {
    "SPAWNING ESCORTS!",       // English
    "GENERANDO ESCOLTAS!",     // Spanish
    "APPARITION D'ESCORTES !", // French
    "ПРИЗЫВ ЭСКОРТОВ!",        // Russian
    "ESKORTE ERSCHEINEN!",     // German
    "护卫出现！"                 // Chinese
};

static const char* suppressing_threats_text[] = {
    "SUPPRESSING THREATS!",          // English
    "SUPRIMIENDO AMENAZAS!",         // Spanish
    "SUPPRESSION DES MENACES !",     // French
    "ПОДАВЛЕНИЕ УГРОЗ!",             // Russian
    "UNTERDRÜCKUNG VON BEDROHUNGEN!",// German
    "压制威胁！"                      // Chinese
};

static const char* core_destabilizing_text[] = {
    "CORE DESTABILIZING!",         // English
    "¡NÚCLEO DESESTABILIZÁNDOSE!", // Spanish
    "DÉSTABILISATION DU NŒUD !",   // French
    "ЯДРО ДЕСТАБИЛИЗИРУЕТСЯ!",     // Russian
    "KERN DESTABILISIERT!",        // German
    "核心不稳！"                     // Chinese
};

static const char* nexus_shattered_text[] = {
    "NEXUS SHATTERED",      // English
    "NEXO DESTRUIDO",       // Spanish
    "NEXUS BRISÉ",          // French
    "НЕКСУС РАЗРУШЕН",      // Russian
    "NEXUS ZERSCHMETTERT",  // German
    "枢纽已粉碎"             // Chinese
};

static const char* crystalline_collapse_text[] = {
    "CRYSTALLINE COLLAPSE",      // English
    "COLAPSO CRISTALINO",        // Spanish
    "EFFONDREMENT CRISTALLIN",   // French
    "КРИСТАЛЛИЧЕСКИЙ КОЛЛАПС",   // Russian
    "KRISTALLINER ZUSAMMENBRUCH",// German
    "晶体坍缩"                    // Chinese
};

static const char* perfect_destruction_text[] = {
    "PERFECT DESTRUCTION!",  // English
    "DESTRUCCIÓN PERFECTA!", // Spanish
    "DESTRUCTION PARFAITE !", // French
    "ИДЕАЛЬНОЕ УНИЧТОЖЕНИЕ!",// Russian
    "PERFEKTE ZERSTÖRUNG!",  // German
    "完美摧毁！"               // Chinese
};

static const char* main_menu_items[][10] = {
    {   // English
        "CONTINUE",
        "NEW GAME",
        "HIGH SCORES",
        "SAVE GAME",
        "LOAD GAME",
        "AUDIO",
        "LANGUAGE",
        "HELP",
        "FULLSCREEN",
        "QUIT"
    },
    {   // Spanish
        "CONTINUAR",
        "NUEVA PARTIDA",
        "PUNTUACIONES",
        "GUARDAR JUEGO",
        "CARGAR JUEGO",
        "AUDIO",
        "IDIOMA (Language)",
        "AYUDA",
        "PANTALLA COMPLETA",
        "SALIR"
    },
    {   // French
        "CONTINUER",
        "NOUVELLE PARTIE",
        "MEILLEURS SCORES",
        "SAUVEGARDER",
        "CHARGER",
        "AUDIO",
        "LANGUE (Language)",
        "AIDE",
        "PLEIN ÉCRAN",
        "QUITTER"
    },
    {   // Russian
        "ПРОДОЛЖИТЬ",
        "НОВАЯ ИГРА",
        "РЕКОРДЫ",
        "СОХРАНИТЬ ИГРУ",
        "ЗАГРУЗИТЬ ИГРУ",
        "АУДИО",
        "ЯЗЫК (Language)",
        "СПРАВКА",
        "ПОЛНЫЙ ЭКРАН",
        "ВЫХОД"
    },
    {   // German
        "FORTFAHREN",
        "NEUES SPIEL",
        "HIGHSCORES",
        "SPIEL SPEICHERN",
        "SPIEL LADEN",
        "AUDIO",
        "SPRACHE (Language)",
        "HILFE",
        "VOLLBILD",
        "BEENDEN"
    },
    {   // Chinese
        "继续",
        "新游戏",
        "高分榜",
        "保存游戏",
        "加载游戏",
        "音频",
        "语言 (Language)",
        "帮助",
        "全屏",
        "退出"
    }
};

static const char* hint_select_close[][1] = {
    { "Up/Down/Enter to select; ESC to close" },          // EN
    { "Arriba/Abajo/Enter para elegir; ESC para cerrar" },// ES
    { "Haut/Bas/Entrée pour choisir; Échap pour fermer" },// FR
    { "Вверх/Вниз/Enter — выбрать; ESC — закрыть" },      // RU
    { "Auf/Ab/Enter zum Auswählen; ESC zum Schließen" },  // DE
    { "上/下/回车 选择；ESC 关闭" }                         // ZH
};

static const char* label_high_scores[] = {
    "HIGH SCORES",      // EN
    "PUNTUACIONES",     // ES
    "MEILLEURS SCORES", // FR
    "РЕКОРДЫ",          // RU
    "HIGHSCORES",       // DE
    "高分榜"             // ZH
};

static const char* hint_continue_back[] = {
    "ENTER to continue | ESC to go back",         // EN
    "ENTER para continuar | ESC para volver",     // ES
    "ENTRÉE pour continuer | ÉCHAP pour revenir", // FR
    "ENTER — продолжить | ESC — назад",           // RU
    "ENTER zum Fortfahren | ESC zum Zurück",      // DE
    "回车 继续 | ESC 返回"                          // ZH
};

static const char* label_audio_settings[] = {
    "AUDIO SETTINGS",    // EN
    "AJUSTES DE AUDIO",  // ES
    "RÉGLAGES AUDIO",    // FR
    "НАСТРОЙКИ АУДИО",   // RU
    "AUDIOEINSTELLUNGEN",// DE
    "音频设置"            // ZH
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
    },
    {   // ZH
        "音乐音量",
        "音效音量"
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
    },
    {   // ZH
        "简单",
        "普通",
        "困难"
    }
};

static const char* hint_name_entry[] = {
    "Type your name | ENTER to save | BackSpace to delete",           // EN
    "Escribe tu nombre | ENTER para guardar | BackSpace para borrar",  // ES
    "Tapez votre nom | ENTRÉE pour valider | BackSpace pour effacer",  // FR
    "Введите имя | ENTER — сохранить | BackSpace — удалить",          // RU
    "Geben Sie Ihren Namen ein | ENTER zum Speichern | BackSpace zum Löschen", // DE
    "输入姓名 | 回车 保存 | 退格 删除"                                   // ZH
};

static const char* hint_name_entry2[] = {
    "Click buttons or use keyboard (A-Z, Space, Enter/Done to submit)", // EN
    "Haz clic en los botones o usa el teclado (A‑Z, Espacio, Enter/Done para enviar)", // ES
    "Cliquez sur les boutons ou utilisez le clavier (A‑Z, Espace, Entrée/Done pour valider)", // FR
    "Нажимайте кнопки или используйте клавиатуру (A‑Z, Пробел, Enter/Done для подтверждения)", // RU
    "Klicken Sie auf die Schaltflächen oder verwenden Sie die Tastatur (A‑Z, Leertaste, Enter/Done zum Bestätigen)", // DE
    "点击按钮或使用键盘（A-Z、空格、回车/完成 提交）"                       // ZH
};

static const char* key_space[] = {
    "SPACE",    // EN
    "Espacio",  // ES
    "Espace",   // FR
    "Пробел",   // RU
    "Leertaste",// DE
    "空格"       // ZH
};

static const char* hint_max_chars[] = {
    "Max 32 characters", // EN
    "Máx. 32 caracteres",// ES
    "Max 32 caractères", // FR
    "Макс. 32 символа",  // RU
    "Max 32 Zeichen",    // DE
    "最多32个字符"         // ZH
};

static const char* fmt_score_wave[] = {
    "Score: %d | Wave: %d",         // EN
    "Puntuación: %d | Oleada: %d",  // ES
    "Score : %d | Vague : %d",      // FR
    "Счёт: %d | Волна: %d",         // RU
    "Punktzahl: %d | Welle: %d",    // DE
    "得分：%d | 波次：%d"             // ZH
};

static const char* label_select_difficulty[] = {
    "SELECT DIFFICULTY",          // EN
    "SELECCIONA DIFICULTAD",      // ES
    "SÉLECTIONNEZ LA DIFFICULTÉ", // FR
    "ВЫБЕРИТЕ СЛОЖНОСТЬ",         // RU
    "SCHWIERIGKEITSGRAD WÄHLEN",  // DE
    "选择难度"                     // ZH
};

static const char* hint_adjust_menu[] = {
    "UP/DOWN to select | LEFT/RIGHT to adjust | ESC to go back",               // EN
    "ARRIBA/ABAJO para elegir | IZQ/DER para ajustar | ESC para volver",       // ES
    "HAUT/BAS pour choisir | GAUCHE/DROITE pour régler | ÉCHAP pour revenir",  // FR
    "ВВЕРХ/ВНИЗ — выбрать | ВЛЕВО/ВПРАВО — изменить | ESC — назад",           // RU
    "OBEN/UNTEN zum Auswählen | LINKS/RECHTS zum Anpassen | ESC zum Zurück",   // DE
    "上/下 选择 | 左/右 调整 | ESC 返回"                                        // ZH
};

static const char* label_new_high_score[] = {
    "NEW HIGH SCORE!", // EN
    "¡NUEVA PUNTUACIÓN!", // ES
    "NOUVEAU RECORD !", // FR
    "НОВЫЙ РЕКОРД!",   // RU
    "NEUER HIGHSCORE!",// DE
    "新高分！"           // ZH
};

static const char* label_enter_name[] = {
    "Enter Your Name:",           // EN
    "Ingresa tu nombre:",         // ES
    "Entrez votre nom :",         // FR
    "Введите имя:",               // RU
    "Geben Sie Ihren Namen ein:", // DE
    "输入您的姓名："               // ZH
};

static const char* label_paused[] = {
    "PAUSED", // EN
    "PAUSA",  // ES
    "PAUSE",  // FR
    "ПАУЗА",  // RU
    "PAUSE",  // DE
    "暂停"    // ZH
};

static const char* label_game_paused[] = {
    "The game is paused",     // EN
    "El juego está en pausa", // ES
    "Le jeu est en pause",    // FR
    "Игра на паузе",          // RU
    "Das Spiel ist pausiert", // DE
    "游戏已暂停"               // ZH
};

static const char* fmt_pause_stats[] = {
    "Wave %d | Score %d | Lives %d",       // EN
    "Oleada %d | Puntuación %d | Vidas %d",// ES
    "Vague %d | Score %d | Vies %d",       // FR
    "Волна %d | Счёт %d | Жизни %d",       // RU
    "Welle %d | Punktzahl %d | Leben %d",  // DE
    "第%d波 | 得分 %d | 生命 %d"            // ZH
};

static const char* hint_resume_p[] = {
    "Press P or the bottom button\non the joystick to Resume",              // EN
    "Pulsa P o el botón inferior\ndel joystick para reanudar",              // ES
    "Appuyez sur P ou sur le bouton\ninférieur du joystick pour reprendre", // FR
    "Нажмите P или нижнюю кнопку\nна джойстике, чтобы продолжить",         // RU
    "Drücke P oder die untere Taste\nam Joystick zum Fortfahren",           // DE
    "按P或手柄下方按钮\n继续游戏"                                             // ZH
};

static const char* hint_esc_menu[] = {
    "ESC for Menu",       // EN
    "ESC para Menú",      // ES
    "ÉCHAP pour le menu", // FR
    "ESC — меню",         // RU
    "ESC für Menü",       // DE
    "ESC 菜单"             // ZH
};

static const char* label_cheat_menu[] = {
    "CHEAT MENU",     // EN
    "MENÚ DE TRUCOS", // ES
    "MENU DES CODES", // FR
    "МЕНЮ ЧИТОВ",     // RU
    "CHEAT-MENÜ",     // DE
    "作弊菜单"         // ZH
};

static const char* fmt_cheat_wave[] = {
    "Wave: %d/30 (LEFT/RIGHT to adjust)",        // EN
    "Oleada: %d/30 (IZQ/DER para ajustar)",      // ES
    "Vague : %d/30 (GAUCHE/DROITE pour régler)", // FR
    "Волна: %d/30 (ВЛЕВО/ВПРАВО изменить)",      // RU
    "Welle: %d/30 (LINKS/RECHTS zum Anpassen)",  // DE
    "波次：%d/30（左/右调整）"                     // ZH
};

static const char* fmt_cheat_lives[] = {
    "Lives: %d/20 (LEFT/RIGHT to adjust)",        // EN
    "Vidas: %d/20 (IZQ/DER para ajustar)",        // ES
    "Vies : %d/20 (GAUCHE/DROITE pour régler)",   // FR
    "Жизни: %d/20 (ВЛЕВО/ВПРАВО изменить)",       // RU
    "Leben: %d/20 (LINKS/RECHTS zum Anpassen)",   // DE
    "生命：%d/20（左/右调整）"                      // ZH
};

static const char* fmt_cheat_missiles[] = {
    "Missiles: %d/99 (LEFT/RIGHT to adjust)",        // EN
    "Misiles: %d/99 (IZQ/DER para ajustar)",         // ES
    "Missiles : %d/99 (GAUCHE/DROITE pour régler)",  // FR
    "Ракеты: %d/99 (ВЛЕВО/ВПРАВО изменить)",         // RU
    "Raketen: %d/99 (LINKS/RECHTS zum Anpassen)",    // DE
    "导弹：%d/99（左/右调整）"                         // ZH
};

static const char* fmt_cheat_bombs[] = {
    "Bombs: %d/99 (LEFT/RIGHT to adjust)",        // EN
    "Bombas: %d/99 (IZQ/DER para ajustar)",       // ES
    "Bombes : %d/99 (GAUCHE/DROITE pour régler)", // FR
    "Бомбы: %d/99 (ВЛЕВО/ВПРАВО изменить)",       // RU
    "Bomben: %d/99 (LINKS/RECHTS zum Anpassen)",  // DE
    "炸弹：%d/99（左/右调整）"                      // ZH
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
    },
    {   // ZH
        "难度：简单（左/右调整）",
        "难度：普通（左/右调整）",
        "难度：困难（左/右调整）"
    }
};

static const char* label_apply[] = {
    "APPLY",     // EN
    "APLICAR",   // ES
    "APPLIQUER", // FR
    "ПРИМЕНИТЬ", // RU
    "ANWENDEN",  // DE
    "应用"        // ZH
};

static const char* label_cancel[] = {
    "CANCEL",   // EN
    "CANCELAR", // ES
    "ANNULER",  // FR
    "ОТМЕНА",   // RU
    "ABBRECHEN",// DE
    "取消"       // ZH
};

static const char* label_select_language[] = {
    "SELECT LANGUAGE",        // EN
    "SELECCIONA IDIOMA",      // ES
    "SÉLECTIONNEZ LA LANGUE", // FR
    "ВЫБЕРИТЕ ЯЗЫК",          // RU
    "SPRACHE WÄHLEN",         // DE
    "选择语言"                 // ZH
};

static const char* weapon_name_bullets[] = {
    "BULLETS", // EN
    "BALAS",   // ES
    "BALLES",  // FR
    "ПУЛИ",    // RU
    "KUGELN",  // DE
    "子弹"      // ZH
};

static const char* weapon_name_missiles[] = {
    "MISSILES", // EN
    "MISILES",  // ES
    "MISSILES", // FR
    "РАКЕТЫ",   // RU
    "RAKETEN",  // DE
    "导弹"       // ZH
};

static const char* weapon_name_bombs[] = {
    "BOMBS",  // EN
    "BOMBAS", // ES
    "BOMBES", // FR
    "БОМБЫ",  // RU
    "BOMBEN", // DE
    "炸弹"     // ZH
};

static const char* weapon_name_spread_fire[] = {
    "SPREAD FIRE",     // EN
    "FUEGO DISPERSO",  // ES
    "TIR DISPERSÉ",    // FR
    "РАССЕЯННЫЙ ОГОНЬ",// RU
    "STREUFEUER",      // DE
    "散射"              // ZH
};

static const char* shield2_label_text[] = {
    "SHIELD",   // EN
    "ESCUDO",   // ES
    "BOUCLIER", // FR
    "ЩИТОК",    // RU
    "SCHILD",   // DE
    "护盾"       // ZH
};

static const char* missiles2_label_text[] = {
    "MISSILES", // EN
    "MISILES",  // ES
    "MISSILES", // FR
    "РАКЕТЫ",   // RU
    "RAKETEN",  // DE
    "导弹"       // ZH
};

static const char* energy_used_text[] = {
    "ENERGY USED",       // EN
    "ENERGÍA USADA",     // ES
    "ÉNERGIE UTILISÉE",  // FR
    "ЭНЕРГИЯ ИСПОЛЬЗОВАНА",// RU
    "ENERGIE VERBRAUCHT",// DE
    "能量已使用"           // ZH
};

static const char* shield_hit_text[] = {
    "SHIELD HIT",       // EN
    "ESCUDO IMPACTADO", // ES
    "BOUCLIER TOUCHÉ",  // FR
    "ЩИТОК ПОРАЖЕН",    // RU
    "SCHILD GETROFFEN", // DE
    "护盾受击"           // ZH
};

static const char* mothership_down_text[] = {
    "MOTHERSHIP DOWN",          // EN
    "NAVE MADRE DERRIBADA",     // ES
    "VAISSEAU-MÈRE ABATTU",     // FR
    "МАТЕРИНСКИЙ КОРАБЛЬ СБИТ", // RU
    "MUTTERSCHIFF ABGESCHOSSEN",// DE
    "母舰击落"                   // ZH
};

static const char* multiplier_bonus_text[] = {
    "MULTIPLIER BONUS",          // EN
    "BONIFICACIÓN MULTIPLICADOR",// ES
    "BONUS MULTIPLICATEUR",      // FR
    "БОНУС МНОЖИТЕЛЯ",           // RU
    "MULTIPLIKATOR-BONUS",       // DE
    "倍增器奖励"                  // ZH
};

static const char* perfect_victory_text[] = {
    "PERFECT VICTORY!",   // EN
    "¡VICTORIA PERFECTA!",// ES
    "VICTOIRE PARFAITE !", // FR
    "ИДЕАЛЬНАЯ ПОБЕДА!",  // RU
    "PERFEKTER SIEG!",    // DE
    "完美胜利！"            // ZH
};

static const char* score_label_text[] = {
    "SCORE",     // EN
    "PUNTUACIÓN",// ES
    "SCORE",     // FR
    "СЧЁТ",      // RU
    "PUNKTZAHL", // DE
    "得分"        // ZH
};

static const char* lives_label_text[] = {
    "LIVES", // EN
    "VIDAS", // ES
    "VIES",  // FR
    "ЖИЗНИ", // RU
    "LEBEN", // DE
    "生命"    // ZH
};

static const char* shield_label_text[] = {
    "SHIELD",   // EN
    "ESCUDO",   // ES
    "BOUCLIER", // FR
    "ЩИТОК",    // RU
    "SCHILD",   // DE
    "护盾"       // ZH
};

static const char* HUD_WAVE_LABEL[] = {
    "WAVE %d",   // EN
    "OLEADA %d", // ES
    "VAGUE %d",  // FR
    "ВОЛНА %d",  // RU
    "WELLE %d",  // DE
    "第%d波"      // ZH
};

static const char* next_wave_in_text[] = {
    "NEXT WAVE IN",          // EN
    "PRÓXIMA OLEADA EN",     // ES
    "PROCHAINE VAGUE DANS",  // FR
    "СЛЕДУЮЩАЯ ВОЛНА ЧЕРЕЗ", // RU
    "NÄCHSTE WELLE IN",      // DE
    "下一波倒计时"             // ZH
};

static const char* destroyed_label_text[] = {
    "DESTROYED", // EN
    "DESTRUIDOS",// ES
    "DÉTRUITS",  // FR
    "УНИЧТОЖЕНО",// RU
    "ZERSTÖRT",  // DE
    "已摧毁"      // ZH
};

static const char* energy_label_text[] = {
    "ENERGY",  // EN
    "ENERGÍA", // ES
    "ÉNERGIE", // FR
    "ЭНЕРГИЯ", // RU
    "ENERGIE", // DE
    "能量"      // ZH
};

static const char* missiles_label_text[] = {
    "MISSILES", // EN
    "MISILES",  // ES
    "MISSILES", // FR
    "РАКЕТЫ",   // RU
    "RAKETEN",  // DE
    "导弹"       // ZH
};

static const char* bombs_label_text[] = {
    "BOMBS",  // EN
    "BOMBAS", // ES
    "BOMBES", // FR
    "БОМБЫ",  // RU
    "BOMBEN", // DE
    "炸弹"     // ZH
};

static const char* armed_label_text[] = {
    "ARMED",       // EN
    "ARMADAS",     // ES
    "ARMÉES",      // FR
    "ВООРУЖЕННЫЕ", // RU
    "BEWAFFNET",   // DE
    "已装备"        // ZH
};

static const char* final_score_label_text[] = {
    "FINAL SCORE",     // EN
    "PUNTUACIÓN FINAL",// ES
    "SCORE FINAL",     // FR
    "ИТОГОВЫЙ СЧЁТ",   // RU
    "ENDSCORE",        // DE
    "最终得分"          // ZH
};

static const char* wave_reached_label_text[] = {
    "WAVE REACHED",    // EN
    "OLEADA ALCANZADA",// ES
    "VAGUE ATTEINTE",  // FR
    "ВОЛНА ДОСТИГНУТА",// RU
    "WELLE ERREICHT",  // DE
    "到达波次"          // ZH
};

static const char* death_star_approaches_text[] = {
    "DEATH STAR APPROACHES",             // EN
    "ESTRELLA DE LA MUERTE SE APROXIMA", // ES
    "L'ÉTOILE DE LA MORT S'APPROCHE",    // FR
    "ЗВЕЗДА СМЕРТИ ПРИБЛИЖАЕТСЯ",        // RU
    "TODESSTERN NÄHERT SICH",            // DE
    "死星正在接近"                         // ZH
};

static const char* spawn_queen_rises_text[] = {
    "QUEEN RISES",         // EN
    "LA REINA SE LEVANTA", // ES
    "LA REINE S'ÉLÈVE",    // FR
    "КОРОЛЕВА ВОССТАЕТ",   // RU
    "KÖNIGIN ERHEBT SICH", // DE
    "女王崛起"              // ZH
};

static const char* void_nexus_emerges_text[] = {
    "VOID NEXUS EMERGES",          // EN
    "NEXO DEL VACÍO EMERGE",       // ES
    "LE NEXUS DU VIDE ÉMERGE",     // FR
    "ПУСТОТНЫЙ НЕКСУС ПОЯВЛЯЕТСЯ", // RU
    "LEERER NEXUS ERSCHEINT",      // DE
    "虚空枢纽出现"                  // ZH
};

static const char* harbinger_descends_text[] = {
    "HARBINGER DESCENDS",   // EN
    "EL HERALDO DESCIENDE", // ES
    "LE PRÉCURSEUR DESCEND",// FR
    "ВЕСТНИК НИСХОДИТ",     // RU
    "VORBOTE HERABSTEIGT",  // DE
    "预兆降临"               // ZH
};

static const char* star_vortex_awakens_text[] = {
    "STAR VORTEX AWAKENS",           // EN
    "EL VÓRTICE ESTELAR DESPIERTA",  // ES
    "LE VORTEX STELLAIRE S'ÉVEILLE", // FR
    "ЗВЁЗДНЫЙ ВИХРЬ ПРОБУЖДАЕТСЯ",   // RU
    "STERNWIRBEL ERWACHT",           // DE
    "星涡觉醒"                        // ZH
};

static const char* star_vortex_destroyed_text[] = {
    "STAR VORTEX DESTROYED",    // EN
    "VÓRTICE ESTELAR DESTRUIDO",// ES
    "VORTEX STELLAIRE DÉTRUIT", // FR
    "ЗВЁЗДНЫЙ ВИХРЬ УНИЧТОЖЕН", // RU
    "STERNWIRBEL ZERSTÖRT",     // DE
    "星涡已摧毁"                  // ZH
};

static const char* ultimate_threat_detected_text[] = {
    "ULTIMATE THREAT DETECTED",     // EN
    "AMENAZA DEFINITIVA DETECTADA", // ES
    "MENACE ULTIME DÉTECTÉE",       // FR
    "СМЕРТЕЛЬНАЯ УГРОЗА ОБНАРУЖЕНА",// RU
    "ULTIMATIVE BEDROHUNG ERKANNT", // DE
    "终极威胁已发现"                  // ZH
};

static const char* help_menu_text[][1] = {
    {   // ENGLISH
        R"(CONTROLS

W or Up Arrow        - Forward thrust
A or Left Arrow      - Turn left
D or Right Arrow     - Turn right
S or Down Arrow      - Backward thrust
Q                    - Toggle weapons (missiles auto-select when gained)
SPACE or X           - Boost
CTRL                 - Fire forward
Z                    - Omnidirectional fire
P                    - Pause/Resume
F5                   - Quick save (slot 10)
F7                   - Quick load (slot 10)
F11                  - Toggle fullscreen
C in main menu       - Hidden Cheat Menu)"
    },
    {   // SPANISH
        R"(CONTROLES

W o Flecha Arriba    - Impulso hacia adelante
A o Flecha Izquierda - Girar a la izquierda
D o Flecha Derecha   - Girar a la derecha
S o Flecha Abajo     - Impulso hacia atrás
Q                    - Cambiar armas (misiles se seleccionan automáticamente)
ESPACIO o X          - Aceleración
CTRL                 - Disparo frontal
Z                    - Disparo omnidireccional
P                    - Pausar/Reanudar
F5                   - Guardado rápido (ranura 10)
F7                   - Carga rápida (ranura 10)
F11                  - Pantalla completa
C en el menú principal - Menú de trucos oculto)"
    },
    {   // FRENCH
        R"(COMMANDES

W ou Flèche Haut     - Poussée avant
A ou Flèche Gauche   - Tourner à gauche
D ou Flèche Droite   - Tourner à droite
S ou Flèche Bas      - Poussée arrière
Q                    - Changer d'arme (missiles auto-sélectionnés)
ESPACE ou X          - Boost
CTRL                 - Tir vers l'avant
Z                    - Tir omnidirectionnel
P                    - Pause/Reprise
F5                   - Sauvegarde rapide (emplacement 10)
F7                   - Chargement rapide (emplacement 10)
F11                  - Plein écran
C dans le menu principal - Menu de triche caché)"
    },
    {   // RUSSIAN
        R"(УПРАВЛЕНИЕ

W или Стрелка Вверх  - Тяга вперёд
A или Стрелка Влево  - Поворот влево
D или Стрелка Вправо - Поворот вправо
S или Стрелка Вниз   - Тяга назад
Q                    - Переключить оружие (ракеты выбираются автоматически)
ПРОБЕЛ или X         - Ускорение
CTRL                 - Огонь вперёд
Z                    - Огонь во все стороны
P                    - Пауза/Продолжить
F5                   - Быстрое сохранение (слот 10)
F7                   - Быстрая загрузка (слот 10)
F11                  - Полный экран
C в главном меню     - Скрытое меню читов)"
    },
    {   // GERMAN
        R"(STEUERUNG

W oder Pfeil Hoch    - Schub nach vorne
A oder Pfeil Links   - Nach links drehen
D oder Pfeil Rechts  - Nach rechts drehen
S oder Pfeil Unten   - Rückwärtsschub
Q                    - Waffen wechseln (Raketen werden automatisch gewählt)
LEERTASTE oder X     - Boost
STRG                 - Nach vorne feuern
Z                    - Rundumfeuer
P                    - Pause/Fortsetzen
F5                   - Schnellspeicherung (Slot 10)
F7                   - Schnellladevorgang (Slot 10)
F11                  - Vollbild umschalten
C im Hauptmenü       - Verstecktes Cheat-Menü)"
    },
    {   // CHINESE
        R"(控制

W 或 向上箭头        - 向前推进
A 或 向左箭头        - 向左转
D 或 向右箭头        - 向右转
S 或 向下箭头        - 向后推进
Q                    - 切换武器（获得导弹后自动选择）
空格 或 X            - 加速
CTRL                 - 向前射击
Z                    - 全向射击
P                    - 暂停/继续
F5                   - 快速保存（槽位10）
F7                   - 快速加载（槽位10）
F11                  - 切换全屏
主菜单中按 C         - 隐藏作弊菜单)"
    }
};

#endif
