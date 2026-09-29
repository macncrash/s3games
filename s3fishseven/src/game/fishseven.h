// S3 FISH SEVEN — fish the lake until one side is first to seven, then leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fishseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FISH SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    // 0 title, 1 on the water, 2 called, 3 win, 4 short
    int marker() const;

private:
    enum class Phase { Title, Cast, Call, Win, Lose };

    struct Fish {
        float x = 0;
        float y = 0;
        float vx = 0;
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
    void rivalTick();
    void land(int i);
    void finish(bool yours);
    void blip(float hz, float vol);
    static float wrap(float x);
    static float wrapDelta(float to, float from);

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    Fish fish_[5]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool lureOn_ = false;
    int you_ = 0;
    int them_ = 0;
    int t_ = 0;
    int callT_ = 0;
    int rivalT_ = 0;
    int hooked_ = -1;
    int target_ = -1;
    float boatX_ = 80;
    float rivalX_ = 220;
    float lureX_ = 80;
    float lureY_ = 58;
    char call_[40] = {};
};

}  // namespace fishseven
