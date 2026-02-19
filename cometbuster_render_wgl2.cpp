// cometbuster_render_wgl.cpp
// Win32 child window with WGL OpenGL context embedded in GTK3

#ifdef _WIN32

#include <windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>   // GDK Win32 backend
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

#include "cometbuster.h"
#include "visualization.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

static struct {
    HWND   hwnd;          // Our child HWND
    HDC    hdc;           // Device context for child window
    HGLRC  hglrc;         // WGL rendering context
    HWND   parent_hwnd;   // The GTK placeholder's HWND

    bool   initialized;
    bool   glew_ok;

    int    x, y, w, h;   // Last known geometry (GDK coords)
} g_wgl;

// ============================================================================
// WIN32 WINDOW PROC  (minimal – we only need WM_PAINT suppressed)
// ============================================================================

static LRESULT CALLBACK WGLChildWndProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        // Prevent Win32 from painting over our GL surface
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;  // claim we erased it (suppress flicker)
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ============================================================================
// PIXEL FORMAT / CONTEXT CREATION
// ============================================================================

static bool wgl_choose_pixel_format(HDC hdc)
{
    // Try WGL_ARB_pixel_format for multisampling first
    if (wglewIsSupported("WGL_ARB_pixel_format")) {
        int attribs[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     32,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            WGL_SAMPLE_BUFFERS_ARB, 1,
            WGL_SAMPLES_ARB,        4,   // 4x MSAA
            0
        };
        int  fmt = 0;
        UINT num = 0;
        if (wglChoosePixelFormatARB(hdc, attribs, NULL, 1, &fmt, &num) && num > 0) {
            PIXELFORMATDESCRIPTOR pfd = {};
            DescribePixelFormat(hdc, fmt, sizeof(pfd), &pfd);
            return SetPixelFormat(hdc, fmt, &pfd) == TRUE;
        }
    }

    // Fallback: legacy PIXELFORMATDESCRIPTOR
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize        = sizeof(pfd);
    pfd.nVersion     = 1;
    pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType   = PFD_TYPE_RGBA;
    pfd.cColorBits   = 32;
    pfd.cDepthBits   = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType   = PFD_MAIN_PLANE;

    int fmt = ChoosePixelFormat(hdc, &pfd);
    if (!fmt) return false;
    return SetPixelFormat(hdc, fmt, &pfd) == TRUE;
}

// ============================================================================
// PUBLIC API – called from comet_main.cpp (Win32 path)
// ============================================================================

/**
 * wgl_init_context()
 *
 * Call this from on_realize-equivalent, after the GTK placeholder widget
 * has been mapped and its HWND is available.
 *
 * @param placeholder  GtkWidget* used as the size/position anchor in GTK
 */
