// S3 SLED BOX — take the sled and stop inside the box.
// The clock is the other crew. The whole sled has to be inside the rope,
// stopped, before their time runs out. Close to the rope is still outside.
#pragma once
#include "art.h"
#include "console/system.h"

namespace sledbox {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SLED BOX"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    const char* report() const { return report_; }
    float seconds() const { return raceTime_; }
    float crewLeft() const { return crew_ > 0.f ? crew_ : 0.f; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const;
    // 0 title, 1 on the way, 2 in the box, 3 holding the stop, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    struct Flake {
        float x = 0, y = 0, v = 0, w = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float steer, float throttle);
    void corner(int i, float& wx, float& wy) const;
    bool hullInside() const;
    bool pastRope() const;
    void win();
    void fail(const char* why);
    void yip();
    void blip(float freq);
    void chime(int notes);
    void thud();
    void audio();
    void camera();
    void crewPose(float& x, float& y, float& h) const;
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int sledFrame(float heading) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int chimeN_ = 0;
    int chimeStep_ = 0;
    int puffCursor_ = 0;
    int lastSec_ = 0;
    float t_ = 0;
    float raceTime_ = 0;
    float crew_ = 0;
    float holdT_ = 0;
    float outT_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float vx_ = 0, vy_ = 0;
    float throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0, sprayT_ = 0, yipT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Puff puffs_[14]{};
    Flake flakes_[20]{};
    char why_[64] = {};
    char report_[180] = {};
};

}  // namespace sledbox
