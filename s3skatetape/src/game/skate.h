// S3 SKATETAPE — a short skate. The drawer has to match the tape.
// A firm pop on OLLIE, GRIND or KICK drops that slip in the drawer.
// POP, SLIDE and SHOVE pay the same and stay out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace skate {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKATETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    int pops() const { return pops_; }
    int drawerScore() const;

private:
    enum class Mode { Title, Ride, Judge, Leave, Lose, Over };

    void toTitle();
    void newRun();
    void pop();
    void botRide();
    void humanRide(const gs::Pad& pad);
    void judge();
    void blip(float freq, float vol);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, float x, float y, float w, float h, int pal, bool flip = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool held_[kTapeN] = {};
    bool firm_ = false;
    int pops_ = 0;
    int mark_ = -1;
    int line_ = -1;
    int judgeT_ = 0;
    int leaveT_ = 0;
    int titleT_ = 0;
    int cool_ = 0;
    char note_[40] = {};
    float x_ = 80.f;
    float vx_ = 0.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float hop_ = 0.f;
    float cam_ = 0.f;
};

}  // namespace skate
