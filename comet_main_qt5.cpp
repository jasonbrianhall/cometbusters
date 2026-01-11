// ============================================================
// IMPORTANT: Include order matters!
// GLEW must come BEFORE OpenGL headers
// Qt headers can come in any order
// ============================================================

// Qt5 Core includes - MUST come BEFORE GLEW!
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QDialog>
#include <QSlider>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QScreen>
#include <QGuiApplication>
#include <QSurfaceFormat>
#include <QDesktopWidget>
#include <QStackedWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QPainter>
#include <QImage>

// OpenGL/GLEW - Include after Qt Core but before QOpenGL* classes
#include <GL/glew.h>
#include <GL/gl.h>

// Now include QOpenGL* classes AFTER GLEW
#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOffscreenSurface>

// Other libraries
#include <cairo.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <string>

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <appmodel.h>
#else
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/types.h>
#endif

#include "cometbuster.h"
#include "comet_main_qt5.h"  // ← CRITICAL: Include the header with class declarations

#include "visualization.h"
#include "audio_wad.h"

// Forward declarations of game functions (implemented in other .cpp files)
extern void update_comet_buster(Visualizer *vis, double delta_time);
extern void draw_comet_buster(Visualizer *vis, cairo_t *cr);

// ============================================================
// CAIRO WIDGET IMPLEMENTATION
// ============================================================

CairoWidget::CairoWidget(Visualizer *vis, QWidget *parent)
    : QWidget(parent), visualizer(vis) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);  // Enable continuous mouse move events
}

void CairoWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    if (!visualizer) return;

    int widget_width = this->width();
    int widget_height = this->height();
    
    if (widget_width <= 0 || widget_height <= 0) return;

    // Game always renders at 1920x1080
    int game_width = 1920;
    int game_height = 1080;

    // Create image buffer at game resolution
    QImage image(game_width, game_height, QImage::Format_ARGB32);
    image.fill(Qt::black);

    // Create Cairo surface from QImage
    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        image.bits(),
        CAIRO_FORMAT_ARGB32,
        image.width(),
        image.height(),
        image.bytesPerLine()
    );

    cairo_t *cr = cairo_create(surface);

    // Always render at 1920x1080
    visualizer->width = game_width;
    visualizer->height = game_height;

    // Call the game rendering function
    draw_comet_buster(visualizer, cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Now scale the rendered image to fit the widget
    QPainter painter(this);
    
    // Calculate scale to fit widget while maintaining aspect ratio
    double scale_x = (double)widget_width / game_width;
    double scale_y = (double)widget_height / game_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Calculate destination size
    int dest_width = (int)(game_width * scale);
    int dest_height = (int)(game_height * scale);
    
    // Game fills widget from top-left (no centering)
    // Draw scaled image - use QRect for proper scaling
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRect(0, 0, dest_width, dest_height), image);
}

void CairoWidget::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    handleKeyEvent(event, true);
}

void CairoWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    handleKeyEvent(event, false);
}

void CairoWidget::mousePressEvent(QMouseEvent *event) {
    if (!visualizer) return;
    
    // Transform widget coordinates to game coordinates (1920x1080)
    int game_x, game_y;
    transformMouseCoordinates(event->x(), event->y(), game_x, game_y);
    
    visualizer->mouse_x = game_x;
    visualizer->mouse_y = game_y;
    visualizer->mouse_just_moved = true;

    if (event->button() == Qt::LeftButton) {
        visualizer->mouse_left_pressed = true;
    } else if (event->button() == Qt::RightButton) {
        visualizer->mouse_right_pressed = true;
    } else if (event->button() == Qt::MiddleButton) {
        visualizer->mouse_middle_pressed = true;
    }
}

void CairoWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (!visualizer) return;

    if (event->button() == Qt::LeftButton) {
        visualizer->mouse_left_pressed = false;
    } else if (event->button() == Qt::RightButton) {
        visualizer->mouse_right_pressed = false;
    } else if (event->button() == Qt::MiddleButton) {
        visualizer->mouse_middle_pressed = false;
    }
}

void CairoWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!visualizer) return;
    
    // Transform widget coordinates to game coordinates (1920x1080)
    int game_x, game_y;
    transformMouseCoordinates(event->x(), event->y(), game_x, game_y);
    
    visualizer->mouse_x = game_x;
    visualizer->mouse_y = game_y;
    visualizer->last_mouse_x = game_x;
    visualizer->last_mouse_y = game_y;
    visualizer->mouse_just_moved = true;
    visualizer->mouse_movement_timer = 2.0;
}

