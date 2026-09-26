// S3 HOOPBELL — three tries at one rim.
// The bell hangs in the net. Only a clean pass through its mouth rings it.
// Soft, hot, iron, and misses die. Leave when it rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hoopbell {

enum class Kind { Swish, Iron, Short, Hot, Miss };

struct Ball {
    float x = 0, y = 0, z = 0;
    float vx = 0, vy = 0, vz = 0;
};

struct Trace {
    bool clean = false;
    bool rim = false;
    bool bank = false;
    bool crossed = false;
    float rad = 99.f;
    float crossX = 0;
    float crossZ = 0;
    float flight = 0;
    float scoreAt = 0;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOPBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && tr_.flight > 0.18f && tr_.flight < 0.72f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Flight, Dead, Ring, Leave, Pause, Over };

    void toTitle();
    void newGame();
    void beginAim();
    bool launch(float meter);
    void fly(float dt);
    void finishShot();
    void ring();
    void dieTry();
    void botAim(float dt);
    void humanAim(const gs::Pad& p, bool fire, float dt);
    void dribble();
    void blip(int ch, float freq, float vol, float hold);
    void pump(float dt);
    void backdrop();
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false, bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    bool audit();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Kind hint_ = Kind::Miss;
    Kind last_ = Kind::Miss;
    Ball ball_{};
    Trace tr_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool rimSnd_ = false;
    bool bankSnd_ = false;
    bool wasSweet_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    const char* why_ = "";
    float feet_ = 0;
    float aimZ_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.18f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    float flash_ = 0;
    float dribS_ = 0;
};

}  // namespace hoopbell
