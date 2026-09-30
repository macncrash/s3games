// S3 LENSBELL — one short lens, three tries.
// Frame the swinging bell and park the focus ring, then trip the shutter.
// A true plate rings the bell. A soft or wide plate dies.
// The third dead try ends it. The bell has to ring before that.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lensbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LENSBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Flash, Dead, Ring, Over };

    void begin();
    void nextTry();
    void trip();
    void judge();
    void dieTry();
    void ring();
    void botAim();
    void humanAim();
    float bellX() const;
    void pumpAudio();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool sharp_ = false;
    bool framed_ = false;
    int dead_ = 0;
    int tryNo_ = 1;
    int t_ = 0;
    int flashT_ = 0;
    int holdT_ = 0;
    int settle_ = 0;
    int beep_ = 0;
    float focus_ = 0.22f;
    float pan_ = -46.f;
    const char* why_ = "";
};

}  // namespace lensbell
