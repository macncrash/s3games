// S3 SUB MARK — set the sub down on the painted mark before the end of the leg.
// Passing the end without being set fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace submark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(race_); }
    const char* why() const { return why_; }
    float x() const { return float(x_); }
    float y() const { return float(y_); }
    float speed() const { return float(vx_); }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 over the mark, 3 holding the set, 4 set
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Bubble {
        double x, y, life, r;
    };

    void showTitle();
    void startRun();
    void controls(double& thrust, double& climb);
    void pilot(double& thrust, double& climb);
    void physics(double thrust, double climb);
    bool onMark() const;
    bool settled() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip = false);
    void banner(const gs::Mipped& m, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    double t_ = 0, race_ = 0, hold_ = 0;
    double x_ = 0, y_ = 0, vx_ = 0, vy_ = 0, throttle_ = 0, climb_ = 0;
    float camX_ = 0, camY_ = 12, tone_ = 0, chimeT_ = 0;
    char why_[64] = {};
    Bubble bub_[10]{};
};

}  // namespace submark
