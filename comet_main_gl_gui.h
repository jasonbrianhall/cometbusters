#ifndef COMET_MAIN_GL_GUI_H
#define COMET_MAIN_GL_GUI_H

// ============================================================
// LOCAL HIGH SCORE ENTRY STATE (Not in header - local only)
// ============================================================

#include "cometbuster.h"
#include "visualization.h"
#include "comet_preferences.h"

typedef enum {
    HIGH_SCORE_ENTRY_NONE = 0,
    HIGH_SCORE_ENTRY_ACTIVE = 1,
    HIGH_SCORE_ENTRY_SAVED = 2
} HighScoreEntryState;

typedef struct {
    int state;                  // HighScoreEntryState
    char name_input[32];        // Player's typed name
    int cursor_pos;             // Current cursor position
    
    // Virtual keyboard state
    int kb_selected_index;      // Currently selected key for keyboard nav
    bool kb_show;               // Show virtual keyboard
} HighScoreEntryUI;

// ============================================================
// LOCAL CHEAT MENU STATE
// ============================================================

typedef enum {
    CHEAT_MENU_CLOSED = 0,
    CHEAT_MENU_OPEN = 1
} CheatMenuState;

typedef struct {
    int state;              // CHEAT_MENU_OPEN or CHEAT_MENU_CLOSED
    int selection;          // 0=Wave, 1=Lives, 2=Missiles, 3=Bombs, 4=Apply, 5=Cancel
    int wave;               // Selected wave (1-30)
    int lives;              // Selected lives (1-20)
    int missiles;           // Selected missiles
    int bombs;              // Selected bombs
    int cheat_difficulty;   // Cheat Difficulty
} CheatMenuUI;

typedef struct {
    SDL_Window *window;
    SDL_GLContext gl_context;
    Visualizer visualizer;
    AudioManager audio;
    
    int window_width;
    int window_height;
    bool fullscreen;
    bool running;
    bool game_paused;
    
    // Menu state
    bool show_menu;
    int menu_selection;  // 0=Continue, 1=New Game, 2=High Scores, 3=Audio, 4=Language, 5=Help, 6=Fullscreen, 7=Quit
    int menu_state;      // 0=Main Menu, 1=Difficulty Select, 2=High Scores Display, 3=Audio Menu, 4=Language Menu
    int gui_difficulty_level; // 1-3
    
    // Help overlay state
    bool show_help_overlay;      // Display help text overlay
    int help_scroll_offset;      // Track scrolling position in help text
    
    // Main Menu state
    int main_menu_scroll_offset; // Track scrolling position in main menu
    
    // Language Menu state
    int lang_menu_scroll_offset; // Track scrolling position in language menu
    
    // Music/Audio state tracking
    bool finale_music_started;  // Tracks if finale music has been played
    
    int frame_count;
    double total_time;
    double delta_time;
    uint32_t last_frame_ticks;
    
    // Joystick state
    SDL_Joystick *joystick;
    uint32_t last_joystick_axis_time;  // For throttling axis inputs
    int last_axis_0_state;              // Previous state of axis 0
    int last_axis_1_state;              // Previous state of axis 1
    int music_volume;
    int sfx_volume;
    CometPreferences preferences;  // Persistent user preferences (language, volumes)
} CometGUI;

typedef struct {
    int x, y, width, height;
    char character;
    const char *label;
    bool is_special;
} KeyboardButton;

void handle_events(CometGUI *gui, HighScoreEntryUI *hs_entry, CheatMenuUI *cheat_menu);
void handle_keyboard_input(SDL_Event *event, CometGUI *gui, HighScoreEntryUI *hs_entry);
void play_intro(CometGUI *gui, int language);
void handle_keyboard_input_special(SDL_Event *event, CometGUI *gui);
int get_keyboard_button_at_pos(int mouse_x, int mouse_y);
KeyboardButton* get_keyboard_buttons(int *out_count);
void add_character_to_input(HighScoreEntryUI *hs_entry, char c);
void render_virtual_keyboard(CometGUI *gui, HighScoreEntryUI *hs_entry, int selected_index);
void init_joystick(CometGUI *gui);

#endif
