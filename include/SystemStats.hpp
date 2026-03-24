#ifndef PROCSIGHT_SYSTEMSTATS_HPP
#define PROCSIGHT_SYSTEMSTATS_HPP

#include <chrono>
#include <string>

struct CPUStats {
    unsigned long long idle, total;
};

struct InternetStats {
    double rx_bytes;
    double tx_bytes;
};

struct MemoryStats {
    unsigned long long total_kb;
    unsigned long long available_kb;
};

struct GPUStats {
    bool available;
    int util_percent;                 // 0..100, -1 if unknown
    unsigned long long mem_used_mb;   // 0 if unknown
    unsigned long long mem_total_mb;  // 0 if unknown
};

std::string format_speed(double bytes_per_sec);
CPUStats get_cpu_times();
double calculate_cpu_usage(CPUStats& prev);
InternetStats get_internet_stats();
InternetStats calculate_internet_speed(InternetStats& prev, const std::chrono::milliseconds& interval);
MemoryStats get_memory_stats();
GPUStats get_gpu_stats();

#endif
