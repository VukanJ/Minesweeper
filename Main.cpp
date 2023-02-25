#include <curses.h>

#include "MineField.hpp"

void init_ncurses()
{
    initscr();
    clear();
    noecho();
    nonl();
    curs_set(0);
    cbreak();
    mouseinterval(0);

    mousemask(ALL_MOUSE_EVENTS, NULL);
    keypad(stdscr, true);
    start_color();
}

void shutdown_ncurses()
{
    endwin();
}

int main()
{
    setlocale(LC_ALL, "");

    init_ncurses();

    // MineField mf(60, 20, 170);
    MineField mf(30, 10, 30);
    mf.draw();

    move(0,  0);

    refresh();

    MEVENT mevent;

    while(true) {
        int c = getch();
        if (c == KEY_MOUSE) {
            if (getmouse(&mevent) == OK) {
                if (mevent.bstate & BUTTON1_PRESSED) {
                    mf.user_LMB(mevent.x, mevent.y);
                }
                else if (mevent.bstate & BUTTON2_PRESSED) {
                    mf.user_MMB(mevent.x, mevent.y);
                }
                else if (mevent.bstate & BUTTON3_PRESSED) {
                    mf.user_RMB(mevent.x, mevent.y);
                }
            }
        }
    }

    shutdown_ncurses();
}
