// S3 PINSBELL — three tries. The bell has to ring before the third one dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinsbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINSBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rolling() const { return mode_ == Mode::Roll && ballLive_ && rollT_ > 0.28f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return rungTry_; }

private:
    enum class Mode { Title, Approach, Swing, Roll, Dead, Ring, Leave, Over, Pause };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        float fall = 0;
        bool down = false;
    };

    void newGame();
    void beginApproach();
    void resetRack();
    void beginSwing();
    void release();
    void ring();
    void dieTry();
    void physics(float dt);
    bool rollDone() const;
    float meter() const;
    float aimX() const;
    bool onLine() const;
    int pose() const;
    void draw();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool ballLive_ = false;
    bool gutter_ = false;
    bool gutterSnd_ = false;
    bool driven_ = false;
    bool pocket_ = false;
    bool crossed_ = false;
    bool wasLine_ = false;
    int dead_ = 0;
    int rungTry_ = 0;
    float clock_ = 0;
    float life_ = 0;
    float swingT_ = 0;
    float rollT_ = 0;
    float stance_ = 0;
    float hook_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float bellT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0;
    float flash_ = 0;
    float deadWait_ = 0;
    float ringWait_ = 0;
    float leaveT_ = 0;
    float driveT_ = 0;
    float ripple_ = -1;
    float entry_ = 0;
    float ballX_ = 0, ballZ_ = 0, ballVx_ = 0, ballVz_ = 0;
    Pin pin_[10];
};

}  // namespace pinsbell
