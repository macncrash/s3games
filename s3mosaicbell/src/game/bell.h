// S3 MOSAIC BELL — lay a short mosaic. The bell rings if the picture
// holds before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mosaicbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return reason_; }

private:
    enum class Phase { Title, Lay, Ring, Leave, Over };

    void update();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h);
    void blip(float freq);
    void chime();
    bool filled() const;
    bool confirm() const;
    void stampHere();
    void killTry(const char* why);
    void armTry();
    const char* inkName() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int board_[CELLS] = {};
    int cx_ = 2;
    int cy_ = 0;
    int ink_ = 1;
    int tryNo_ = 1;
    int dead_ = 0;
    int fuse_ = 0;
    int t_ = 0;
    int beep_ = 0;
    int chimeLeft_ = 0;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool paused_ = false;
    const char* reason_ = "bell silent";
};

}  // namespace mosaicbell
