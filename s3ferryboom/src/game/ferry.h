// S3 FERRY BOOM — the ferry has one job: deliver the drive to the boom.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace fboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(clock0_ - clock_); }
    const char* why() const { return why_; }
    float x() const { return float(x_); }
    float y() const { return float(y_); }
    float heading() const { return float(heading_); }
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };
    struct Bounds {
        double minX, maxX, minY, maxY;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(double& steer, double& bow);
    void pilot(double& steer, double& bow);
    void physics(double steer, double bow);
    Bounds driveBounds() const;
    bool driveInside() const;
    double flood() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void audio();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal);
    int frameOf(double h) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    double t_ = 0, clock_ = 0, clock0_ = 0, hold_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, sway_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, chimeT_ = 0, hornT_ = 0, shake_ = 0;
    char why_[64] = {};
    Puff foam_[10]{};
    int foamN_ = 0;
};

}  // namespace fboom
