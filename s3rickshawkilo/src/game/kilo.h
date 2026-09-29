// S3 RICKSHAW KILO — take the rickshaw and finish the kilometer
// without touching a wheel. The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace kilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RICKSHAW KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(time_); }
    int meters() const { return int(s_); }
    // 0 title, 1 street, 2 among the wheels, 3 last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& steer, bool& pedal) const;
    void physics(float steer, bool pedal);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    double time_ = 0, t_ = 0, s_ = 0, v_ = 0, lat_ = 0, latV_ = 0;
    float chimeT_ = 0, beep_ = 0, shake_ = 0;
    int chime_ = -1;
    int passed_ = 0;
    char whyBuf_[72] = {};
};

}  // namespace kilo
