#include "../include/App.hpp"

#include <clocale>
#include <csignal>
#include <chrono>
#include <algorithm>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>

#include <curses.h>

#include "../include/ProcessList.hpp"
#include "../include/SystemStats.hpp"

#include "../include/NcursesSession.hpp"
#include "../include/Renderer.hpp"
#include "../include/Theme.hpp"

namespace procsight {
namespace {
static volatile sig_atomic_t g_running = 1;
constexpr int kInputPollMs = 50;
constexpr std::chrono::milliseconds kProcessRefreshInterval(700);

static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        g_running = 0;
    }
}

std::string get_user_input(int y, int x, int max_len = 256) {
    echo();
    curs_set(1);
    timeout(-1);
    std::string input;
    move(y, x);
    clrtoeol();
    refresh();
    int ch;
    while ((ch = getch()) != '\n' && ch != '\r' &&
           input.length() < static_cast<std::string::size_type>(max_len)) {
        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == 8) {
            if (!input.empty()) {
                input.pop_back();
                move(y, x);
                clrtoeol();
                mvprintw(y, x, "%s", input.c_str());
            }
        } else if (std::isprint(static_cast<unsigned char>(ch))) {
            input += static_cast<char>(ch);
            mvprintw(y, x, "%s", input.c_str());
        }
    }
    noecho();
    curs_set(0);
    timeout(kInputPollMs);
    return input;
}
} // namespace

App::App() = default;

