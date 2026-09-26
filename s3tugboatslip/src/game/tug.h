// S3 TUGBOAT SLIP — berth the tug in the slip before the tide turns.
#pragma once
#include "art.h"
#include "console/system.h"

namespace tugslip {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TUGBOAT SLIP"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return raceTime_; }
    float tideLeft() const;
    float x() const { return x_; }
    float y() const { return y_; }
    float heading() const { return heading_; }
    float speed() const { return surge_; }
    bool inSlip() const { return inSlip_; }
    bool inEnd() const { return inEnd_; }
    const char* why() const { return why_; }
    // 0 title, 1 fairway, 2 in the slip, 3 in the berth, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    void begin();
    void showTitle();
    void startRun();
    void human(float& steer, float& throttle);
    void pilot(float& steer, float& throttle);
    void physics(float dt, float steer, float throttle);
    void particles(float dt);
    void succeed();
    void fail(const char* why);
    void blip(float freq);
    void chime(int notes);
    void audio(float dt);
    void camera();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void place(const gs::Mipped& m, float wx, float wy, float worldH, int pal, float minPx = 0);
    int hullFrame() const;
    const char* hint() const;
    const char* orderName() const;
    float tideU() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool inSlip_ = false;
    bool inEnd_ = false;
    bool committed_ = false;
    bool entered_ = false;
    int chimeN_ = 0, chimeStep_ = 0;
    int wakeCursor_ = 0, smokeCursor_ = 0;
    float t_ = 0, raceTime_ = 0;
    float x_ = 0, y_ = 0, heading_ = 0, surge_ = 0, yaw_ = 0, throttle_ = 0;
    float camX_ = 0, camY_ = 0, zoom_ = 1.f;
    float settle_ = 0, short_ = 0;
    float tone0_ = 0, tone1_ = 0, chimeT_ = 0, thumpT_ = 0;
    float wakeT_ = 0, smokeT_ = 0, tickT_ = 0, hornT_ = 0;
    float stuckT_ = 0, stuckX_ = 0, stuckY_ = 0;
    Puff wakes_[12]{};
    Puff smoke_[10]{};
    char why_[48] = {};
};

}  // namespace tugslip
