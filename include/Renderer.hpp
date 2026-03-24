#pragma once

#include <chrono>
#include <string>
#include <vector>

#include "ProcessList.hpp"
#include "SystemStats.hpp"
#include "../include/Theme.hpp"

namespace procsight {

struct UiFrame {
    double cpuPercent = 0.0;
    InternetStats netSpeed{0.0, 0.0};
    MemoryStats mem{0, 0};
    GPUStats gpu{false, -1, 0, 0};
    double memUsedRatioSmooth = 0.0; // 0..1
    double gpuUtilRatioSmooth = 0.0; // 0..1
    std::vector<ProcessInfo> procs;
    int selectedIndex = 0;
    int scrollOffset = 0;
    std::string statusLine;
    std::string filter;
    std::chrono::milliseconds refreshRate{1000};
    std::chrono::milliseconds processRefreshRate{700};
    int smoothAlphaPercent = 22; // 0..100
    int refreshStepMs = 100;
    bool settingsOpen = false;
    int settingsSelection = 0;
};

class Renderer {
public:
    explicit Renderer(const Theme& theme);

    void draw(const UiFrame& frame);

private:
    void drawBar(int y, int x, int width, double percent);
    void drawMiniBar(int y, int x, int width, double ratio01, short pairFilled, short pairEmpty);

    const Theme& theme_;
};

} // namespace procsight

