#include "cometbuster.h"
#include <cstdio>
#include <cstring>

/**
 * Load high scores from disk (text format)
 */
void high_scores_load(CometBusterGame *game) {
    if (!game) return;
    
    game->high_score_count = 0;
    for (int i = 0; i < MAX_HIGH_SCORES; i++) {
        game->high_scores[i].score = 0;
        game->high_scores[i].wave = 0;
        game->high_scores[i].timestamp = 0;
        game->high_scores[i].player_name[0] = '\0';
    }
    
    const char *path = high_scores_get_path();
    fprintf(stdout, "[HIGH SCORES] Loading from: %s\n", path);
    
    FILE *fp = fopen(path, "r");
    
    if (!fp) {
        fprintf(stdout, "[HIGH SCORES] No existing high scores file\n");
        return;
    }
    
    fprintf(stdout, "[HIGH SCORES] File opened successfully\n");
    
    // DEBUG: Read entire file first to see what's in it
    fprintf(stdout, "[HIGH SCORES DEBUG] File contents:\n");
    rewind(fp);
    char debug_line[256];
    int line_num = 0;
    while (fgets(debug_line, sizeof(debug_line), fp) != NULL) {
        line_num++;
        fprintf(stdout, "  Line %d: %s", line_num, debug_line);
    }
    
    // Now parse the file properly
    rewind(fp);
    printf("[HIGH SCORES DEBUG] Now parsing file:\n");
    
    char line[256];
    while (game->high_score_count < MAX_HIGH_SCORES && fgets(line, sizeof(line), fp) != NULL) {
        int score, wave;
        time_t timestamp;
        char name[32];
        
        // Parse: score wave timestamp name (name can have spaces)
        // Format: 38565 5 1765397762 John Doe
        int items_read = sscanf(line, "%d %d %ld", &score, &wave, &timestamp);
        
        fprintf(stdout, "[HIGH SCORES DEBUG] Line %d: Read %d items - ", 
                game->high_score_count + 1, items_read);
        
        if (items_read != 3) {
            fprintf(stdout, "PARSE FAILED (expected at least 3 items)\n");
            continue;
        }
        
        // Extract name - everything after the third field
        char *name_start = line;
        int field_count = 0;
        int pos = 0;
        
        // Skip past score, wave, and timestamp fields
        while (field_count < 3 && pos < (int)strlen(line)) {
            if (line[pos] == ' ') {
                field_count++;
                pos++;
                // Skip multiple spaces
                while (line[pos] == ' ') pos++;
            } else {
                pos++;
            }
        }
        
        name_start = &line[pos];
        
        // Remove trailing newline from name
        int name_len = strlen(name_start);
        if (name_len > 0 && name_start[name_len - 1] == '\n') {
            name_start[name_len - 1] = '\0';
            name_len--;
        }
        
        // Copy name
        strncpy(name, name_start, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        
        fprintf(stdout, "score=%d wave=%d ts=%ld name=%s\n", 
                score, wave, timestamp, name);
        
        HighScore *hs = &game->high_scores[game->high_score_count];
        hs->score = score;
        hs->wave = wave;
        hs->timestamp = timestamp;
        strncpy(hs->player_name, name, sizeof(hs->player_name) - 1);
        hs->player_name[sizeof(hs->player_name) - 1] = '\0';
        
        fprintf(stdout, "[HIGH SCORES DEBUG] Stored score: %s = %d (W%d)\n", 
                hs->player_name, hs->score, hs->wave);
        
        game->high_score_count++;
    }
    
    fclose(fp);
    fprintf(stdout, "[HIGH SCORES] Loaded %d high scores\n", game->high_score_count);
}

/**
 * Save high scores to disk (text format)
 */
void high_scores_save(CometBusterGame *game) {
    if (!game) return;

    const char *path = high_scores_get_path();
    printf("[HIGH SCORES] Saving to: %s\n", path);

    FILE *fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "[HIGH SCORES] Failed to open file for writing\n");
        return;
    }

    // Save only the valid high scores (up to high_score_count)
    for (int i = 0; i < game->high_score_count; i++) {
        HighScore *hs = &game->high_scores[i];
        fprintf(fp, "%d %d %ld %s\n",
                hs->score,
                hs->wave,
                (long)hs->timestamp,
                hs->player_name);
        fprintf(stdout, "[HIGH SCORES DEBUG] Wrote: %s = %d (W%d, ts=%ld)\n",
                hs->player_name,
                hs->score,
                hs->wave,
                (long)hs->timestamp);
    }
    fflush(fp);   // force write to disk
    fclose(fp);
    printf("[HIGH SCORES] Saved %d high scores\n", game->high_score_count);
}

