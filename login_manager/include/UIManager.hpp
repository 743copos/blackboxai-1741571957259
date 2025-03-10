#pragma once

#include <string>
#include <memory>
#include <functional>
#include <X11/Xlib.h>
#include "LoginManager.hpp"

class UIManager {
public:
    UIManager(std::shared_ptr<LoginManager> loginManager);
    ~UIManager();

    // Initialize the UI
    bool initialize();

    // Main UI loop
    void run();

    // UI event callbacks
    using LoginCallback = std::function<void(const std::string&, const std::string&)>;
    using SessionSelectCallback = std::function<void(const std::string&)>;

    void setLoginCallback(LoginCallback callback) { m_loginCallback = callback; }
    void setSessionSelectCallback(SessionSelectCallback callback) { m_sessionSelectCallback = callback; }

    // UI state management
    void showError(const std::string& error);
    void clearError();
    void setLoading(bool loading);

private:
    // X11 setup and cleanup
    bool setupX11();
    void cleanupX11();

    // Event handling
    void handleEvents();
    void handleKeyPress(XKeyEvent& event);
    void handleButtonPress(XButtonEvent& event);

    // Drawing functions
    void drawBackground();
    void drawLoginBox();
    void drawInputFields();
    void drawButtons();
    void drawSessionSelector();
    void drawErrorMessage();
    void drawLoadingIndicator();

    // Helper functions
    void createWindow();
    void setupColors();
    void setupFonts();
    bool isPointInRect(int x, int y, int rx, int ry, int rw, int rh);

    // X11 variables
    Display* m_display;
    Window m_window;
    GC m_gc;
    int m_screen;
    unsigned long m_background;
    unsigned long m_foreground;
    unsigned long m_highlight;
    unsigned long m_error;

    // UI state
    std::string m_username;
    std::string m_password;
    std::string m_selectedSession;
    std::string m_errorMessage;
    bool m_isLoading;
    bool m_shouldExit;

    // Input field states
    bool m_usernameActive;
    bool m_passwordActive;

    // Callbacks
    LoginCallback m_loginCallback;
    SessionSelectCallback m_sessionSelectCallback;

    // Reference to login manager
    std::shared_ptr<LoginManager> m_loginManager;

    // Window dimensions
    int m_width;
    int m_height;
};
