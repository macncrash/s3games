// S3 KART LANE — take the kart and stay in the lane for the whole leg.
// The clock is the other crew. Leaving the lane or missing the end fails it.
#pragma once
#include <vector>

#include "art.h"
#include "console/system.h"

namespace kartlane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KART LANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_ ? why_ : ""; }
    float seconds() const { return race_; }
    float x() const { return x_; }
    float z() const { return z_; }
    float heading() const { return heading_; }
    float speed() const { return speed_; }
    float lateral() const;
    // 0 title, 1 the lane, 2 near the edge, 3 the end is ahead, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };
    enum class Kind { Cone, Bale, Pole, Post, Bar, Flag };

    struct Prop {
        float x, z, h;
        Kind kind;
        int pal;
    };
    struct Puff {
        float x, z, life;
    };

    void begin();
    void showTitle();
    void startRun();
    void buildCourse();
    void controls(float& steer, float& gas);
    void pilot(float& steer, float& gas);
    void physics(float steer, float gas);
    void judge();
    void win();
    void fail(const char* why);
    void blip(float freq);
    void chime();
    void audio();
    void draw();
    void drawLane();
    void drawWorld();
    void drawSky();
    void drawHud();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0, bool shadow = false);
    bool project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const;
    int bankFrame() const;
    float eye() const;
    float back() const;
    float horizon() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool warned_ = false;
    bool sawEnd_ = false;
    const char* why_ = "";
    int chimeN_ = 0, chimeStep_ = 0;
    int puffN_ = 0;
    float t_ = 0, race_ = 0;
    float x_ = 0, z_ = 0, heading_ = 0, speed_ = 0;
    float gas_ = 0, yaw_ = 0, camBob_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, puffT_ = 0;
    Puff puffs_[8]{};
    std::vector<Prop> props_;
};

}  // namespace kartlane
