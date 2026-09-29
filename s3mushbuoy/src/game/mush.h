// S3 MUSH BUOY — take the team around the buoys and beat the other crew home.
// The same dock is the only finish. The clock is that other crew.
#pragma once
#include "art.h"
#include "console/system.h"

namespace mush {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSH BUOY"; }
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
    float speed() const;
    float seconds() const { return raceTime_; }
    // 0 title, 1 out on the ice, 2 rounding, 3 the return, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void advanceMarks();
    void moveCrew(float dt);
    void crewAt(float dist, float& ox, float& oy, float& oh) const;
    bool inHome() const;
    bool inDecoy() const;
    void finish();
    void fail(const char* why);
    void yip();
    void chime(int notes);
    void audio(float dt);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    int sledFrame(float heading) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int leg_ = 0;
    int wp_ = 0;
    int chimeN_ = 0;
    int chimeStep_ = 0;
    int puffCursor_ = 0;
    float t_ = 0;
    float raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float vx_ = 0, vy_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 0.58f;
    float throttle_ = 0;
    float crewDist_ = 0;
    float pathLen_ = 1;
    float tone0_ = 0, chimeT_ = 0, yipT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    float decoyT_ = 0;
    Puff spray_[16]{};
    char why_[64] = {};
    char line_[48] = {};
};

}  // namespace mush
