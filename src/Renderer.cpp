#include "../include//Renderer.hpp"

#include <algorithm>
#include <cstring>

#include <curses.h>

namespace procsight {

Renderer::Renderer(const Theme& theme) : theme_(theme) {}

void Renderer::drawMiniBar(int y, int x, int width, double ratio01, short pairFilled, short pairEmpty) {
    if (width <= 0) return;
    if (ratio01 < 0.0) ratio01 = 0.0;
    if (ratio01 > 1.0) ratio01 = 1.0;
    const int filled = static_cast<int>(ratio01 * width + 0.5);
    for (int i = 0; i < width; ++i) {
        if (i < filled) {
            attron(COLOR_PAIR(pairFilled));
            mvaddch(y, x + i, ' ' | A_REVERSE);
            attroff(COLOR_PAIR(pairFilled));
        } else {
            attron(COLOR_PAIR(pairEmpty));
            mvaddch(y, x + i, '.');
            attroff(COLOR_PAIR(pairEmpty));
        }
    }
}

void Renderer::drawBar(int y, int x, int width, double percent) {
    if (percent < 0.0) percent = 0.0;
    if (percent > 100.0) percent = 100.0;

    double filled_width = (width * percent) / 100.0;
    mvprintw(y, x, "[");
    for (int i = 0; i < width; ++i) {
        if (i < filled_width) {
            const short pair = theme_.pairForPercent(percent / 100.0);
            attron(COLOR_PAIR(pair));
            mvaddch(y, x + 1 + i, ' ' | A_REVERSE);
            attroff(COLOR_PAIR(pair));
        } else {
            attron(COLOR_PAIR(theme_.pairFrame()) | A_DIM);
            mvaddch(y, x + 1 + i, '.');
            attroff(COLOR_PAIR(theme_.pairFrame()) | A_DIM);
        }
    }
    mvprintw(y, x + width + 1, "] %.1f%%", percent);
}

void Renderer::draw(const UiFrame& f) {
    int row, col;
    getmaxyx(stdscr, row, col);
    erase();

    attron(COLOR_PAIR(theme_.pairFrame()));
    box(stdscr, 0, 0);
    mvprintw(0, 2, "[ PROCSIGHT v1.4 | UI:%ldms PROC:%ldms ]",
             f.refreshRate.count(),
             f.processRefreshRate.count());
    attroff(COLOR_PAIR(theme_.pairFrame()));

    mvprintw(1, 2, "CPU:");
    drawBar(1, 7, 30, f.cpuPercent);

    // MEM
    {
        const unsigned long long totalKb = f.mem.total_kb ? f.mem.total_kb : 1ULL;
        const unsigned long long availKb = (f.mem.available_kb <= totalKb) ? f.mem.available_kb : 0ULL;
        const unsigned long long usedKb = totalKb - availKb;
        const double usedRatio = f.memUsedRatioSmooth;

        const int ramBoxW = 32;
        const int ramBoxX = std::max(2, col - ramBoxW - 2);
        attron(COLOR_PAIR(theme_.pairFrame()));
        mvprintw(1, ramBoxX, "MEM:");
        attroff(COLOR_PAIR(theme_.pairFrame()));

        const int barW = 14;
        const int barX = ramBoxX + 5;
        const short filledPair = theme_.pairForPercent(usedRatio);
        drawMiniBar(1, barX, barW, usedRatio, filledPair, theme_.pairFrame());

        const double usedGb = static_cast<double>(usedKb) / (1024.0 * 1024.0);
        const double totalGb = static_cast<double>(totalKb) / (1024.0 * 1024.0);
        attron(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
        mvprintw(1, barX + barW + 1, "%4.1f/%4.1fG", usedGb, totalGb);
        attroff(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
    }

    // GPU
    {
        const int gpuBoxW = 32;
        const int gpuBoxX = std::max(2, col - gpuBoxW - 2);
        attron(COLOR_PAIR(theme_.pairFrame()));
        mvprintw(2, gpuBoxX, "GPU:");
        attroff(COLOR_PAIR(theme_.pairFrame()));

        if (!f.gpu.available) {
            attron(COLOR_PAIR(theme_.pairWarn()));
            mvprintw(2, gpuBoxX + 5, "n/a");
            attroff(COLOR_PAIR(theme_.pairWarn()));
        } else {
            const int util = (f.gpu.util_percent < 0) ? 0 : f.gpu.util_percent;
            const double utilRatio = f.gpuUtilRatioSmooth;
            const int barW = 14;
            const int barX = gpuBoxX + 5;
            const short filledPair = theme_.pairForPercent(utilRatio);
            drawMiniBar(2, barX, barW, utilRatio, filledPair, theme_.pairFrame());

            if (f.gpu.mem_total_mb > 0) {
                mvprintw(2, barX + barW + 1, "%3d%% %4llu/%4lluMB",
                         util,
                         static_cast<unsigned long long>(f.gpu.mem_used_mb),
                         static_cast<unsigned long long>(f.gpu.mem_total_mb));
            } else {
                mvprintw(2, barX + barW + 1, "%3d%%", util);
            }
        }
    }

    // NET
    {
        std::string rx_str = format_speed(f.netSpeed.rx_bytes);
        std::string tx_str = format_speed(f.netSpeed.tx_bytes);

        attron(COLOR_PAIR(theme_.pairFrame()));
        const int netX = 5;
        mvprintw(2, netX, "┌── download ┐");
        mvprintw(3, netX, "▼ %-10s", rx_str.c_str());
        mvprintw(5, netX, "▲ %-10s", tx_str.c_str());
        mvprintw(6, netX, "└─ upload ───┘");
        attroff(COLOR_PAIR(theme_.pairFrame()));
    }

    int table_start_row = 8;
    mvhline(table_start_row - 1, 1, ACS_HLINE, std::max(0, col - 2));

    attron(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
    mvprintw(table_start_row, 3, "%-8s %-10s %-20s %-20s", "PID", "RAM(MB)", "RAM GRAPH", "COMMAND");
    attroff(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);

    mvhline(table_start_row + 1, 1, ACS_HLINE, std::max(0, col - 2));

    // process summary
    long maxRssKb = 1;
    long long totalRssKb = 0;
    for (const auto& p : f.procs) {
        if (p.rss_kb > maxRssKb) maxRssKb = p.rss_kb;
        totalRssKb += p.rss_kb;
    }
    const double totalProcMb = static_cast<double>(totalRssKb) / 1024.0;

    int maxVisible = row - (table_start_row + 5);
    if (maxVisible < 0) maxVisible = 0;

    const int graphWidth = 18;
    int displayCount = 0;
    for (int i = f.scrollOffset; i < static_cast<int>(f.procs.size()) && displayCount < maxVisible; ++i) {
        const double memMb = static_cast<double>(f.procs[i].rss_kb) / 1024.0;
        const double ratio = (maxRssKb > 0) ? (static_cast<double>(f.procs[i].rss_kb) / static_cast<double>(maxRssKb)) : 0.0;

        if (i == f.selectedIndex) {
            attron(A_REVERSE | COLOR_PAIR(theme_.pairOk()));
            mvhline(table_start_row + 2 + displayCount, 1, ' ', std::max(0, col - 2));
        }
        const int y = table_start_row + 2 + displayCount;
        mvprintw(y, 3, "%-8d %-10.1f ", f.procs[i].pid, memMb);
        const int barX = 3 + 8 + 1 + 10 + 1;

        const short filledPair = theme_.pairForPercent(ratio);
        drawMiniBar(y, barX, graphWidth, ratio, filledPair, theme_.pairFrame());
        mvprintw(y, barX + graphWidth + 1, "%-20s", f.procs[i].name.c_str());
        if (i == f.selectedIndex) attroff(A_REVERSE | COLOR_PAIR(theme_.pairOk()));
        displayCount++;
    }

    mvhline(row - 4, 1, ACS_HLINE, std::max(0, col - 2));
    // Pretty "button-like" controls
    int controlsY = row - 3;
    int x = 2;
    auto draw_btn = [&](const char* key, const char* text) {
        attron(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
        mvprintw(controlsY, x, "[%s]", key);
        attroff(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
        x += static_cast<int>(std::strlen(key)) + 2;
        attron(COLOR_PAIR(theme_.pairFrame()));
        mvprintw(controlsY, x, " %s  ", text);
        attroff(COLOR_PAIR(theme_.pairFrame()));
        x += static_cast<int>(std::strlen(text)) + 3;
    };
    draw_btn("Arrows", "Navigate");
    draw_btn("Enter/k", "Kill");
    draw_btn("f", "Filter");
    draw_btn("+/-", "UI speed");
    draw_btn("s", "Settings");
    draw_btn("q", "Quit");

    if (!f.statusLine.empty()) {
        attron(COLOR_PAIR(theme_.pairWarn()));
        mvprintw(row - 1, 2, " STATUS: %s ", f.statusLine.c_str());
        attroff(COLOR_PAIR(theme_.pairWarn()));
    } else {
        attron(COLOR_PAIR(theme_.pairFrame()));
        mvprintw(row - 1, 2, " Proc RSS sum: %.1f MB | smooth:%d%% | step:%dms ",
                 totalProcMb, f.smoothAlphaPercent, f.refreshStepMs);
        attroff(COLOR_PAIR(theme_.pairFrame()));
    }

    if (f.settingsOpen) {
        const int w = std::min(64, std::max(40, col - 8));
        const int h = 9;
        const int sx = (col - w) / 2;
        const int sy = (row - h) / 2;
        attron(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
        for (int yy = sy; yy < sy + h; ++yy) {
            mvhline(yy, sx, ' ', w);
        }
        attroff(COLOR_PAIR(theme_.pairAccent()) | A_BOLD);
        attron(COLOR_PAIR(theme_.pairFrame()) | A_BOLD);
        mvprintw(sy, sx + 2, " SETTINGS ");
        attroff(COLOR_PAIR(theme_.pairFrame()) | A_BOLD);

        const char* labels[4] = {
            "UI refresh (ms)",
            "Process refresh (ms)",
            "Smoothing alpha (%)",
            "Speed step (ms)"
        };
        const long values[4] = {
            f.refreshRate.count(),
            f.processRefreshRate.count(),
            f.smoothAlphaPercent,
            f.refreshStepMs
        };
        for (int i = 0; i < 4; ++i) {
            if (i == f.settingsSelection) attron(A_REVERSE | COLOR_PAIR(theme_.pairOk()));
            mvprintw(sy + 2 + i, sx + 2, "%-22s : %4ld", labels[i], values[i]);
            if (i == f.settingsSelection) attroff(A_REVERSE | COLOR_PAIR(theme_.pairOk()));
        }
        attron(COLOR_PAIR(theme_.pairFrame()));
        mvprintw(sy + h - 2, sx + 2, "Arrows: select/change  |  Enter/s: close");
        attroff(COLOR_PAIR(theme_.pairFrame()));
    }

    refresh();
}

} // namespace procsight

