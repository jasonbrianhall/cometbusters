#ifndef WAD_H
#define WAD_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    char *data;
    size_t size;
} WadFile;

typedef struct {
    char filename[4096];  // Path to .wad file
    void *zip_archive;   // mz_zip_archive pointer (opaque)
} WadArchive;

// WAD file operations - FROM FILE
bool wad_open(WadArchive *wad, const char *filename);
void wad_close(WadArchive *wad);

// ============================================================================
// NEW: WAD file operations - FROM MEMORY BUFFER
// Call this instead of wad_open() when you have WAD data in memory
// ============================================================================
bool wad_open_from_memory(WadArchive *wad, const unsigned char *wad_data, size_t wad_size);

// Extract file from WAD
bool wad_extract_file(WadArchive *wad, const char *internal_path, WadFile *out_file);

// Free extracted file
void wad_free_file(WadFile *file);

// List files in WAD
int wad_get_file_count(WadArchive *wad);
const char* wad_get_filename(WadArchive *wad, int index);

// Check if file exists in WAD
bool wad_file_exists(WadArchive *wad, const char *internal_path);

#endif
