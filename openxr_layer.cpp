#include "openxr_layer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
    #include <GL/wglext.h>
#else
    #include <X11/Xlib.h>
    #include <GL/glx.h>
#endif

#define CHECK_XR(x) do { \
    XrResult res = (x); \
    if (!XR_SUCCEEDED(res)) { \
        fprintf(stderr, "[XR] Error at %s:%d: %d\n", __FILE__, __LINE__, res); \
        return -1; \
    } \
} while(0)

int openxr_init(OpenXRContext *ctx) {
    if (!ctx) return -1;
    memset(ctx, 0, sizeof(OpenXRContext));
    
    fprintf(stderr, "[XR] Initializing OpenXR...\n");
    
    // Create instance with OpenGL extension enabled
    const char *enabled_extensions[] = {
        XR_KHR_OPENGL_ENABLE_EXTENSION_NAME
    };
    uint32_t enabled_extension_count = 1;
    
    XrApplicationInfo app_info;
    memset(&app_info, 0, sizeof(XrApplicationInfo));
    strcpy(app_info.applicationName, "CometBusters");
    strcpy(app_info.engineName, "CometBusters");
    app_info.applicationVersion = 1;
    app_info.engineVersion = 1;
    app_info.apiVersion = XR_CURRENT_API_VERSION;
    
    XrInstanceCreateInfo instance_info;
    memset(&instance_info, 0, sizeof(XrInstanceCreateInfo));
    instance_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instance_info.applicationInfo = app_info;
    instance_info.enabledExtensionCount = enabled_extension_count;
    instance_info.enabledExtensionNames = enabled_extensions;
    instance_info.enabledApiLayerCount = 0;
    
    CHECK_XR(xrCreateInstance(&instance_info, &ctx->instance));
    fprintf(stderr, "[XR] Instance created\n");
    
    // Get system
    XrSystemGetInfo system_info;
    memset(&system_info, 0, sizeof(XrSystemGetInfo));
    system_info.type = XR_TYPE_SYSTEM_GET_INFO;
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    
    XrSystemId system_id;
    CHECK_XR(xrGetSystem(ctx->instance, &system_info, &system_id));
    fprintf(stderr, "[XR] System found\n");
    
    // Create session with graphics binding
#ifdef _WIN32
    XrGraphicsBindingOpenGLWin32KHR graphics_binding;
    memset(&graphics_binding, 0, sizeof(XrGraphicsBindingOpenGLWin32KHR));
    graphics_binding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
    graphics_binding.hDC = wglGetCurrentDC();
    graphics_binding.hGLRC = wglGetCurrentContext();
#else
    XrGraphicsBindingOpenGLXlibKHR graphics_binding;
    memset(&graphics_binding, 0, sizeof(XrGraphicsBindingOpenGLXlibKHR));
    graphics_binding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
    graphics_binding.xDisplay = glXGetCurrentDisplay();
    graphics_binding.glxFBConfig = 0;  // Let runtime choose
    graphics_binding.glxDrawable = glXGetCurrentDrawable();
    graphics_binding.glxContext = glXGetCurrentContext();
#endif
    
    XrSessionCreateInfo session_info;
    memset(&session_info, 0, sizeof(XrSessionCreateInfo));
    session_info.type = XR_TYPE_SESSION_CREATE_INFO;
    session_info.systemId = system_id;
    session_info.next = &graphics_binding;
    
    CHECK_XR(xrCreateSession(ctx->instance, &session_info, &ctx->session));
    fprintf(stderr, "[XR] Session created\n");
    
    // Create reference space
    XrReferenceSpaceCreateInfo space_info;
    memset(&space_info, 0, sizeof(XrReferenceSpaceCreateInfo));
    space_info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
    space_info.poseInReferenceSpace.orientation.w = 1.0f;
    
    CHECK_XR(xrCreateReferenceSpace(ctx->session, &space_info, &ctx->play_space));
    fprintf(stderr, "[XR] Reference space created\n");
    
    // Create swapchain
    XrSwapchainCreateInfo swapchain_info;
    memset(&swapchain_info, 0, sizeof(XrSwapchainCreateInfo));
    swapchain_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapchain_info.usageFlags = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.format = GL_RGBA8;
    swapchain_info.sampleCount = 1;
    swapchain_info.width = 1920;   // Per eye
    swapchain_info.height = 1080;
    swapchain_info.faceCount = 1;
    swapchain_info.arraySize = 1;
    swapchain_info.mipCount = 1;
    
    CHECK_XR(xrCreateSwapchain(ctx->session, &swapchain_info, &ctx->swapchain));
    ctx->swapchain_width = 1920;
    ctx->swapchain_height = 1080;
    fprintf(stderr, "[XR] Swapchain created\n");
    
    // Get swapchain images
    CHECK_XR(xrEnumerateSwapchainImages(ctx->swapchain, 0, &ctx->swapchain_length, NULL));
    
    XrSwapchainImageOpenGLKHR *gl_images = 
        (XrSwapchainImageOpenGLKHR*)malloc(ctx->swapchain_length * sizeof(XrSwapchainImageOpenGLKHR));
    ctx->swapchain_textures = (GLuint*)malloc(ctx->swapchain_length * sizeof(GLuint));
    ctx->swapchain_fbos = (GLuint*)malloc(ctx->swapchain_length * sizeof(GLuint));
    
    for (uint32_t i = 0; i < ctx->swapchain_length; i++) {
        memset(&gl_images[i], 0, sizeof(XrSwapchainImageOpenGLKHR));
        gl_images[i].type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
    }
    
    CHECK_XR(xrEnumerateSwapchainImages(ctx->swapchain, ctx->swapchain_length, 
                                        &ctx->swapchain_length, 
                                        (XrSwapchainImageBaseHeader*)gl_images));
    
    // Create framebuffer objects for each swapchain image
    for (uint32_t i = 0; i < ctx->swapchain_length; i++) {
        ctx->swapchain_textures[i] = gl_images[i].image;
        
        glGenFramebuffers(1, &ctx->swapchain_fbos[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, ctx->swapchain_fbos[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                              GL_TEXTURE_2D, ctx->swapchain_textures[i], 0);
        
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "[XR] FBO error: %x\n", status);
            return -1;
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    free(gl_images);
    fprintf(stderr, "[XR] Swapchain images bound to FBOs (%d images)\n", ctx->swapchain_length);
    
    ctx->session_running = false;
    ctx->frame_time = 0.0;
    
    return 0;
}

int openxr_begin_frame(OpenXRContext *ctx) {
    if (!ctx || !ctx->session) return -1;
    
    // Begin frame
    XrFrameBeginInfo frame_begin;
    memset(&frame_begin, 0, sizeof(XrFrameBeginInfo));
    frame_begin.type = XR_TYPE_FRAME_BEGIN_INFO;
    
    CHECK_XR(xrBeginFrame(ctx->session, &frame_begin));
    
    // Get frame state
    XrFrameState frame_state;
    memset(&frame_state, 0, sizeof(XrFrameState));
    frame_state.type = XR_TYPE_FRAME_STATE;
    CHECK_XR(xrWaitFrame(ctx->session, NULL, &frame_state));
    
    ctx->frame_state = frame_state;
    ctx->frame_time = (double)ctx->frame_state.predictedDisplayTime / 1e9;
    
    if (!ctx->frame_state.shouldRender) {
        return 0;  // Skip this frame
    }
    
    // Locate views
    XrViewLocateInfo view_locate;
    memset(&view_locate, 0, sizeof(XrViewLocateInfo));
    view_locate.type = XR_TYPE_VIEW_LOCATE_INFO;
    view_locate.viewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    view_locate.displayTime = ctx->frame_state.predictedDisplayTime;
    view_locate.space = ctx->play_space;
    
    uint32_t view_count = 2;
    XrViewState view_state;
    memset(&view_state, 0, sizeof(XrViewState));
    view_state.type = XR_TYPE_VIEW_STATE;
    
    CHECK_XR(xrLocateViews(ctx->session, &view_locate, &view_state, 
                           view_count, &view_count, ctx->views));
    
    ctx->view_state = view_state;
    
    return 1;  // Frame should render
}

int openxr_end_frame(OpenXRContext *ctx) {
    if (!ctx || !ctx->session) return -1;
    
    // For now, just end the frame without submitting layers
    // Full implementation would submit composition layers here
    
    XrFrameEndInfo frame_end;
    memset(&frame_end, 0, sizeof(XrFrameEndInfo));
    frame_end.type = XR_TYPE_FRAME_END_INFO;
    frame_end.displayTime = ctx->frame_state.predictedDisplayTime;
    frame_end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    frame_end.layerCount = 0;
    frame_end.layers = NULL;
    
    CHECK_XR(xrEndFrame(ctx->session, &frame_end));
    
    return 0;
}

void openxr_get_eye_matrices(OpenXRContext *ctx, int eye,
                             float *projection_matrix,
                             float *view_matrix,
                             int *viewport_width, int *viewport_height) {
    if (!ctx || eye < 0 || eye > 1) return;
    
    // Get projection matrix from XrFovf
    XrFovf fov = ctx->views[eye].fov;
    
    // Build perspective projection
    float tan_left = tanf(fov.angleLeft);
    float tan_right = tanf(fov.angleRight);
    float tan_up = tanf(fov.angleUp);
    float tan_down = tanf(fov.angleDown);
    
    float width = tan_right - tan_left;
    float height = tan_up - tan_down;
    
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
    
    memset(projection_matrix, 0, 16 * sizeof(float));
    projection_matrix[0] = 2.0f / width;
    projection_matrix[5] = 2.0f / height;
    projection_matrix[8] = (tan_right + tan_left) / width;
    projection_matrix[9] = (tan_up + tan_down) / height;
    projection_matrix[10] = -(far_plane + near_plane) / (far_plane - near_plane);
    projection_matrix[11] = -1.0f;
    projection_matrix[14] = -(2.0f * far_plane * near_plane) / (far_plane - near_plane);
    
    // Get view matrix from pose
    XrPosef pose = ctx->views[eye].pose;
    
    // Build view matrix from position and orientation
    float qx = pose.orientation.x;
    float qy = pose.orientation.y;
    float qz = pose.orientation.z;
    float qw = pose.orientation.w;
    
    // Convert quaternion to rotation matrix
    float rot[16] = {
        1-2*(qy*qy+qz*qz), 2*(qx*qy-qz*qw),     2*(qx*qz+qy*qw),     0,
        2*(qx*qy+qz*qw),   1-2*(qx*qx+qz*qz),   2*(qy*qz-qx*qw),     0,
        2*(qx*qz-qy*qw),   2*(qy*qz+qx*qw),     1-2*(qx*qx+qy*qy),   0,
        0,                  0,                     0,                   1
    };
    
    // Translation part (negate for view matrix)
    rot[12] = -pose.position.x;
    rot[13] = -pose.position.y;
    rot[14] = -pose.position.z;
    
    memcpy(view_matrix, rot, 16 * sizeof(float));
    
    *viewport_width = ctx->swapchain_width;
    *viewport_height = ctx->swapchain_height;
}

void openxr_get_controller_state(OpenXRContext *ctx,
                                 float *stick_x, float *stick_y,
                                 uint32_t *buttons) {
    (void)ctx;  // Unused for now
    
    // Placeholder - implement controller tracking
    if (stick_x) *stick_x = 0.0f;
    if (stick_y) *stick_y = 0.0f;
    if (buttons) *buttons = 0;
}

void openxr_shutdown(OpenXRContext *ctx) {
    if (!ctx) return;
    
    if (ctx->play_space) xrDestroySpace(ctx->play_space);
    if (ctx->swapchain) xrDestroySwapchain(ctx->swapchain);
    if (ctx->session) xrDestroySession(ctx->session);
    if (ctx->instance) xrDestroyInstance(ctx->instance);
    
    if (ctx->swapchain_textures) free(ctx->swapchain_textures);
    if (ctx->swapchain_fbos) free(ctx->swapchain_fbos);
    
    memset(ctx, 0, sizeof(OpenXRContext));
    fprintf(stderr, "[XR] Shutdown complete\n");
}
