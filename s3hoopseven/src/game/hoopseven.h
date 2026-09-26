// S3 HOOP SEVEN — a short court. First to seven.
// The arc is 3, the mid-range is 2, the paint is 1.
// The green band is a make. Anything else is a miss.
// Six does not finish the game. The first side to reach 7 wins.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hoopseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOP SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int lane() const { return lane_; }
    const char* phase() const;

private:
    enum class Mode { Title, Aim, Flight, Call, Win, Lose, Pause };
    enum class Call { Swish, Count, Bank, Rim, Short, Long, Air };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
    };
    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
    };

    void beginMatch();
    void beginAim();
    void toTitle();
    void launch(float meter);
    void stepBall(float dt);
    void fly(float dt);
    void finishShot();
    void afterCall();
    void win();
    void lose();
    void steer(float dt, float axis);
    void tickMeter(bool shoot, float dt);
    void dribble();
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;
    Mode shown() const;

    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void hush();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Call call_ = Call::Air;
    Ball ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    bool clean_ = false;
    bool scored_ = false;
    bool rimHit_ = false;
    bool bank_ = false;
    bool crossed_ = false;
    bool rimSnd_ = false;
    bool bankSnd_ = false;
    int you_ = 0;
    int lane_ = 0;
    int youN_ = 0;
    int laneN_ = 0;
    int worth_ = 3;
    int ending_ = 0;
    float feetX_ = 74.f;
    float meter_ = 0.5f;
    float meterDir_ = 1.f;
    float aimOff_ = 0.f;
    float clock_ = 0.f;
    float flight_ = 0.f;
    float scoreAt_ = -1.f;
    float callT_ = 0.f;
    float aimT_ = 0.f;
    float beep_ = 0.f;
    float flash_ = 0.f;
    float crossX_ = 0.f;
    float prevSin_ = 0.f;
};

}  // namespace hoopseven
