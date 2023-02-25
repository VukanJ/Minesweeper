#ifndef MINEFIELD_HPP
#define MINEFIELD_HPP

#include <ncurses.h>
#include <vector>
#include <chrono>
#include <random>
#include <cassert>
#include <algorithm>
#include <stack>


class MineField {
    public:
        enum Color {
            HIDDEN = 1, // Covered field
            EMPTY,      // Zero field
            NUMBER,     // Number
            FLAG,       // Flag color
            MINE,       // Exploded mine
        };
        MineField(int w, int h, int mines);

        void draw() const;
        void draw_truth() const;
        void user_LMB(int x, int y);
        void user_MMB(int x, int y);
        void user_RMB(int x, int y);

        WINDOW* win;
    private:
        int win_offset_x = 0;
        int win_offset_y = 0;
        struct Square {
            std::uint8_t mine  : 1 {0},
                         flag  : 1 {0},
                         clear : 1 {0},
                         quest : 1 {0},
                         num   : 4 {0};

            void inc();
            void dec();
            void inline cleared();
            char ch() const;
            char ch_truth() const;
            attr_t chattr() const;
            attr_t chattr_truth() const;
        };

        bool validPos(int x, int y) const;
        int countFlagsAt(int x, int y) const;
        int countCovered(int x, int y) const;
        void autoDig(int x, int y);
        void autoFlag(int x, int y);
        void GameOver() const;
        bool mouse_windowpos(int& x, int& y) const;
        void plantMines(int amt);
        void updateMineValue(int x, int y);
        void flood_dig(int startx, int starty);

        int width;
        int height;
    
        std::vector<std::vector<Square>> field;
        std::mt19937 rng{(long unsigned int)time(0)};
};

#endif // MINEFIELD_HPP
