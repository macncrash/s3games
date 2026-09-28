// S3 MOSAIC GOLD — slide the medallion home.
// Only the gold tesserae count double. Cream counts one.
// Leave when the picture is whole and the double is what clears the line.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mosaicgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool arranged() const;
    int score() const { return score_; }
    int bare() const { return bare_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int moves() const { return moves_; }
    int line() const { return LINE; }
    // 0 title, 1 sliding, 2 sealed picture, 3 left
    int marker() const;

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
    void begin();
    void buildPath();
    bool slide(int dir, bool count);
    void undo();
    void tally();
    void pumpAudio();
    void click();
    void bonk();
    void fan(int step);
    bool confirm() const;
    int cellX(int i) const { return OX + (i % COLS) * PITCH; }
    int cellY(int i) const { return OY + (i / COLS) * PITCH; }
    int slideFrames() const { return bot_ ? 2 : 6; }
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    Anim anim_{};
    int grid_[CELLS] = {};
    int blank_ = BLANK;
    int moves_ = 0;
    int step_ = 0;
    int t_ = 0;
    int score_ = 0;
    int bare_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int clickLeft_ = 0;
    int bonkLeft_ = 0;
    int fanLeft_ = 0;
    uint32_t rng_ = 1;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool paused_ = false;
    std::vector<int> path_;
    std::vector<int> solve_;
    std::vector<int> hist_;
};

}  // namespace mosaicgold
