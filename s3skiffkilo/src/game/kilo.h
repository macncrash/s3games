// S3 SKIFF KILO — finish the kilometer without touching a wheel.
// The clock is the other crew. When it runs out, they have the kilometer.
#pragma once
#include "art.h"
#include "console/system.h"

namespace kilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SKIFF KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 away from the dock, 2 among the wheels, 3 the last stretch, 4 finished
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
    double bowY() const;
    double laneAt(double y) const;
    void sample(double lx, double ly, double& wx, double& wy) const;
    int wheelHit() const;
    bool beached() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool danger_ = false;
    int meters_ = 0;
    int cleared_ = 0;
    int shake_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0;
    int lastSec_ = -1;
    int failPal_ = PAL_ALERT;
    double t_ = 0, raceTime_ = 0, crew_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.2f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0;
    double wakeT_ = 0;
    Wake wakes_[16]{};
    char report_[240] = {};
    char why_[64] = {};
};

}  // namespace kilo
