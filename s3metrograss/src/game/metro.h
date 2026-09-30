// S3 METRO GRASS — ride the water metro onto the grass and come to a full stop.
// Stopping short, or rolling past the pale end, fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace metrograss {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 METRO GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    bool onGrass() const { return onGrass_; }
    bool inEnd() const { return inEnd_; }
    const char* why() const { return why_; }
    // 0 title, 1 on the water, 2 on the grass, 3 in the end, 4 finished
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
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int hullFrame() const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onGrass_ = false;
    bool inEnd_ = false;
    bool landed_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, off_ = 0, heading_ = 0, speed_ = 0;
    float throttle_ = 0, steer_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.85f;
    float settle_ = 0, short_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, washT_ = 0;
    char why_[48] = {};
};

}  // namespace metrograss
