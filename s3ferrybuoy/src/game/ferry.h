// S3 FERRY BUOY — round the buoys to port and stop in the same slip.
// The clock is the other crew. Their berth, down the quay, does not count.
#pragma once
#include "art.h"
#include "console/system.h"

namespace ferry {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FERRY BUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    int leg() const { return leg_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return surge_; }
    float crewLeft() const;
    float roundProg() const;
    // 0 title, 1 outbound, 2 rounding, 3 the return, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Sail, Pause, Win, Fail };

    struct Round {
        float accum = 0, prev = 0;
        bool have = false, near = false;
    };
    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& throttle, float& thruster);
    void pilot(float& steer, float& throttle, float& thruster);
    void steerToward(float tx, float ty, float& steer) const;
    void orbit(int index, float& steer, float& throttle);
    void dockPilot(float& steer, float& throttle, float& thruster);
    void physics(float steer, float throttle, float thruster);
    void roundBuoy();
    bool inSlip() const;
    bool wrongDock() const;
    void win();
    void fail(const char* why, bool crew);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    int boatFrame() const;
    void puffAt(float x, float y);
    void smokeAt(float x, float y);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool crewFail_ = false;
    bool hornTwin_ = false;
    int leg_ = 0;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    int lastSec_ = -1;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, sway_ = 0, yaw_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.56f;
    float throttle_ = 0, thruster_ = 0;
    float hornT_ = 0, hornF_ = 98.f;
    float tone0_ = 0, chimeT_ = 0, thumpT_ = 0, wakeT_ = 0, smokeT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Round round_[3]{};
    Puff wake_[18]{};
    Puff smoke_[10]{};
    char report_[200] = {};
    char why_[72] = {};
};

}  // namespace ferry
