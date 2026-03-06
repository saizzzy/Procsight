ProcSight v1.0 🛰️
A Lightweight TUI Process Monitor for Arch Linux

ProcSight is a minimalist, terminal-based system monitor written in C++17. It provides real-time insights into CPU usage and process management by directly interfacing with the Linux `/proc` filesystem.

---

✨ Features
• Real-time CPU Telemetry: Visual progress bar indicating total CPU load with dynamic color coding (Green/Yellow/Red).

• Dynamic Process List: Automatically updated table of running processes, sorted by memory usage (RSS).

• Live Filtering: Instant search functionality to isolate specific processes by name.

• Process Management: Built-in signals to terminate individual processes by PID or "kill all" by process name.

• Resource Efficient: Low overhead, using native Linux C++ APIs without heavy external dependencies.

---

🛠️ Tech Stack
• Language: C++17

• UI Library: `ncursesw` (Wide-character support for smooth borders and UTF-8)

• Build System: CMake 3.20+

• Data Source: Linux Kernel `/proc` virtual filesystem

---

🚀 Getting Started
Prerequisites

Ensure you are on a Linux distribution (optimized for Arch Linux). You will need `ncurses` headers and a C++ compiler.

```

sudo pacman -S base-devel cmake ncurses

```

Installation & Build

```

# Clone the repository

git clone https://github.com/saizzzy/Procsight.git

cd Procsight

# Create build directory

mkdir build && cd build

# Configure and Compile

cmake ..

make

# Run the application

./procsight

```

---

🎮 Keybindings
---

🏗️ Architecture
The project follows a modular design for maintainability:

• `SystemStats`: Handles CPU Jiffies calculation and system-wide metrics.

• `ProcessList`: Manages process discovery, memory parsing, and signaling.

• `UI/Main`: Orchestrates the ncurses render loop and user input handling.

---

📄 License
Distributed under the MIT License.
