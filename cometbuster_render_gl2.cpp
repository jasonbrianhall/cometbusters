#include <GL/glew.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <pango/pango.h>
#include <gtk/gtk.h>
#include "cometbuster.h"
#include "visualization.h"

// ============================================================================
// OPENGL CONTEXT CALLBACKS
// ============================================================================

gboolean on_realize(GtkGLArea *area, gpointer data) {
    (void)data;
    gtk_gl_area_make_current(area);
    
    static gboolean glew_initialized = FALSE;
    if (!glew_initialized) {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        fprintf(stderr, "[GL] glewInit: %s\n", glewGetErrorString(err));
        glew_initialized = TRUE;
    }
    
    glClearColor(0.04f, 0.06f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);
    
    fprintf(stderr, "[GL] GL Context initialized\n");
    return TRUE;
}

gboolean on_render(GtkGLArea *area, GdkGLContext *context, gpointer data) {
    (void)context;
    Visualizer *vis = (Visualizer *)data;
    if (!vis) return FALSE;
    
    gtk_gl_area_make_current(area);
    
    int window_width = gtk_widget_get_allocated_width(GTK_WIDGET(area));
    int window_height = gtk_widget_get_allocated_height(GTK_WIDGET(area));
    
    if (window_width < 10 || window_height < 10) {
        gtk_widget_queue_draw(GTK_WIDGET(area));
        return TRUE;
    }
    
    vis->width = 1920;
    vis->height = 1080;
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Calculate viewport with letterboxing to maintain 1920x1080 aspect ratio (matches Cairo)
    double scale_x = (double)window_width / 1920.0;
    double scale_y = (double)window_height / 1080.0;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;  // Maintain aspect ratio
    
    int viewport_width = (int)(1920.0 * scale);
    int viewport_height = (int)(1080.0 * scale);
    int viewport_x = 0;  // Align to left edge (no centering, like Cairo)
    int viewport_y = (window_height - viewport_height) / 2;  // Center vertically
    
    glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
    
    // RENDER ONLY - game updates happen in game_update_timer callback
    draw_comet_buster_gl(vis, NULL);
    
    glFlush();
    gtk_widget_queue_draw(GTK_WIDGET(area));
    
    return TRUE;
}
