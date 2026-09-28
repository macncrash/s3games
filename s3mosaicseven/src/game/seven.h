// S3 MOSAIC SEVEN — slide the picture. First bench to seven.
// A count that stops under seven is not the race.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mosaicseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int solved() const { return solved_; }

private:
    enum class Phase { Title, Study, Scramble, Play, Seal, Victory };

    struct Anim {
        bool on = false;
        int id = 0;
        int from = 0;
        int to = 0;
        int t = 0;
        int n = 1;
    };

    void update();
    void draw();
    void drawTitle();
    void drawBoard();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blit(const gs::Image& img, int x, int y, int pal, int w = -1, int h = -1);
    void beginRound();
    void buildPath();
    bool slide(int dir, bool count);
    void undo();
    void rivalTick();
    void finish(bool youWon);
    void pumpAudio();
    void click();
    void bonk();
    void fan(int step);
    bool confirm() const;
    bool arranged() const;
    int cellX(int i) const { return OX + (i % COLS) * PITCH; }
    int cellY(int i) const { return OY + (i / COLS) * PITCH; }
    int slideFrames() const { return bot_ ? 2 : 7; }
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    Anim anim_{};
    int grid_[CELLS] = {};
    int blank_ = BLANK;
    int picture_ = 0;
    int moves_ = 0;
    int step_ = 0;
    int t_ = 0;
    int you_ = 0;
    int them_ = 0;
    int solved_ = 0;
    int rivalWork_ = 0;
    int clickLeft_ = 0;
    int bonkLeft_ = 0;
    int fanLeft_ = 0;
    uint32_t rng_ = 1;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool rules_ = false;
    bool paused_ = false;
    bool racing_ = false;
    std::vector<int> path_;
    std::vector<int> solve_;
    std::vector<int> hist_;
};

}  // namespace mosaicseven
