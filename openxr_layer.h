#ifndef OPENXR_LAYER_H
#define OPENXR_LAYER_H

// GLEW MUST come before any other GL headers
#include <GL/glew.h>
#include <GL/gl.h>

// X11 headers next
#include <X11/Xlib.h>
#include <GL/glx.h>

// Platform-specific defines MUST come before including openxr headers
#define XR_USE_GRAPHICS_API_OPENGL
#define XR_USE_PLATFORM_XLIB

// OpenXR last
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

typedef struct {
    XrInstance instance;
    XrSession session;
    XrSessionState session_state;
    XrSpace play_space;
    
    XrSwapchain swapchain;
    uint32_t swapchain_length;
    GLuint *swapchain_textures;
    GLuint *swapchain_fbos;
    int swapchain_width;
    int swapchain_height;
    
    XrFrameState frame_state;
    XrViewState view_state;
    XrView views[2];  // Left and right eye
    
    bool session_running;
    double frame_time;
} OpenXRContext;

// Initialize OpenXR
int openxr_init(OpenXRContext *ctx);

// Get eye projection matrices and viewport info
void openxr_get_eye_matrices(OpenXRContext *ctx, int eye, 
                             float *projection_matrix, 
                             float *view_matrix,
                             int *viewport_width, int *viewport_height);

// Begin frame
int openxr_begin_frame(OpenXRContext *ctx);

// End frame (submit to compositor)
int openxr_end_frame(OpenXRContext *ctx);

// Get controller state (simplified)
void openxr_get_controller_state(OpenXRContext *ctx, 
                                 float *stick_x, float *stick_y,
                                 uint32_t *buttons);

// Cleanup
void openxr_shutdown(OpenXRContext *ctx);

#endif
