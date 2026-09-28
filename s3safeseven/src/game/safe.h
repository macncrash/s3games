// S3 SAFE SEVEN — a short safe. First opener to seven wins.
// One correct pull is a point, not the end. Under seven the door is still the job.
#pragma once

#include <random>

#include "console/system.h"
#include "game/art.h"

namespace safeseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Flash, Win, Lose };

    void newMatch();
    void deal();
    void botAct();
    void human();
    void nudgeSel(int d);
    void nudgeDial(int d);
    void pull();
    void deskTick();
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(float x, float y, float w, float h, int pal);
    bool matched() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int code_[3] = {1, 1, 1};
    int dial_[3] = {};
    int sel_ = 0;
    int you_ = 0;
    int them_ = 0;
    int t_ = 0;
    int flash_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int desk_ = 0;
    int hold_[4] = {};
    float slide_ = 0;
    std::mt19937 rng_{7};
};

}  // namespace safeseven
