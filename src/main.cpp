#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <winreg.h>
#else
    #include <sys/utsname.h>
    #include <unistd.h>
#endif

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <iterator>
#include <algorithm>

using std::string;
using std::to_string;
using std::vector;
using std::max;
using std::stringstream;
using std::getline;
using std::cout;
using std::endl;

const string LOGO = R"(
  ____                                     
 / ___|  _   _  ___  ___  _ __   ___   ____ 
 \___ \ | | | |/ __|/ __|| '_ \ / _ \ /  _/
  ___) || |_| |\__ \\__ \| |_) |  __/|  |_
 |____/  \__, ||___/|___/| .__/ \___| \___\
         |___/           |_|              
)";

struct InfoPair {
    string key;
    string value;
};

InfoPair getUsername() {
#ifdef _WIN32
    char buffer[257];
    DWORD size = sizeof(buffer);
    if (GetComputerNameA(buffer, &size)) {
        return { "Hostname", string(buffer) };
    }
    return { "Hostname", "N/A" };
#else
    char buffer[256];  
    if (gethostname(buffer, sizeof(buffer)) == 0) {
        return { "Hostname", string(buffer) };
    }
    return { "Hostname", "N/A" };
#endif
}

InfoPair getOSInfo() {
#ifdef _WIN32
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
        return { "OS", "N/A" };
    }

    char buffer[256];
    DWORD buffersize = sizeof(buffer);

    if (RegQueryValueEx(hkey, TEXT("ProductName"), NULL, NULL, (LPBYTE)buffer, &buffersize) == ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return { "OS", string(buffer) };
    }

    RegCloseKey(hkey);
    return { "OS", "N/A" };
#else
    struct utsname buffer;
    if (uname(&buffer) == 0) {
        return { "OS", string(buffer.sysname) + " " + string(buffer.release) };
    }
    return { "OS", "N/A" };
#endif
}

InfoPair getCPUInfo() {
#ifdef _WIN32
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
        return { "CPU", "N/A" };
    }

    char buffer[256];
    DWORD buffersize = sizeof(buffer);
    if (RegQueryValueEx(hkey, TEXT("ProcessorNameString"), NULL, NULL, (LPBYTE)buffer, &buffersize) == ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return { "CPU", string(buffer) };
    }
    RegCloseKey(hkey);
    return { "CPU", "N/A" };
#else
    //Linux/macOS CPU info ( from /proc/cpuinfo)
    return { "CPU", "N/A (Linux/macOS)" };
#endif
}

InfoPair getMemoryInfo() {
#ifdef _WIN32
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);

    unsigned long long totalMB = statex.ullTotalPhys / (1024 * 1024);
    unsigned long long freeMB = statex.ullAvailPhys / (1024 * 1024);
    unsigned long long usedMB = totalMB - freeMB;

    string mem_str = to_string(totalMB) + " MB (Used: " + to_string(usedMB) + " MB, Free: " + to_string(freeMB) + " MB)";
    return { "Memory", mem_str };
#else
    return { "Memory", "N/A" };
#endif
}

InfoPair getGPUInfo() {
#ifdef _WIN32
    HKEY hkey;
    
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e968-e325-11ce-bfc1-08002be10318}\\0000"), 0, KEY_READ, &hkey) != ERROR_SUCCESS) {
        return { "GPU", "N/A" };
    }
    char buffer[256];
    DWORD buffersize = sizeof(buffer);
    if (RegQueryValueEx(hkey, TEXT("DriverDesc"), NULL, NULL, (LPBYTE)buffer, &buffersize) == ERROR_SUCCESS) {
        RegCloseKey(hkey);
        return { "GPU", string(buffer) };
    }
    RegCloseKey(hkey);
    return { "GPU", "N/A" };
#else
    return { "GPU", "N/A" };
#endif
}

InfoPair getDiskInfo() {
#ifdef _WIN32
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes, totalNumberOfFreeBytes;
    if (GetDiskFreeSpaceEx(NULL, &freeBytesAvailable, &totalNumberOfBytes, &totalNumberOfFreeBytes)) {
        unsigned long long totalGB = totalNumberOfBytes.QuadPart / (1024 * 1024 * 1024);
        unsigned long long freeGB = totalNumberOfFreeBytes.QuadPart / (1024 * 1024 * 1024);
        unsigned long long usedGB = totalGB - freeGB;
        string disk_str = to_string(totalGB) + " GB (Used: " + to_string(usedGB) + " GB, Free: " + to_string(freeGB) + " GB)";
        return { "Disk", disk_str };
    }
    return { "Disk", "N/A" };
#else
    return { "Disk", "N/A" };
#endif
}

InfoPair getResolutionInfo() {
#ifdef _WIN32
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    string res_str = to_string(screenWidth) + "x" + to_string(screenHeight);
    return { "Resolution", res_str };
#else
    return { "Resolution", "N/A" };
#endif
}

vector<string> splitString(const string& str) {
    stringstream ss(str);
    string line;
    vector<string> lines;

    while (getline(ss, line)) {
        lines.push_back(line);
    }
    return lines;
}

void printSystemInfo() {
    vector<InfoPair> infoList;
    infoList.push_back(getUsername());
    infoList.push_back(getOSInfo());
    infoList.push_back(getCPUInfo());
    infoList.push_back(getMemoryInfo());
    infoList.push_back(getGPUInfo());
    infoList.push_back(getDiskInfo());
    infoList.push_back(getResolutionInfo());

    vector<string> logoLines = splitString(LOGO);
    size_t max_logo_width = 0;
    for (const auto& line : logoLines) {
        max_logo_width = max(max_logo_width, line.length());
    }

    size_t max_lines = max(logoLines.size(), infoList.size());
    for (size_t i = 0; i < max_lines; ++i) {
        if (i < logoLines.size()) {
            cout << logoLines[i];
            cout << string(max_logo_width - logoLines[i].length(), ' ');
        } else {
            cout << string(max_logo_width, ' ');
        }

        cout << "  ";

        if (i < infoList.size()) {
            cout << "  " << infoList[i].key << ": " << infoList[i].value << endl;
        } else {
            cout << endl;
        }
    }
}

int main() {
    printSystemInfo();
    return 0;
}
