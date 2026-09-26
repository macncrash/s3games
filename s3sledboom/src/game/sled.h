// S3 SLED BOOM — take the sled and deliver the drive to the boom.
// The clock is the other crew. When it runs out, they have the boom.
#pragma once
#include "art.h"
#include "console/system.h"

namespace sledboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    float seconds() const { return raceTime_; }
    float crewLeft() const { return crew_ > 0.f ? crew_ : 0.f; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    // 0 title, 1 on the tongue, 2 in the notch, 3 the hook is closing, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0, vx = 0, vy = 0;
    };
    struct Flake {
        float x = 0, y = 0, v = 20, s = 3;
    };

    void begin();
    void showTitle();
    void startRun();
    void human(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void succeed();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio(float dt);
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool shadow = false);
    void driveAt(float& dx, float& dy) const;
    int frameOf(float heading) const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inNotch_ = false;
    bool driveOnSled_ = true;
    bool driveOnBoom_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    int puffCursor_ = 0;
    int yipFlip_ = 0;
    float t_ = 0, raceTime_ = 0, crew_ = 0, settle_ = 0, pastT_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, vx_ = 0, vy_ = 0, speed_ = 0, throttle_ = 0;
    float lostX_ = 0, lostY_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.2f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, sprayT_ = 0, yipT_ = 0, tickT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Puff puffs_[12]{};
    Flake flakes_[22]{};
    char why_[48] = {};
    char report_[220] = {};
};

}  // namespace sledboom
