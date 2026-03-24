#include "../include/Theme.hpp"

#include <algorithm>
#include <iterator>

#include <curses.h>

namespace procsight {
namespace {
short clamp1000(int v) {
    if (v < 0) return 0;
    if (v > 1000) return 1000;
    return static_cast<short>(v);
}
} // namespace

Theme::Theme() {
    initPalette();
}

short Theme::pairForPercent(double percent01) const {
    if (percent01 < 0.0) percent01 = 0.0;
    if (percent01 > 1.0) percent01 = 1.0;

    if (has256_) {
        int idx = static_cast<int>(percent01 * (kGradSteps - 1) + 0.5);
        idx = std::clamp(idx, 0, kGradSteps - 1);
        return static_cast<short>(kPairGradStart + idx);
    }

    const double pct = percent01 * 100.0;
    return (pct < 50.0) ? kPairOk : (pct < 80.0) ? kPairWarn : kPairHot;
}

void Theme::initPalette() {
    use_default_colors();

    has256_ = (COLORS >= 256);

    init_pair(kPairFrame, COLOR_CYAN, -1);
    init_pair(kPairOk, COLOR_GREEN, -1);
    init_pair(kPairWarn, COLOR_YELLOW, -1);
    init_pair(kPairHot, COLOR_RED, -1);
    init_pair(kPairAccent, COLOR_MAGENTA, -1);

    if (!has256_ || !can_change_color()) return;

    const short cFrame = 37;   // soft cyan
    const short cAccent = 141; // purple accent

    init_color(cFrame, 200, 820, 820);
    init_color(cAccent, 720, 520, 980);

    init_pair(kPairFrame, cFrame, -1);
    init_pair(kPairAccent, cAccent, -1);

    constexpr short base = 200;
    struct RGB { int r, g, b; };
    const RGB stops[] = {
        {120, 900, 780},
        {180, 720, 980},
        {420, 520, 980},
        {720, 520, 980},
        {980, 520, 820},
    };
    auto lerp = [](int a, int b, double t) { return static_cast<int>(a + (b - a) * t + 0.5); };

    for (int i = 0; i < kGradSteps; ++i) {
        const double t = (kGradSteps == 1) ? 0.0 : (static_cast<double>(i) / (kGradSteps - 1));
        const double seg = t * (static_cast<double>(std::size(stops) - 1));
        const int s0 = static_cast<int>(seg);
        const int s1 = std::min<int>(s0 + 1, static_cast<int>(std::size(stops) - 1));
        const double lt = seg - s0;
        const int r = lerp(stops[s0].r, stops[s1].r, lt);
        const int g = lerp(stops[s0].g, stops[s1].g, lt);
        const int b = lerp(stops[s0].b, stops[s1].b, lt);
        init_color(base + i, clamp1000(r), clamp1000(g), clamp1000(b));
        init_pair(static_cast<short>(kPairGradStart + i), static_cast<short>(base + i), -1);
    }
}

} // namespace procsight

