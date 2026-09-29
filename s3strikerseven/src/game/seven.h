// S3 STRIKER SEVEN — one high striker, two hammers.
// A tick counts 1, the pair mark counts 2, the bell counts 3.
// First tally to reach 7 wins. A 6 is still short, and the match goes on.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace strikerseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKER SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool shortSix() const { return shortSix_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Ready, Strike, Rise, Show, Win, Lose };

    struct Hit {
        int pts = 0;
        bool yours = false;
    };

    void begin();
    void launch(float power);
    void settle();
    void win();
    void lose();
    void nextTurn();
    bool ledgerOk() const;
    int pointsFor(float power) const;
    float meter() const;
    int pose() const;
    void botPlay();

    void blip(bool high);
    void thunk();
    void chime();
    void whistle(float h);

    void backdrop();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void img(const gs::Image& im, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void meterBar(int row);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Hit log_[24]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool shortSix_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int gain_ = 0;
    int hits_ = 0;
    float phase_ = 0;
    float power_ = 0;
    float apex_ = 0;
    float puck_ = 0;
    float puckV_ = 0;
    float modeT_ = 0;
    float bellT_ = 0;
    float leaveT_ = 0;
    const char* say_ = "";
};

}  // namespace strikerseven
