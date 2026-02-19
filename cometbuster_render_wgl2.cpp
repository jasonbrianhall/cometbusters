// cometbuster_render_wgl2.cpp
// Raw WGL OpenGL context embedded in a GTK3 GtkDrawingArea on Win32.
//
// Key design decisions:
//   - Pure WGL — no SDL involved in context creation or swap
//   - The placeholder GtkDrawingArea's "draw" signal returns TRUE (no-op)
//     to suppress GTK painting white over our GL surface each frame
//   - GTK owns all input and focus events — we never touch the Win32 message loop
//   - Child HWND is always at position (0,0) within its native parent HWND

#ifdef _WIN32

#include <windows.h>
#include <GL/glew.h>
#include <GL/wglew.h>
#include <gtk/gtk.h>
#include <gdk/gdkwin32.h>
#define SDL_MAIN_HANDLED  // Prevent SDL from redefining main() on Windows
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

#include "cometbuster.h"
#include "visualization.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

static struct {
    HWND   hwnd;          // Child HWND parented to GTK placeholder
    HDC    hdc;           // CS_OWNDC device context — stays valid for window lifetime
    HGLRC  hglrc;         // WGL rendering context

    bool   initialized;
    int    w, h;          // Last known size (always at 0,0 within parent)
} g_wgl;

// ============================================================================
// WIN32 WINDOW PROC — suppress WM_PAINT so Win32 doesn't clear the GL surface
// ============================================================================

