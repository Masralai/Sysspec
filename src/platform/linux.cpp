#ifndef _WIN32
#include "sysspec/platform.hpp"
#include "sysspec/utils.hpp"
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <cstring>

namespace sysspec {

InfoPair getHostname() {
    char buf[256];
    if (gethostname(buf, sizeof(buf))==0) return {"Hostname", std::string(buf)};
    return {"Hostname", "unavailable (gethostname failed)"};
}

InfoPair getOSInfo() {
    std::string root = getHostRoot();
    std::string osRelease = readFile(root + "/etc/os-release");
    if (osRelease.empty()) osRelease = readFile("/etc/os-release");
    std::string pretty;
    std::string name, version;
    std::stringstream ss(osRelease);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind("PRETTY_NAME=",0)==0) {
            pretty = line.substr(12);
            // strip quotes
            if (!pretty.empty() && (pretty.front()=='"' || pretty.front()=='\'')) pretty = pretty.substr(1, pretty.size()-2);
        } else if (line.rfind("NAME=",0)==0) {
            name = line.substr(5);
            if (!name.empty() && (name.front()=='"' || name.front()=='\'')) name = name.substr(1, name.size()-2);
        } else if (line.rfind("VERSION=",0)==0) {
            version = line.substr(8);
            if (!version.empty() && (version.front()=='"' || version.front()=='\'')) version = version.substr(1, version.size()-2);
        }
    }
    std::string os;
    if (!pretty.empty()) os = pretty;
    else if (!name.empty()) os = name + (version.empty()?"":" "+version);
    struct utsname u;
    if (uname(&u)==0) {
        if (!os.empty()) os += " ";
        os += std::string(u.sysname) + " " + u.release + " " + u.machine;
        if (!os.empty()) return {"OS", os};
    }
    if (!os.empty()) return {"OS", os};
    return {"OS", "unavailable (no os-release / uname)"};
}

InfoPair getCPUInfo() {
    std::string root = getHostRoot();
    std::string path = root + "/proc/cpuinfo";
    std::string content = readFile(path);
    if (content.empty()) content = readFile("/proc/cpuinfo");
    if (content.empty()) return {"CPU", "unavailable (no /proc/cpuinfo)"};
    std::string model;
    int processors = 0;
    std::string mhz;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.rfind("model name",0)==0) {
            auto pos = line.find(':');
            if (pos!=std::string::npos) model = trim(line.substr(pos+1));
        } else if (line.rfind("processor",0)==0) {
            processors++;
        } else if (line.rfind("cpu MHz",0)==0 && mhz.empty()) {
            auto pos = line.find(':');
            if (pos!=std::string::npos) mhz = trim(line.substr(pos+1));
        }
    }
    struct utsname u; std::string arch;
    if (uname(&u)==0) arch = u.machine;
    std::string out = model.empty() ? "unknown" : model;
    if (processors>0) out += " (" + std::to_string(processors) + " threads";
    if (!arch.empty()) out += ", " + arch;
    if (!mhz.empty()) out += ", " + mhz + " MHz";
    if (processors>0) out += ")";
    return {"CPU", out};
}

InfoPair getMemoryInfo() {
    std::string root = getHostRoot();
    std::string content = readFile(root + "/proc/meminfo");
    if (content.empty()) content = readFile("/proc/meminfo");
    if (content.empty()) return {"Memory", "unavailable (no /proc/meminfo)"};
    unsigned long long memTotalKb=0, memAvailKb=0, swapTotalKb=0;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss,line)) {
        unsigned long long v=0;
        if (sscanf(line.c_str(), "MemTotal: %llu", &v)==1) memTotalKb=v;
        else if (sscanf(line.c_str(), "MemAvailable: %llu", &v)==1) memAvailKb=v;
        else if (sscanf(line.c_str(), "SwapTotal: %llu", &v)==1) swapTotalKb=v;
    }
    if (memTotalKb==0) return {"Memory", "unavailable (parse failed)"};
    unsigned long long avail = memAvailKb ? memAvailKb : 0;
    unsigned long long usedKb = (memTotalKb > avail) ? (memTotalKb - avail) : 0;
    std::string s = humanizeKb(memTotalKb) + " (Used: " + humanizeKb(usedKb) + ", Free: " + humanizeKb(avail) + ")";
    if (swapTotalKb) s += " Swap: " + humanizeKb(swapTotalKb);
    return {"Memory", s};
}

