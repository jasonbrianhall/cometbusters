#include <cairo.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "cometbuster.h"
#include "visualization.h"

#ifndef M_PI
#define M_PI 3.1415926535
#endif

// ============================================================================
// RENDER CACHE INITIALIZATION
// ============================================================================

void render_cache_init(RenderCache *cache) {
    if (!cache) return;
    
    memset(cache, 0, sizeof(RenderCache));
    
    fprintf(stdout, "[*] Initializing render cache for Cairo optimization...\n");
    
    // ====== GLOW PARTICLE CACHE ======
    // Pre-render glow particles at 5 different sizes for reuse
    int glow_sizes[] = {4, 8, 12, 16, 20};
    
    for (int i = 0; i < 5; i++) {
        int size = glow_sizes[i];
        cache->glow_particle_surface[i] = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, size * 2, size * 2);
        
        cairo_t *cr = cairo_create(cache->glow_particle_surface[i]);
        
        // Clear with full transparency
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        
        // Draw glow with multiple layers for depth
        double center = size;
        
        // Outer glow (most transparent)
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.1);
        cairo_arc(cr, center, center, size * 0.9, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Middle glow
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.3);
        cairo_arc(cr, center, center, size * 0.6, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        // Inner bright core
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.8);
        cairo_arc(cr, center, center, size * 0.3, 0, 2.0 * M_PI);
        cairo_fill(cr);
        
        cairo_destroy(cr);
        
        fprintf(stdout, "[*] Cached glow particle size %d\n", size);
    }
    
    // ====== LINE PARTICLE CACHE ======
    // Pre-render directional line particles at 3 different widths
    int line_widths[] = {2, 4, 8};
    
    for (int i = 0; i < 3; i++) {
        cache->line_particle_surface[i] = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32, 4, 40);
        
        cairo_t *cr = cairo_create(cache->line_particle_surface[i]);
        cairo_set_source_rgba(cr, 0, 0, 0, 0);
        cairo_paint(cr);
        
        // Draw vertical line (will be rotated when used in render)
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
        cairo_set_line_width(cr, line_widths[i]);
        cairo_move_to(cr, 2, 0);
        cairo_line_to(cr, 2, 40);
        cairo_stroke(cr);
        
        cairo_destroy(cr);
    }
    
    cache->initialized = true;
    fprintf(stdout, "[*] Render cache initialized successfully\n");
}

// ============================================================================
// RENDER CACHE CLEANUP
// ============================================================================

void render_cache_cleanup(RenderCache *cache) {
    if (!cache) return;
    
    fprintf(stdout, "[*] Cleaning up render cache...\n");
    
    // Destroy glow particles
    for (int i = 0; i < 5; i++) {
        if (cache->glow_particle_surface[i]) {
            cairo_surface_destroy(cache->glow_particle_surface[i]);
            cache->glow_particle_surface[i] = NULL;
        }
    }
    
    // Destroy line particles
    for (int i = 0; i < 3; i++) {
        if (cache->line_particle_surface[i]) {
            cairo_surface_destroy(cache->line_particle_surface[i]);
            cache->line_particle_surface[i] = NULL;
        }
    }
    
    // Destroy grid cache
    if (cache->grid_surface) {
        cairo_surface_destroy(cache->grid_surface);
        cache->grid_surface = NULL;
    }
    
    cache->initialized = false;
    fprintf(stdout, "[*] Render cache cleaned up\n");
}

// ============================================================================
// GRID CACHING (BIGGEST WIN!)
// ============================================================================
// Your grid is redrawn every frame - this optimization eliminates that!

