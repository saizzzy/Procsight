#include "ProcessList.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <cstring>

namespace {

bool tryParsePid(std::string_view pidStr, int& pidOut) {
    if (pidStr.empty()) return false;
    int value = 0;
    const char* begin = pidStr.data();
    const char* end = pidStr.data() + pidStr.size();
    const auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end) return false;
    if (value <= 0) return false;
    pidOut = value;
    return true;
}

} // namespace

std::vector<ProcessInfo> collectProcessesSortedByRss(const std::string& filter) {
    std::vector<ProcessInfo> out;
    out.reserve(512);

    const auto opts = std::filesystem::directory_options::skip_permission_denied;
    for (const auto& entry : std::filesystem::directory_iterator("/proc", opts)) {
        const auto pidFilename = entry.path().filename().string();
        int pid = 0;
        if (!tryParsePid(pidFilename, pid)) continue;

        const std::string procDir = entry.path().string();

        std::ifstream commFile(procDir + "/comm");
        std::string name;
        if (!commFile.is_open() || !std::getline(commFile, name) || name.empty()) continue;
        if (!filter.empty() && name.find(filter) == std::string::npos) continue;

        // Prefer VmRSS (kB) to match top/htop RES
        std::ifstream statusFile(procDir + "/status");
        long rss_kb = 0;
        if (statusFile.is_open()) {
            std::string line;
            while (std::getline(statusFile, line)) {
                if (line.rfind("VmRSS:", 0) == 0) {
                    // "VmRSS:\t  12345 kB"
                    const char* s = line.c_str() + 6;
                    while (*s == ' ' || *s == '\t') ++s;
                    long value = 0;
                    const char* e = s + std::strlen(s);
                    const auto [ptr, ec] = std::from_chars(s, e, value);
                    if (ec == std::errc{}) rss_kb = value;
                    break;
                }
            }
        }

        out.push_back(ProcessInfo{pid, std::move(name), rss_kb});
    }

    std::sort(out.begin(), out.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
        return a.rss_kb > b.rss_kb;
    });

    return out;
}

bool terminateProcess(const std::string& pid) {
    int pidInt = 0;
    if (!tryParsePid(pid, pidInt)) {
        return false;
    }
    return kill(pidInt, SIGTERM) == 0;
}

void killAllByName(const std::string& targetName) {
    if (targetName.empty()) return;

    const auto opts = std::filesystem::directory_options::skip_permission_denied;
    for (const auto& entry : std::filesystem::directory_iterator("/proc", opts)) {
        const auto pidStr = entry.path().filename().string();
        int pidInt = 0;
        if (!tryParsePid(pidStr, pidInt)) {
            continue;
        }

        std::ifstream commFile(entry.path().string() + "/comm");
        std::string currentName;
        if (std::getline(commFile, currentName) && currentName.find(targetName) != std::string::npos) {
            kill(pidInt, SIGTERM);
        }
    }
}

