// S3 PUTT SEVEN — one green, one cup. First to seven holes, then leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace puttseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PUTT SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int strokes() const { return strokes_; }

private:
    enum class Mode { Title, Play, Hold, Result, Pause };

    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
        int cup = 0;
        int bumps = 0;
        bool rest = true;
    };

    void begin();
    void place();
    void strike(float ang, float spd);
    void advance(Ball& b) const;
    void bounce(Ball& b, float nx, float ny) const;
    void collide(Ball& b, float x, float y, float w, float h) const;
    int trial(float x, float y, float ang, float spd) const;
    bool solve();
    void settle();
    void blip(float freq);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void patch(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void lamps();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    Ball ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool swinging_ = false;
    bool shotOk_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int strokes_ = 0;
    int themPutts_ = 0;
    int fate_ = 0;
    int rollFrames_ = 0;
    float aim_ = 0;
    float humanAim_ = 0;
    float aimShot_ = 0;
    float spdShot_ = 140;
    float aimMiss_ = 0;
    float spdMiss_ = 90;
    float meter_ = 0;
    float meterDir_ = 1;
    float t_ = 0;
    float holdT_ = 0;
    float bannerT_ = 0;
    float cpuWait_ = 0;
    float beep_ = 0;
};

}  // namespace puttseven
