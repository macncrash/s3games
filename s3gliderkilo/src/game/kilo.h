// S3 GLIDER KILO — the glider has one job: finish the kilometer without touching wheels.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace gkilo {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER KILO"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(time_); }
    // 0 title, 1 airborne, 2 among the wheels, 3 the last stretch, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };

    void showTitle();
    void startRun();
    void pilot(double& nose) const;
    void physics(double nose);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void sky();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip = false, int fog = 0);
    void sprAnchor(const gs::Mipped& m, float ax, float ay, float sx, float sy, float destH, int pal, int fog = 0);
    void sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal, int fog = 0);
    int nextWheel() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int cue_ = -2;
    int chime_ = -1;
    int puffN_ = 0;
    double time_ = 0, t_ = 0;
    double x_ = 0, h_ = 0, v_ = 0, vy_ = 0, att_ = 0;
    double camX_ = 0, camH_ = 0, camS_ = 6.1;
    float chimeT_ = 0, beep_ = 0, shake_ = 0;
    const char* why_ = "";
    char whyBuf_[64] = {};
    Puff puffs_[6]{};
};

}  // namespace gkilo
