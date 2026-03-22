
# Sysspec 

**Sysspec** is a high-performance, cross-platform C++ CLI utility designed to fetch and display system hardware information in a beautiful, side-by-side terminal format. It leverages native OS APIs (WinAPI for Windows and POSIX for Linux) to provide hardware insights with zero overhead.

```text
  ____                                     
 / ___|  _   _  ___  ___  _ __   ___   ____ 
 \___ \ | | | |/ __|/ __|| '_ \ / _ \ /  _/ 
  ___) || |_| |\__ \\__ \| |_) |  __/|  |_  
 |____/  \__, ||___/|___/| .__/ \___| \___\ 
         |___/           |_|                
                                           Hostname: Dev
                                           OS: Windows 11 Pro
                                           CPU: Intel(R) Core(TM) i7...
```

##  Features

* **Native Performance:** Written in C++17 with direct system calls.
* **Cross-Platform:** Supports Windows (Registry-based) and Linux/macOS (Kernel-based).
* **Clean UI:** Side-by-side ASCII logo and system specs alignment.
* **Containerized:** Full Docker support for reproducible builds and environment isolation.
* **CI/CD Ready:** Integrated GitHub Actions for automated CMake builds.

## 🛠 Hardware Data Points

| Category | Windows Implementation | Linux/Unix Implementation |
| :--- | :--- | :--- |
| **OS Info** | Registry (`ProductName`) | `uname` (sysname + release) |
| **CPU** | Registry (`ProcessorNameString`) | `/proc/cpuinfo` (Stubbed) |
| **Memory** | `GlobalMemoryStatusEx` | `/proc/meminfo` (Planned) |
| **Disk** | `GetDiskFreeSpaceEx` | `statvfs` (Planned) |
| **GPU** | Registry (`DriverDesc`) | PCI Bus lookup (Planned) |

---

##  Quick Start

### Prerequisites
* **CMake** (v3.10+)
* **Compiler:** GCC 11+, Clang 12+, or MSVC 2019+
* **Docker** (Optional, for containerized execution)

### Native Build
```bash
# Generate build files
cmake -B build -S .

# Compile the project
cmake --build build --config Release

# Run the binary
./build/bin/Sysspec
```

### Docker Build & Run
To run Sysspec inside a container while still accessing your host machine's hardware info:

```bash
# Build the image
docker build -t sysspec .

# Run with host privileges
docker run --rm -it --pid="host" --network="host" sysspec
```

---

##  Project Structure
```text
SYSSPEC/
├── .github/workflows/  # CI/CD Pipelines (Native & Docker)
├── src/
│   └── main.cpp        # Core logic & OS-specific macros
├── CMakeLists.txt      # Build configuration
├── Dockerfile          # Multi-stage production build
└── compose.yml         # Container orchestration with host access
```

