#ifndef PROCESSLIST_HPP
#define PROCESSLIST_HPP

#include <string>
#include <vector>

struct ProcessInfo {
    std::string pid;
    std::string name;
    long rss;
};

// Убить один процесс по PID
bool terminateProcess(const std::string& pid);

// Убить все процессы по имени
void killAllByName(const std::string& targetName);

#endif