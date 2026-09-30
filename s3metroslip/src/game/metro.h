// S3 METRO SLIP — berth the water metro in the slip before the tide turns.
// Running past the end of the leg fails it.
#pragma once
#include "art.h"
#include "console/system.h"

namespace metroslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    int tideLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float speed() const { return speed_; }
    bool inSlip() const { return inSlip_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the metro, 2 in the slip mouth, 3 holding the berth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void showTitle();
    void startRun();
    void pose();
    void human(float& throttle, float& steer);
    void pilot(float& throttle, float& steer);
    void physics(float dt, float throttle, float steer);
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
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    bool mouthOpen() const;
    float laneX(float y) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inSlip_ = false;
    bool inMouth_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, off_ = 0, speed_ = 0;
    float throttle_ = 0, steer_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.9f;
    float settle_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, washT_ = 0;
    char why_[48] = {};
};

}  // namespace metroslip
