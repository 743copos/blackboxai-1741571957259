#include "../include/UIManager.hpp"
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xft/Xft.h>
#include <iostream>
#include <algorithm>

UIManager::UIManager(std::shared_ptr<LoginManager> loginManager)
    : m_loginManager(loginManager)
    , m_display(nullptr)
    , m_window(0)
    , m_gc(0)
    , m_screen(0)
    , m_width(800)
    , m_height(600)
    , m_isLoading(false)
    , m_shouldExit(false)
    , m_usernameActive(true)
    , m_passwordActive(false) {
}

UIManager::~UIManager() {
    cleanupX11();
}

bool UIManager::initialize() {
    if (!setupX11()) {
        std::cerr << "Failed to initialize X11" << std::endl;
        return false;
    }
    
    createWindow();
    setupColors();
    setupFonts();
    
    return true;
}

bool UIManager::setupX11() {
    m_display = XOpenDisplay(nullptr);
    if (!m_display) {
        return false;
    }

    m_screen = DefaultScreen(m_display);
    m_background = BlackPixel(m_display, m_screen);
    m_foreground = WhitePixel(m_display, m_screen);
    m_highlight = 0x2980b9;  // Nice blue color
    m_error = 0xe74c3c;      // Error red color

    return true;
}

void UIManager::createWindow() {
    // Get screen dimensions
    Screen* screen = DefaultScreenOfDisplay(m_display);
    m_width = screen->width;
    m_height = screen->height;

    // Create main window
    m_window = XCreateSimpleWindow(
        m_display,
        RootWindow(m_display, m_screen),
        0, 0,
        m_width, m_height,
        0,
        m_foreground,
        m_background
    );

    // Set window properties
    XSetWindowAttributes attributes;
    attributes.override_redirect = True;  // Bypass window manager
    XChangeWindowAttributes(m_display, m_window, CWOverrideRedirect, &attributes);

    // Select input events
    XSelectInput(m_display, m_window,
                ExposureMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask);

    // Create GC
    m_gc = XCreateGC(m_display, m_window, 0, nullptr);
    XSetForeground(m_display, m_gc, m_foreground);

    // Show window
    XMapWindow(m_display, m_window);
    XRaiseWindow(m_display, m_window);
}

void UIManager::run() {
    while (!m_shouldExit) {
        handleEvents();
        drawBackground();
        drawLoginBox();
        drawInputFields();
        drawButtons();
        drawSessionSelector();
        
        if (!m_errorMessage.empty()) {
            drawErrorMessage();
        }
        
        if (m_isLoading) {
            drawLoadingIndicator();
        }
        
        XFlush(m_display);
    }
}

void UIManager::handleEvents() {
    XEvent event;
    while (XPending(m_display)) {
        XNextEvent(m_display, &event);
        
        switch (event.type) {
            case Expose:
                // Redraw needed
                break;
                
            case KeyPress:
                handleKeyPress(event.xkey);
                break;
                
            case ButtonPress:
                handleButtonPress(event.xbutton);
                break;
        }
    }
}

void UIManager::handleKeyPress(XKeyEvent& event) {
    char buffer[32];
    KeySym keysym;
    XComposeStatus compose;
    int count = XLookupString(&event, buffer, sizeof(buffer), &keysym, &compose);

    if (keysym == XK_Tab) {
        // Switch between username and password fields
        m_usernameActive = !m_usernameActive;
        m_passwordActive = !m_passwordActive;
        return;
    }

    if (keysym == XK_Return || keysym == XK_KP_Enter) {
        // Attempt login
        if (!m_username.empty() && !m_password.empty() && m_loginCallback) {
            m_loginCallback(m_username, m_password);
        }
        return;
    }

    if (keysym == XK_BackSpace) {
        // Handle backspace
        if (m_usernameActive && !m_username.empty()) {
            m_username.pop_back();
        } else if (m_passwordActive && !m_password.empty()) {
            m_password.pop_back();
        }
        return;
    }

    if (count == 1 && isprint(buffer[0])) {
        // Add character to active field
        if (m_usernameActive) {
            m_username += buffer[0];
        } else if (m_passwordActive) {
            m_password += buffer[0];
        }
    }
}

