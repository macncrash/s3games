// S3 BUS MARK — set the bus down on the mark. Missing the end fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace busmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS MARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return float(race_); }
    const char* why() const { return why_; }
    float x() const { return float(x_); }
    float y() const { return float(y_); }
    float heading() const { return float(heading_); }
    float speed() const { return float(speed_); }
    float nose() const { return float(noseDist_); }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 bumper on the mark, 3 holding the set, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void showTitle();
    void startRun();
    void controls();
    void pilot();
    void physics();
    void noseAt(double& nx, double& ny) const;
    void measure();
    const char* stopWhy() const;
    const char* hint() const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, bool flip = false);
    void worldRect(const gs::Mipped& m, double wx, double wy, double ww, double hh, int pal);
    int busFrame() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onMark_ = false;
    bool noseOk_ = false;
    bool announced_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    double t_ = 0, race_ = 0, hold_ = 0, outT_ = 0, stuckT_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, speed_ = 0, throttle_ = 0, steer_ = 0;
    double nx_ = 0, ny_ = 0, noseDist_ = 0, prevNose_ = 0, stuckX_ = 0, stuckY_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, shake_ = 0;
    char why_[80] = {};
};

}  // namespace busmark
