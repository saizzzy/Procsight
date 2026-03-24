#pragma once

namespace procsight {

class Theme {
public:
    Theme();

    short pairFrame() const { return kPairFrame; }
    short pairOk() const { return kPairOk; }
    short pairWarn() const { return kPairWarn; }
    short pairHot() const { return kPairHot; }
    short pairAccent() const { return kPairAccent; }

    short pairForPercent(double percent01) const; // 0..1
    bool hasGradient() const { return has256_; }

private:
    void initPalette();

    static constexpr short kPairFrame = 1;
    static constexpr short kPairOk = 2;
    static constexpr short kPairWarn = 3;
    static constexpr short kPairHot = 4;
    static constexpr short kPairAccent = 5;

    static constexpr short kPairGradStart = 10;
    static constexpr int kGradSteps = 10;

    bool has256_ = false;
};

} // namespace procsight

