#include "SystemStats.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>

// Форматирование вывода сетевой скорости
std::string format_speed(double bytes_per_sec) {
    const char* units[] = {"B/s", "KiB/s", "MiB/s", "GiB/s"};
    int unit_index = 0;
    double speed = bytes_per_sec;

    while (speed >= 1024.0 && unit_index < 3) {
        speed /= 1024.0;
        unit_index++;
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << speed << " " << units[unit_index];
    return out.str();
}

// Реализация функций CPU
CPUStats get_cpu_times() {
    std::ifstream file("/proc/stat");
    std::string cpu;
    unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    if (!(file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal)) {
        return {0, 0};
    }
    return {idle + iowait, user + nice + system + idle + iowait + irq + softirq + steal};
}

double calculate_cpu_usage(CPUStats& prev) {
    CPUStats curr = get_cpu_times();
    const unsigned long long idle_delta = (curr.idle >= prev.idle) ? (curr.idle - prev.idle) : 0ULL;
    const unsigned long long total_delta = (curr.total >= prev.total) ? (curr.total - prev.total) : 0ULL;
    prev = curr;
    if (total_delta <= 0) return 0.0;
    return 100.0 * (1.0 - static_cast<double>(idle_delta) / total_delta);
}

// Реализация функций Network
InternetStats get_internet_stats() {
    std::ifstream file("/proc/net/dev");
    std::string line;
    long long rx_total = 0, tx_total = 0;

    std::getline(file, line);
    std::getline(file, line);

    while (std::getline(file, line)) {
        size_t colon_pos = line.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string iface = line.substr(0, colon_pos);
        // trim spaces (format is "  eth0: ...")
        const auto first = iface.find_first_not_of(" \t");
        const auto last = iface.find_last_not_of(" \t");
        iface = (first == std::string::npos) ? std::string{} : iface.substr(first, last - first + 1);
        if (iface == "lo") continue;

        std::string data = line.substr(colon_pos + 1);
        std::istringstream iss(data);
        long long rx_bytes = 0;
        long long tx_bytes = 0;
        long long skip = 0;
        // Fields after ':' are:
        // rx_bytes rx_packets rx_errs rx_drop rx_fifo rx_frame rx_compressed rx_multicast
        // tx_bytes tx_packets ...
        if (iss >> rx_bytes) {
            for (int i = 0; i < 7; ++i) {
                if (!(iss >> skip)) { rx_bytes = 0; break; }
            }
            if (iss >> tx_bytes) {
                rx_total += rx_bytes;
                tx_total += tx_bytes;
            }
        }
    }
    return {static_cast<double>(rx_total), static_cast<double>(tx_total)};
}

InternetStats calculate_internet_speed(InternetStats& prev, const std::chrono::milliseconds& interval) {
    InternetStats curr = get_internet_stats();
    InternetStats speed = {0.0, 0.0};

    double interval_s = interval.count() / 1000.0;
    if (interval_s > 0) {
        speed.rx_bytes = (curr.rx_bytes - prev.rx_bytes) / interval_s;
        speed.tx_bytes = (curr.tx_bytes - prev.tx_bytes) / interval_s;
    }

    prev = curr;
    return speed;
}

MemoryStats get_memory_stats() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    unsigned long long value = 0;
    std::string unit;
    unsigned long long total_kb = 0;
    unsigned long long available_kb = 0;

    while (file >> key >> value >> unit) {
        if (key == "MemTotal:") total_kb = value;
        else if (key == "MemAvailable:") available_kb = value;
        if (total_kb && available_kb) break;
    }
    return {total_kb, available_kb};
}

static bool parse_nvidia_smi_line(const char* line, GPUStats& out) {
    // expected: "<util>, <used>, <total>\n" (csv, no units)
    int util = -1;
    unsigned long long used = 0;
    unsigned long long total = 0;
    if (std::sscanf(line, " %d , %llu , %llu", &util, &used, &total) == 3) {
        out.available = true;
        out.util_percent = util;
        out.mem_used_mb = used;
        out.mem_total_mb = total;
        return true;
    }
    return false;
}

GPUStats get_gpu_stats() {
    GPUStats out{false, -1, 0, 0};

    // NVIDIA: prefer nvidia-smi if present/working
    {
        FILE* fp = popen("nvidia-smi --query-gpu=utilization.gpu,memory.used,memory.total --format=csv,noheader,nounits 2>/dev/null | head -n 1", "r");
        if (fp) {
            char buf[256];
            if (std::fgets(buf, sizeof(buf), fp)) {
                parse_nvidia_smi_line(buf, out);
            }
            pclose(fp);
            if (out.available) return out;
        }
    }

    // Generic DRM: gpu_busy_percent (Intel/AMD drivers sometimes expose it)
    {
        std::ifstream f("/sys/class/drm/card0/device/gpu_busy_percent");
        int util = -1;
        if (f.is_open() && (f >> util)) {
            out.available = true;
            out.util_percent = util;
            out.mem_used_mb = 0;
            out.mem_total_mb = 0;
            return out;
        }
    }

    return out;
}

void display_network_ui(const InternetStats& speed) {
    std::string rx_str = format_speed(speed.rx_bytes);
    std::string tx_str = format_speed(speed.tx_bytes);

    std::printf("┌── download ┐\n");
    std::printf("▼ %-10s\n", rx_str.c_str());
    std::printf("\n");
    std::printf("▲ %-10s\n", tx_str.c_str());
    std::printf("└─ upload ───┘\n");
}