void UIManager::handleButtonPress(XButtonEvent& event) {
    // Check if click is in username field
    if (isPointInRect(event.x, event.y, 300, 250, 200, 30)) {
        m_usernameActive = true;
        m_passwordActive = false;
        return;
    }

    // Check if click is in password field
    if (isPointInRect(event.x, event.y, 300, 300, 200, 30)) {
        m_usernameActive = false;
        m_passwordActive = true;
        return;
    }

    // Check if click is on login button
    if (isPointInRect(event.x, event.y, 350, 350, 100, 30)) {
        if (!m_username.empty() && !m_password.empty() && m_loginCallback) {
            m_loginCallback(m_username, m_password);
        }
        return;
    }
}

void UIManager::drawBackground() {
    XSetForeground(m_display, m_gc, m_background);
    XFillRectangle(m_display, m_window, m_gc, 0, 0, m_width, m_height);
}

void UIManager::drawLoginBox() {
    // Draw centered login box
    int boxWidth = 400;
    int boxHeight = 300;
    int x = (m_width - boxWidth) / 2;
    int y = (m_height - boxHeight) / 2;

    // Draw box background
    XSetForeground(m_display, m_gc, m_foreground);
    XDrawRectangle(m_display, m_window, m_gc, x, y, boxWidth, boxHeight);
}

void UIManager::drawInputFields() {
    // Username field
    XSetForeground(m_display, m_gc, m_foreground);
    XDrawRectangle(m_display, m_window, m_gc, 300, 250, 200, 30);
    XDrawString(m_display, m_window, m_gc, 305, 270, 
                m_username.c_str(), m_username.length());

    // Password field (show asterisks)
    XDrawRectangle(m_display, m_window, m_gc, 300, 300, 200, 30);
    std::string asterisks(m_password.length(), '*');
    XDrawString(m_display, m_window, m_gc, 305, 320,
                asterisks.c_str(), asterisks.length());

    // Draw labels
    XDrawString(m_display, m_window, m_gc, 220, 270, "Username:", 9);
    XDrawString(m_display, m_window, m_gc, 220, 320, "Password:", 9);
}

void UIManager::drawButtons() {
    // Login button
    XSetForeground(m_display, m_gc, m_highlight);
    XFillRectangle(m_display, m_window, m_gc, 350, 350, 100, 30);
    XSetForeground(m_display, m_gc, m_background);
    XDrawString(m_display, m_window, m_gc, 380, 370, "Login", 5);
}

void UIManager::drawSessionSelector() {
    // Draw session selector dropdown
    XSetForeground(m_display, m_gc, m_foreground);
    XDrawRectangle(m_display, m_window, m_gc, 300, 200, 200, 30);
    XDrawString(m_display, m_window, m_gc, 220, 220, "Session:", 8);
    XDrawString(m_display, m_window, m_gc, 305, 220,
                m_selectedSession.c_str(), m_selectedSession.length());
}

void UIManager::drawErrorMessage() {
    if (!m_errorMessage.empty()) {
        XSetForeground(m_display, m_gc, m_error);
        XDrawString(m_display, m_window, m_gc, 
                   (m_width - m_errorMessage.length() * 6) / 2,
                   m_height / 2 + 200,
                   m_errorMessage.c_str(), m_errorMessage.length());
    }
}

void UIManager::drawLoadingIndicator() {
    if (m_isLoading) {
        XSetForeground(m_display, m_gc, m_highlight);
        XFillArc(m_display, m_window, m_gc,
                 m_width / 2 - 15, m_height / 2 - 15,
                 30, 30, 0, 360 * 64);
    }
}

void UIManager::showError(const std::string& error) {
    m_errorMessage = error;
}

void UIManager::clearError() {
    m_errorMessage.clear();
}

void UIManager::setLoading(bool loading) {
    m_isLoading = loading;
}

bool UIManager::isPointInRect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && x <= rx + rw && y >= ry && y <= ry + rh;
}

void UIManager::setupColors() {
    // Additional color setup if needed
}

void UIManager::setupFonts() {
    // Font setup if needed
}

void UIManager::cleanupX11() {
    if (m_display) {
        if (m_gc) {
            XFreeGC(m_display, m_gc);
        }
        if (m_window) {
            XDestroyWindow(m_display, m_window);
        }
        XCloseDisplay(m_display);
    }
}
