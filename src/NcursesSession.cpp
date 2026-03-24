#include "../include/NcursesSession.hpp"

#include <curses.h>

namespace procsight {

NcursesSession::NcursesSession() {
    initscr();
    start_color();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
}

NcursesSession::~NcursesSession() {
    endwin();
}

} // namespace procsight