void CairoWidget::wheelEvent(QWheelEvent *event) {
    if (!visualizer) return;
    
    if (event->angleDelta().y() > 0) {
        visualizer->scroll_direction = 1;
    } else if (event->angleDelta().y() < 0) {
        visualizer->scroll_direction = -1;
    }
}

void CairoWidget::transformMouseCoordinates(int widget_x, int widget_y, int &game_x, int &game_y) {
    // Game always renders at 1920x1080
    int game_width = 1920;
    int game_height = 1080;
    
    int widget_width = this->width();
    int widget_height = this->height();
    
    // Calculate scale to fit game into widget
    // Game is on LEFT side, not centered
    double scale_x = (double)widget_width / game_width;
    double scale_y = (double)widget_height / game_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Transform widget coordinates directly to game coordinates
    // No offset needed - game fills from top-left
    if (scale > 0) {
        game_x = (int)(widget_x / scale);
        game_y = (int)(widget_y / scale);
        
        // Clamp to game bounds
        if (game_x < 0) game_x = 0;
        if (game_y < 0) game_y = 0;
        if (game_x >= game_width) game_x = game_width - 1;
        if (game_y >= game_height) game_y = game_height - 1;
    } else {
        game_x = 0;
        game_y = 0;
    }
}

void CairoWidget::handleKeyEvent(QKeyEvent *event, bool pressed) {
    if (!visualizer) return;

    int key = event->key();

    switch (key) {
        case Qt::Key_A:
        case Qt::Key_Left:
            visualizer->key_a_pressed = pressed;
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            visualizer->key_d_pressed = pressed;
            break;
        case Qt::Key_W:
        case Qt::Key_Up:
            visualizer->key_w_pressed = pressed;
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            visualizer->key_s_pressed = pressed;
            break;
        case Qt::Key_Z:
            visualizer->key_z_pressed = pressed;
            break;
        case Qt::Key_X:
            visualizer->key_x_pressed = pressed;
            break;
        case Qt::Key_Space:
            visualizer->key_space_pressed = pressed;
            break;
        case Qt::Key_Control:
            visualizer->key_ctrl_pressed = pressed;
            break;
        case Qt::Key_Q:
            visualizer->key_q_pressed = pressed;
            break;
    }
}

// ============================================================
// OPENGL WIDGET IMPLEMENTATION
// ============================================================

GLWidget::GLWidget(Visualizer *vis, QWidget *parent)
    : QOpenGLWidget(parent), visualizer(vis) {
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);  // Enable continuous mouse move events
    
    // Request OpenGL 3.3 core profile
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
}

void GLWidget::initializeGL() {
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "GLEW initialization failed: %s\n", glewGetErrorString(err));
        return;
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    printf("OpenGL Version: %s\n", glGetString(GL_VERSION));
    printf("GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
}

void GLWidget::resizeGL(int w, int h) {
    if (h == 0) h = 1;
    
    glViewport(0, 0, w, h);
    
    if (visualizer) {
        visualizer->width = w;
        visualizer->height = h;
    }
}

void GLWidget::paintGL() {
    // Clear the OpenGL buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!visualizer) return;

    // Game always renders at 1920x1080
    int game_width = 1920;
    int game_height = 1080;
    
    int widget_width = this->width();
    int widget_height = this->height();
    
    if (widget_width <= 0 || widget_height <= 0) return;

    // Create Cairo image at game resolution
    QImage image(game_width, game_height, QImage::Format_ARGB32);
    image.fill(Qt::black);

    cairo_surface_t *surface = cairo_image_surface_create_for_data(
        image.bits(),
        CAIRO_FORMAT_ARGB32,
        image.width(),
        image.height(),
        image.bytesPerLine()
    );

    cairo_t *cr = cairo_create(surface);
    visualizer->width = game_width;
    visualizer->height = game_height;
    
    // Render game with Cairo
    draw_comet_buster(visualizer, cr);
    
    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Use QPainter to draw the image on the OpenGL widget
    // This is the reliable way that works with modern OpenGL
    QPainter painter(this);
    
    // Calculate scale to fit widget while maintaining aspect ratio
    double scale_x = (double)widget_width / game_width;
    double scale_y = (double)widget_height / game_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Calculate destination size
    int dest_width = (int)(game_width * scale);
    int dest_height = (int)(game_height * scale);
    
    // Game fills widget from top-left (no centering)
    // Draw scaled image - QPainter handles OpenGL context automatically
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(QRect(0, 0, dest_width, dest_height), image);
    painter.end();
}

