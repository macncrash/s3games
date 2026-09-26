// S3 SLED KILO — take the kick sled and finish the kilometer without touching a wheel.
// The clock is the other crew. When it runs out, they have the kilometer.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace sledkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    // 0 title, 1 away from the start, 2 among the wheels, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(double& steer, bool& kick, bool& brake);
    void pilot(double& steer, bool& kick, bool& brake);
    void physics(double steer, bool kick, bool brake);
    double frontY() const;
    double laneAt(double y) const;
    void sample(double lx, double ly, double& wx, double& wy) const;
    const char* hitWhat() const;
    bool offSnow() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool danger_ = false;
    int meters_ = 0;
    int post_ = 0;
    int shake_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int puffCursor_ = 0;
    int lastSec_ = -1;
    double t_ = 0, raceTime_ = 0, crew_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    double steer_ = 0, kickCd_ = 0, puffT_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 4.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0;
    Puff puffs_[10]{};
    char report_[240] = {};
    char why_[64] = {};
};

}  // namespace sledkilo
