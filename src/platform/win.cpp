#ifdef _WIN32
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#define NOMINMAX
#include "sysspec/platform.hpp"
#include "sysspec/utils.hpp"
#include <windows.h>
#include <winreg.h>
#include <string>
#include <vector>
#include <cstdio>
#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

namespace sysspec {

namespace {
class RegKey {
public:
    RegKey() : h_(nullptr) {}
    explicit RegKey(HKEY h) : h_(h) {}
    ~RegKey() { if (h_) RegCloseKey(h_); }
    RegKey(const RegKey&) = delete;
    RegKey& operator=(const RegKey&) = delete;
    RegKey(RegKey&& o) noexcept : h_(o.h_) { o.h_ = nullptr; }
    HKEY get() const { return h_; }
    HKEY* addr() { return &h_; }
    explicit operator bool() const { return h_ != nullptr; }
private:
    HKEY h_;
};

std::string queryRegString(HKEY root, const char* subkey, const char* value) {
    HKEY h = nullptr;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &h) != ERROR_SUCCESS) return "";
    RegKey key(h);
    DWORD type = 0, size = 0;
    if (RegQueryValueExA(key.get(), value, nullptr, &type, nullptr, &size) != ERROR_SUCCESS) return "";
    if (type != REG_SZ && type != REG_EXPAND_SZ) return "";
    std::string buf(size, '\0');
    if (RegQueryValueExA(key.get(), value, nullptr, nullptr, reinterpret_cast<LPBYTE>(&buf[0]), &size) != ERROR_SUCCESS) return "";
    // buf contains null terminator; trim it
    if (!buf.empty() && buf.back() == '\0') buf.pop_back();
    // handle embedded nulls
    size_t n = buf.find('\0');
    if (n != std::string::npos) buf.resize(n);
    return trim(buf);
}
} // anon

InfoPair getHostname() {
    char buffer[257];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) return {"Hostname", std::string(buffer)};
    return {"Hostname", "unavailable (GetComputerNameA failed)"};
}

InfoPair getOSInfo() {
    std::string name = queryRegString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "ProductName");
    if (!name.empty()) {
        std::string displayVersion = queryRegString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "DisplayVersion");
        std::string build = queryRegString(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", "CurrentBuild");
        if (!displayVersion.empty()) name += " " + displayVersion;
        if (!build.empty()) name += " (Build " + build + ")";
        return {"OS", name};
    }
    return {"OS", "unavailable (registry read failed)"};
}

InfoPair getCPUInfo() {
    std::string name = queryRegString(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString");
    if (name.empty()) return {"CPU", "unavailable (registry read failed)"};
    SYSTEM_INFO si; GetSystemInfo(&si);
    int cores = (int)si.dwNumberOfProcessors;
    // Try to get arch
    std::string arch = "unknown";
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: arch = "x86_64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: arch = "x86"; break;
        case PROCESSOR_ARCHITECTURE_ARM: arch = "ARM"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: arch = "ARM64"; break;
        default: break;
    }
    return {"CPU", name + " (" + std::to_string(cores) + " cores, " + arch + ")"};
}

InfoPair getMemoryInfo() {
    MEMORYSTATUSEX st; st.dwLength = sizeof(st);
    if (!GlobalMemoryStatusEx(&st)) return {"Memory", "unavailable (GlobalMemoryStatusEx failed)"};
    unsigned long long totalMib = st.ullTotalPhys / (1024*1024);
    unsigned long long availMib = st.ullAvailPhys / (1024*1024);
    unsigned long long usedMib = totalMib - availMib;
    std::string s = humanizeMib(totalMib) + " (Used: " + humanizeMib(usedMib) + ", Free: " + humanizeMib(availMib) + ")";
    // include swap
    unsigned long long totalPageMib = st.ullTotalPageFile / (1024*1024);
    if (totalPageMib > totalMib) {
        s += " Swap: " + humanizeMib(totalPageMib - totalMib);
    }
    return {"Memory", s};
}