void GLWidget::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    handleKeyEvent(event, true);
}

void GLWidget::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) return;
    handleKeyEvent(event, false);
}

void GLWidget::mousePressEvent(QMouseEvent *event) {
    if (!visualizer) return;
    
    // Transform widget coordinates to game coordinates (1920x1080)
    int game_x, game_y;
    transformMouseCoordinates(event->x(), event->y(), game_x, game_y);
    
    visualizer->mouse_x = game_x;
    visualizer->mouse_y = game_y;
    visualizer->mouse_just_moved = true;

    if (event->button() == Qt::LeftButton) {
        visualizer->mouse_left_pressed = true;
    } else if (event->button() == Qt::RightButton) {
        visualizer->mouse_right_pressed = true;
    } else if (event->button() == Qt::MiddleButton) {
        visualizer->mouse_middle_pressed = true;
    }
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (!visualizer) return;

    if (event->button() == Qt::LeftButton) {
        visualizer->mouse_left_pressed = false;
    } else if (event->button() == Qt::RightButton) {
        visualizer->mouse_right_pressed = false;
    } else if (event->button() == Qt::MiddleButton) {
        visualizer->mouse_middle_pressed = false;
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!visualizer) return;
    
    // Transform widget coordinates to game coordinates (1920x1080)
    int game_x, game_y;
    transformMouseCoordinates(event->x(), event->y(), game_x, game_y);
    
    visualizer->mouse_x = game_x;
    visualizer->mouse_y = game_y;
    visualizer->last_mouse_x = game_x;
    visualizer->last_mouse_y = game_y;
    visualizer->mouse_just_moved = true;
    visualizer->mouse_movement_timer = 2.0;
}

void GLWidget::wheelEvent(QWheelEvent *event) {
    if (!visualizer) return;
    
    if (event->angleDelta().y() > 0) {
        visualizer->scroll_direction = 1;
    } else if (event->angleDelta().y() < 0) {
        visualizer->scroll_direction = -1;
    }
}

void GLWidget::handleKeyEvent(QKeyEvent *event, bool pressed) {
    if (!visualizer) return;

    int key = event->key();

    switch (key) {
        case Qt::Key_A:
        case Qt::Key_Left:
            visualizer->key_a_pressed = pressed;
            break;
        case Qt::Key_D:
        case Qt::Key_Right:
            visualizer->key_d_pressed = pressed;
            break;
        case Qt::Key_W:
        case Qt::Key_Up:
            visualizer->key_w_pressed = pressed;
            break;
        case Qt::Key_S:
        case Qt::Key_Down:
            visualizer->key_s_pressed = pressed;
            break;
        case Qt::Key_Z:
            visualizer->key_z_pressed = pressed;
            break;
        case Qt::Key_X:
            visualizer->key_x_pressed = pressed;
            break;
        case Qt::Key_Space:
            visualizer->key_space_pressed = pressed;
            break;
        case Qt::Key_Control:
            visualizer->key_ctrl_pressed = pressed;
            break;
        case Qt::Key_Q:
            visualizer->key_q_pressed = pressed;
            break;
    }
}

void GLWidget::transformMouseCoordinates(int widget_x, int widget_y, int &game_x, int &game_y) {
    // Game always renders at 1920x1080
    int game_width = 1920;
    int game_height = 1080;
    
    int widget_width = this->width();
    int widget_height = this->height();
    
    // Calculate scale (same as in paintGL)
    // Game is on LEFT side, not centered
    double scale_x = (double)widget_width / game_width;
    double scale_y = (double)widget_height / game_height;
    double scale = (scale_x < scale_y) ? scale_x : scale_y;
    
    // Transform widget coordinates directly to game coordinates
    // No offset needed - game fills from top-left
    if (scale > 0) {
        game_x = (int)(widget_x / scale);
        game_y = (int)(widget_y / scale);
        
        // Clamp to game bounds
        if (game_x < 0) game_x = 0;
        if (game_y < 0) game_y = 0;
        if (game_x >= game_width) game_x = game_width - 1;
        if (game_y >= game_height) game_y = game_height - 1;
    } else {
        game_x = 0;
        game_y = 0;
    }
}

