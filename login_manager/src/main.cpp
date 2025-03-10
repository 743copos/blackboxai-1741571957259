#include <iostream>
#include <memory>
#include <signal.h>
#include "../include/LoginManager.hpp"
#include "../include/UIManager.hpp"

// Global pointer for signal handling
static std::shared_ptr<UIManager> g_uiManager;

void signalHandler(int signum) {
    if (g_uiManager) {
        g_uiManager->setLoading(false);
        g_uiManager->showError("Received signal " + std::to_string(signum));
    }
}

int main(int argc, char* argv[]) {
    // Set up signal handlers
    signal(SIGTERM, signalHandler);
    signal(SIGINT, signalHandler);

    try {
        // Create login manager
        auto loginManager = std::make_shared<LoginManager>();
        if (!loginManager->initialize()) {
            std::cerr << "Failed to initialize login manager: "
                     << loginManager->getLastError() << std::endl;
            return 1;
        }

        // Create UI manager
        g_uiManager = std::make_shared<UIManager>(loginManager);
        if (!g_uiManager->initialize()) {
            std::cerr << "Failed to initialize UI manager" << std::endl;
            return 1;
        }

        // Set up login callback
        g_uiManager->setLoginCallback(
            [loginManager, &g_uiManager](const std::string& username, const std::string& password) {
                g_uiManager->setLoading(true);
                g_uiManager->clearError();

                if (loginManager->authenticate(username, password)) {
                    // Get selected session (default to "default" if none selected)
                    std::string sessionType = "default";
                    auto sessions = loginManager->getAvailableSessionTypes();
                    if (!sessions.empty()) {
                        sessionType = sessions[0];
                    }

                    // Launch session
                    if (!loginManager->launchSession(sessionType)) {
                        g_uiManager->showError(loginManager->getLastError());
                    }
                } else {
                    g_uiManager->showError(loginManager->getLastError());
                }

                g_uiManager->setLoading(false);
            }
        );

        // Set up session selection callback
        g_uiManager->setSessionSelectCallback(
            [](const std::string& session) {
                // Handle session selection
                std::cout << "Selected session: " << session << std::endl;
            }
        );

        // Run the UI
        g_uiManager->run();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
