// ============================================================
// IMPORTANT: Include order matters!
// GLEW must come BEFORE OpenGL headers
// Qt headers can come in any order
// ============================================================

// Qt5 Core includes - MUST come BEFORE GLEW!
// NOTE: Do NOT include QOpenGLWidget, QOpenGLContext, or QOpenGLFunctions here
//       They conflict with GLEW. We'll include them after GLEW in the classes.
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

// IMPORTANT: If visualization.h includes GTK headers, you'll get a compilation error:
// "gtk/gtk.h: No such file or directory"
// 
// SOLUTION: Edit visualization.h and comment out/remove GTK includes:
//   // #include <gtk/gtk.h>
//   // #include <gdk/gdk.h>
// 
// See FIX_VISUALIZATION_H.md for detailed instructions.
// GTK includes in visualization.h are only needed for the old GTK build,
// not for Qt5. The structs in visualization.h don't depend on GTK.
#include "visualization.h"

#include "audio_wad.h"

// ============================================================
// CAIRO WIDGET IMPLEMENTATION
// ============================================================

CairoWidget::CairoWidget(Visualizer *vis, QWidget *parent)
    : QWidget(parent), visualizer(vis) {
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
}

void CairoWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    if (!visualizer) return;

    // Create Cairo surface from widget
    int width = this->width();
    int height = this->height();
    
    if (width <= 0 || height <= 0) return;

    // Create image buffer
    QImage image(width, height, QImage::Format_ARGB32);
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

    // Update visualizer dimensions
    visualizer->width = width;
    visualizer->height = height;

    // Call the game rendering function
    // This would call your existing Cairo rendering functions
    // e.g., render_frame(visualizer, cr, width, height);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Draw the image to the widget
    QPainter painter(this);
    painter.drawImage(0, 0, image);
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
    
    visualizer->mouse_x = event->x();
    visualizer->mouse_y = event->y();
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
    
    visualizer->mouse_x = event->x();
    visualizer->mouse_y = event->y();
    visualizer->last_mouse_x = event->x();
    visualizer->last_mouse_y = event->y();
    visualizer->mouse_just_moved = true;
    visualizer->mouse_movement_timer = 2.0;  // Show for 2 seconds
}

void CairoWidget::wheelEvent(QWheelEvent *event) {
    if (!visualizer) return;
    
    if (event->angleDelta().y() > 0) {
        visualizer->scroll_direction = 1;  // Scroll up
    } else if (event->angleDelta().y() < 0) {
        visualizer->scroll_direction = -1;  // Scroll down
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (!visualizer) return;

    // Your OpenGL rendering code here
    // e.g., render_frame_gl(visualizer);
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
    
    visualizer->mouse_x = event->x();
    visualizer->mouse_y = event->y();
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
    
    visualizer->mouse_x = event->x();
    visualizer->mouse_y = event->y();
    visualizer->last_mouse_x = event->x();
    visualizer->last_mouse_y = event->y();
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
    
    // Initialize audio
    audio_init(&audio);
    
    // Load saved volumes
    SettingsManager::loadVolumes(musicVolume, sfxVolume);
    
    // Create UI
    createUI();
    
    // Setup game timer
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &CometBusterWindow::updateGame);
    gameTimer->start(16);  // ~60 FPS
    
    resize(1024, 768);
}

CometBusterWindow::~CometBusterWindow() {
    if (gameTimer) {
        gameTimer->stop();
    }
    audio_cleanup(&audio);
}

void CometBusterWindow::updateGame() {
    if (!gamePaused) {
        // Update game state
        // comet_buster_update(&visualizer.comet_buster);
        
        // Trigger render
        if (renderingEngine == 0) {
            if (cairoWidget) cairoWidget->update();
        } else {
            if (glWidget) glWidget->update();
        }
    }
}

void CometBusterWindow::onNewGameEasy() {
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, EASY);
    gamePaused = false;
    statusLabel->setText("Game Started - Easy");
}

void CometBusterWindow::onNewGameMedium() {
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, MEDIUM);
    gamePaused = false;
    statusLabel->setText("Game Started - Medium");
}

void CometBusterWindow::onNewGameHard() {
    comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, HARD);
    gamePaused = false;
    statusLabel->setText("Game Started - Hard");
}

void CometBusterWindow::onTogglePause() {
    gamePaused = !gamePaused;
    if (gamePaused) {
        audio_stop_music(&audio);
        statusLabel->setText("Game Paused");
        fprintf(stdout, "[*] Game Paused\n");
    } else {
        statusLabel->setText("Game Resumed");
        fprintf(stdout, "[*] Game Resumed\n");
    }
}

void CometBusterWindow::onToggleFullscreen() {
    if (isFullScreen()) {
        showNormal();
        statusLabel->setText("Windowed Mode");
    } else {
        showFullScreen();
        statusLabel->setText("Fullscreen Mode");
    }
}

void CometBusterWindow::onVolumeChanged(int value) {
    musicVolume = value;
    audio_set_music_volume(&audio, musicVolume);
    SettingsManager::saveVolumes(musicVolume, sfxVolume);
}

void CometBusterWindow::onSFXVolumeChanged(int value) {
    sfxVolume = value;
    audio_set_sfx_volume(&audio, sfxVolume);
    SettingsManager::saveVolumes(musicVolume, sfxVolume);
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
    
    QLabel *sfxLabel = new QLabel("SFX Volume:");
    QSlider *sfxSlider = new QSlider(Qt::Horizontal);
    sfxSlider->setMinimum(0);
    sfxSlider->setMaximum(128);
    sfxSlider->setValue(sfxVolume);
    
    layout->addWidget(musicLabel);
    layout->addWidget(musicSlider);
    layout->addWidget(sfxLabel);
    layout->addWidget(sfxSlider);
    
    QPushButton *okButton = new QPushButton("OK");
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(okButton);
    
    if (dialog.exec() == QDialog::Accepted) {
        musicVolume = musicSlider->value();
        sfxVolume = sfxSlider->value();
        audio_set_music_volume(&audio, musicVolume);
        audio_set_sfx_volume(&audio, sfxVolume);
        SettingsManager::saveVolumes(musicVolume, sfxVolume);
    }
}

void CometBusterWindow::onSwitchToCairo() {
    renderingEngine = 0;
    renderingStack->setCurrentWidget(cairoWidget);
    cairoWidget->setFocus();
    statusLabel->setText("Switched to Cairo Rendering");
}

void CometBusterWindow::onSwitchToOpenGL() {
    renderingEngine = 1;
    renderingStack->setCurrentWidget(glWidget);
    glWidget->setFocus();
    statusLabel->setText("Switched to OpenGL Rendering");
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

    // Create rendering stack
    renderingStack = new QStackedWidget();
    
    cairoWidget = new CairoWidget(&visualizer);
    glWidget = new GLWidget(&visualizer);
    
    renderingStack->addWidget(cairoWidget);
    renderingStack->addWidget(glWidget);
    renderingStack->setCurrentWidget(cairoWidget);  // Start with Cairo

    mainLayout->addWidget(renderingStack);

    // Status bar
    statusLabel = new QLabel("Ready");
    statusBar()->addWidget(statusLabel);
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
// MAIN APPLICATION
// ============================================================

int main(int argc, char *argv[]) {
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

    return app.exec();
}

// ============================================================
// DO NOT include the .moc file here!
// The Makefile compiles the moc file separately and links it.
// ============================================================
