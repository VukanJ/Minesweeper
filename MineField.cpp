#include "MineField.hpp"


MineField::MineField(int w, int h, int mines) 
    : width(w), height(h)
{
    win = newwin(height + 2, width + 2, win_offset_y, win_offset_x);
    box(win, 0, 0);
    refresh();

    field.resize(height + 2);
    for (auto& row : field) {
        row.resize(width + 2);
    }

    totalMines = mines;
    minesLeft = mines;
    numFlags = 0;

    plantMines(mines);

    // Init colors
    init_pair(Color::HIDDEN, COLOR_BLACK, COLOR_WHITE); // reversed
    init_pair(Color::EMPTY,  COLOR_BLACK,   COLOR_BLACK); // standard
    init_pair(Color::NUMBER, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Color::FLAG,   COLOR_BLUE,    COLOR_CYAN);
    init_pair(Color::MINE,   COLOR_WHITE,   COLOR_RED);
    init_pair(Color::WIN,    COLOR_GREEN,   COLOR_BLACK);
}

void MineField::draw() const
{
    for (int r = 1; r < height + 1; ++r) {
        for (int c = 1; c < width + 1; ++c) {
            mvwaddch(win, r, c, field[r][c].ch() | COLOR_PAIR(field[r][c].chattr()));
        }
    }
    printStatus();
    wrefresh(win);
}

void MineField::user_LMB(int x, int y)
{
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

void MineField::user_MMB(int x, int y)
{
    bool valid = mouse_windowpos(x, y);
    if (!valid) return;

    field[y][x].quest = 1 - field[y][x].quest;
    draw();
}

void MineField::user_RMB(int x, int y)
{
    bool valid = mouse_windowpos(x, y);
    if (!valid) return;

    auto& s = field[y][x];
    if (s.clear && s.num > 0) {
        autoFlag(x, y);
    }
    else if (s.clear == 0) {
        numFlags -= 2 * s.flag - 1;
        s.flag = s.flag ? 0 : 1;
    }
    draw();
}

void MineField::draw_truth() const
{
    for (int r = 1; r < height + 1; ++r) {
        for (int c = 1; c < width + 1; ++c) {
            mvwaddch(win, r, c, field[r][c].ch_truth() | COLOR_PAIR(field[r][c].chattr_truth()));
        }
    }
    wrefresh(win);
}

bool MineField::validPos(int x, int y) const
{
    if (x < 1 || y < 1) return false;
    if (x > width || y > height) return false;
    return true;
}

int MineField::countFlagsAt(int x, int y) const
{
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

int MineField::countCovered(int x, int y) const
{
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

void MineField::autoDig(int x, int y)
{
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
                        // s.cleared();
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

void MineField::autoFlag(int x, int y)
{
    auto nc = countCovered(x, y);
    if (nc == field[y][x].num) {
        for (int i = y - 1; i < y + 2; ++i) {
            for (int j = x - 1; j < x + 2; ++j) {
                if (validPos(j, i)) {
                auto& s = field[i][j];
                    if (s.clear == 0 && s.flag == 0) {
                        numFlags += 1;
                        s.flag = 1;
                    }
                }
            }
        }
        draw();
    }

}

void MineField::GameOver() const
{
    printw("GAME OVER");
    draw_truth();
}

bool MineField::mouse_windowpos(int& x, int& y) const
{
    x -= win_offset_x;
    y -= win_offset_y;
    return validPos(x, y);
}

void MineField::plantMines(int amt)
{
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

void MineField::updateMineValue(int x, int y)
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

void MineField::flood_dig(int startx, int starty)
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
                        if (field[i][j].clear == 0 && field[i][j].flag == 0) {
                            field[i][j].cleared();
                            visit.push({j, i});
                        }
                    }
                }
            }
        }
    }
}


void MineField::Square::inc()
{
    num = num < 9 ? num + 1 : 8;
}

void MineField::Square::dec()
{
    num = num == 0 ? 0 : num - 1;
}

void MineField::Square::cleared()
{
    clear = 1;
    quest = flag = 0;
}

char MineField::Square::ch() const
{
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
            return '`';
        }
    }

}

char MineField::Square::ch_truth() const
{
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

attr_t MineField::Square::chattr() const
{
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

attr_t MineField::Square::chattr_truth() const
{
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

void MineField::printStatus() const
{
    mvprintw(height + 2, 0, "Mines left: %i", minesLeft - numFlags);
    clrtoeol();

    // Win condition = Number of covered cells == number of mines
    int covered = 0;
    for (int r = 1; r < height + 1; ++r) {
        for (int c = 1; c < width + 1; ++c) {
            covered += 1 - field[r][c].clear;
        }
    }
    if (covered == totalMines) {
        move(0, 0);
        attron(COLOR_PAIR(Color::WIN));
        printw("YOU WIN");
        attroff(COLOR_PAIR(Color::WIN));
    }

    move(0, 0);
}
