#include "SystemStats.hpp"
#include <fstream>
#include <string>

CPUStats get_cpu_times() {
    std::ifstream file("/proc/stat");
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq, steal;
    if (!(file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) {
        return {0, 0};
    }
    return {idle + iowait, user + nice + system + idle + iowait + irq + softirq + steal};
}

double calculate_cpu_usage(CPUStats& prev) {
    CPUStats curr = get_cpu_times();
    long idle_delta = curr.idle - prev.idle;
    long total_delta = curr.total - prev.total;
    prev = curr;
    if (total_delta <= 0) return 0.0;
    return 100.0 * (1.0 - (double)idle_delta / total_delta);
}