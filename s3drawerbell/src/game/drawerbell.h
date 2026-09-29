// S3 DRAWERBELL — one short drawer, three tries.
// File the slip. Shut the drawer only when the till is the posted sum.
// A false shut kills the try. The bell rings, and you leave,
// only if that happens before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drawerbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWERBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool left() const { return left_; }
    bool rules() const { return rules_; }
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    int drawerCents() const;
    int posted() const { return kTarget; }
    bool filing() const { return mode_ == Mode::Play && t_ > 8; }

private:
    enum class Mode { Title, Play, Ring, Leave, Lose };

    struct Coin {
        int cents = 0;
        int kind = 0;
        bool want = false;
        bool in = false;
        float x = 0;
    };

    void begin();
    void resetTill();
    void file(int i);
    void shut();
    void dieTry();
    void ring();
    void botAct();
    void human(const gs::Pad& pad);
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool matched() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Coin coin_[kCoins]{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool pause_ = false;
    int sel_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int hold_ = 0;
    int swing_ = 0;
    int foul_ = 0;
    int t_ = 0;
    float clerkX_ = 36.f;
};

}  // namespace drawerbell
