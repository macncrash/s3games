// S3 MOSAIC TAPE — slide the drawer until it matches the tape.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC TAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int swaps() const { return swaps_; }
    bool matched() const;
    // 0 title, 1 sliding the drawer, 2 the drawer just matched, 3 finished
    int marker() const;

private:
    enum class Phase { Title, Slide, Seal, Victory };

    void update();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h, int pal);
    void blip(float freq);
    void swapRight();
    void shuffle();
    uint32_t rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int drawer_[N] = {};
    int cx_ = 0;
    int swaps_ = 0;
    int t_ = 0;
    int beep_ = 0;
    uint32_t rng_ = 0x51a7e3u;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool paused_ = false;
};

}  // namespace tape
