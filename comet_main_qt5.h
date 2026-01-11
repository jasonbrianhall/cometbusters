#ifndef COMET_MAIN_QT5_H
#define COMET_MAIN_QT5_H

#include <QWidget>
#include <QOpenGLWidget>
#include <QLabel>
#include <QTimer>
#include <QStackedWidget>
#include <QMainWindow>
#include <QStatusBar>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPaintEvent>

#include "cometbuster.h"
#include "visualization.h"
#include "audio_wad.h"

/**
 * Cairo Rendering Widget
 * Uses Qt's raster backend to render Cairo graphics
 */
class CairoWidget : public QWidget {
    Q_OBJECT
    
public:
    explicit CairoWidget(Visualizer *vis, QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Visualizer *visualizer;
    
    /**
     * Handle key events for the visualizer
     */
    void handleKeyEvent(QKeyEvent *event, bool pressed);
};

/**
 * OpenGL Rendering Widget
 * Uses Qt's QOpenGLWidget for hardware-accelerated OpenGL rendering
 * This is the recommended choice for best performance and stability
 */
class GLWidget : public QOpenGLWidget {
    Q_OBJECT
    
public:
    explicit GLWidget(Visualizer *vis, QWidget *parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    Visualizer *visualizer;
    
    /**
     * Handle key events for the visualizer
     */
    void handleKeyEvent(QKeyEvent *event, bool pressed);
};

/**
 * Main Application Window
 * Manages the game state, UI, and game loop
 */
class CometBusterWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit CometBusterWindow(QWidget *parent = nullptr);
    virtual ~CometBusterWindow();

private slots:
    /**
     * Game update loop - called ~60 times per second
     */
    void updateGame();
    
    // Game menu actions
    void onNewGameEasy();
    void onNewGameMedium();
    void onNewGameHard();
    void onTogglePause();
    void onToggleFullscreen();
    
    // Settings
    void onVolumeChanged(int value);
    void onSFXVolumeChanged(int value);
    void onShowVolumeDialog();
    
    // Rendering engine selection
    void onSwitchToCairo();
    void onSwitchToOpenGL();
    
    // Help
    void onAbout();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    /**
     * Initialize UI (menu bar, rendering widgets, etc.)
     */
    void createUI();
    
    /**
     * Create menu bar with all menus and actions
     */
    void createMenuBar();
    
    /**
     * Get the directory where the executable is located
     * Used for finding resource files like .wad audio files
     */
    std::string getExecutableDir();
    
    /**
     * Load high scores from disk
     */
    void loadHighScores(CometBusterGame *game);
    
    // UI Components
    QTimer *gameTimer;                          // Game update timer (~60 FPS)
    QStackedWidget *renderingStack;             // Switches between Cairo/OpenGL
    CairoWidget *cairoWidget;                   // Cairo rendering surface
    GLWidget *glWidget;                         // OpenGL rendering surface
    QLabel *statusLabel;                        // Status bar label
    
    // Game State
    Visualizer visualizer;                      // Game visualization state
    AudioManager audio;                         // Audio system
    
    // Settings
    int musicVolume;                            // Current music volume (0-128)
    int sfxVolume;                              // Current SFX volume (0-128)
    bool gamePaused;                            // Is game paused?
    int renderingEngine;                        // 0 = Cairo, 1 = OpenGL
};

/**
 * Settings Management Utility
 * Handles persistence of game settings and high scores
 */
class SettingsManager {
public:
    /**
     * Get the path to the settings directory
     * Platform-independent - returns correct path for Windows/Linux/macOS
     */
    static QString getSettingsPath();
    
    /**
     * Load saved volume settings
     * @return true if settings were loaded, false if using defaults
     */
    static bool loadVolumes(int &musicVol, int &sfxVol);
    
    /**
     * Save volume settings to disk
     * @return true on success
     */
    static bool saveVolumes(int musicVol, int sfxVol);
    
    /**
     * Get the path to high scores file
     */
    static QString getHighScorePath();
};

#endif // COMET_MAIN_QT5_H
