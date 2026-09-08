#include "sysspec/utils.hpp"
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace sysspec {

std::string getHostRoot() {
    const char* env = std::getenv("SYSSPEC_HOST_ROOT");
    if (env && env[0] != '\0') return std::string(env);
    return "";
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string readFirstLine(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::string line;
    std::getline(f, line);
    return line;
}

std::vector<std::string> splitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;
    while (std::getline(ss, line)) lines.push_back(line);
    return lines;
}

std::string trim(const std::string& s) {
    size_t a = 0;
    while (a < s.size() && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    size_t b = s.size();
    while (b > a && std::isspace(static_cast<unsigned char>(s[b-1]))) --b;
    return s.substr(a, b-a);
}

std::string humanizeKb(unsigned long long kb) {
    unsigned long long mib = kb / 1024;
    if (mib < 1024) return std::to_string(mib) + " MiB";
    double gib = mib / 1024.0;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f GiB (%llu MiB)", gib, mib);
    return std::string(buf);
}

std::string humanizeMib(unsigned long long mib) {
    if (mib < 1024) return std::to_string(mib) + " MiB";
    double gib = mib / 1024.0;
    char buf[64];
    snprintf(buf, sizeof(buf), "%.2f GiB (%llu MiB)", gib, mib);
    return std::string(buf);
}

} // namespace sysspec
