#ifndef SYSTEMSTATS_HPP
#define SYSTEMSTATS_HPP

struct CPUStats {
    long idle, total;
};

// Получение сырых данных из /proc/stat
CPUStats get_cpu_times();

// Расчет процента загрузки
double calculate_cpu_usage(CPUStats& prev);

#endif