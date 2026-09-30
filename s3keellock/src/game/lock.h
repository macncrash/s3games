// S3 KEEL LOCK — take the keel through one canal lock. A scrape on a gate fails.
// The other crew holds the clock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace keellock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KEEL LOCK"; }
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
    void pilot(float& thrust, float& steer);
    void step(float thrust, float steer);
    void scrape(const char* why);
    void finish();
    void blip(float freq);
    void draw();
    bool project(float wx, float wz, float& sx, float& sy, float& sc) const;
    void stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal);
    void worldQuad(const gs::Mipped& m, float x0, float z0, float x1, float z1, float lift, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float bow() const;
    float stern() const;
    float rivalLeft() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float z_ = 0, x_ = 0, heading_ = 0, speed_ = 0;
    float lo_ = 1, hi_ = 0, fill_ = 0;
    float camZ_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace keellock
