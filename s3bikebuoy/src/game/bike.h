// S3 BIKE BUOY — ride the water bike around the buoys and stop in the same dock.
// The end is that slip. The other dock, or the quay beside the mouth, misses the leg.
#pragma once
#include "art.h"
#include "console/system.h"

namespace bike {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKE BUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    int leg() const { return leg_; }
    int waypoint() const { return wp_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    float seconds() const { return raceTime_; }
    // 0 title, 1 on the water, 2 rounding, 3 the return, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void blip();
    void chime();
    void audio();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool armed_ = false;
    int leg_ = 0;
    int wp_ = 0;
    int puffN_ = 0;
    float t_ = 0;
    float raceTime_ = 0;
    float hold_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, speed_ = 0;
    float camX_ = 0, camY_ = 0;
    float tone_ = 0;
    Puff puffs_[16]{};
    char why_[40] = {};
};

}  // namespace bike