InfoPair getGPUInfo() {
    std::vector<std::string> gpus;
    const char* base = "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}";
    HKEY hBase = nullptr;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, base, 0, KEY_READ, &hBase) == ERROR_SUCCESS) {
        RegKey baseKey(hBase);
        for (DWORD i = 0; i < 64; ++i) {
            char sub[16]; snprintf(sub, sizeof(sub), "%04lu", (unsigned long)i);
            std::string desc = queryRegString(HKEY_LOCAL_MACHINE, (std::string(base) + "\\" + sub).c_str(), "DriverDesc");
            if (!desc.empty()) gpus.push_back(desc);
        }
    }
    if (gpus.empty()) {
        std::string single = queryRegString(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000", "DriverDesc");
        if (!single.empty()) gpus.push_back(single);
    }
    if (gpus.empty()) return {"GPU", "unavailable (no GPU registry entry)"};
    std::string out;
    for (size_t i=0;i<gpus.size();++i) {
        if (i) out += ", ";
        out += gpus[i];
    }
    return {"GPU", out};
}

InfoPair getDiskInfo() {
    // enumerate drives
    DWORD drives = GetLogicalDrives();
    std::vector<std::string> parts;
    for (char c='A'; c<='Z'; ++c) {
        if (!(drives & (1 << (c-'A')))) continue;
        std::string root; root += c; root += ":\\";
        UINT type = GetDriveTypeA(root.c_str());
        if (type != DRIVE_FIXED) continue;
        ULARGE_INTEGER freeAvail, total, freeTotal;
        if (GetDiskFreeSpaceExA(root.c_str(), &freeAvail, &total, &freeTotal)) {
            unsigned long long totalGb = total.QuadPart / (1024ull*1024*1024);
            unsigned long long freeGb = freeTotal.QuadPart / (1024ull*1024*1024);
            unsigned long long usedGb = totalGb - freeGb;
            parts.push_back(std::string(1,c) + ": " + std::to_string(totalGb) + " GB (Used: " + std::to_string(usedGb) + " GB, Free: " + std::to_string(freeGb) + " GB)");
        }
    }
    if (parts.empty()) {
        // fallback to C: or root
        ULARGE_INTEGER freeAvail, total, freeTotal;
        if (GetDiskFreeSpaceExA(nullptr, &freeAvail, &total, &freeTotal)) {
            unsigned long long totalGb = total.QuadPart / (1024ull*1024*1024);
            unsigned long long freeGb = freeTotal.QuadPart / (1024ull*1024*1024);
            unsigned long long usedGb = totalGb - freeGb;
            return {"Disk", std::to_string(totalGb) + " GB (Used: " + std::to_string(usedGb) + " GB, Free: " + std::to_string(freeGb) + " GB)"};
        }
        return {"Disk", "unavailable (GetDiskFreeSpaceEx failed)"};
    }
    std::string out;
    for (size_t i=0;i<parts.size();++i) { if(i) out += " | "; out += parts[i]; }
    return {"Disk", out};
}

InfoPair getResolutionInfo() {
    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);
    if (w==0 || h==0) return {"Resolution", "unavailable (GetSystemMetrics failed)"};
    // try enum monitors for count
    int count = GetSystemMetrics(SM_CMONITORS);
    std::string s = std::to_string(w) + "x" + std::to_string(h);
    if (count > 1) s += " (" + std::to_string(count) + " monitors)";
    return {"Resolution", s};
}

InfoPair getUptimeInfo() {
#ifdef GetTickCount64
    ULONGLONG ms = GetTickCount64();
#else
    DWORD ms = GetTickCount();
#endif
    unsigned long long sec = (unsigned long long)ms/1000;
    unsigned long long h = sec/3600; sec%=3600;
    unsigned long long m = sec/60; sec%=60;
    char buf[64]; snprintf(buf,sizeof(buf),"%lluh %llum %llus",h,m,sec);
    return {"Uptime", std::string(buf)};
}

} // namespace sysspec
#endif
