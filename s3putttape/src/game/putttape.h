// S3 PUTTTAPE — play putt until the drawer matches the tape, then leave.
// The tape wants a quarter, a dime and a nickel. The token is also .25.
// That cents match is not the quarter, so it does not go in the drawer.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace putttape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTTTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool matched() const { return have_[0] && have_[1] && have_[2]; }
    bool held(int i) const { return i >= 0 && i < 3 && have_[i]; }
    bool rolling() const { return mode_ == Mode::Roll && rollT_ > 0.1f && rollT_ < 1.2f; }
    int putts() const { return putts_; }
    int drawerCents() const;
    const char* tapeLabel(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }

    struct Lie {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool rest = true;
    };

private:
    enum class Mode { Title, Aim, Roll, Pocket, Judge, Leave, Lose, Pause, Over };

    bool audit();
    bool solveCup(int cup, float& ang, float& spd) const;
    int rollCup(float ang, float spd) const;
    void toTitle();
    void newGame();
    void beginAim();
    void putt(float ang, float spd);
    void settle(int hole);
    void beginLeave();
    void beginLose();
    int nextCup() const;
    void blip(float freq, float vol = 0.06f);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Lie ball_{};
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool layoutOk_ = false;
    bool swinging_ = false;
    bool have_[3] = {};
    int putts_ = 0;
    int flying_ = -1;
    int lastHole_ = -1;
    int t_ = 0;
    int shake_ = 0;
    float aim_ = -1.6f;
    float meter_ = 0;
    float meterDir_ = 1;
    float botSpd_ = 160;
    float shotAng_[4] = {};
    float shotSpd_[4] = {};
    float clock_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float flyT_ = 0;
    float judgeT_ = 0;
    float leaveT_ = 0;
    float loseT_ = 0;
    float beep_ = 0;
    float walk_ = 0;
};

}  // namespace putttape
