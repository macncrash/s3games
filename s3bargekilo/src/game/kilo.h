// S3 BARGE KILO — take the cut and finish the kilometer without touching a wheel.
// The other crew owns the clock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace bkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BARGE KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(time_); }
    // 0 title, 1 on the cut, 2 among the wheels, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(double& stick, bool& pole) const;
    void physics(double stick, bool pole);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void sky();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    int nextWheel() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cue_ = -2;
    int chime_ = -1;
    double time_ = 0, t_ = 0;
    double x_ = 0, v_ = 0, lat_ = 0.5, latV_ = 0;
    double camX_ = 0;
    float chimeT_ = 0, beep_ = 0, shake_ = 0;
    char whyBuf_[72] = {};
};

}  // namespace bkilo
