// S3 SLED GRASS — mush off the snow, land on the grass, full-stop in the pale band.
// The end is that band. Stopping short, rolling past the fence, or leaving the grass fails the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace sledgrass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED GRASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    bool onGrass() const { return onGrass_; }
    bool inEnd() const { return inEnd_; }
    const char* why() const { return why_; }
    // 0 title, 1 snow, 2 on the grass, 3 in the end, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
        int grass = 0;
    };
    struct Flake {
        float x = 0, y = 0, v = 0, w = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void human(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void succeed();
    void fail(const char* why);
    void blip(float freq);
    void yip();
    void chime(int notes);
    void audio(float dt);
    void camera();
    void flurry(float dt);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int teamFrame() const;
    const char* hint() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool onGrass_ = false;
    bool inEnd_ = false;
    bool landed_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    int puffCursor_ = 0;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float vx_ = 0, vy_ = 0, pace_ = 0, speed_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.9f;
    float settle_ = 0, short_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, puffT_ = 0, yipT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Puff puffs_[16]{};
    Flake flakes_[14]{};
    char why_[48] = {};
};

}  // namespace sledgrass
