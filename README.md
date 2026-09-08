
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

## Features

* **Native Performance:** Written in C++17 with direct system calls.
* **Cross-Platform:** Supports Windows (Registry-based) and Linux/macOS (Kernel-based).
* **Clean UI:** Side-by-side ASCII logo and system specs alignment.
* **Containerized:** Full Docker support for reproducible builds and environment isolation.
* **CI/CD Ready:** Integrated GitHub Actions for automated CMake builds.

## Hardware Data Points

| Category | Windows Implementation | Linux Implementation |
| :--- | :--- | :--- |
| **OS Info** | Registry (`ProductName`+`DisplayVersion`+`CurrentBuild`) | `/etc/os-release` + `uname` |
| **CPU** | Registry (`ProcessorNameString`) + `GetSystemInfo` | `/proc/cpuinfo` + `uname -m` |
| **Memory** | `GlobalMemoryStatusEx` (humanized GiB/MiB + swap) | `/proc/meminfo` (humanized) |
| **Disk** | `GetLogicalDrives` + `GetDiskFreeSpaceEx` (multi-disk) | `statvfs` + `/proc/mounts` |
| **GPU** | Registry enumeration `{4d36e968...}\*` `DriverDesc` | `/sys/class/drm` + `uevent` |
| **Resolution** | `GetSystemMetrics` + `SM_CMONITORS` | `/sys/class/drm/*/modes` |
| **Uptime** | `GetTickCount` / `GetTickCount64` | `/proc/uptime` |

---

## Quick Start

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

To run Sysspec inside a container while still accessing your host machine's hardware info (no privileged mode needed):

```bash
# Build the image
docker build -t sysspec .

# Run via compose (read-only mounts + HOST_ROOT)
docker compose up

# Or manual
docker run --rm -it -v /proc:/host/proc:ro -v /sys:/host/sys:ro -v /etc/os-release:/host/etc/os-release:ro -e SYSSPEC_HOST_ROOT=/host sysspec --plain
```

### CLI Options

```bash
./build/bin/Sysspec --help
./build/bin/Sysspec --plain              # no logo/color
./build/bin/Sysspec --json               # JSON output
./build/bin/Sysspec --fields=cpu,memory  # filter fields
./build/bin/Sysspec --version
```

---

## Project Structure

```text
SYSSPEC/
├── .github/workflows/  # CI/CD Pipelines (Native & Docker)
├── include/sysspec/    # types.hpp, platform.hpp, utils.hpp
├── src/
│   ├── main.cpp        # CLI + display logic
│   └── platform/
│       ├── common.cpp  # shared helpers (HOST_ROOT, humanize)
│       ├── win.cpp     # Windows Registry/API impl
│       └── linux.cpp   # Linux procfs/sysfs impl
├── spec/               # spec-sysspec-overhaul.md
├── CMakeLists.txt      # Build configuration (C++17, -Wall, install)
├── Dockerfile          # Multi-stage, stripped, non-root
└── compose.yml         # Read-only mounts, no privileged
```
