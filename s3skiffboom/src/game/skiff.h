// S3 SKIFF BOOM — take the skiff and deliver the drive to the boom.
// The clock is the other crew. When it runs out, they have the boom.
#pragma once
#include "art.h"
#include "console/system.h"

namespace skiffboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    float seconds() const { return raceTime_; }
    float crewLeft() const { return crew_ > 0.f ? crew_ : 0.f; }
    // 0 title, 1 on the reach, 2 in the pocket, 3 holding the drive, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Wake {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void human(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void stepRival(float dt);
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
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0.f);
    int frameOf(float heading) const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inPocket_ = false;
    bool driveOnBoom_ = false;
    bool driveLost_ = false;
    bool entered_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0;
    float t_ = 0, raceTime_ = 0, crew_ = 0, settle_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float rivalX_ = 0, rivalY_ = 0, rivalH_ = 0, rivalVy_ = 0;
    float lostX_ = 0, lostY_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.15f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, wakeT_ = 0, tickT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Wake wakes_[16]{};
    char why_[48] = {};
    char report_[200] = {};
};

}  // namespace skiffboom