// ============================================================
// SETTINGS MANAGER IMPLEMENTATION
// ============================================================

QString SettingsManager::getSettingsPath() {
    QString path;
#ifdef _WIN32
    path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
#else
    path = QDir::homePath() + "/.config/cometbuster";
#endif
    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return path;
}

QString SettingsManager::getHighScorePath() {
    return getSettingsPath() + "/highscores.dat";
}

bool SettingsManager::loadVolumes(int &musicVol, int &sfxVol) {
    QSettings settings("CometBuster", "CometBuster");
    musicVol = settings.value("musicVolume", 100).toInt();
    sfxVol = settings.value("sfxVolume", 100).toInt();
    return true;
}

bool SettingsManager::saveVolumes(int musicVol, int sfxVol) {
    QSettings settings("CometBuster", "CometBuster");
    settings.setValue("musicVolume", musicVol);
    settings.setValue("sfxVolume", sfxVol);
    settings.sync();
    return true;
}

// ============================================================
// COMET BUSTER WINDOW IMPLEMENTATION
// ============================================================

CometBusterWindow::CometBusterWindow(QWidget *parent)
    : QMainWindow(parent), 
      gameTimer(nullptr),
      renderingStack(nullptr),
      cairoWidget(nullptr),
      glWidget(nullptr),
      statusLabel(nullptr),
      musicVolume(100),
      sfxVolume(100),
      gamePaused(false),
      renderingEngine(0) {
    
    setWindowTitle("Comet Busters");
    setWindowIcon(QIcon("cometbuster.ico"));
    
    // Initialize audio system
    fprintf(stdout, "[INIT] Initializing audio system\n");
    memset(&audio, 0, sizeof(AudioManager));
    
    if (!audio_init(&audio)) {
        fprintf(stderr, "[AUDIO] Warning: Audio initialization failed\n");
    }
    
    // Load WAD file with sounds
    fprintf(stdout, "[INIT] Loading audio WAD file\n");
#ifdef _WIN32
    {
        std::string wadPath = getExecutableDir() + "\\cometbuster.wad";
        if (!audio_load_wad(&audio, wadPath.c_str())) {
            fprintf(stderr, "[AUDIO] Warning: Could not load cometbuster.wad\n");
        }
    }
#else
    if (!audio_load_wad(&audio, "cometbuster.wad")) {
        fprintf(stderr, "[AUDIO] Warning: Could not load cometbuster.wad\n");
    }
#endif
    
    // Load saved volumes and apply them
    SettingsManager::loadVolumes(musicVolume, sfxVolume);
    audio_set_music_volume(&audio, musicVolume);
    audio_set_sfx_volume(&audio, sfxVolume);
    
    // Pre-load background music tracks
#ifdef ExternalSound
    fprintf(stdout, "[INIT] Pre-loading music tracks\n");
    audio_play_music(&audio, "music/track1.mp3", false);
    audio_play_music(&audio, "music/track2.mp3", false);
    audio_play_music(&audio, "music/track3.mp3", false);
    audio_play_music(&audio, "music/track4.mp3", false);
    audio_play_music(&audio, "music/track5.mp3", false);
    audio_play_music(&audio, "music/track6.mp3", false);
    
    fprintf(stdout, "[INIT] Starting intro music\n");
    audio_play_intro_music(&audio, "music/intro.mp3");
#endif
    
    // Initialize game
    fprintf(stdout, "[INIT] Initializing game state\n");
    memset(&visualizer, 0, sizeof(Visualizer));
    visualizer.width = 1920;
    visualizer.height = 1080;
    
    // Initialize mouse coordinates to center of screen
    visualizer.mouse_x = visualizer.width / 2;
    visualizer.mouse_y = visualizer.height / 2;
    visualizer.last_mouse_x = visualizer.width / 2;
    visualizer.last_mouse_y = visualizer.height / 2;
    
    // Copy audio to visualizer
    visualizer.audio = audio;
    
    // Show splash screen
    fprintf(stdout, "[INIT] Showing splash screen\n");
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, true, EASY);
    
    // Create UI
    createUI();
    
    // Setup game timer - 60 FPS
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &CometBusterWindow::updateGame);
    gameTimer->start(16);  // ~60 FPS (16.67ms per frame)
    
    // Set window size and grab focus
    resize(1024, 768);
    
    // Grab keyboard focus
    if (cairoWidget) {
        cairoWidget->setFocus();
        cairoWidget->grabKeyboard();
    }
    
    fprintf(stdout, "[INIT] Initialization complete\n");
}

