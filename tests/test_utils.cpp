#include "sysspec/utils.hpp"
#include <cassert>
#include <iostream>

using namespace sysspec;

int main() {
    // trim
    assert(trim("  hello  ") == "hello");
    assert(trim("\t\nfoo\r") == "foo");
    assert(trim("") == "");

    // splitLines
    auto lines = splitLines("a\nb\nc");
    assert(lines.size()==3 && lines[0]=="a" && lines[2]=="c");
    assert(splitLines("").size()==0);

    // humanizeKb
    assert(humanizeKb(512) == "0 MiB" || humanizeKb(512).find("MiB")!=std::string::npos);
    assert(humanizeKb(1024) == "1 MiB");
    assert(humanizeKb(1048576).find("GiB")!=std::string::npos); // 1GiB
    // humanizeMib
    assert(humanizeMib(512) == "512 MiB");
    assert(humanizeMib(2048).find("2.00 GiB")!=std::string::npos);

    // getHostRoot env indirection
    // default empty when SYSSPEC_HOST_ROOT not set
    // we don't set it in test, expect empty or /host if env set externally
    std::string root = getHostRoot();
    // just ensure call doesn't crash

    // readFile nonexistent returns empty
    assert(readFile("/nonexistent/path/xyz").empty());
    assert(readFirstLine("/nonexistent").empty());

    // read fixtures
    std::string mem = readFile("tests/fixtures/meminfo.sample");
    assert(!mem.empty() && mem.find("MemTotal")!=std::string::npos);
    std::string cpu = readFile("tests/fixtures/cpuinfo.sample");
    assert(cpu.find("model name")!=std::string::npos);
    std::string os = readFile("tests/fixtures/os-release.sample");
    assert(os.find("PRETTY_NAME")!=std::string::npos);

    std::cout << "All utils tests passed\n";
    return 0;
}
