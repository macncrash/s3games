// S3 RICKSHAW BOOM — take the rickshaw and deliver the drive to the boom.
// The clock is the other crew.
#pragma once
#include "art.h"
#include "console/system.h"

namespace rickboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(race_); }
    float clockLeft() const { return float(clock_); }
    const char* why() const { return why_; }
    float x() const { return float(x_); }
    float speed() const { return float(speed_); }
    int phase() const { return phase_; }
    // 0 title, 1 on the lane, 2 drive on the boom, 3 holding, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void showTitle();
    void startRun();
    void controls();
    void pilot();
    void physics();
    double driveC() const;
    bool driveOnBoom() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void draw();
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int pedal_ = 0;
    double t_ = 0, race_ = 0, clock_ = 0, hold_ = 0, outT_ = 0, roll_ = 0;
    double x_ = 0, speed_ = 0, pedalIn_ = 0, brakeIn_ = 0;
    float camX_ = 0, shake_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0;
    char why_[72] = {};
};

}  // namespace rickboom
