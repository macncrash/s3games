// S3 SUB BOOM — take the sub along the leg and set the drive on the boom.
// Missing the end of the leg fails it.
#pragma once
#include "art.h"
#include "console/system.h"

namespace subboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SUB BOOM"; }
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
    // 0 title, 1 on the leg, 2 drive in the boom, 3 holding, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Box {
        double minX, maxX, minY, maxY;
    };
    struct Bubble {
        double x, y, life, r;
    };

    void showTitle();
    void startRun();
    void controls(double& thrust, double& climb);
    void pilot(double& thrust, double& climb);
    void physics(double thrust, double climb);
    Box driveBox() const;
    bool driveInside() const;
    bool driveAligned() const;
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
    float camX_ = 0, camY_ = 16, tone_ = 0, chimeT_ = 0;
    char why_[64] = {};
    Bubble bub_[12]{};
};

}  // namespace subboom
