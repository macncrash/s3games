// S3 SAFEBELL — three dials. The room has the numbers.
// A wrong handle kills that try. The bell rings only when the dials match
// while a try is still alive. The third dead try leaves the bell silent.
#pragma once

#include <random>
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace safebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool rung() const { return rung_; }
    bool playing() const { return mode_ == Mode::Play; }
    bool solved() const;
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    int dial(int i) const { return dial_[i]; }
    int combo(int i) const { return code_[i]; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Ring, Leave, Dead };

    void deal();
    void checkRules();
    void botAct();
    void human();
    void nudgeSel(int d);
    void nudgeDial(int d);
    void pull();
    void killTry();
    void ring();
    void blip(float freq);
    void draw();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool rung_ = false;
    int code_[3] = {};
    int dial_[3] = {};
    int sel_ = 0;
    int tryNo_ = 1;
    int dead_ = 0;
    int t_ = 0;
    int ringT_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int bellTick_ = 0;
    int hold_[4] = {};
    float slide_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.12f;
    const char* reason_ = "bell silent";
    std::mt19937 rng_{1};
};

}  // namespace safebell
