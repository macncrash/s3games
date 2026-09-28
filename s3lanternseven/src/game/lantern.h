// S3 LANTERN SEVEN — a short lantern. First to seven.
// Seven lamps on one lane. A catch under seven is not the end.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lanternseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERN SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    void newMatch();
    void light(int i);
    void botAct();
    void human();
    void nightTick();
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void box(float x, float y, float w, float h, int pal);
    float hangX(int i) const;
    float hangY(int i) const;
    float theta(int i) const;
    int nearest() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool lit_[7] = {};
    float phase_[7] = {};
    float youX_ = 160;
    int you_ = 0;
    int them_ = 0;
    int t_ = 0;
    int night_ = 0;
    int raise_ = 0;
    int beep_ = 0;
    int holdL_ = 0;
    int holdR_ = 0;
};

}  // namespace lanternseven