CometBusterWindow::~CometBusterWindow() {
    if (gameTimer) {
        gameTimer->stop();
    }
    audio_cleanup(&audio);
    fprintf(stdout, "[CLEANUP] Game shutdown complete\n");
}

void CometBusterWindow::updateGame() {
    static int frameCounter = 0;
    
    if (!gamePaused) {
        // Sync audio state to visualizer
        visualizer.audio = audio;
        
        // Update game state
        update_comet_buster(&visualizer, 1.0 / 60.0);
        
        // Reset scroll wheel input after processing
        visualizer.scroll_direction = 0;
        
        // Check if game ended and it's a high score
        if ((visualizer.comet_buster.game_over || visualizer.comet_buster.ship_lives <= 0) &&
            comet_buster_is_high_score(&visualizer.comet_buster, visualizer.comet_buster.score)) {
            gamePaused = true;
            // TODO: Show high score dialog
            fprintf(stdout, "[HIGH SCORE] New high score: %d\n", visualizer.comet_buster.score);
        }
        
        // Check if music finished and queue next
#ifdef ExternalSound
        if (!gamePaused && !audio_is_music_playing(&audio)) {
            fprintf(stdout, "[AUDIO] Queuing next music track\n");
            audio_play_random_music(&audio);
        }
#endif
        
        // Update status bar every 60 frames
        frameCounter++;
        if (frameCounter % 60 == 0) {
            QString status = QString("Score: %1 | Lives: %2 | Wave: %3 | Bombs: %4")
                .arg(visualizer.comet_buster.score)
                .arg(visualizer.comet_buster.ship_lives)
                .arg(visualizer.comet_buster.current_wave + 1)
                .arg(visualizer.comet_buster.bomb_ammo);
            statusLabel->setText(status);
        }
        
        // Trigger render
        if (renderingEngine == 0) {
            if (cairoWidget) cairoWidget->update();
        } else {
            if (glWidget) glWidget->update();
        }
    }
}

void CometBusterWindow::onNewGameEasy() {
    audio_stop_music(&audio);
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, EASY);
#ifdef ExternalSound
    audio_play_music(&audio, "music/intro.mp3", false);
#endif
    gamePaused = false;
    statusLabel->setText("Game Started - Easy");
    fprintf(stdout, "[GAME] New game started - EASY\n");
}

void CometBusterWindow::onNewGameMedium() {
    audio_stop_music(&audio);
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, MEDIUM);
#ifdef ExternalSound
    audio_play_music(&audio, "music/intro.mp3", false);
#endif
    gamePaused = false;
    statusLabel->setText("Game Started - Medium");
    fprintf(stdout, "[GAME] New game started - MEDIUM\n");
}

void CometBusterWindow::onNewGameHard() {
    audio_stop_music(&audio);
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, HARD);
#ifdef ExternalSound
    audio_play_music(&audio, "music/intro.mp3", false);
#endif
    gamePaused = false;
    statusLabel->setText("Game Started - Hard");
    fprintf(stdout, "[GAME] New game started - HARD\n");
}

void CometBusterWindow::onTogglePause() {
    gamePaused = !gamePaused;
    if (gamePaused) {
        audio_stop_music(&audio);
        statusLabel->setText("Game Paused");
        fprintf(stdout, "[GAME] Game Paused\n");
    } else {
        audio_play_random_music(&audio);
        statusLabel->setText("Game Resumed");
        fprintf(stdout, "[GAME] Game Resumed\n");
    }
}

void CometBusterWindow::onToggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
        statusLabel->setText("Windowed Mode");
        fprintf(stdout, "[DISPLAY] Switched to Windowed Mode\n");
    } else {
        showFullScreen();
        statusLabel->setText("Fullscreen Mode");
        fprintf(stdout, "[DISPLAY] Switched to Fullscreen\n");
    }
}

void CometBusterWindow::onVolumeChanged(int value) {
    musicVolume = value;
    audio_set_music_volume(&audio, musicVolume);
    SettingsManager::saveVolumes(musicVolume, sfxVolume);
    fprintf(stdout, "[AUDIO] Music volume set to %d\n", musicVolume);
}

