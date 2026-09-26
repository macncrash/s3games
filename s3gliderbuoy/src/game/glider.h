// S3 GLIDER BUOY — round the buoys and set down on the same dock.
// The clock is the other crew. When it runs out, they have the dock.
#pragma once
#include "art.h"
#include "console/system.h"

namespace gbuoy {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GLIDER BUOY"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    int leg() const { return leg_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float alt() const { return alt_; }
    float heading() const { return heading_; }
    float speed() const { return spd_; }
    float roundProg() const;
    // 0 title, 1 outbound, 2 rounding, 3 the return, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Fly, Pause, Fail, Win };

    struct Mark {
        float accum = 0, prev = 0;
        bool have = false, near = false;
    };
    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startFly();
    void controls(float& bank, float& pitch);
    void pilot(float& bank, float& pitch);
    void steerToward(float tx, float ty, float& bank) const;
    void physics(float bankCmd, float pitchCmd);
    void roundBuoy();
    void tryDock();
    void win();
    void fail(const char* why, bool crew);
    void blip(float freq);
    void chime(int notes);
    void audio();
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int wingFrame() const;
    float crewLeft() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool crewFail_ = false;
    bool final_ = false;
    int leg_ = 0;
    int wp_ = 0;
    int orbitSign_ = 1;
    int chimeN_ = 0, chimeStep_ = 0;
    int puffCursor_ = 0;
    int lastSec_ = -1;
    float t_ = 0, raceTime_ = 0, legTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, spd_ = 0, alt_ = 0, vs_ = 0;
    float bank_ = 0, pitch_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.46f;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, varioT_ = 0, puffT_ = 0, thumpT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Mark mark_[3]{};
    Puff puffs_[12]{};
    char report_[200] = {};
    char why_[64] = {};
};

}  // namespace gbuoy
