// S3 MAZE GOLD — walk the hedge, then leave.
// A gold coin counts two. A cream coin counts one.
// Cream that would reach the toll does not count.
// The gate lets you out only once gold has doubled the toll
// and the undoubled pile is still under it.
#pragma once

#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace mazegold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAZE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool atExit() const { return cx_ == ex_ && cy_ == ey_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return gold_ * 2 + cream_; }
    int steps() const { return steps_; }
    int line() const { return kLine; }
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Play, Won };

    struct Coin {
        int x = 0;
        int y = 0;
        bool gold = false;
    };

    void buildWorld();
    bool planRoute();
    int nextMask(int mask, int x, int y) const;
    void tally(int mask, int& g, int& c) const;
    bool canLeave() const;
    void recount();
    void clearRun();
    void resetRun();
    bool blocked(int x, int y) const;
    bool tryStep(int nx, int ny);
    void faceDir(int dx, int dy);
    void updateMotion();
    void readMove();
    void arrive();
    void draw();
    void drawTitle();
    void drawPlay();
    void backdrop();
    void glow();
    void tileText(int col, int row, const char* s);
    void put(const gs::Image& img, int x, int y, int pal, bool flip, bool shadow = false);
    void center(const gs::Image& img, int y, int pal);
    void blip(float freq);
    void chord(float a, float b, float c, int hold);
    void thud();
    void hush();

    gs::System* sys_ = nullptr;
    Art art_{};
    Coin coins_[kCoins]{};
    Mode mode_ = Mode::Title;
    const char* say_ = "ONLY THE GOLD COUNTS DOUBLE";
    bool bot_ = false;
    bool won_ = false;
    bool left_ = false;
    bool over_ = false;
    bool finisherGold_ = false;
    bool moving_ = false;
    bool flip_ = false;
    int face_ = 0;
    int sx_ = 1, sy_ = 1, ex_ = 13, ey_ = 9;
    int cx_ = 1, cy_ = 1, tx_ = 1, ty_ = 1;
    int steps_ = 0;
    int tick_ = 0;
    int bump_ = 0;
    int toneLeft_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int got_ = 0;
    int routeAt_ = 0;
    std::vector<uint8_t> hedge_;
    std::vector<int> route_;
};

}  // namespace mazegold
