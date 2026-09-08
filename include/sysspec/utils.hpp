#pragma once
#include <string>
#include <vector>

namespace sysspec {

std::string getHostRoot();
std::string readFirstLine(const std::string& path);
std::string readFile(const std::string& path);
std::vector<std::string> splitLines(const std::string& str);
std::string trim(const std::string& s);
std::string humanizeMib(unsigned long long mib);
std::string humanizeKb(unsigned long long kb);

} // namespace sysspec
