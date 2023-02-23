#include <chrono>
#include <thread>
#include <clocale>
#include <stack>
#include <ctime>
#include <algorithm>
#include <curses.h>
#include <random>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <ncurses.h>

class MineField {
    public:
        enum Color {
            HIDDEN = 1, // Covered field
            EMPTY,      // Zero field
            NUMBER,     // Number
            FLAG,       // Flag color
            MINE,       // Exploded mine
        };
        MineField(int w, int h, int mines) :  width(w), height(h) {
            win = newwin(height + 2, width + 2, win_offset_y, win_offset_x);
            box(win, 0, 0);
            refresh();

            field.resize(height + 2);
            for (auto& row : field) {
                row.resize(width + 2);
            }

            plantMines(mines);

            // Init colors
            init_pair(Color::HIDDEN, COLOR_BLACK, COLOR_WHITE); // reversed
            init_pair(Color::EMPTY,  COLOR_BLACK, COLOR_BLACK); // standard
            init_pair(Color::NUMBER, COLOR_MAGENTA,  COLOR_BLACK);
            init_pair(Color::FLAG,   COLOR_BLUE,  COLOR_CYAN);
            init_pair(Color::MINE,   COLOR_WHITE, COLOR_RED);
        }

        void draw() {
            for (int r = 1; r < height + 1; ++r) {
                for (int c = 1; c < width + 1; ++c) {
                    mvwaddch(win, r, c, field[r][c].ch() | COLOR_PAIR(field[r][c].chattr()));
                }
            }
            wrefresh(win);
        }

        void user_LMB(int x, int y) {
            bool valid = mouse_windowpos(x, y);
            if (!valid) return;

            auto& s = field[y][x];

            if (s.clear && s.num > 0) {
                autoDig(x, y);
                return;
            }

            if (s.clear || s.flag || s.quest) return;
            if (s.mine) {
                GameOver();
                return;
            }

            // Good dig square
            if (s.num == 0) {
                // Trigger flood dig
                flood_dig(x, y);
            }
            else {
                s.cleared();
            }
            draw();
        }

        void user_MMB(int x, int y) {
            bool valid = mouse_windowpos(x, y);
            if (!valid) return;

            field[y][x].quest = 1 - field[y][x].quest;
            draw();
        }

        void user_RMB(int x, int y)
        {
            bool valid = mouse_windowpos(x, y);
            if (!valid) return;

            auto& s = field[y][x];
            if (s.clear && s.num > 0) {
                autoFlag(x, y);
            }
            else if (s.clear == 0) {
                s.flag = s.flag ? 0 : 1;
            }
            draw();
        }

        void draw_truth() {
            for (int r = 1; r < height + 1; ++r) {
                for (int c = 1; c < width + 1; ++c) {
                    mvwaddch(win, r, c, field[r][c].ch_truth() | COLOR_PAIR(field[r][c].chattr_truth()));
                }
            }
            wrefresh(win);
        }
        WINDOW* win;
    private:
        int win_offset_x = 0;
        int win_offset_y = 0;
        struct Square {
            Square() {
                
            }
            std::uint8_t mine  : 1 {0},
                         flag  : 1 {0},
                         clear : 1 {0},
                         quest : 1 {0},
                         num   : 4 {0};

            void inc() {
                num = num < 9 ? num + 1 : 8;
            }

            void dec() {
                num = num == 0 ? 0 : num - 1;
            }

            void inline cleared() {
                clear = 1;
                quest = flag = 0;
            }

            char ch() {
                if (clear) {
                    if (num > 0) {
                        return '0' + num;
                    }
                    return '~';
                }
                else {
                    if (flag) {
                        return '+';
                    }
                    else if (quest) {
                        return '?';
                    }
                    else {
                        return ' ';
                    }
                }
            }

            char ch_truth() {
                if (mine) {
                    return '*';
                }
                if (flag) {
                    return '+';
                }
                if (num > 0) {
                    return '0' + num;
                }
                if (quest) {
                    return '?';
                }
                return ' ';
            }

            attr_t chattr() {
                if (clear) {
                    if (num > 0) {
                        return Color::NUMBER;
                    }
                    return Color::EMPTY;
                }
                else {
                    if (flag) return Color::FLAG | A_UNDERLINE;
                    return Color::HIDDEN;
                }
            }

