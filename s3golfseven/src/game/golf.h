// S3 GOLF SEVEN — one short links, two players, one pitch apiece.
// A ball that finishes in the cup scores 1. A leave, a bunker, or the water scores 0.
// Six does not finish the match. The first side to reach 7 wins.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golfseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLF SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Aim, Flight, Call, Win, Lose, Pause };

    struct Ball {
        float x = kTee, y = kBallR, vx = 0, vy = 0;
        bool rolling = false;
        bool rest = true;
        bool holed = false;
        bool wet = false;
        bool ob = false;
        int still = 0;
    };
    struct Hole {
        float cup;
        float wind;
        float water0, water1;
        float sand0, sand1;
    };
    struct Window {
        bool ok = false;
        float lo = 0, hi = 1;
    };

    void begin();
    void nextTee();
    void launch(float power);
    bool stepBall(float dt);
    bool predict(float power, const Hole& h) const;
    Window windowFor(const Hole& h) const;
    void finishFlight();
    void afterCall();
    void swingTone();
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    const Hole& hole() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Ball ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool yours_ = true;
    bool made_ = false;
    int you_ = 0;
    int them_ = 0;
    int played_ = 0;
    int flightT_ = 0;
    int swingT_ = 0;
    float power_ = 0.5f;
    float meter_ = 0;
    float meterDir_ = 1;
    float callT_ = 0;
    float toneT_ = 0;
    float clock_ = 0;
    Window win_{};
    uint32_t rng_ = 0x60F7u;
};

}  // namespace golfseven