void CometBusterWindow::onSFXVolumeChanged(int value) {
    sfxVolume = value;
    audio_set_sfx_volume(&audio, sfxVolume);
    SettingsManager::saveVolumes(musicVolume, sfxVolume);
    fprintf(stdout, "[AUDIO] SFX volume set to %d\n", sfxVolume);
}

void CometBusterWindow::onShowVolumeDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle("Volume Settings");
    
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    
    QLabel *musicLabel = new QLabel("Music Volume:");
    QSlider *musicSlider = new QSlider(Qt::Horizontal);
    musicSlider->setMinimum(0);
    musicSlider->setMaximum(128);
    musicSlider->setValue(musicVolume);
    connect(musicSlider, QOverload<int>::of(&QSlider::valueChanged), 
            this, &CometBusterWindow::onVolumeChanged);
    
    QLabel *sfxLabel = new QLabel("SFX Volume:");
    QSlider *sfxSlider = new QSlider(Qt::Horizontal);
    sfxSlider->setMinimum(0);
    sfxSlider->setMaximum(128);
    sfxSlider->setValue(sfxVolume);
    connect(sfxSlider, QOverload<int>::of(&QSlider::valueChanged), 
            this, &CometBusterWindow::onSFXVolumeChanged);
    
    layout->addWidget(musicLabel);
    layout->addWidget(musicSlider);
    layout->addWidget(sfxLabel);
    layout->addWidget(sfxSlider);
    
    QPushButton *okButton = new QPushButton("OK");
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(okButton);
    
    dialog.exec();
}

void CometBusterWindow::onSwitchToCairo() {
    renderingEngine = 0;
    renderingStack->setCurrentWidget(cairoWidget);
    cairoWidget->setFocus();
    cairoWidget->grabKeyboard();
    statusLabel->setText("Switched to Cairo Rendering");
    fprintf(stdout, "[RENDER] Switched to Cairo rendering\n");
}

void CometBusterWindow::onSwitchToOpenGL() {
    renderingEngine = 1;
    renderingStack->setCurrentWidget(glWidget);
    glWidget->setFocus();
    glWidget->grabKeyboard();
    statusLabel->setText("Switched to OpenGL Rendering");
    fprintf(stdout, "[RENDER] Switched to OpenGL rendering\n");
}

void CometBusterWindow::onAbout() {
    QMessageBox::information(this, "About Comet Busters",
        "Comet Busters - An arcade-style space shooter\n\n"
        "Controls:\n"
        "A/D or Arrow Keys - Turn Left/Right\n"
        "W/S or Up/Down - Thrust Forward/Backward\n"
        "Z - Fire\n"
        "X - Boost\n"
        "Space - Special Weapon\n"
        "P or Escape - Pause\n"
        "F11 - Fullscreen\n"
        "V - Volume Control\n\n"
        "Press Escape to return to menu");
}

void CometBusterWindow::createUI() {
    // Central widget
    QWidget *centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // Create menu bar
    createMenuBar();

    // Create horizontal layout for game and content
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Create rendering stack on LEFT side
    renderingStack = new QStackedWidget();
    renderingStack->setMinimumWidth(1200);  // Game takes up more space on left
    
    cairoWidget = new CairoWidget(&visualizer);
    glWidget = new GLWidget(&visualizer);
    
    renderingStack->addWidget(cairoWidget);
    renderingStack->addWidget(glWidget);
    renderingStack->setCurrentWidget(cairoWidget);  // Start with Cairo

    contentLayout->addWidget(renderingStack);

    // Add right side panel for future content/narrative
    QWidget *rightPanel = new QWidget();
    rightPanel->setStyleSheet("background-color: black;");
    rightPanel->setMinimumWidth(300);
    contentLayout->addWidget(rightPanel);

    // Add content layout to main layout
    mainLayout->addLayout(contentLayout);

    // Status bar
    statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);
    
    fprintf(stdout, "[UI] UI created\n");
}