            attr_t chattr_truth() {
                if (clear) {
                    if (num > 0) {
                        return Color::NUMBER;
                    }
                    return Color::EMPTY;
                }
                else {
                    if (flag) return Color::FLAG | A_UNDERLINE;
                    if (mine) return Color::MINE | A_BLINK | A_BOLD;
                    return Color::HIDDEN;
                }
            }
        };

        bool validPos(int x, int y) const {
            if (x < 1 || y < 1) return false;
            if (x > width || y > height) return false;
            return true;
        }

        int countFlagsAt(int x, int y) {
            // This function is not called if user tries to dig flag
            int nf = 0;
            for (int i = y - 1; i < y + 2; ++i) {
                for (int j = x - 1; j < x + 2; ++j) {
                    if (validPos(j, i)){
                        nf += field[i][j].flag;
                    }
                }
            }
            return nf;
        }

        int countCovered(int x, int y) {
            // This function is not called if user tries to dig flag
            int nc = 0;
            for (int i = y - 1; i < y + 2; ++i) {
                for (int j = x - 1; j < x + 2; ++j) {
                    if (validPos(j, i)){
                        nc += 1 - field[i][j].clear;
                    }
                }
            }
            return nc;
        }

        void autoDig(int x, int y) {
            // Reveal squares around a given number if it is satisfied by flags
            auto nf = countFlagsAt(x, y);
            if (nf == field[y][x].num) {
                for (int i = y - 1; i < y + 2; ++i) {
                    for (int j = x - 1; j < x + 2; ++j) {
                        if (!validPos(j, i)) continue;
                        auto& s = field[i][j];
                        if (s.mine && s.flag == 0) {
                            // Game over by auto dig
                            GameOver();
                            return;
                        }
                        if (s.clear == 0 && s.flag == 0) {
                            if (s.num == 0) {
                                s.cleared();
                                flood_dig(j, i);
                            }
                            else {
                                s.cleared();
                            }
                        }
                    }
                }
                draw();
            }
        }

        void autoFlag(int x, int y) {
            auto nc = countCovered(x, y);
            if (nc == field[y][x].num) {
                for (int i = y - 1; i < y + 2; ++i) {
                    for (int j = x - 1; j < x + 2; ++j) {
                        auto& s = field[i][j];
                        if (s.clear == 0 && s.flag == 0) {
                            s.flag = 1;
                        }
                    }
                }
                draw();
            }

        }

        void setObviousFlags(int x, int y) {
            // Reveal squares around a given number if it is satisfied by flags
            auto nf = countFlagsAt(x, y);

        }

        void GameOver() {
            printw("GAME OVER");
            draw_truth();
        }

        bool mouse_windowpos(int& x, int& y) const {
            x -= win_offset_x;
            y -= win_offset_y;
            return validPos(x, y);
        }

        void plantMines(int amt) {
            assert(amt > 0);
            assert(amt <= width * height);

            std::vector<bool> m(width * height, false);
            std::fill(m.begin(), m.begin() + amt, true);
            std::shuffle(std::begin(m), std::end(m), rng);

            for (std::size_t i = 0; i < m.size(); ++i) {
                if (m[i]) {
                    field[i / width + 1][i % width + 1].mine = 1;
                    updateMineValue(i / width + 1, i % width + 1);
                }
            }
        }

        void updateMineValue(int x, int y)
        {
            field[x - 1][y - 1].inc();
            field[x - 1][y].inc();
            field[x - 1][y + 1].inc();
            field[x][y - 1].inc();
            field[x][y + 1].inc();
            field[x + 1][y - 1].inc();
            field[x + 1][y].inc();
            field[x + 1][y + 1].inc();
        }

        void flood_dig(int startx, int starty)
        {
            std::stack<std::pair<int, int>> visit;
            visit.push({startx, starty});
            field[starty][startx].cleared();

            while (!visit.empty()) {
                auto [x, y] = visit.top();
                auto& s = field[y][x];
                visit.pop();

                if (s.num == 0) {
                    // Add new squares to flood fill
                    for (int i = y - 1; i < y + 2; ++i) {
                        for (int j = x - 1; j < x + 2; ++j) {
                            // Check position validity
                            if (validPos(j, i)) {
                                if (field[i][j].clear == 0) {
                                    field[i][j].cleared();
                                    visit.push({j, i});
                                }
                            }
                        }
                    }
                }
            }
        }

        std::vector<std::vector<Square>> field;

        int width;
        int height;
    
        std::mt19937 rng{(long unsigned int)time(0)};

};

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
    //  srand(time(0));
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
