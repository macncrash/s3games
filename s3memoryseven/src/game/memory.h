// S3 MEMORY SEVEN — pairs on the cloth. First seat to seven matches wins.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace memoryseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORY SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };
    enum class Phase { Choose, Peek };

    void newMatch();
    void deal();
    void flip(int i);
    void resolve();
    void botAct();
    void human();
    void youPick();
    void themPick();
    int mateOf(int i, bool opp) const;
    int faceDown(int notKind, int skip) const;
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(float x, float y, float w, float h, int pal);
    void cardPos(int i, float& cx, float& cy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Choose;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    int kind_[N] = {};
    bool matched_[N] = {};
    bool known_[N] = {};
    bool opp_[N] = {};
    int upA_ = -1;
    int upB_ = -1;
    int cursor_ = 0;
    int you_ = 0;
    int them_ = 0;
    int t_ = 0;
    int peek_ = 0;
    int act_ = 0;
    int beep_ = 0;
    int hold_[4] = {};
    uint32_t rng_ = 0x5E7u;
};

}  // namespace memoryseven
