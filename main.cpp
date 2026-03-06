#include <curses.h>
#include <clocale>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include "SystemStats.hpp"
#include "ProcessList.hpp"

// Вынес отрисовку бара сюда, чтобы не загромождать
void draw_bar(int y, int x, int width, double percentage) {
    int filled_width = (width * percentage) / 100.0;
    mvprintw(y, x, "[");
    for (int i = 0; i < width; ++i) {
        if (i < filled_width) {
            int pair = (percentage < 50) ? 2 : (percentage < 80) ? 3 : 4;
            attron(COLOR_PAIR(pair));
            mvaddch(y, x + 1 + i, ' ' | A_REVERSE);
            attroff(COLOR_PAIR(pair));
        } else {
            attron(COLOR_PAIR(1));
            mvaddch(y, x + 1 + i, '.');
            attroff(COLOR_PAIR(1));
        }
    }
    mvprintw(y, x + width + 1, "] %.1f%%", percentage);
}

void init_styles() {
    initscr();
    start_color();
    noecho();
    curs_set(0);
    timeout(1000);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_RED, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
}

int main() {
    setlocale(LC_ALL, "");
    init_styles();

    std::string filter = "";
    CPUStats prev_cpu = get_cpu_times();

    while (true) {
        int row, col;
        getmaxyx(stdscr, row, col);
        erase();

        attron(COLOR_PAIR(1));
        box(stdscr, 0, 0);
        mvprintw(0, 2, " PROCSIGHT v1.0 [Arch Linux] ");
        attroff(COLOR_PAIR(1));

        draw_bar(1, 13, 20, calculate_cpu_usage(prev_cpu));

        attron(COLOR_PAIR(5) | A_BOLD);
        mvprintw(3, 2, "%-8s %-12s %-20s", "PID", "RAM (MB)", "COMMAND");
        attroff(COLOR_PAIR(5) | A_BOLD);
        mvhline(4, 1, ACS_HLINE, col - 2);

        // Сбор данных процессов прямо здесь для простоты
        std::vector<ProcessInfo> procs;
        try {
            for (const auto& entry : std::filesystem::directory_iterator("/proc")) {
                std::string pid = entry.path().filename().string();
                if (std::all_of(pid.begin(), pid.end(), ::isdigit)) {
                    std::ifstream commFile("/proc/" + pid + "/comm");
                    std::string name; std::getline(commFile, name);
                    std::ifstream statmFile("/proc/" + pid + "/statm");
                    long size, rss;
                    if (statmFile >> size >> rss && !name.empty()) {
                        if (filter.empty() || name.find(filter) != std::string::npos)
                            procs.push_back({pid, name, rss});
                    }
                }
            }
        } catch (...) {}

        std::sort(procs.begin(), procs.end(), [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.rss > b.rss;
        });

        for (int i = 0; i < (int)procs.size() && i < (row - 7); ++i) {
            double memMb = (procs[i].rss * 4096.0) / (1024 * 1024);
            mvprintw(5 + i, 2, "%-8s %-12.1f %-20s", procs[i].pid.c_str(), memMb, procs[i].name.c_str());
        }

        mvprintw(row - 2, 2, "[f] Filter | [k] Kill | [a] Kill All | [q] Quit");
        refresh();

        int ch = getch();
        if (ch == 'q') break;
        if (ch == 'f') {
            echo(); curs_set(1);
            mvprintw(row - 1, 2, "Set Filter: ");
            char buf[32]; mvgetnstr(row - 1, 14, buf, 31);
            filter = buf;
            noecho(); curs_set(0);
        }
        // ... аналогично k и a ...
    }
    endwin();
    return 0;
}