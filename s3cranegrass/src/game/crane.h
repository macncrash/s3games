// S3 CRANE GRASS — land the crane on the grass and come to a full stop.
#pragma once
#include <cmath>

#include "art.h"
#include "console/system.h"

namespace cranegrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANE GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return clock_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return std::fabs(speed_); }
    bool onGrass() const { return planted_; }
    int phase() const { return phase_; }
    // 0 title, 1 the yard, 2 on the grass, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& steer, float& gas, float& brake);
    void pilot(float& steer, float& gas, float& brake);
    void physics(float steer, float gas, float brake);
    void corners(float& x0, float& y0, float& x1, float& y1, float& x2, float& y2, float& x3, float& y3) const;
    bool insideGrass(float x, float y, float inset) const;
    bool hullPlanted(float inset) const;
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void worldSpr(const gs::Mipped& m, float wx, float wy, float worldH, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool planted_ = false;
    bool deep_ = false;
    bool rolled_ = false;
    int phase_ = 0;
    int chimeN_ = 0;
    int chimeStep_ = 0;
    int puffI_ = 0;
    float t_ = 0, clock_ = 0, hold_ = 0, shortT_ = 0, noseT_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 10.f;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0, shake_ = 0;
    char why_[64] = {};
    Puff puffs_[6]{};
};

}  // namespace cranegrass
