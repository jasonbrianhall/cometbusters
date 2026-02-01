/*
 * ttfconvert_unicode.c - Convert TTF to C header using FreeType
 * UNICODE VERSION: Supports Spanish, French, Russian, and English
 * 
 * Compile:
 *   gcc -o ttf_to_header ttfconvert_unicode.c `freetype-config --cflags --libs` -lm
 * 
 * Usage:
 *   ./ttf_to_header <font.ttf> <font_size> <output.h>
 * 
 * Example:
 *   ./ttf_to_header /usr/share/fonts/dejavu-sans-fonts/DejaVuSans-Bold.ttf 48 Monospace.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct {
    unsigned char *bitmap;
    int width;
    int height;
    int advance;
    int yoffset;
    unsigned int codepoint;  // Unicode codepoint
} Glyph;

// Character set: ASCII + Spanish + French + Russian
static unsigned int charset[] = {
    // ASCII printable (32-126)
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126,
    
    // Spanish accented characters
    0x00A1,  // ¡
    0x00BF,  // ¿
    0x00C1,  // Á
    0x00C9,  // É
    0x00CD,  // Í
    0x00D1,  // Ñ
    0x00D3,  // Ó
    0x00DA,  // Ú
    0x00DC,  // Ü
    0x00E1,  // á
    0x00E9,  // é
    0x00ED,  // í
    0x00F1,  // ñ
    0x00F3,  // ó
    0x00FA,  // ú
    0x00FC,  // ü
    
    // French accented characters
    0x00C0,  // À
    0x00C2,  // Â
    0x00C7,  // Ç
    0x00C8,  // È
    0x00CA,  // Ê
    0x00CB,  // Ë
    0x00CE,  // Î
    0x00CF,  // Ï
    0x00D4,  // Ô
    0x00D9,  // Ù
    0x00DB,  // Û
    0x00E0,  // à
    0x00E2,  // â
    0x00E7,  // ç
    0x00E8,  // è
    0x00EA,  // ê
    0x00EB,  // ë
    0x00EE,  // î
    0x00EF,  // ï
    0x00F4,  // ô
    0x00F9,  // ù
    0x00FB,  // û
    0x0152,  // Œ
    0x0153,  // œ
    
    // Russian Cyrillic - Uppercase (0x0410-0x042F)
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    
    // Russian Cyrillic - Lowercase (0x0430-0x044F)
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F
};

static int charset_size = sizeof(charset) / sizeof(charset[0]);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <font.ttf> <font_size> <output.h>\n", argv[0]);
        return 1;
    }
    
    const char *font_file = argv[1];
    int font_size = atoi(argv[2]);
    const char *output_file = argv[3];
    
    FT_Library library;
    FT_Face face;
    
    // Initialize FreeType
    if (FT_Init_FreeType(&library)) {
        fprintf(stderr, "Error: Could not initialize FreeType\n");
        return 1;
    }
    
    // Load font
    if (FT_New_Face(library, font_file, 0, &face)) {
        fprintf(stderr, "Error: Could not load font %s\n", font_file);
        FT_Done_FreeType(library);
        return 1;
    }
    
    // Set font size
    FT_Set_Pixel_Sizes(face, 0, font_size);
    
    printf("Font converter using FreeType - Unicode Support\n");
    printf("===============================================\n");
    printf("Font file: %s\n", font_file);
    printf("Font size: %d\n", font_size);
    printf("Output: %s\n\n", output_file);
    
    printf("Face: %s\n", face->family_name);
    printf("Charset size: %d characters (ASCII + Spanish + French + Russian)\n", charset_size);
    printf("Rendering glyphs...\n\n");
    
    // Allocate glyph array
    Glyph *glyphs = malloc(charset_size * sizeof(Glyph));
    memset(glyphs, 0, charset_size * sizeof(Glyph));
    
    int max_width = 0;
    int max_height = 0;
    int rendered_count = 0;
    
    // Render all glyphs from charset
    for (int i = 0; i < charset_size; i++) {
        unsigned int codepoint = charset[i];
        FT_UInt glyph_index = FT_Get_Char_Index(face, codepoint);
        
        if (glyph_index == 0) {
            fprintf(stderr, "Warning: Character U+%04X not in font\n", codepoint);
            continue;
        }
        
        if (FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT)) {
            fprintf(stderr, "Warning: Could not load glyph for U+%04X\n", codepoint);
            continue;
        }
        
        if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
            fprintf(stderr, "Warning: Could not render glyph for U+%04X\n", codepoint);
            continue;
        }
        
        FT_Bitmap *bitmap = &face->glyph->bitmap;
        int width = bitmap->width;
        int height = bitmap->rows;
        
        // For spaces and other zero-width chars, use advance width
        if (width == 0) {
            width = (face->glyph->advance.x >> 6);
            if (width < 1) width = font_size / 2;
        }
        
        if (height == 0) {
            height = font_size;
        }
        
        if (width > max_width) max_width = width;
        if (height > max_height) max_height = height;
        
        glyphs[i].width = width;
        glyphs[i].height = height;
        glyphs[i].advance = face->glyph->advance.x >> 6;
        glyphs[i].yoffset = face->glyph->bitmap_top;
        glyphs[i].codepoint = codepoint;
        
        // Allocate and copy bitmap data
        if (width > 0 && height > 0) {
            int size = width * height;
            glyphs[i].bitmap = malloc(size);
            
            if (bitmap->buffer && bitmap->rows > 0) {
                for (int row = 0; row < bitmap->rows; row++) {
                    for (int col = 0; col < bitmap->width; col++) {
                        glyphs[i].bitmap[row * width + col] = bitmap->buffer[row * bitmap->width + col];
                    }
                }
            } else {
                memset(glyphs[i].bitmap, 0, size);
            }
            rendered_count++;
        }
        
        if (i % 20 == 0) {
            printf("  %d/%d (U+%04X)\n", i + 1, charset_size, codepoint);
        }
    }
    
    printf("\nMax glyph size: %dx%d\n", max_width, max_height);
    printf("Successfully rendered: %d glyphs\n\n", rendered_count);
    
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
    fprintf(out, "// Characters: %d (ASCII + Spanish + French + Russian)\n", charset_size);
    fprintf(out, "// Supports: English, Spanish, French, Russian (UTF-8)\n\n");
    
    // Glyph structure
    fprintf(out, "typedef struct {\n");
    fprintf(out, "    int width;\n");
    fprintf(out, "    int height;\n");
    fprintf(out, "    int advance;\n");
    fprintf(out, "    int yoffset;\n");
    fprintf(out, "    unsigned int codepoint;\n");
    fprintf(out, "    const unsigned char *bitmap;\n");
    fprintf(out, "} GlyphData;\n\n");
    
    // Write glyph bitmaps
    fprintf(out, "// Glyph bitmaps\n\n");
    
    for (int i = 0; i < charset_size; i++) {
        if (!glyphs[i].bitmap) continue;
        
        fprintf(out, "// Character U+%04X\n", glyphs[i].codepoint);
        fprintf(out, "static const unsigned char glyph_u%04x_bitmap[] = {\n", glyphs[i].codepoint);
        
        int size = glyphs[i].width * glyphs[i].height;
        for (int j = 0; j < size; j++) {
            if (j % 16 == 0) fprintf(out, "    ");
            fprintf(out, "0x%02X", glyphs[i].bitmap[j]);
            if (j < size - 1) fprintf(out, ",");
            if ((j + 1) % 16 == 0) fprintf(out, "\n");
        }
        fprintf(out, "\n};\n\n");
    }
    
    // Write glyph structures
    fprintf(out, "// Glyph definitions\n\n");
    
    for (int i = 0; i < charset_size; i++) {
        if (!glyphs[i].bitmap) continue;
        
        fprintf(out, "static const GlyphData glyph_u%04x = {\n", glyphs[i].codepoint);
        fprintf(out, "    %d, %d, %d, %d, 0x%04X,\n", 
                glyphs[i].width, glyphs[i].height, glyphs[i].advance, glyphs[i].yoffset, glyphs[i].codepoint);
        fprintf(out, "    glyph_u%04x_bitmap\n", glyphs[i].codepoint);
        fprintf(out, "};\n\n");
    }
    
    // Write lookup table
    fprintf(out, "// Glyph lookup table - maps codepoint to glyph\n");
    fprintf(out, "typedef struct {\n");
    fprintf(out, "    unsigned int codepoint;\n");
    fprintf(out, "    const GlyphData *glyph;\n");
    fprintf(out, "} GlyphLookup;\n\n");
    
    fprintf(out, "static const GlyphLookup glyph_lookup_table[] = {\n");
    for (int i = 0; i < charset_size; i++) {
        if (!glyphs[i].bitmap) continue;
        fprintf(out, "    {0x%04X, &glyph_u%04x},\n", glyphs[i].codepoint, glyphs[i].codepoint);
    }
    fprintf(out, "    {0, NULL}  // sentinel\n");
    fprintf(out, "};\n\n");
    
    // Write lookup function
    fprintf(out, "// Get glyph by Unicode codepoint (linear search)\n");
    fprintf(out, "static inline const GlyphData *get_glyph(unsigned int codepoint) {\n");
    fprintf(out, "    for (int i = 0; glyph_lookup_table[i].glyph; i++) {\n");
    fprintf(out, "        if (glyph_lookup_table[i].codepoint == codepoint) {\n");
    fprintf(out, "            return glyph_lookup_table[i].glyph;\n");
    fprintf(out, "        }\n");
    fprintf(out, "    }\n");
    fprintf(out, "    return NULL;\n");
    fprintf(out, "}\n\n");
    
    // Write constants
    fprintf(out, "#define FONT_MAX_WIDTH %d\n", max_width);
    fprintf(out, "#define FONT_MAX_HEIGHT %d\n", max_height);
    fprintf(out, "#define FONT_GLYPH_COUNT %d\n\n", rendered_count);
    
    fprintf(out, "#endif // FONT_TTF_H\n");
    
    fclose(out);
    
    // Cleanup
    for (int i = 0; i < charset_size; i++) {
        if (glyphs[i].bitmap) free(glyphs[i].bitmap);
    }
    free(glyphs);
    
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    
    printf("✓ Success! Created %s\n", output_file);
    printf("  Max size: %dx%d\n", max_width, max_height);
    printf("  Total glyphs: %d\n", rendered_count);
    printf("  Supports: English, Spanish, French, Russian (UTF-8)\n");
    
    return 0;
}