static LRESULT CALLBACK WGLChildWndProc(HWND hwnd, UINT msg,
                                         WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_ERASEBKGND:
            return 1;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

bool sdl_wgl_init(GtkWidget *placeholder)
{
    if (g_wgl.initialized) return true;
    memset(&g_wgl, 0, sizeof(g_wgl));

    // ------------------------------------------------------------------ 1.
    // Get the native HWND of the GTK placeholder widget.
    GdkWindow *gdk_win = gtk_widget_get_window(placeholder);
    if (!gdk_win) {
        SDL_Log("[WGL] placeholder GdkWindow is NULL\n");
        return false;
    }
    HWND parent_hwnd = (HWND)GDK_WINDOW_HWND(gdk_win);
    if (!parent_hwnd) {
        SDL_Log("[WGL] GDK_WINDOW_HWND returned NULL\n");
        return false;
    }

    // ------------------------------------------------------------------ 2.
    // Register window class (CS_OWNDC so HDC stays valid permanently)
    // and create child HWND at (0,0) filling parent.
    WNDCLASSEXW wc = {};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WGLChildWndProc;
    wc.hInstance     = GetModuleHandleW(NULL);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"CometBusterWGL";
    RegisterClassExW(&wc);  // safe to call multiple times

    GtkAllocation alloc;
    gtk_widget_get_allocation(placeholder, &alloc);
    g_wgl.w = (alloc.width  < 1) ? 640 : alloc.width;
    g_wgl.h = (alloc.height < 1) ? 480 : alloc.height;

    // Position is (0,0) — child coords are relative to the native parent HWND,
    // not the GTK container. alloc.x/y are GTK coords and must not be used here.
    g_wgl.hwnd = CreateWindowExW(
        0,
        L"CometBusterWGL", L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, g_wgl.w, g_wgl.h,
        parent_hwnd,
        NULL, GetModuleHandleW(NULL), NULL
    );
    if (!g_wgl.hwnd) {
        SDL_Log("[WGL] CreateWindowExW failed: %lu\n", GetLastError());
        return false;
    }

    // CS_OWNDC: GetDC returns the same HDC every time; no need to release it.
    g_wgl.hdc = GetDC(g_wgl.hwnd);

    // ------------------------------------------------------------------ 3.
    // Bootstrap GLEW via a throwaway window so wglChoosePixelFormatARB
    // and wglCreateContextAttribsARB are available.
    // CRITICAL: SetPixelFormat can only be called ONCE per HDC on Windows.
    // We must not call it on our real HDC during bootstrap.
    {
        HWND dummy = CreateWindowExW(0, L"CometBusterWGL", L"",
                                      WS_OVERLAPPED, 0, 0, 1, 1,
                                      NULL, NULL, GetModuleHandleW(NULL), NULL);
        HDC dummy_dc = GetDC(dummy);

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;
        pfd.iLayerType = PFD_MAIN_PLANE;

        int fmt = ChoosePixelFormat(dummy_dc, &pfd);
        SetPixelFormat(dummy_dc, fmt, &pfd);
        HGLRC dummy_ctx = wglCreateContext(dummy_dc);
        wglMakeCurrent(dummy_dc, dummy_ctx);

        glewExperimental = GL_TRUE;
        glewInit();  // populate wglew extension pointers; errors OK here

        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_ctx);
        ReleaseDC(dummy, dummy_dc);
        DestroyWindow(dummy);
    }

    // ------------------------------------------------------------------ 4.
    // Set pixel format on our real HDC.
    // Try ARB (MSAA) first, fall back to legacy.
    {
        bool fmt_set = false;

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
                WGL_SAMPLES_ARB,        4,
                0
            };
            int  chosen = 0;
            UINT num    = 0;
            if (wglChoosePixelFormatARB(g_wgl.hdc, attribs, NULL, 1, &chosen, &num) && num > 0) {
                PIXELFORMATDESCRIPTOR pfd = {};
                DescribePixelFormat(g_wgl.hdc, chosen, sizeof(pfd), &pfd);
                fmt_set = (SetPixelFormat(g_wgl.hdc, chosen, &pfd) == TRUE);
                SDL_Log("[WGL] ARB pixel format %d set: %s\n", chosen, fmt_set ? "OK" : "FAILED");
            }
        }

        if (!fmt_set) {
            PIXELFORMATDESCRIPTOR pfd = {};
            pfd.nSize        = sizeof(pfd);
            pfd.nVersion     = 1;
            pfd.dwFlags      = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType   = PFD_TYPE_RGBA;
            pfd.cColorBits   = 32;
            pfd.cDepthBits   = 24;
            pfd.cStencilBits = 8;
            pfd.iLayerType   = PFD_MAIN_PLANE;
            int fmt = ChoosePixelFormat(g_wgl.hdc, &pfd);
            fmt_set = (SetPixelFormat(g_wgl.hdc, fmt, &pfd) == TRUE);
            SDL_Log("[WGL] Legacy pixel format %d set: %s\n", fmt, fmt_set ? "OK" : "FAILED");
        }

        if (!fmt_set) {
            SDL_Log("[WGL] Failed to set any pixel format\n");
            return false;
        }
    }

    // ------------------------------------------------------------------ 5.
    // Create WGL context. Try core 3.3 first, fall back to compatibility.
    if (wglewIsSupported("WGL_ARB_create_context")) {
        int ctx_attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        g_wgl.hglrc = wglCreateContextAttribsARB(g_wgl.hdc, NULL, ctx_attribs);
        if (g_wgl.hglrc)
            SDL_Log("[WGL] Created OpenGL 3.3 core profile context\n");
    }
    if (!g_wgl.hglrc) {
        g_wgl.hglrc = wglCreateContext(g_wgl.hdc);
        SDL_Log("[WGL] Using compatibility context\n");
    }
    if (!g_wgl.hglrc) {
        SDL_Log("[WGL] wglCreateContext failed: %lu\n", GetLastError());
        return false;
    }

    wglMakeCurrent(g_wgl.hdc, g_wgl.hglrc);

    // ------------------------------------------------------------------ 6.
    // Re-init GLEW with the real context active.
    glewExperimental = GL_TRUE;
    GLenum glew_err = glewInit();
    SDL_Log("[WGL] glewInit (real ctx): %s\n", glewGetErrorString(glew_err));

    // ------------------------------------------------------------------ 7.
    // Basic GL state — mirrors on_realize() in cometbuster_render_gl2.cpp.
    glClearColor(0.04f, 0.06f, 0.15f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_MULTISAMPLE);

    gl_init();

    g_wgl.initialized = true;
    SDL_Log("[WGL] Initialized OK — HWND=%p HDC=%p HGLRC=%p\n",
            (void*)g_wgl.hwnd, (void*)g_wgl.hdc, (void*)g_wgl.hglrc);
    return true;
}

