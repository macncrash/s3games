// S3 DRUM SEVEN — three drums, one lamp. First to seven clean hits wins.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace drumseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRUM SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Play, Win, Lose };

    void open();
    void scoreYou();
    void scoreThem();
    void settle();
    void advance();
    void botAct();
    void human();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    int drumPal(int i) const;
    float drumX(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int them_ = 0;
    int t_ = 0;
    int lamp_ = 1;
    int hold_ = 0;
    int foe_ = 0;
    int flash_ = 0;
    int struck_ = -1;
};

}  // namespace drumseven
