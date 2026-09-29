// S3 FISH GOLD — a short fish. Only the gold counts double.
// Cream scores its face. A cream that would reach the line does not count.
// Leave when a gold double puts the score over the line and the bare faces
// are still short of it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fishgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISH GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool goldOut() const { return finisherGold_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int caught() const { return caught_; }
    int line() const { return kLine; }
    // 0 title, 1 on the water, 2 called, 3 win, 4 short
    int marker() const;

private:
    enum class Phase { Title, Cast, Fight, Call, Win, Lose };

    struct Fish {
        int kind = 0;
        float x = 0;
        float y = 0;
        float vx = 0;
        bool gone = false;
    };

    void seed();
    void backdrop();
    void draw();
    void hud(int x, int y, const char* s, int pal);
    void stamp(const gs::Image& img, float x, float y, int pal, bool flip = false);
    void playTick();
    void botSteer();
    void humanSteer();
    void moveFish();
    void moveLure();
    void tryHook();
    void land(int i);
    void finish();
    void blip(float hz, float vol);
    static float wrap(float x);
    static float wrapDelta(float to, float from);

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    Fish fish_[6]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool leave_ = false;
    bool refused_ = false;
    bool finisherGold_ = false;
    bool lureOn_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int caught_ = 0;
    int casts_ = 0;
    int t_ = 0;
    int fightT_ = 0;
    int callT_ = 0;
    int hooked_ = -1;
    int target_ = -1;
    float boatX_ = 160;
    float lureX_ = 160;
    float lureY_ = 52;
    char call_[32] = {};
};

}  // namespace fishgold