void sdl_wgl_resize(GtkWidget *placeholder)
{
    if (!g_wgl.hwnd) return;

    GtkAllocation alloc;
    gtk_widget_get_allocation(placeholder, &alloc);

    int new_w = (alloc.width  < 1) ? 1 : alloc.width;
    int new_h = (alloc.height < 1) ? 1 : alloc.height;

    if (new_w == g_wgl.w && new_h == g_wgl.h) return;

    g_wgl.w = new_w;
    g_wgl.h = new_h;

    SetWindowPos(g_wgl.hwnd, HWND_TOP,
                 0, 0, g_wgl.w, g_wgl.h,
                 SWP_NOZORDER | SWP_NOACTIVATE);
}

void sdl_wgl_render_frame(Visualizer *vis)
{
    if (!g_wgl.initialized || !vis) return;
    if (g_wgl.w < 10 || g_wgl.h < 10) return;

    static int count = 0;
    if (count++ < 5) SDL_Log("[WGL] render_frame #%d w=%d h=%d hdc=%p hglrc=%p\n",
                              count, g_wgl.w, g_wgl.h, (void*)g_wgl.hdc, (void*)g_wgl.hglrc);

    wglMakeCurrent(g_wgl.hdc, g_wgl.hglrc);

    vis->width  = 1920;
    vis->height = 1080;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Letterbox viewport — identical logic to cometbuster_render_gl2.cpp
    double scale_x = (double)g_wgl.w / 1920.0;
    double scale_y = (double)g_wgl.h / 1080.0;
    double scale   = (scale_x < scale_y) ? scale_x : scale_y;

    int vp_w = (int)(1920.0 * scale);
    int vp_h = (int)(1080.0 * scale);
    int vp_x = 0;
    int vp_y = (g_wgl.h - vp_h) / 2;

    glViewport(vp_x, vp_y, vp_w, vp_h);

    draw_comet_buster_gl(vis, NULL);

    SwapBuffers(g_wgl.hdc);

    // GTK's GdkWindow compositor repaints on top of our child HWND after each
    // swap. Force our window back to the top of the Z-order every frame so it
    // always sits above whatever GTK just painted.
    SetWindowPos(g_wgl.hwnd, HWND_TOP, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void sdl_wgl_cleanup(void)
{
    if (g_wgl.hglrc) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(g_wgl.hglrc);
        g_wgl.hglrc = NULL;
    }
    // CS_OWNDC: ReleaseDC is a no-op but call it for correctness
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

// ============================================================================
// GTK SIGNAL CALLBACKS
// ============================================================================

void on_sdl_placeholder_realize(GtkWidget *widget, gpointer user_data)
{
    (void)user_data;
    gdk_window_ensure_native(gtk_widget_get_window(widget));
    sdl_wgl_init(widget);
}

void on_sdl_placeholder_size_allocate(GtkWidget *widget,
                                       GdkRectangle *allocation,
                                       gpointer user_data)
{
    (void)allocation;
    (void)user_data;
    sdl_wgl_resize(widget);
}

// Returns TRUE to suppress GTK's white background fill, which would otherwise
// paint over our WGL child window on every GTK draw cycle.
gboolean on_sdl_placeholder_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    (void)widget;
    (void)cr;
    (void)user_data;
    static int count = 0;
    if (count++ < 5) SDL_Log("[WGL] on_sdl_placeholder_draw called (suppressing GTK paint) #%d\n", count);
    return TRUE;
}

#endif  // _WIN32
