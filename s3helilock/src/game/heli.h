// S3 HELILOCK — fly the lock. A scrape on a gate fails the leg.
// Missing the far pad fails it too.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace helilock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HELILOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    enum class Phase { Approach, Shut, Fill, Open, Leave };

    void begin();
    void pilot(float& thrust, float& climb);
    void step(float thrust, float climb);
    void fail(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void quad(float x0, float y0, float x1, float y1, const gs::Mipped& m, int pal);
    void text(const char* s, float x, float y, float scale, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float lo_ = 1, hi_ = 0, fill_ = 0;
    float hold_ = 0;
    float cam_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int rotor_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace helilock
