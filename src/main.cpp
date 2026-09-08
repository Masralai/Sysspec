#include "sysspec/platform.hpp"
#include "sysspec/utils.hpp"
#include "sysspec/types.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <set>
#include <cstdio>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

using namespace sysspec;
using std::string;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;

const string LOGO = R"(
  ____
 / ___|  _   _  ___  ___  _ __   ___   ____
 \___ \ | | | |/ __|/ __|| '_ \ / _ \ /  _/
  ___) || |_| |\__ \\__ \| |_) |  __/|  |_
 |____/  \__, ||___/|___/| .__/ \___| \___\
         |___/           |_|
)";

static string toLower(string s) {
    for (auto &c: s) c = (char)std::tolower((unsigned char)c);
    return s;
}

static string jsonEscape(const string& s) {
    string out; out.reserve(s.size()+8);
    for (unsigned char c: s) {
        if (c=='"') out += "\\\"";
        else if (c=='\\') out += "\\\\";
        else if (c=='\n') out += "\\n";
        else if (c=='\r') out += "\\r";
        else if (c=='\t') out += "\\t";
        else if (c < 0x20) { char buf[7]; snprintf(buf,sizeof(buf),"\\u%04x",c); out+=buf; }
        else out += (char)c;
    }
    return out;
}

static void printHelp(const char* prog) {
    cout << "Sysspec 1.0.0 - cross-platform system profiler\n";
    cout << "Usage: " << prog << " [options]\n";
    cout << "Options:\n";
    cout << "  --help            show help\n";
    cout << "  --version         print version\n";
    cout << "  --json            JSON output to stdout\n";
    cout << "  --plain           no logo, no color\n";
    cout << "  --fields=LIST     comma list filter e.g., --fields=hostname,os,cpu,memory,gpu,disk,resolution,uptime\n";
}

static vector<InfoPair> collectAll() {
    vector<InfoPair> v;
    v.push_back(getHostname());
    v.push_back(getOSInfo());
    v.push_back(getCPUInfo());
    v.push_back(getMemoryInfo());
    v.push_back(getGPUInfo());
    v.push_back(getDiskInfo());
    v.push_back(getResolutionInfo());
    v.push_back(getUptimeInfo());
    return v;
}

static vector<InfoPair> filterFields(const vector<InfoPair>& all, const std::set<string>& fields) {
    if (fields.empty()) return all;
    vector<InfoPair> out;
    for (auto &p: all) {
        if (fields.count(toLower(p.key))) out.push_back(p);
    }
    return out;
}

static void printJson(const vector<InfoPair>& list) {
    cout << "{\n";
    for (size_t i=0;i<list.size();++i) {
        cout << "  \"" << jsonEscape(toLower(list[i].key)) << "\": \"" << jsonEscape(list[i].value) << "\"";
        if (i+1<list.size()) cout << ",";
        cout << "\n";
    }
    cout << "}\n";
}

static void printSystemInfo(const vector<InfoPair>& infoList, bool plain) {
    if (plain) {
        for (auto &p: infoList) {
            cout << p.key << ": " << p.value << "\n";
        }
        return;
    }
    vector<string> logoLines = splitLines(LOGO);
    size_t max_logo_width = 0;
    for (auto &line: logoLines) max_logo_width = std::max(max_logo_width, line.length());

    bool useColor = false;
#ifndef _WIN32
    useColor = isatty(fileno(stdout));
#else
    // MinGW 6.3: avoid _fileno portability — disable color on Win for old toolchain
    useColor = false;
#endif
    const char* cyan = "\033[36m";
    const char* reset = "\033[0m";
    const char* bold = "\033[1m";

    size_t max_lines = std::max(logoLines.size(), infoList.size());
    for (size_t i=0;i<max_lines;++i) {
        if (i < logoLines.size()) {
            if (useColor) cout << cyan;
            cout << logoLines[i];
            if (useColor) cout << reset;
            cout << string(max_logo_width - logoLines[i].length(), ' ');
        } else {
            cout << string(max_logo_width, ' ');
        }
        cout << "  ";
        if (i < infoList.size()) {
            if (useColor) cout << bold << infoList[i].key << reset << ": " << infoList[i].value;
            else cout << infoList[i].key << ": " << infoList[i].value;
            cout << "\n";
        } else {
            cout << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    bool json=false, plain=false, help=false, version=false;
    std::set<string> fields;

    for (int i=1;i<argc;++i) {
        string arg = argv[i];
        if (arg=="--help" || arg=="-h") help=true;
        else if (arg=="--version" || arg=="-v") version=true;
        else if (arg=="--json") json=true;
        else if (arg=="--plain") plain=true;
        else if (arg.rfind("--fields=",0)==0) {
            string list = arg.substr(9);
            string cur;
            for (char c: list) {
                if (c==',') { if(!cur.empty()) { fields.insert(toLower(trim(cur))); cur.clear(); } }
                else cur+=c;
            }
            if(!cur.empty()) fields.insert(toLower(trim(cur)));
        } else {
            cerr << "Unknown option: " << arg << "\n";
            printHelp(argv[0]);
            return 2;
        }
    }

    if (help) { printHelp(argv[0]); return 0; }
    if (version) { cout << "Sysspec 1.0.0\n"; return 0; }

    auto all = collectAll();
    auto filtered = filterFields(all, fields);
    if (filtered.empty() && !fields.empty()) {
        cerr << "No matching fields for filter\n";
        return 2;
    }

    if (json) printJson(filtered);
    else printSystemInfo(filtered, plain);

    return 0;
}
