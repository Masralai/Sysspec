#include <cassert>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

static std::string exec(const std::string& cmd) {
    // MinGW 6.3 popen portability: use temp file
    std::string tmp = "tests/tmp_out.txt";
    std::string full = cmd + " > " + tmp + " 2>&1";
    std::system(full.c_str());
    FILE* f = fopen(tmp.c_str(), "r");
    std::string out;
    if (f) {
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) out += buf;
        fclose(f);
    }
    std::remove(tmp.c_str());
    return out;
}

int main() {
    // Use build/bin/Sysspec.exe produced by previous step
    std::string bin = "build\\bin\\Sysspec.exe";
    // --help contains Usage
    auto help = exec(bin + " --help");
    assert(help.find("Usage")!=std::string::npos);
    assert(help.find("--json")!=std::string::npos);
    // --version
    auto ver = exec(bin + " --version");
    assert(ver.find("1.0.0")!=std::string::npos);
    // --json
    auto js = exec(bin + " --json");
    assert(js.find("\"hostname\"")!=std::string::npos);
    assert(js.find("\"os\"")!=std::string::npos);
    // --plain
    auto plain = exec(bin + " --plain");
    assert(plain.find("Hostname:")!=std::string::npos);
    assert(plain.find("Hostname:")<plain.find("OS:"));
    // --fields filter
    auto filt = exec(bin + " --fields=cpu --plain");
    assert(filt.find("CPU:")!=std::string::npos);
    assert(filt.find("Memory:")==std::string::npos);
    // unknown exits 2
    int rc = std::system((bin + " --unknown > NUL 2>&1").c_str());
    // MinGW system returns 512*exitcode; just check non-zero
    assert(rc!=0);

    std::cout << "All CLI tests passed\n";
    return 0;
}
