#include "../include/LoginManager.hpp"
#include <iostream>
#include <fstream>
#include <cstring>
#include <filesystem>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>

LoginManager::LoginManager()
    : m_pamHandle(nullptr)
    , m_isAuthenticated(false)
    , m_configPath("/etc/login_manager/login.conf")
    , m_logPath("/var/log/login_manager.log") {
}

LoginManager::~LoginManager() {
    cleanup();
}

bool LoginManager::initialize() {
    try {
        if (!initializePAM()) {
            m_lastError = "Failed to initialize PAM";
            return false;
        }
        
        // TODO: Load configuration from m_configPath
        // TODO: Initialize logging system
        
        return true;
    } catch (const std::exception& e) {
        m_lastError = std::string("Initialization error: ") + e.what();
        return false;
    }
}

bool LoginManager::authenticate(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        m_lastError = "Username or password cannot be empty";
        return false;
    }

    // Store credentials temporarily for PAM conversation
    struct pam_conv conv = {
        LoginManager::pamConversation,
        const_cast<void*>(reinterpret_cast<const void*>(&password))
    };

    // Start PAM transaction
    int ret = pam_start("login", username.c_str(), &conv, &m_pamHandle);
    if (ret != PAM_SUCCESS) {
        m_lastError = "Failed to start PAM session";
        return false;
    }

    // Authenticate user
    ret = pam_authenticate(m_pamHandle, 0);
    if (ret != PAM_SUCCESS) {
        m_lastError = "Authentication failed";
        logAttempt(username, false);
        return false;
    }

    // Check account validity
    ret = pam_acct_mgmt(m_pamHandle, 0);
    if (ret != PAM_SUCCESS) {
        m_lastError = "Account is invalid or expired";
        logAttempt(username, false);
        return false;
    }

    m_currentUser = username;
    m_isAuthenticated = true;
    logAttempt(username, true);
    return true;
}

bool LoginManager::launchSession(const std::string& sessionType) {
    if (!m_isAuthenticated) {
        m_lastError = "User not authenticated";
        return false;
    }

    // Get user info
    struct passwd* pw = getpwnam(m_currentUser.c_str());
    if (!pw) {
        m_lastError = "Failed to get user information";
        return false;
    }

    // Set up session environment
    if (pam_open_session(m_pamHandle, 0) != PAM_SUCCESS) {
        m_lastError = "Failed to open PAM session";
        return false;
    }

    // Fork and execute session
    pid_t pid = fork();
    if (pid < 0) {
        m_lastError = "Failed to fork process";
        return false;
    }

    if (pid == 0) {
        // Child process
        
        // Set user ID and group ID
        if (initgroups(pw->pw_name, pw->pw_gid) != 0 ||
            setgid(pw->pw_gid) != 0 ||
            setuid(pw->pw_uid) != 0) {
            exit(1);
        }

        // Set environment variables
        setenv("HOME", pw->pw_dir, 1);
        setenv("SHELL", pw->pw_shell, 1);
        setenv("USER", pw->pw_name, 1);
        setenv("LOGNAME", pw->pw_name, 1);
        setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/bin", 1);
        setenv("DESKTOP_SESSION", sessionType.c_str(), 1);

        // Change to user's home directory
        if (chdir(pw->pw_dir) != 0) {
            exit(1);
        }

        // Execute session
        // TODO: Implement proper session launching based on session type
        execl("/bin/sh", "-sh", nullptr);
        exit(1);
    }

    return true;
}

std::vector<std::string> LoginManager::getAvailableSessionTypes() const {
    std::vector<std::string> sessions;
    const std::string xsessions_path = "/usr/share/xsessions";
    const std::string wayland_sessions_path = "/usr/share/wayland-sessions";

    try {
        // Look for X11 sessions
        if (std::filesystem::exists(xsessions_path)) {
            for (const auto& entry : std::filesystem::directory_iterator(xsessions_path)) {
                if (entry.path().extension() == ".desktop") {
                    sessions.push_back(entry.path().stem());
                }
            }
        }

        // Look for Wayland sessions
        if (std::filesystem::exists(wayland_sessions_path)) {
            for (const auto& entry : std::filesystem::directory_iterator(wayland_sessions_path)) {
                if (entry.path().extension() == ".desktop") {
                    sessions.push_back(entry.path().stem());
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        // Log error but continue
        std::cerr << "Error reading session directories: " << e.what() << std::endl;
    }

    if (sessions.empty()) {
        sessions.push_back("failsafe");
    }

    return sessions;
}

std::string LoginManager::getLastError() const {
    return m_lastError;
}

int LoginManager::pamConversation(int num_msg, const struct pam_message **msg,
                                struct pam_response **resp, void *appdata_ptr) {
    if (num_msg <= 0 || !msg || !resp || !appdata_ptr) {
        return PAM_CONV_ERR;
    }

    // Allocate response array
    *resp = static_cast<pam_response*>(calloc(num_msg, sizeof(struct pam_response)));
    if (!*resp) {
        return PAM_BUF_ERR;
    }

    // Get password from appdata
    const std::string& password = *reinterpret_cast<std::string*>(appdata_ptr);

    // Handle messages
    for (int i = 0; i < num_msg; ++i) {
        switch (msg[i]->msg_style) {
            case PAM_PROMPT_ECHO_OFF:
                (*resp)[i].resp = strdup(password.c_str());
                if (!(*resp)[i].resp) {
                    return PAM_BUF_ERR;
                }
                break;

            case PAM_ERROR_MSG:
                std::cerr << "PAM error: " << msg[i]->msg << std::endl;
                break;

            case PAM_TEXT_INFO:
                std::cout << "PAM info: " << msg[i]->msg << std::endl;
                break;

            default:
                return PAM_CONV_ERR;
        }
    }

    return PAM_SUCCESS;
}

bool LoginManager::initializePAM() {
    // PAM will be initialized during authentication
    return true;
}

void LoginManager::cleanup() {
    if (m_pamHandle) {
        pam_end(m_pamHandle, PAM_SUCCESS);
        m_pamHandle = nullptr;
    }
    m_isAuthenticated = false;
    m_currentUser.clear();
}

void LoginManager::logAttempt(const std::string& username, bool success) {
    try {
        std::ofstream log(m_logPath, std::ios::app);
        if (log.is_open()) {
            time_t now = time(nullptr);
            log << std::ctime(&now) << "Login attempt: "
                << "user=" << username << " "
                << "success=" << (success ? "yes" : "no") << " "
                << "ip=" << "localhost" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to log attempt: " << e.what() << std::endl;
    }
}
