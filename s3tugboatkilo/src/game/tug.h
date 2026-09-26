// S3 TUGBOAT KILO — finish the kilometer without touching a wheel.
// Missing the end fails the leg.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tugkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    // 0 title, 1 on the leg, 2 among the wheels, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(double& steer);
    void pilot(double& steer);
    void physics(double steer);
    void sample(double lf, double lr, double& wx, double& wy) const;
    double laneAt(double y) const;
    double wheelX(int i) const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void horn(float seconds);
    void chime(int notes);
    void cosmetics();
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal);
    void worldBox(const gs::Mipped& m, double wx, double wy, double ww, double wh, int pal);
    int hullFrame() const;

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
    int wakeCursor_ = 0, smokeCursor_ = 0;
    double t_ = 0, race_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, engine_ = 0, steerF_ = 0, throttle_ = 0;
    double smokeT_ = 0, wakeT_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 2.8f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, hornT_ = 0;
    char why_[72] = {};
    Puff wake_[14]{};
    Puff smoke_[10]{};
};

}  // namespace tugkilo
