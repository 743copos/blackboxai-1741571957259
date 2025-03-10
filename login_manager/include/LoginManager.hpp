#pragma once

#include <string>
#include <memory>
#include <vector>
#include <security/pam_appl.h>

class LoginManager {
public:
    LoginManager();
    ~LoginManager();

    // Initialize the login manager
    bool initialize();

    // Authenticate user credentials
    bool authenticate(const std::string& username, const std::string& password);

    // Launch a new session for the authenticated user
    bool launchSession(const std::string& sessionType);

    // Get available session types
    std::vector<std::string> getAvailableSessionTypes() const;

    // Get last error message
    std::string getLastError() const;

private:
    // PAM conversation function
    static int pamConversation(int num_msg, const struct pam_message **msg,
                             struct pam_response **resp, void *appdata_ptr);

    // Internal methods
    bool initializePAM();
    void cleanup();
    void logAttempt(const std::string& username, bool success);

    // Member variables
    pam_handle_t* m_pamHandle;
    std::string m_lastError;
    std::string m_currentUser;
    bool m_isAuthenticated;

    // Configuration
    std::string m_configPath;
    std::string m_logPath;
};
