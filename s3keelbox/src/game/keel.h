// S3 KEEL BOX — stop the keel inside the box. Missing the end fails the leg.
#pragma once
#include <cmath>

#include "art.h"
#include "console/system.h"

namespace keelbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEL BOX"; }
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
    float speed() const { return float(std::fabs(surge_)); }
    float sheet() const { return float(sheet_); }
    int phase() const { return phase_; }
    // 0 title, 1 on the leg, 2 in the box, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        double x = 0, y = 0, life = 0;
    };
    struct Ext {
        double minX, maxX, minY, maxY;
    };

    void begin();
    void showTitle();
    void startRun();
    void slewSheet(double want);
    void controls(double& steer);
    void pilot(double& steer);
    void physics(double steer);
    Ext extents() const;
    bool hullInside() const;
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
    void sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void place(const gs::Mipped& m, double wx, double wy, float worldH, int pal, float minPx = 0, bool flip = false);
    void worldRect(const gs::Mipped& m, double wx, double wy, double ww, double hh, int pal);
    int hullFrame() const;
    void puff(double x, double y);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool announced_ = false;
    int phase_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0;
    double t_ = 0, race_ = 0, hold_ = 0, outT_ = 0;
    double x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, sheet_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0, shake_ = 0, wakeT_ = 0;
    char why_[64] = {};
    Puff wake_[16]{};
};

}  // namespace keelbox
