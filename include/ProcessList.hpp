#ifndef PROCSIGHT_PROCESSLIST_HPP
#define PROCSIGHT_PROCESSLIST_HPP

#include <string>
#include <vector>

struct ProcessInfo {
    int pid;
    std::string name;
    long rss_kb; // VmRSS from /proc/[pid]/status
};

// Сбор процессов и сортировка по RSS (desc) для отображения
std::vector<ProcessInfo> collectProcessesSortedByRss(const std::string& filter);

// Убить один процесс по PID
bool terminateProcess(const std::string& pid);

// Убить все процессы по имени
void killAllByName(const std::string& targetName);

#endif
