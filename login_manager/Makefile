CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I/usr/include/X11
LDFLAGS = -lX11 -lXft -lpam -lpthread

# Directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files
SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Binary name
TARGET = $(BIN_DIR)/arch-login

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Build target
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Install targets
install: all
	@echo "Installing arch-login..."
	sudo install -m 755 $(TARGET) /usr/local/bin/
	sudo install -m 644 arch-login.service /etc/systemd/system/
	sudo mkdir -p /etc/login_manager
	@echo "Enabling service..."
	sudo systemctl daemon-reload
	sudo systemctl enable arch-login.service
	@echo "Installation complete!"

# Uninstall targets
uninstall:
	@echo "Uninstalling arch-login..."
	sudo systemctl disable arch-login.service
	sudo rm -f /usr/local/bin/arch-login
	sudo rm -f /etc/systemd/system/arch-login.service
	sudo rm -rf /etc/login_manager
	sudo systemctl daemon-reload
	@echo "Uninstallation complete!"

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Debug build
debug: CXXFLAGS += -g -DDEBUG
debug: all

# Release build
release: CXXFLAGS += -O2 -DNDEBUG
release: all

.PHONY: all directories clean install uninstall debug release
