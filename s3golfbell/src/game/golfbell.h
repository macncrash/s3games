// S3 GOLFBELL — three tries at one cup.
// The bell hangs on the pin. Only a ball that drops in the hole rings it.
// A shot that dies on the fairway, the lip, or past the pin kills the try.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golfbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLFBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Fly; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Fly, Dead, Ring, Leave, Over };

    struct Ball {
        float x = kTeeX;
        float y = 0;
        float vx = 0;
        float vy = 0;
        bool live = false;
        bool holed = false;
    };

    void begin();
    void audit();
    void launch(float angDeg, float spd);
    bool stepBall(Ball& b, float dt) const;
    bool flyUntil(Ball b, float ang, float spd, Ball& out) const;
    void pickShot();
    void finishFly();
    void ring();
    void dieTry(const char* why);
    void resetTee();
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Ball ball_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool planned_ = false;
    bool charging_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int hold_ = 0;
    const char* why_ = "";
    float aim_ = 36.f;
    float meter_ = 0.2f;
    float meterDir_ = 1.f;
    float botAng_ = 36.f;
    float botSpd_ = 280.f;
    float clock_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.15f;
};

}  // namespace golfbell