void render_cache_update_grid(RenderCache *cache, int width, int height, cairo_t *cr) {
    if (!cache || !cr) return;
    
    // Check if grid cache is still valid (size hasn't changed)
    if (cache->cached_grid_width == width && 
        cache->cached_grid_height == height && 
        cache->grid_surface) {
        
        // Size hasn't changed - reuse cached grid!
        cairo_set_source_surface(cr, cache->grid_surface, 0, 0);
        cairo_paint(cr);
        return;
    }
    
    // Window size changed or cache doesn't exist - recreate it
    if (cache->grid_surface) {
        cairo_surface_destroy(cache->grid_surface);
        fprintf(stdout, "[*] Grid cache invalidated (size changed)\n");
    }
    
    // Create new grid cache surface
    cache->grid_surface = cairo_image_surface_create(
        CAIRO_FORMAT_ARGB32, width, height);
    
    cairo_t *grid_cr = cairo_create(cache->grid_surface);
    
    // Draw background (space-like dark color)
    cairo_set_source_rgb(grid_cr, 0.04, 0.06, 0.15);
    cairo_paint(grid_cr);
    
    // Draw grid lines
    cairo_set_source_rgb(grid_cr, 0.1, 0.15, 0.35);
    cairo_set_line_width(grid_cr, 0.5);
    
    // Vertical lines
    for (int i = 0; i <= width + 50; i += 50) {
        cairo_move_to(grid_cr, i, 0);
        cairo_line_to(grid_cr, i, height);
    }
    
    // Horizontal lines
    for (int i = 0; i <= height; i += 50) {
        cairo_move_to(grid_cr, 0, i);
        cairo_line_to(grid_cr, width + 50, i);
    }
    
    // Stroke all lines at once (efficient!)
    cairo_stroke(grid_cr);
    
    cairo_destroy(grid_cr);
    
    cache->cached_grid_width = width;
    cache->cached_grid_height = height;
    
    fprintf(stdout, "[*] Grid cache created (%d x %d)\n", width, height);
    
    // Now draw the cached grid to the main surface
    cairo_set_source_surface(cr, cache->grid_surface, 0, 0);
    cairo_paint(cr);
}

// ============================================================================
// BOSS EXPLOSION CACHED DRAWING
// ============================================================================
// Use pre-rendered surfaces instead of drawing shapes every frame

void boss_explosion_draw_cached(BossExplosion *explosion, cairo_t *cr, 
                                RenderCache *cache) {
    if (!explosion || !cr || !cache || !cache->initialized) {
        // Fallback to original draw function if cache not available
        if (explosion && cr) {
            boss_explosion_draw(explosion, cr);
        }
        return;
    }
    
    // BATCH 1: Draw all radial lines at once
    // ========================================
    cairo_save(cr);
    
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active || !p->is_radial_line) continue;
        
        // Calculate end point of line
        double end_x = p->x + cos(p->angle) * p->length;
        double end_y = p->y + sin(p->angle) * p->length;
        double alpha = p->glow_intensity;
        
        // Draw thick glow line (outer glow)
        cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], 
                            alpha * 0.3);
        cairo_set_line_width(cr, p->width * 4.0);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_move_to(cr, p->x, p->y);
        cairo_line_to(cr, end_x, end_y);
        
        // Draw bright inner line
        cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha);
        cairo_set_line_width(cr, p->width);
        cairo_move_to(cr, p->x, p->y);
        cairo_line_to(cr, end_x, end_y);
    }
    
    // Single stroke() call for all lines - much faster than individual strokes!
    cairo_stroke(cr);
    
    cairo_restore(cr);
    
    // BATCH 2: Draw all glow particles using cached surfaces
    // =======================================================
    for (int i = 0; i < explosion->particle_count; i++) {
        BossExplosionParticle *p = &explosion->particles[i];
        if (!p->active || p->is_radial_line) continue;
        
        // Select glow size (0-5) based on particle width
        int glow_index = (int)(p->width * 1.25);
        if (glow_index < 0) glow_index = 0;
        if (glow_index >= 5) glow_index = 4;
        
        cairo_surface_t *glow_surface = cache->glow_particle_surface[glow_index];
        
        // Get cached surface size
        int glow_size = cairo_image_surface_get_width(glow_surface) / 2;
        
        // Apply color and draw cached glow
        double alpha = p->glow_intensity;
        cairo_set_source_rgba(cr, p->color[0], p->color[1], p->color[2], alpha);
        cairo_set_source_surface(cr, glow_surface, 
                                p->x - glow_size, p->y - glow_size);
        cairo_paint(cr);
    }
}

// ============================================================================
// DEBUG FUNCTION - Verify cache is working
// ============================================================================

void render_cache_debug_info(RenderCache *cache) {
    if (!cache) return;
    
    fprintf(stdout, "\n[DEBUG] Render Cache Status:\n");
    fprintf(stdout, "================================\n");
    
    if (!cache->initialized) {
        fprintf(stdout, "Status: NOT INITIALIZED\n");
        fprintf(stdout, "================================\n");
        return;
    }
    
    fprintf(stdout, "Status: ACTIVE\n");
    fprintf(stdout, "  Glow particles: %s\n", 
           cache->glow_particle_surface[0] ? "YES (5 sizes)" : "NO");
    fprintf(stdout, "  Line particles: %s\n",
           cache->line_particle_surface[0] ? "YES (3 widths)" : "NO");
    fprintf(stdout, "  Grid cache: %s (%d x %d)\n",
           cache->grid_surface ? "YES" : "NO",
           cache->cached_grid_width, cache->cached_grid_height);
    fprintf(stdout, "================================\n\n");
}