InfoPair getDiskInfo() {
    // try parsing /proc/mounts for real filesystems
    std::string root = getHostRoot();
    std::string mounts = readFile(root + "/proc/mounts");
    if (mounts.empty()) mounts = readFile("/proc/mounts");
    std::vector<std::string> mountPoints;
    if (!mounts.empty()) {
        std::stringstream ss(mounts);
        std::string dev, mp, fs;
        while (ss >> dev >> mp >> fs) {
            std::string rest; std::getline(ss, rest);
            // filter pseudo
            if (fs=="tmpfs" || fs=="devtmpfs" || fs=="proc" || fs=="sysfs" || fs=="cgroup2" || fs=="overlay" || fs=="squashfs") continue;
            if (mp.rfind("/host",0)==0) continue;
            mountPoints.push_back(mp);
            if (mountPoints.size()>=4) break;
        }
    }
    if (mountPoints.empty()) mountPoints.push_back("/");
    std::vector<std::string> parts;
    for (auto &mp: mountPoints) {
        struct statvfs st;
        if (statvfs(mp.c_str(), &st)==0) {
            unsigned long long total = (unsigned long long)st.f_blocks * st.f_frsize;
            unsigned long long freeb = (unsigned long long)st.f_bavail * st.f_frsize;
            unsigned long long used = total - freeb;
            unsigned long long totalGb = total / (1024ull*1024*1024);
            unsigned long long freeGb = freeb / (1024ull*1024*1024);
            unsigned long long usedGb = totalGb - freeGb;
            (void)used;
            parts.push_back(mp + ": " + std::to_string(totalGb) + " GB (Used: " + std::to_string(usedGb) + " GB, Free: " + std::to_string(freeGb) + " GB)");
        }
    }
    if (parts.empty()) return {"Disk", "unavailable (statvfs failed)"};
    std::string out;
    for (size_t i=0;i<parts.size();++i) { if(i) out+=" | "; out+=parts[i]; }
    return {"Disk", out};
}

InfoPair getGPUInfo() {
    std::string root = getHostRoot();
    std::string drmPath = root + "/sys/class/drm";
    if (root.empty()) drmPath = "/sys/class/drm";
    DIR* d = opendir(drmPath.c_str());
    std::vector<std::string> gpus;
    if (d) {
        struct dirent* ent;
        while ((ent=readdir(d))!=nullptr) {
            std::string n = ent->d_name;
            if (n.rfind("card",0)!=0) continue;
            if (n.find('-')!=std::string::npos) continue; // skip card0-HDMI etc
            // try to read device/vendor + device/device
            std::string base = drmPath + "/" + n + "/device";
            std::string vendor = trim(readFirstLine(base + "/vendor"));
            std::string device = trim(readFirstLine(base + "/device"));
            std::string modalias; // fallback
            // Try uevent for DRIVER
            std::string uevent = readFile(base + "/uevent");
            std::string driver;
            std::stringstream ss(uevent);
            std::string line;
            while (std::getline(ss,line)) if (line.rfind("DRIVER=",0)==0) driver=line.substr(7);
            std::string info = n;
            if (!driver.empty()) info += " " + driver;
            if (!vendor.empty() || !device.empty()) info += " ["+vendor+":"+device+"]";
            // also try to read subsystem vendor name via lspci not available; keep short
            gpus.push_back(info);
        }
        closedir(d);
    }
    if (!gpus.empty()) {
        std::string out;
        for (size_t i=0;i<gpus.size();++i) { if(i) out+=", "; out+=gpus[i]; }
        return {"GPU", out};
    }
    // fallback: try lspci if exists
    // we keep minimal: check /proc
    return {"GPU", "unavailable (no /sys/class/drm card; container without GPU passthrough)"};
}

InfoPair getResolutionInfo() {
    // try DRM modes
    std::string root = getHostRoot();
    std::string drmPath = root + "/sys/class/drm";
    if (root.empty()) drmPath = "/sys/class/drm";
    DIR* d = opendir(drmPath.c_str());
    std::vector<std::string> modes;
    if (d) {
        struct dirent* ent;
        while ((ent=readdir(d))!=nullptr) {
            std::string n = ent->d_name;
            if (n.find('-')==std::string::npos) continue;
            std::string mode = trim(readFirstLine(drmPath + "/" + n + "/modes"));
            if (!mode.empty()) {
                // first mode is preferred
                std::stringstream ss(mode);
                std::string first; ss >> first;
                if (!first.empty()) modes.push_back(first + " (" + n + ")");
            }
        }
        closedir(d);
    }
    if (!modes.empty()) {
        std::string out;
        for (size_t i=0;i<modes.size();++i) { if(i) out+=", "; out+=modes[i]; }
        return {"Resolution", out};
    }
    return {"Resolution", "unavailable (no DRM modes; headless or no display)"};
}

InfoPair getUptimeInfo() {
    std::string root = getHostRoot();
    std::string up = readFirstLine(root + "/proc/uptime");
    if (up.empty()) up = readFirstLine("/proc/uptime");
    if (up.empty()) return {"Uptime", "unavailable (no /proc/uptime)"};
    double sec=0; sscanf(up.c_str(), "%lf", &sec);
    unsigned long long s = (unsigned long long)sec;
    unsigned long long h = s/3600; s%=3600;
    unsigned long long m = s/60; s%=60;
    char buf[64]; snprintf(buf,sizeof(buf),"%lluh %llum %llus",h,m,s);
    return {"Uptime", std::string(buf)};
}

} // namespace sysspec
#endif
