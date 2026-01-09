// ============================================================================
// WINDOWS DIRECT2D HEADER
// File: windows_direct2d.h
// Use this in comet_main.cpp to call Direct2D functions
// ============================================================================

#ifndef WINDOWS_DIRECT2D_H
#define WINDOWS_DIRECT2D_H

#ifdef _WIN32

#include <cairo.h>

// ============================================================================
// PUBLIC FUNCTIONS FOR DIRECT2D
// ============================================================================

/**
 * Initialize Direct2D factory and resources
 * Call this once at startup, in main() after GTK initialization
 * Safe to call multiple times (checks if already initialized)
 */
void windows_direct2d_init(void);

/**
 * Create a Direct2D-backed Cairo surface
 * Returns NULL if Direct2D not available (falls back to GDI)
 * Parameters:
 *   width, height - surface dimensions
 */
cairo_surface_t* windows_direct2d_surface_create(int width, int height);

/**
 * Get current Direct2D status as a string
 * Useful for debugging/logging
 * Returns: "Direct2D ENABLED (5-10x faster)" or "GDI backend (slow)"
 */
const char* windows_direct2d_status(void);

/**
 * Cleanup Direct2D resources
 * Call this in cleanup code before program exit
 * Safe to call multiple times
 */
void windows_direct2d_cleanup(void);

#endif // _WIN32

#endif // WINDOWS_DIRECT2D_H
