// S3 SKIFF BOX — take the skiff and stop inside the box.
// The clock is the other crew. When it runs out, they have the box.
#pragma once
#include "art.h"
#include "console/system.h"

namespace skiffbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF BOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 on the way, 2 in the box, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Wake {
        double x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(double& steer, double& throttle);
    void pilot(double& steer, double& throttle);
    void physics(double steer, double throttle);
    bool hullInside() const;
    double groundSpeed() const;
    void win();
    void fail();
    void thud();
    void blip(float freq);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, float minPx = 0);
    int hullFrame() const;
    int handFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0;
    int lastSec_ = -1;
    double t_ = 0, raceTime_ = 0, crew_ = 0, holdT_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.86f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, wakeT_ = 0;
    Wake wakes_[20]{};
    char report_[180] = {};
};

}  // namespace skiffbox