void CometBusterWindow::createMenuBar() {
    // Game Menu
    QMenu *gameMenu = menuBar()->addMenu(tr("&Game"));
    
    QMenu *newGameMenu = gameMenu->addMenu(tr("&New Game"));
    QAction *newGameEasyAction = newGameMenu->addAction(tr("&Easy"));
    connect(newGameEasyAction, &QAction::triggered, this, &CometBusterWindow::onNewGameEasy);
    
    QAction *newGameMediumAction = newGameMenu->addAction(tr("&Medium"));
    connect(newGameMediumAction, &QAction::triggered, this, &CometBusterWindow::onNewGameMedium);
    
    QAction *newGameHardAction = newGameMenu->addAction(tr("&Hard"));
    connect(newGameHardAction, &QAction::triggered, this, &CometBusterWindow::onNewGameHard);
    
    gameMenu->addSeparator();
    
    QAction *pauseAction = gameMenu->addAction(tr("&Pause (P)"));
    connect(pauseAction, &QAction::triggered, this, &CometBusterWindow::onTogglePause);
    
    gameMenu->addSeparator();
    
    QAction *quitAction = gameMenu->addAction(tr("&Quit"));
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    // View Menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    
    QAction *fullscreenAction = viewMenu->addAction(tr("&Fullscreen (F11)"));
    connect(fullscreenAction, &QAction::triggered, this, &CometBusterWindow::onToggleFullscreen);
    
    QAction *cairoAction = viewMenu->addAction(tr("&Cairo Rendering"));
    connect(cairoAction, &QAction::triggered, this, &CometBusterWindow::onSwitchToCairo);
    
    QAction *openglAction = viewMenu->addAction(tr("&OpenGL Rendering"));
    connect(openglAction, &QAction::triggered, this, &CometBusterWindow::onSwitchToOpenGL);

    // Settings Menu
    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    
    QAction *volumeAction = settingsMenu->addAction(tr("&Volume (V)"));
    connect(volumeAction, &QAction::triggered, this, &CometBusterWindow::onShowVolumeDialog);

    // Help Menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    
    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    connect(aboutAction, &QAction::triggered, this, &CometBusterWindow::onAbout);
    
    fprintf(stdout, "[UI] Menu bar created\n");
}

std::string CometBusterWindow::getExecutableDir() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string path(buffer);
    size_t pos = path.find_last_of("\\/");
    return (pos == std::string::npos) ? "." : path.substr(0, pos);
#else
    return ".";
#endif
}

void CometBusterWindow::loadHighScores(CometBusterGame *game) {
    if (!game) return;
    // TODO: Implement high score loading from disk
    game->high_score_count = 0;
}

void CometBusterWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_F11) {
        onToggleFullscreen();
        return;
    }
    if (event->key() == Qt::Key_P || event->key() == Qt::Key_Escape) {
        onTogglePause();
        return;
    }
    if (event->key() == Qt::Key_V) {
        onShowVolumeDialog();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

void CometBusterWindow::closeEvent(QCloseEvent *event) {
    if (gameTimer) {
        gameTimer->stop();
    }
    QMainWindow::closeEvent(event);
}

// ============================================================
// HIGH SCORE MANAGEMENT
// ============================================================

bool comet_buster_is_high_score(CometBusterGame *game, int score) {
    if (!game) return false;
    
    printf("IS HIGH SCORE CALLED\n");
    
    // If we haven't filled the high score list yet, any score is a high score
    if (game->high_score_count < MAX_HIGH_SCORES) {
        printf("List not full yet (%d/%d), score %d qualifies\n", 
               game->high_score_count, MAX_HIGH_SCORES, score);
        return true;
    }
    
    // List is full - check if score beats the lowest (last) score
    if (score > game->high_scores[MAX_HIGH_SCORES - 1].score) {
        printf("Is a High Score: %d (beats lowest of %d)\n", 
               score, game->high_scores[MAX_HIGH_SCORES - 1].score);
        return true;
    }
    
    printf("Not a high score: %d\n", score);
    return false;
}

// ============================================================
// MAIN APPLICATION
// ============================================================

int main(int argc, char *argv[]) {
    fprintf(stdout, "[MAIN] Comet Busters Qt5 Starting...\n");
    
    QApplication app(argc, argv);

    // Configure application
    QApplication::setApplicationName("Comet Busters");
    QApplication::setApplicationVersion("1.0.0");
    
    // Set up QSettings paths
    QApplication::setApplicationName("CometBuster");
    QApplication::setOrganizationName("CometBuster");

    // Create and show main window
    CometBusterWindow window;
    window.show();

    fprintf(stdout, "[MAIN] Entering Qt event loop\n");
    int result = app.exec();
    fprintf(stdout, "[MAIN] Qt event loop ended, shutting down\n");
    
    return result;
}
