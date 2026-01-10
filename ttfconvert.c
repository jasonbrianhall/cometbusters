/*
 * ttf_to_header_freetype.c - Convert TTF to C header using FreeType
 * 
 * This version properly renders all characters using FreeType.
 * 
 * Compile:
 *   gcc -o ttf_to_header ttf_to_header_freetype.c `freetype-config --cflags --libs` -lm
 * 
 * Usage:
 *   ./ttf_to_header <font.ttf> <font_size> <output.h>
 * 
 * Example:
 *   ./ttf_to_header Monospace.ttf 16 font.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define NUM_CHARS (127 - 32)  // Printable ASCII
#define START_CHAR 32         // Space
#define END_CHAR 127          // ~

typedef struct {
    unsigned char *bitmap;
    int width;
    int height;
    int advance;
} Glyph;

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <font.ttf> <font_size> <output.h>\n", argv[0]);
        fprintf(stderr, "Example: %s Monospace.ttf 16 font.h\n", argv[0]);
        return 1;
    }
    
    const char *font_file = argv[1];
    int font_size = atoi(argv[2]);
    const char *output_file = argv[3];
    
    printf("Font converter using FreeType\n");
    printf("==============================\n");
    printf("Font file: %s\n", font_file);
    printf("Font size: %d\n", font_size);
    printf("Output: %s\n\n", output_file);
    
    // Initialize FreeType
    FT_Library library;
    FT_Face face;
    
    if (FT_Init_FreeType(&library)) {
        fprintf(stderr, "Error: Could not initialize FreeType\n");
        return 1;
    }
    
    // Load font
    if (FT_New_Face(library, font_file, 0, &face)) {
        fprintf(stderr, "Error: Could not load font file: %s\n", font_file);
        FT_Done_FreeType(library);
        return 1;
    }
    
    // Set font size
    if (FT_Set_Pixel_Sizes(face, 0, font_size)) {
        fprintf(stderr, "Error: Could not set font size\n");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }
    
    printf("Face: %s\n", face->family_name);
    printf("Rendering %d characters...\n\n", NUM_CHARS);
    
    // Render all glyphs
    Glyph glyphs[256];
    memset(glyphs, 0, sizeof(glyphs));
    
    int max_width = 0;
    int max_height = 0;
    
    for (int ch = START_CHAR; ch < END_CHAR; ch++) {
        FT_UInt glyph_index = FT_Get_Char_Index(face, ch);
        
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
            fprintf(stderr, "Warning: Could not load glyph for character %d\n", ch);
            continue;
        }
        
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf(stderr, "Warning: Could not render glyph for character %d\n", ch);
            continue;
        }
        
        FT_Bitmap *bitmap = &face->glyph->bitmap;
        int width = bitmap->width;
        int height = bitmap->rows;
        
        if (width > max_width) max_width = width;
        if (height > max_height) max_height = height;
        
        // Allocate and copy bitmap data
        if (width > 0 && height > 0) {
            int size = width * height;
            glyphs[ch].bitmap = malloc(size);
            memcpy(glyphs[ch].bitmap, bitmap->buffer, size);
        }
        
        glyphs[ch].width = width;
        glyphs[ch].height = height;
        glyphs[ch].advance = face->glyph->advance.x >> 6;
        
        if ((ch - START_CHAR) % 10 == 0) {
            printf("  %d/%d ('%c')\n", ch - START_CHAR + 1, NUM_CHARS, ch);
        }
    }
    
    printf("\nMax glyph size: %dx%d\n", max_width, max_height);
    
    // Write output file
    FILE *out = fopen(output_file, "w");
    if (!out) {
        fprintf(stderr, "Error: Could not open output file: %s\n", output_file);
        return 1;
    }
    
    printf("Writing header file...\n");
    
    // Write header
    fprintf(out, "#ifndef FONT_TTF_H\n");
    fprintf(out, "#define FONT_TTF_H\n\n");
    fprintf(out, "// Generated from: %s\n", font_file);
    fprintf(out, "// Font size: %dpx\n", font_size);
    fprintf(out, "// Characters: %d (ASCII %d-%d)\n\n", NUM_CHARS, START_CHAR, END_CHAR - 1);
    
    // Glyph structure
    fprintf(out, "typedef struct {\n");
    fprintf(out, "    int width;\n");
    fprintf(out, "    int height;\n");
    fprintf(out, "    int advance;\n");
    fprintf(out, "    const unsigned char *bitmap;\n");
    fprintf(out, "} GlyphData;\n\n");
    
    // Write glyph data
    fprintf(out, "// Glyph bitmaps\n\n");
    
    for (int ch = START_CHAR; ch < END_CHAR; ch++) {
        if (!glyphs[ch].bitmap) continue;
        
        char ch_display = (ch >= 32 && ch < 127) ? ch : '?';
        
        fprintf(out, "// Character '%c' (0x%02X)\n", ch_display, ch);
        fprintf(out, "static const unsigned char glyph_%d_bitmap[] = {\n", ch);
        
        int size = glyphs[ch].width * glyphs[ch].height;
        for (int i = 0; i < size; i++) {
            if (i % 16 == 0) fprintf(out, "    ");
            fprintf(out, "0x%02X", glyphs[ch].bitmap[i]);
            if (i < size - 1) fprintf(out, ",");
            if ((i + 1) % 16 == 0) fprintf(out, "\n");
        }
        fprintf(out, "\n};\n\n");
    }
    
    // Write glyph structures
    fprintf(out, "// Glyph definitions\n\n");
    
    for (int ch = START_CHAR; ch < END_CHAR; ch++) {
        if (!glyphs[ch].bitmap) continue;
        
        fprintf(out, "static const GlyphData glyph_%d = {\n", ch);
        fprintf(out, "    %d, %d, %d,\n", glyphs[ch].width, glyphs[ch].height, glyphs[ch].advance);
        fprintf(out, "    glyph_%d_bitmap\n", ch);
        fprintf(out, "};\n\n");
    }
    
    // Write lookup function
    fprintf(out, "// Get glyph by character\n");
    fprintf(out, "static inline const GlyphData *get_glyph(unsigned char ch) {\n");
    fprintf(out, "    static const GlyphData *glyphs[] = {\n");
    
    for (int i = 0; i < 256; i++) {
        if (i >= START_CHAR && i < END_CHAR && glyphs[i].bitmap) {
            fprintf(out, "        [%d] = &glyph_%d,\n", i, i);
        } else {
            fprintf(out, "        [%d] = NULL,\n", i);
        }
    }
    
    fprintf(out, "    };\n");
    fprintf(out, "    return (ch < 256) ? glyphs[ch] : NULL;\n");
    fprintf(out, "}\n\n");
    
    // Write constants
    fprintf(out, "#define FONT_MAX_WIDTH %d\n", max_width);
    fprintf(out, "#define FONT_MAX_HEIGHT %d\n", max_height);
    fprintf(out, "#define FONT_START_CHAR %d\n", START_CHAR);
    fprintf(out, "#define FONT_END_CHAR %d\n\n", END_CHAR);
    
    fprintf(out, "#endif // FONT_TTF_H\n");
    
    fclose(out);
    
    // Cleanup
    for (int ch = 0; ch < 256; ch++) {
        if (glyphs[ch].bitmap) free(glyphs[ch].bitmap);
    }
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    
    printf("\n✓ Success! Created %s\n", output_file);
    printf("  Max size: %dx%d\n", max_width, max_height);
    
    return 0;
}
