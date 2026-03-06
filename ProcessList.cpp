#include "ProcessList.hpp"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <csignal>

bool terminateProcess(const std::string& pid) {
    if (pid.empty()) return false;
    return kill(std::stoi(pid), SIGKILL) == 0;
}

void killAllByName(const std::string& targetName) {
    if (targetName.empty()) return;
    for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
        std::string pid = entry.path().filename().string();
        if (std::all_of(pid.begin(), pid.end(), ::isdigit)) {
            std::ifstream commFile("/proc/" + pid + "/comm");
            std::string currentName;
            if (std::getline(commFile, currentName)) {
                if (currentName.find(targetName) != std::string::npos) {
                    kill(std::stoi(pid), SIGKILL);
                }
            }
        }
    }
}