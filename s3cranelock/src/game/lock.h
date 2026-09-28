// S3 CRANE LOCK — take the crane through one canal lock.
// A scrape on a gate fails the job. The other crew is the clock.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace cranelock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANE LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return shiftT_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Shift, Fail, Win };

    void begin();
    void update(float dt);
    void draw();
    void botPlan(float& ax, float& ay, float& slew, bool& open);
    bool hitsGate(float gx0, float gx1, float gy) const;
    void scrape();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool openLo_ = false;
    bool openHi_ = false;
    float lo_ = 0;
    float hi_ = 0;
    float hx_ = 160;
    float hy_ = 176;
    float ang_ = 0.7f;
    float shiftT_ = 0;
    float t_ = 0;
    const char* why_ = "the other crew took the lock";
};

}  // namespace cranelock
