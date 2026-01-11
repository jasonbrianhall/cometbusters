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
// CUSTOM RENDERING WIDGETS
// ============================================================

/**
 * Cairo rendering widget using Qt's raster backend
 */
class CairoWidget : public QWidget {
    Q_OBJECT
public:
    CairoWidget(Visualizer *vis, QWidget *parent = nullptr)
        : QWidget(parent), visualizer(vis) {
        setFocusPolicy(Qt::StrongFocus);
        setAttribute(Qt::WA_OpaquePaintEvent);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
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

    void keyPressEvent(QKeyEvent *event) override {
        if (event->isAutoRepeat()) return;
        
        handleKeyEvent(event, true);
    }

    void keyReleaseEvent(QKeyEvent *event) override {
        if (event->isAutoRepeat()) return;
        
        handleKeyEvent(event, false);
    }

    void mousePressEvent(QMouseEvent *event) override {
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

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (!visualizer) return;

        if (event->button() == Qt::LeftButton) {
            visualizer->mouse_left_pressed = false;
        } else if (event->button() == Qt::RightButton) {
            visualizer->mouse_right_pressed = false;
        } else if (event->button() == Qt::MiddleButton) {
            visualizer->mouse_middle_pressed = false;
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!visualizer) return;
        
        visualizer->mouse_x = event->x();
        visualizer->mouse_y = event->y();
        visualizer->last_mouse_x = event->x();
        visualizer->last_mouse_y = event->y();
        visualizer->mouse_just_moved = true;
        visualizer->mouse_movement_timer = 2.0;  // Show for 2 seconds
    }

    void wheelEvent(QWheelEvent *event) override {
        if (!visualizer) return;
        
        if (event->angleDelta().y() > 0) {
            visualizer->scroll_direction = 1;  // Scroll up
        } else if (event->angleDelta().y() < 0) {
            visualizer->scroll_direction = -1;  // Scroll down
        }
    }

private:
    Visualizer *visualizer;

    void handleKeyEvent(QKeyEvent *event, bool pressed) {
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
};

/**
 * OpenGL rendering widget for hardware-accelerated rendering
 */
class GLWidget : public QOpenGLWidget {
    Q_OBJECT
public:
    GLWidget(Visualizer *vis, QWidget *parent = nullptr)
        : QOpenGLWidget(parent), visualizer(vis) {
        setFocusPolicy(Qt::StrongFocus);
        
        // Configure surface format for OpenGL 3.3 Core
        QSurfaceFormat format;
        format.setVersion(3, 3);
        format.setProfile(QSurfaceFormat::CoreProfile);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        setFormat(format);
    }

protected:
    void initializeGL() override {
        // Initialize GLEW
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK) {
            fprintf(stderr, "[GL] GLEW initialization failed: %s\n", 
                    glewGetErrorString(err));
        } else {
            fprintf(stdout, "[GL] GLEW initialized successfully\n");
        }

        // Print OpenGL info
        fprintf(stdout, "[GL] OpenGL Version: %s\n", glGetString(GL_VERSION));
        fprintf(stdout, "[GL] GLSL Version: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
        fprintf(stdout, "[GL] Renderer: %s\n", glGetString(GL_RENDERER));
        fprintf(stdout, "[GL] Vendor: %s\n", glGetString(GL_VENDOR));

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void resizeGL(int w, int h) override {
        if (!visualizer) return;
        
        visualizer->width = w;
        visualizer->height = h;
        
        glViewport(0, 0, w, h);
    }

    void paintGL() override {
        if (!visualizer) return;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Call the game rendering function
        // e.g., render_frame_gl(visualizer, width(), height());
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (event->isAutoRepeat()) return;
        
        handleKeyEvent(event, true);
    }

    void keyReleaseEvent(QKeyEvent *event) override {
        if (event->isAutoRepeat()) return;
        
        handleKeyEvent(event, false);
    }

    void mousePressEvent(QMouseEvent *event) override {
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

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (!visualizer) return;

        if (event->button() == Qt::LeftButton) {
            visualizer->mouse_left_pressed = false;
        } else if (event->button() == Qt::RightButton) {
            visualizer->mouse_right_pressed = false;
        } else if (event->button() == Qt::MiddleButton) {
            visualizer->mouse_middle_pressed = false;
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!visualizer) return;
        
        visualizer->mouse_x = event->x();
        visualizer->mouse_y = event->y();
        visualizer->last_mouse_x = event->x();
        visualizer->last_mouse_y = event->y();
        visualizer->mouse_just_moved = true;
        visualizer->mouse_movement_timer = 2.0;
    }

    void wheelEvent(QWheelEvent *event) override {
        if (!visualizer) return;
        
        if (event->angleDelta().y() > 0) {
            visualizer->scroll_direction = 1;
        } else if (event->angleDelta().y() < 0) {
            visualizer->scroll_direction = -1;
        }
    }

private:
    Visualizer *visualizer;

    void handleKeyEvent(QKeyEvent *event, bool pressed) {
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
};

// ============================================================
// SETTINGS MANAGEMENT
// ============================================================

class SettingsManager {
public:
    static QString getSettingsPath() {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        return appDataPath + "/settings.ini";
    }

    static bool loadVolumes(int &musicVol, int &sfxVol) {
        QSettings settings(getSettingsPath(), QSettings::IniFormat);
        
        if (!settings.contains("music_volume")) {
            return false;
        }

        musicVol = settings.value("music_volume", 100).toInt();
        sfxVol = settings.value("sfx_volume", 100).toInt();
        
        musicVol = qBound(0, musicVol, 128);
        sfxVol = qBound(0, sfxVol, 128);
        
        return true;
    }

    static bool saveVolumes(int musicVol, int sfxVol) {
        QSettings settings(getSettingsPath(), QSettings::IniFormat);
        settings.setValue("music_volume", musicVol);
        settings.setValue("sfx_volume", sfxVol);
        settings.sync();
        return true;
    }

    static QString getHighScorePath() {
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir dir(appDataPath);
        if (!dir.exists()) {
            dir.mkpath(".");
        }
        return appDataPath + "/highscores.txt";
    }
};

// ============================================================
// MAIN APPLICATION WINDOW
// ============================================================

class CometBusterWindow : public QMainWindow {
    Q_OBJECT

public:
    CometBusterWindow(QWidget *parent = nullptr)
        : QMainWindow(parent), 
          musicVolume(100), sfxVolume(100),
          gamePaused(false), renderingEngine(0) {
        
        setWindowTitle("Comet Busters");
        setWindowIcon(QIcon());  // TODO: Add your icon here

        // Initialize game state
        memset(&visualizer, 0, sizeof(Visualizer));
        memset(&audio, 0, sizeof(AudioManager));

        // Load saved volumes
        if (!SettingsManager::loadVolumes(musicVolume, sfxVolume)) {
            musicVolume = 100;
            sfxVolume = 100;
        }

        // Initialize visualizer
        visualizer.width = 800;
        visualizer.height = 600;
        visualizer.volume_level = 0.5;
        visualizer.mouse_x = 400;
        visualizer.mouse_y = 300;
        visualizer.last_mouse_x = 400;
        visualizer.last_mouse_y = 300;
        visualizer.mouse_movement_timer = 0.0;
        visualizer.mouse_just_moved = false;
        visualizer.scroll_direction = 0;

        // Initialize joystick
        joystick_manager_init(&visualizer.joystick_manager);
        int num_joysticks = joystick_manager_detect(&visualizer.joystick_manager);
        fprintf(stdout, "[INIT] Found %d joystick(s)\n", num_joysticks);

        // Initialize game
        comet_buster_reset_game_with_splash(&visualizer.comet_buster, true, 1);
        high_scores_load(&visualizer.comet_buster);

        // Initialize audio
        if (!audio_init(&audio)) {
            fprintf(stderr, "Warning: Audio initialization failed\n");
        }

        // Load WAD file
#ifdef _WIN32
        std::string wadPath = getExecutableDir() + "\\cometbuster.wad";
        if (!audio_load_wad(&audio, wadPath.c_str())) {
            fprintf(stderr, "Warning: Could not load cometbuster.wad\n");
        }
#else
        if (!audio_load_wad(&audio, "cometbuster.wad")) {
            fprintf(stderr, "Warning: Could not load cometbuster.wad\n");
        }
#endif

        visualizer.audio = audio;

        audio_set_music_volume(&audio, musicVolume);
        audio_set_sfx_volume(&audio, sfxVolume);

        // Create UI
        createUI();

        // Set up game loop timer
        gameTimer = new QTimer(this);
        connect(gameTimer, &QTimer::timeout, this, &CometBusterWindow::updateGame);
        gameTimer->start(17);  // ~60 FPS

        // Set initial window size
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->geometry();
        int w = (int)(screenGeometry.width() * 0.9);
        int h = (int)(screenGeometry.height() * 0.9);
        resize(w, h);
        showMaximized();
    }

    ~CometBusterWindow() {
        if (gameTimer) {
            gameTimer->stop();
        }
        gameTimer = nullptr;

        comet_buster_cleanup(&visualizer.comet_buster);
        joystick_manager_cleanup(&visualizer.joystick_manager);
        audio_cleanup(&audio);
    }

private slots:
    void updateGame() {
        if (!gamePaused) {
            // Update game logic here
            // update_game_frame(&visualizer, 0.017);  // 60 FPS
        }

        // Request redraw
        if (renderingEngine == 0) {
            cairoWidget->update();
        } else {
            glWidget->update();
        }
    }

    void onNewGameEasy() {
        comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, EASY);
        gamePaused = false;
    }

    void onNewGameMedium() {
        comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, MEDIUM);
        gamePaused = false;
    }

    void onNewGameHard() {
        comet_buster_reset_game_with_splash(&visualizer.comet_buster, false, HARD);
        gamePaused = false;
    }

    void onTogglePause() {
        gamePaused = !gamePaused;
        if (gamePaused) {
            audio_stop_music(&audio);
            fprintf(stdout, "[*] Game Paused\n");
        } else {
            fprintf(stdout, "[*] Game Resumed\n");
        }
    }

    void onToggleFullscreen() {
        if (isFullScreen()) {
            showNormal();
        } else {
            showFullScreen();
        }
    }

    void onVolumeChanged(int value) {
        musicVolume = value;
        audio_set_music_volume(&audio, musicVolume);
        SettingsManager::saveVolumes(musicVolume, sfxVolume);
    }

    void onSFXVolumeChanged(int value) {
        sfxVolume = value;
        audio_set_sfx_volume(&audio, sfxVolume);
        SettingsManager::saveVolumes(musicVolume, sfxVolume);
    }

    void onSwitchToCairo() {
        renderingEngine = 0;
        renderingStack->setCurrentWidget(cairoWidget);
        cairoWidget->setFocus();
    }

    void onSwitchToOpenGL() {
        renderingEngine = 1;
        renderingStack->setCurrentWidget(glWidget);
        glWidget->setFocus();
    }

    void onAbout() {
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

    void onShowVolumeDialog() {
        // Create a simple volume dialog
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

protected:
    void keyPressEvent(QKeyEvent *event) override {
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

    void closeEvent(QCloseEvent *event) override {
        gameTimer->stop();
        QMainWindow::closeEvent(event);
    }

private:
    void createUI() {
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

    void createMenuBar() {
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

    std::string getExecutableDir() {
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

    void high_scores_load(CometBusterGame *game) {
        // TODO: Implement high score loading from disk
        // This mirrors your GTK implementation
        if (!game) return;
        game->high_score_count = 0;
    }

private:
    QTimer *gameTimer;
    QStackedWidget *renderingStack;
    CairoWidget *cairoWidget;
    GLWidget *glWidget;
    QLabel *statusLabel;

    Visualizer visualizer;
    AudioManager audio;
    int musicVolume;
    int sfxVolume;
    bool gamePaused;
    int renderingEngine;  // 0 = Cairo, 1 = OpenGL
};

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

// MOC compilation is handled by Makefile.qt5 - no need to include .moc file here
// The MOC-generated code is compiled separately and linked in