bool wgl_init_context(GtkWidget *placeholder)
{
    if (g_wgl.initialized) return true;
    memset(&g_wgl, 0, sizeof(g_wgl));

    // ------------------------------------------------------------------ 1.
    // Get the Win32 HWND that GDK assigned to the placeholder widget.
    // The placeholder must be a GtkDrawingArea (or any realized native widget).
    GdkWindow *gdk_win = gtk_widget_get_window(placeholder);
    if (!gdk_win) {
        SDL_Log("[WGL] placeholder GdkWindow is NULL – is the widget realized?\n");
        return false;
    }
    g_wgl.parent_hwnd = (HWND)GDK_WINDOW_HWND(gdk_win);
    if (!g_wgl.parent_hwnd) {
        SDL_Log("[WGL] GDK_WINDOW_HWND returned NULL\n");
        return false;
    }

    // ------------------------------------------------------------------ 2.
    // Register a minimal window class for our child.
    WNDCLASSEXW wc  = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WGLChildWndProc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"CometBusterWGL";
    RegisterClassExW(&wc);   // ignore ERROR_CLASS_ALREADY_EXISTS

    // ------------------------------------------------------------------ 3.
    // Create the child window, initially same size as placeholder.
    GtkAllocation alloc;
    gtk_widget_get_allocation(placeholder, &alloc);

    g_wgl.x = alloc.x;
    g_wgl.y = alloc.y;
    g_wgl.w = (alloc.width  < 1) ? 640 : alloc.width;
    g_wgl.h = (alloc.height < 1) ? 480 : alloc.height;

    g_wgl.hwnd = CreateWindowExW(
        0,
        L"CometBusterWGL",
        L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        g_wgl.x, g_wgl.y, g_wgl.w, g_wgl.h,
        g_wgl.parent_hwnd,
        NULL,
        GetModuleHandleW(NULL),
        NULL
    );

    if (!g_wgl.hwnd) {
        SDL_Log("[WGL] CreateWindowExW failed: %lu\n", GetLastError());
        return false;
    }

    // ------------------------------------------------------------------ 4.
    // Create a temporary dummy context to bootstrap GLEW / WGL extensions.
    g_wgl.hdc = GetDC(g_wgl.hwnd);

    // Temporary context (needed before wglChoosePixelFormatARB exists)
    PIXELFORMATDESCRIPTOR pfd_tmp = {};
    pfd_tmp.nSize      = sizeof(pfd_tmp);
    pfd_tmp.nVersion   = 1;
    pfd_tmp.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd_tmp.iPixelType = PFD_TYPE_RGBA;
    pfd_tmp.cColorBits = 32;
    pfd_tmp.cDepthBits = 24;
    pfd_tmp.iLayerType = PFD_MAIN_PLANE;

    int tmp_fmt = ChoosePixelFormat(g_wgl.hdc, &pfd_tmp);
    SetPixelFormat(g_wgl.hdc, tmp_fmt, &pfd_tmp);
    HGLRC tmp_ctx = wglCreateContext(g_wgl.hdc);
    wglMakeCurrent(g_wgl.hdc, tmp_ctx);

    // Init GLEW so ARB extensions become available
    glewExperimental = GL_TRUE;
    GLenum glew_err = glewInit();
    SDL_Log("[WGL] glewInit (bootstrap): %s\n", glewGetErrorString(glew_err));

    // Destroy dummy context, release DC, then recreate properly
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tmp_ctx);
    ReleaseDC(g_wgl.hwnd, g_wgl.hdc);

    // ------------------------------------------------------------------ 5.
    // Now create the REAL context with a good pixel format.
    g_wgl.hdc = GetDC(g_wgl.hwnd);

    if (!wgl_choose_pixel_format(g_wgl.hdc)) {
        SDL_Log("[WGL] wgl_choose_pixel_format failed\n");
        return false;
    }

    // Try core profile 3.3 first via WGL_ARB_create_context
    if (wglewIsSupported("WGL_ARB_create_context")) {
        int ctx_attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        g_wgl.hglrc = wglCreateContextAttribsARB(g_wgl.hdc, NULL, ctx_attribs);
    }

    // Fallback: compatibility context
    if (!g_wgl.hglrc) {
        g_wgl.hglrc = wglCreateContext(g_wgl.hdc);
    }

    if (!g_wgl.hglrc) {
        SDL_Log("[WGL] wglCreateContext failed: %lu\n", GetLastError());
        return false;
    }

    wglMakeCurrent(g_wgl.hdc, g_wgl.hglrc);

    // Re-init GLEW with the real context
    glew_err = glewInit();
    SDL_Log("[WGL] glewInit (real ctx): %s\n", glewGetErrorString(glew_err));
    g_wgl.glew_ok = (glew_err == GLEW_OK);

    // ------------------------------------------------------------------ 6.
    // Basic GL state (mirrors on_realize from cometbuster_render_gl2.cpp)
    glClearColor(0.04f, 0.06f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);

    g_wgl.initialized = true;
    SDL_Log("[WGL] Context initialized OK, HWND=%p\n", (void*)g_wgl.hwnd);
    return true;
}

/**
 * wgl_resize()
 *
 * Keep the child HWND in sync with the GTK placeholder's allocation.
 * Connect this to the placeholder's "size-allocate" signal.
 */
