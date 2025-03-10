# Arch Linux Login Manager

A custom, lightweight login manager (display manager) for Arch Linux written in C++.

## Features

- Modern, minimalist X11-based graphical interface
- Secure PAM authentication
- Session type selection
- Systemd service integration
- Resource management and monitoring
- Detailed logging capabilities

## Dependencies

- libpam
- X11/Xlib
- Xft
- C++17 compatible compiler
- systemd

## Building

```bash
make
```

For debug build:
```bash
make debug
```

For release build:
```bash
make release
```

## Installation

```bash
sudo make install
```

This will:
1. Install the binary to /usr/local/bin/
2. Install the systemd service file
3. Create necessary configuration directories
4. Enable the service to start on boot

## Uninstallation

```bash
sudo make uninstall
```

## Configuration

Configuration files are stored in `/etc/login_manager/`.

## Development

### Project Structure

```
login_manager/
├── include/
│   ├── LoginManager.hpp    # Authentication and session management
│   └── UIManager.hpp       # X11 UI implementation
├── src/
│   ├── main.cpp           # Main program entry
│   ├── LoginManager.cpp   # Login manager implementation
│   └── UIManager.cpp      # UI manager implementation
├── Makefile
└── arch-login.service     # Systemd service file
```

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/yourusername/arch-login-manager.git
cd arch-login-manager
```

2. Build the project:
```bash
make
```

3. Install:
```bash
sudo make install
```

## License

MIT License

## Contributing

1. Fork the repository
2. Create your feature branch
3. Commit your changes
4. Push to the branch
5. Create a new Pull Request
# arch-login-manager
