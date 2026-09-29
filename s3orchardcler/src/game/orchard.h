// S3 ORCHARDCLER — one orchard. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace orchard {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ORCHARDCLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const;
    float secondsLeft() const { return clock_ / 60.f; }

private:
    enum class Mode { Title, Play, Win, Lose };

    struct Tree {
        float x, y;
    };
    struct Pile {
        float x, y;
        int kind;
        bool live;
    };

    void layField();
    void resetRound();
    void updatePlay();
    void drawWorld();
    void drawBanner();
    void hudRow(int row, const char* s);
    void spr(const gs::Image& img, float x, float y, int pal, bool flip = false);
    bool hitsTree(float x, float y) const;
    void blip(float freq);
    int nearest() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float px_ = 160, py_ = 120;
    int face_ = 1;
    int clock_ = 0;
    int swing_ = 0;
    int stuck_ = 0;
    int beep_ = 0;
    float aimX_ = 0, aimY_ = 0;
    Tree trees_[11] = {};
    Pile piles_[16] = {};
    int nTrees_ = 0;
    int nPiles_ = 0;
};

}  // namespace orchard