/**
 * Check if a score qualifies as a high score
 */
bool comet_buster_is_high_score(CometBusterGame *game, int score) {
    if (!game) return false;
    
    printf("IS HIGH SCORE CALLED\n");
    
    // If we haven't filled the high score list yet, any score is a high score
    if (game->high_score_count < MAX_HIGH_SCORES) {
        printf("List not full yet (%d/%d), score %d qualifies\n", 
               game->high_score_count, MAX_HIGH_SCORES, score);
        return true;
    }
    
    // List is full - check if score beats the lowest (last) score
    if (score > game->high_scores[MAX_HIGH_SCORES - 1].score) {
        printf("Is a High Score: %d (beats lowest of %d)\n", 
               score, game->high_scores[MAX_HIGH_SCORES - 1].score);
        return true;
    }
    
    printf("Not a high score: %d\n", score);
    return false;
}

/**
 * Add a high score (maintains sorted order)
 */
void high_scores_add(CometBusterGame *game, int score, int wave, const char *name) {
    if (!game || !name) return;

    printf("Adding high score: %d (wave %d, player %s)\n", score, wave, name);
    
    // If list is not full, append to the end
    if (game->high_score_count < MAX_HIGH_SCORES) {
        game->high_scores[game->high_score_count].score = score;
        game->high_scores[game->high_score_count].wave = wave;
        game->high_scores[game->high_score_count].timestamp = time(NULL);
        strcpy(game->high_scores[game->high_score_count].player_name, name);
        game->high_score_count++;
    } else {
        // List is full - insert at the last position
        game->high_scores[MAX_HIGH_SCORES - 1].score = score;
        game->high_scores[MAX_HIGH_SCORES - 1].wave = wave;
        game->high_scores[MAX_HIGH_SCORES - 1].timestamp = time(NULL);
        strcpy(game->high_scores[MAX_HIGH_SCORES - 1].player_name, name);
    }

    // Bubble-up sort to keep scores in descending order (highest first)
    for (int i = game->high_score_count - 1; i > 0; i--) {
        if (game->high_scores[i].score > game->high_scores[i-1].score) {
            HighScore temp = game->high_scores[i];
            game->high_scores[i] = game->high_scores[i-1];
            game->high_scores[i-1] = temp;
        } else {
            break;
        }
    }
    
    printf("High score count after add: %d\n", game->high_score_count);
    for(int i = 0; i < game->high_score_count; i++) {
        printf("  [%d] %s = %d (W%d)\n", i, 
               game->high_scores[i].player_name,
               game->high_scores[i].score,
               game->high_scores[i].wave);
    }
}

// ============================================================
// HIGH SCORE PERSISTENCE FUNCTIONS
// ============================================================

/**
 * Get the high scores file path
 */
const char* high_scores_get_path(void) {
    static char scores_path[512] = {0};
    static bool initialized = false;
    
    if (initialized) return scores_path;
    
#ifdef _WIN32
    char appdata_path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata_path))) {
        snprintf(scores_path, sizeof(scores_path), "%s\\CometBuster\\highscores.txt", appdata_path);
    } else {
        const char *home = getenv("USERPROFILE");
        if (home) {
            snprintf(scores_path, sizeof(scores_path), "%s\\CometBuster\\highscores.txt", home);
        } else {
            strcpy(scores_path, ".\\CometBuster\\highscores.txt");
        }
    }
#else
    const char *home = getenv("HOME");
    if (home) {
        snprintf(scores_path, sizeof(scores_path), "%s/.cometbuster/highscores.txt", home);
    } else {
        strcpy(scores_path, "./.cometbuster/highscores.txt");
    }
#endif
    
    initialized = true;
    return scores_path;
}