void wgl_resize(GtkWidget *placeholder)
{
    if (!g_wgl.hwnd) return;

    GtkAllocation alloc;
    gtk_widget_get_allocation(placeholder, &alloc);

    if (alloc.width  == g_wgl.w  &&
        alloc.height == g_wgl.h  &&
        alloc.x      == g_wgl.x  &&
        alloc.y      == g_wgl.y)
        return;  // nothing changed

    g_wgl.x = alloc.x;
    g_wgl.y = alloc.y;
    g_wgl.w = (alloc.width  < 1) ? 1 : alloc.width;
    g_wgl.h = (alloc.height < 1) ? 1 : alloc.height;

    SetWindowPos(g_wgl.hwnd, HWND_TOP,
                 g_wgl.x, g_wgl.y,
                 g_wgl.w, g_wgl.h,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

/**
 * wgl_make_current()
 * Bind the WGL context before any GL calls.
 */
void wgl_make_current(void)
{
    if (g_wgl.hdc && g_wgl.hglrc)
        wglMakeCurrent(g_wgl.hdc, g_wgl.hglrc);
}

/**
 * wgl_swap_buffers()
 * Present the frame.
 */
void wgl_swap_buffers(void)
{
    if (g_wgl.hdc)
        SwapBuffers(g_wgl.hdc);
}

/**
 * wgl_cleanup()
 * Destroy context and child window.
 */
void wgl_cleanup(void)
{
    if (g_wgl.hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_wgl.hglrc);
        g_wgl.hglrc = NULL;
    }
    if (g_wgl.hdc && g_wgl.hwnd) {
        ReleaseDC(g_wgl.hwnd, g_wgl.hdc);
        g_wgl.hdc = NULL;
    }
    if (g_wgl.hwnd) {
        DestroyWindow(g_wgl.hwnd);
        g_wgl.hwnd = NULL;
    }
    g_wgl.initialized = false;
}

/**
 * wgl_render_frame()
 *
 * Drop-in equivalent of on_render() for the WGL path.
 * Call this from your game_update_timer callback instead of
 * gtk_widget_queue_draw() when rendering_engine == 1 on Windows.
 */
void wgl_render_frame(Visualizer *vis)
{
    if (!g_wgl.initialized || !vis) return;

    wgl_make_current();

    int w = g_wgl.w;
    int h = g_wgl.h;

    vis->width  = 1920;
    vis->height = 1080;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Letterbox viewport – identical logic to cometbuster_render_gl2.cpp
    double scale_x = (double)w / 1920.0;
    double scale_y = (double)h / 1080.0;
    double scale   = (scale_x < scale_y) ? scale_x : scale_y;

    int vp_w = (int)(1920.0 * scale);
    int vp_h = (int)(1080.0 * scale);
    int vp_x = 0;
    int vp_y = (h - vp_h) / 2;

    glViewport(vp_x, vp_y, vp_w, vp_h);

    draw_comet_buster_gl(vis, NULL);

    wgl_swap_buffers();
}

// ============================================================================
// GTK SIGNAL CALLBACKS  (wire these up in comet_main.cpp on Windows)
// ============================================================================

/**
 * on_wgl_placeholder_realize()
 * Connect to "realize" signal of the GtkDrawingArea placeholder.
 *
 *   g_signal_connect(gui.gl_area, "realize",
 *                    G_CALLBACK(on_wgl_placeholder_realize), &gui.visualizer);
 */
void on_wgl_placeholder_realize(GtkWidget *widget, gpointer user_data)
{
    (void)user_data;
    // Force the GdkWindow to have a native HWND before we grab it
    gdk_window_ensure_native(gtk_widget_get_window(widget));
    wgl_init_context(widget);
}

/**
 * on_wgl_placeholder_size_allocate()
 * Connect to "size-allocate" signal of the placeholder.
 *
 *   g_signal_connect(gui.gl_area, "size-allocate",
 *                    G_CALLBACK(on_wgl_placeholder_size_allocate), NULL);
 */
void on_wgl_placeholder_size_allocate(GtkWidget *widget,
                                       GdkRectangle *allocation,
                                       gpointer user_data)
{
    (void)allocation;
    (void)user_data;
    wgl_resize(widget);
}

#endif  // _WIN32
