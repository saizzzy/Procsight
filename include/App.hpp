#pragma once

#include <chrono>
#include <string>

namespace procsight {

class App {
public:
    App();
    int run();

private:
    void handleSignal(int sig);
};

} // namespace procsight

