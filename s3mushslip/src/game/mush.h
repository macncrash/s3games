// S3 MUSH SLIP — berth the team in the slip before the tide turns.
// The clock is the other crew.
#pragma once
#include "art.h"
#include "console/system.h"

namespace mushslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSH SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* why() const { return why_; }
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const;
    float seconds() const { return race_; }
    float tideLeft() const;
    bool inSlip() const { return inSlip_; }
    bool berthed() const { return holding_; }
    // 0 title, 1 on the ice, 2 in the slip, 3 holding the berth, 4 ended
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Win, Fail };

    struct Puff {
        float x, y, life;
    };

    void begin();
    void showTitle();
    void controls(float& steer, float& throttle, float& brake);
    void pilot(float& steer, float& throttle, float& brake);
    void physics(float dt, float steer, float throttle, float brake);
    void rivalStep(float dt);
    void judge();
    void win();
    void fail(const char* why);
    void audio(float dt);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void drawHud();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal);
    void worldToScreen(float wx, float wy, float& sx, float& sy) const;
    int sledFrame(float heading) const;
    bool insideSlip(float px, float py) const;
    bool inBerth(float px, float py) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inSlip_ = false;
    bool holding_ = false;
    float t_ = 0;
    float race_ = 0;
    float hold_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0;
    float vx_ = 0, vy_ = 0;
    float rx_ = 0, ry_ = 0, rv_ = 0, rH_ = 0;
    bool rivalIn_ = false;
    bool rivalDone_ = false;
    float camX_ = 0, camY_ = 0;
    float zoom_ = 5.2f;
    float tone0_ = 0;
    int puffCursor_ = 0;
    Puff spray_[10]{};
    char why_[64] = {};
};

}  // namespace mushslip
