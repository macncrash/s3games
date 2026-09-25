// S3 FERRY — nose the car ferry into the terminal slip before the tide clock.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace ferry {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float tideLeft() const { return clock_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const;
    int phase() const { return phase_; }
    float hold() const { return hold_; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    struct Foam {
        float x, y, life;
    };

    void begin();
    void controls(float& thrust, float& rudder, float& bow);
    void pilot(float& thrust, float& rudder, float& bow);
    void chase(float tx, float ty, float maxSpd, bool lockNorth, float& thrust, float& rudder, float& bow);
    void physics(float dt, float thrust, float rudder, float bow);
    void tideFlow(float& cx, float& cy) const;
    void corners(float xs[4], float ys[4]) const;
    bool pushOut(float px, float py, float& dx, float& dy) const;
    void resolveWalls();
    bool hullInSlip() const;
    bool madeFast() const;
    void blip(float freq);
    void horn();
    void chime();
    void audio(float dt);
    void updateFoam(float dt);
    void draw();
    void drawHud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, bool flip = false);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int shipFrame() const;
    int clockFrame() const;
    float rnd();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool wasHit_ = false;
    int phase_ = 0;
    int chime_ = 0;
    int chimeStep_ = 0;
    float clock_ = 0;
    float hold_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float surge_ = 0, sway_ = 0, yaw_ = 0;
    float worldVx_ = 0, worldVy_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1;
    float shake_ = 0;
    float tone0_ = 0, hornT_ = 0, chimeT_ = 0;
    float foamT_ = 0, wakeT_ = 0;
    float thrustIn_ = 0;
    uint32_t rng_ = 1;
    std::vector<Foam> foams_;
};

}  // namespace ferry
