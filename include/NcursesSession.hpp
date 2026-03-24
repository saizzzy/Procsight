#pragma once

namespace procsight {

class NcursesSession {
public:
    NcursesSession();
    ~NcursesSession();

    NcursesSession(const NcursesSession&) = delete;
    NcursesSession& operator=(const NcursesSession&) = delete;
};

} // namespace procsight