int App::run() {
    setlocale(LC_ALL, "");
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    NcursesSession session;
    timeout(kInputPollMs);
    mouseinterval(0);

    Theme theme;
    Renderer renderer(theme);

    std::string filter;
    CPUStats prev_cpu = get_cpu_times();
    InternetStats prev_net = get_internet_stats();

    double cpu_usage_smooth = 0.0;
    double mem_ratio_smooth = 0.0;
    double gpu_ratio_smooth = 0.0;
    int smooth_alpha_percent = 22;
    int refresh_step_ms = 100;
    bool settings_open = false;
    int settings_selection = 0;

    std::string status_line;
    UiFrame frame;
    frame.refreshRate = std::chrono::milliseconds(1000);
    frame.processRefreshRate = kProcessRefreshInterval;
    frame.smoothAlphaPercent = smooth_alpha_percent;
    frame.refreshStepMs = refresh_step_ms;
    frame.settingsOpen = settings_open;
    frame.settingsSelection = settings_selection;
    auto last_stats_refresh = std::chrono::steady_clock::time_point::min();
    auto last_procs_refresh = std::chrono::steady_clock::time_point::min();
    bool needs_redraw = true;

    int selected_index = 0;
    int scroll_offset = 0;
    int selected_pid = -1;
    std::atomic<bool> procs_worker_stop{false};
    std::atomic<bool> procs_ready{false};
    std::atomic<bool> procs_request_now{true};
    std::atomic<long long> procs_interval_ms{kProcessRefreshInterval.count()};
    std::mutex procs_mtx;
    std::vector<ProcessInfo> latest_procs;
    std::string latest_filter = filter;

    std::thread procs_worker([&]() {
        auto last_fetch = std::chrono::steady_clock::time_point::min();
        while (!procs_worker_stop.load(std::memory_order_relaxed)) {
            const auto now_worker = std::chrono::steady_clock::now();
            bool should_fetch = procs_request_now.exchange(false, std::memory_order_relaxed);
            const auto interval = std::chrono::milliseconds(procs_interval_ms.load(std::memory_order_relaxed));

            if (!should_fetch && last_fetch != std::chrono::steady_clock::time_point::min() &&
                now_worker - last_fetch < interval) {
                std::this_thread::sleep_for(std::chrono::milliseconds(30));
                continue;
            }

            std::string filter_copy;
            {
                std::lock_guard<std::mutex> lock(procs_mtx);
                filter_copy = latest_filter;
            }

            auto procs = collectProcessesSortedByRss(filter_copy);
            {
                std::lock_guard<std::mutex> lock(procs_mtx);
                latest_procs = std::move(procs);
            }
            procs_ready.store(true, std::memory_order_relaxed);
            last_fetch = now_worker;
        }
    });

    while (g_running) {
        auto now = std::chrono::steady_clock::now();
        if (last_stats_refresh == std::chrono::steady_clock::time_point::min() ||
            now - last_stats_refresh >= frame.refreshRate) {

            const double cpu_usage = calculate_cpu_usage(prev_cpu);
            const InternetStats net_speed = calculate_internet_speed(prev_net, frame.refreshRate);
            const MemoryStats mem_stats = get_memory_stats();
            const GPUStats gpu_stats = get_gpu_stats();

            const double alpha = static_cast<double>(smooth_alpha_percent) / 100.0;
            cpu_usage_smooth = cpu_usage_smooth * (1.0 - alpha) + cpu_usage * alpha;
            {
                const unsigned long long totalKb = mem_stats.total_kb ? mem_stats.total_kb : 1ULL;
                const unsigned long long availKb = (mem_stats.available_kb <= totalKb) ? mem_stats.available_kb : 0ULL;
                const unsigned long long usedKb = totalKb - availKb;
                const double ratio = static_cast<double>(usedKb) / static_cast<double>(totalKb);
                mem_ratio_smooth = mem_ratio_smooth * (1.0 - alpha) + ratio * alpha;
            }
            {
                const int util = (gpu_stats.util_percent < 0) ? 0 : gpu_stats.util_percent;
                const double ratio = static_cast<double>(util) / 100.0;
                gpu_ratio_smooth = gpu_ratio_smooth * (1.0 - alpha) + ratio * alpha;
            }

            frame.cpuPercent = cpu_usage_smooth;
            frame.netSpeed = net_speed;
            frame.mem = mem_stats;
            frame.gpu = gpu_stats;
            frame.memUsedRatioSmooth = mem_ratio_smooth;
            frame.gpuUtilRatioSmooth = gpu_ratio_smooth;
            frame.selectedIndex = selected_index;
            frame.scrollOffset = scroll_offset;
            frame.statusLine = status_line;
            frame.filter = filter;
            frame.processRefreshRate = std::chrono::milliseconds(procs_interval_ms.load(std::memory_order_relaxed));
            frame.smoothAlphaPercent = smooth_alpha_percent;
            frame.refreshStepMs = refresh_step_ms;
            frame.settingsOpen = settings_open;
            frame.settingsSelection = settings_selection;

            last_stats_refresh = now;
            needs_redraw = true;
        }

        // Process list refresh is intentionally independent from +/- UI stats refresh
        // to keep navigation responsive even at large refreshRate values like 3200ms.
        // Keep process refresh separately configurable from UI refresh.
        if (procs_ready.exchange(false, std::memory_order_relaxed)) {
            if (!frame.procs.empty() && selected_index >= 0 && selected_index < static_cast<int>(frame.procs.size())) {
                selected_pid = frame.procs[selected_index].pid;
            }

            {
                std::lock_guard<std::mutex> lock(procs_mtx);
                frame.procs = latest_procs;
            }

            if (frame.procs.empty()) {
                selected_index = 0;
                selected_pid = -1;
            } else if (selected_pid > 0) {
                const auto it = std::find_if(frame.procs.begin(), frame.procs.end(),
                                             [selected_pid](const ProcessInfo& p) { return p.pid == selected_pid; });
                if (it != frame.procs.end()) {
                    selected_index = static_cast<int>(std::distance(frame.procs.begin(), it));
                } else if (selected_index >= static_cast<int>(frame.procs.size())) {
                    selected_index = static_cast<int>(frame.procs.size()) - 1;
                }
            } else if (selected_index >= static_cast<int>(frame.procs.size())) {
                selected_index = static_cast<int>(frame.procs.size()) - 1;
            }

            frame.selectedIndex = selected_index;
            last_procs_refresh = now;
            needs_redraw = true;
        }

        if (needs_redraw) {
            // update scroll window based on current terminal size
            int row = getmaxy(stdscr);
            int maxVisible = row - (8 + 5);
            if (maxVisible < 0) maxVisible = 0;
            if (selected_index < scroll_offset) scroll_offset = selected_index;
            if (selected_index >= scroll_offset + maxVisible) scroll_offset = selected_index - maxVisible + 1;
            frame.selectedIndex = selected_index;
            frame.scrollOffset = scroll_offset;
            frame.statusLine = status_line;
            frame.filter = filter;
            frame.processRefreshRate = std::chrono::milliseconds(procs_interval_ms.load(std::memory_order_relaxed));
            frame.smoothAlphaPercent = smooth_alpha_percent;
            frame.refreshStepMs = refresh_step_ms;
            frame.settingsOpen = settings_open;
            frame.settingsSelection = settings_selection;

            renderer.draw(frame);
            needs_redraw = false;
        }

        int ch = getch();
        if (ch == ERR || ch == KEY_MOUSE) {
            if (ch == KEY_MOUSE) flushinp();
            continue;
        }

        switch (ch) {
            case 'q':
                g_running = 0;
                break;
            case 's':
                settings_open = !settings_open;
                needs_redraw = true;
                break;
            case KEY_UP:
                if (settings_open) {
                    settings_selection = (settings_selection + 3) % 4;
                    needs_redraw = true;
                } else if (selected_index > 0) {
                    selected_index--;
                    if (!frame.procs.empty() && selected_index < static_cast<int>(frame.procs.size())) {
                        selected_pid = frame.procs[selected_index].pid;
                    }
                    needs_redraw = true;
                }
                break;
            case KEY_DOWN:
                if (settings_open) {
                    settings_selection = (settings_selection + 1) % 4;
                    needs_redraw = true;
                } else if (selected_index < static_cast<int>(frame.procs.size()) - 1) {
                    selected_index++;
                    if (!frame.procs.empty() && selected_index < static_cast<int>(frame.procs.size())) {
                        selected_pid = frame.procs[selected_index].pid;
                    }
                    needs_redraw = true;
                }
                break;
            case KEY_LEFT:
                if (settings_open) {
                    switch (settings_selection) {
                        case 0:
                            frame.refreshRate = std::chrono::milliseconds(std::max(100L, static_cast<long>(frame.refreshRate.count()) - refresh_step_ms));
                            break;
                        case 1: {
                            const long long v = procs_interval_ms.load(std::memory_order_relaxed);
                            procs_interval_ms.store(std::max<long long>(100, v - refresh_step_ms), std::memory_order_relaxed);
                            break;
                        }
                        case 2:
                            smooth_alpha_percent = std::max(5, smooth_alpha_percent - 1);
                            break;
                        case 3:
                            refresh_step_ms = std::max(50, refresh_step_ms - 50);
                            break;
                    }
                    needs_redraw = true;
                }
                break;
            case KEY_RIGHT:
                if (settings_open) {
                    switch (settings_selection) {
                        case 0:
                            frame.refreshRate = std::chrono::milliseconds(std::min(5000L, static_cast<long>(frame.refreshRate.count()) + refresh_step_ms));
                            break;
                        case 1: {
                            const long long v = procs_interval_ms.load(std::memory_order_relaxed);
                            procs_interval_ms.store(std::min<long long>(5000, v + refresh_step_ms), std::memory_order_relaxed);
                            break;
                        }
                        case 2:
                            smooth_alpha_percent = std::min(60, smooth_alpha_percent + 1);
                            break;
                        case 3:
                            refresh_step_ms = std::min(1000, refresh_step_ms + 50);
                            break;
                    }
                    needs_redraw = true;
                }
                break;
            case '+':
            case '=':
                if (settings_open) break;
                frame.refreshRate = std::chrono::milliseconds(std::max(100L, static_cast<long>(frame.refreshRate.count()) - refresh_step_ms));
                status_line = "Interval: " + std::to_string(frame.refreshRate.count()) + "ms";
                needs_redraw = true;
                break;
            case '-':
                if (settings_open) break;
                frame.refreshRate = std::chrono::milliseconds(std::min(5000L, static_cast<long>(frame.refreshRate.count()) + refresh_step_ms));
                status_line = "Interval: " + std::to_string(frame.refreshRate.count()) + "ms";
                needs_redraw = true;
                break;
            case '\n':
            case '\r':
            case 'k':
                if (settings_open) {
                    settings_open = false;
                    needs_redraw = true;
                    break;
                }
                if (!frame.procs.empty() && selected_index < static_cast<int>(frame.procs.size())) {
                    std::string pid_str = std::to_string(frame.procs[selected_index].pid);
                    if (terminateProcess(pid_str)) status_line = "SIGTERM sent to PID " + pid_str;
                    else status_line = "Failed to terminate PID " + pid_str;
                    procs_request_now.store(true, std::memory_order_relaxed);
                    needs_redraw = true;
                }
                break;
            case 'f': {
                if (settings_open) break;
                const int r = getmaxy(stdscr);
                mvprintw(r - 1, 2, "Set Filter: ");
                clrtoeol();
                filter = get_user_input(r - 1, 14);
                selected_index = 0;
                scroll_offset = 0;
                selected_pid = -1;
                {
                    std::lock_guard<std::mutex> lock(procs_mtx);
                    latest_filter = filter;
                }
                procs_request_now.store(true, std::memory_order_relaxed);
                needs_redraw = true;
                break;
            }
        }
    }

    procs_worker_stop.store(true, std::memory_order_relaxed);
    if (procs_worker.joinable()) procs_worker.join();

    return 0;
}

void App::handleSignal(int) {}

} // namespace procsight

