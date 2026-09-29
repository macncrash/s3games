// S3 BIKE LOCK — ride one canal lock. A scrape on a gate fails the job.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace bikelock {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BIKE LOCK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 approaching the lower gate, 2 in the chamber, 3 leaving, 4 passed
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    enum class Phase { Approach, Settle, Shut, Fill, Open, Leave };

    void begin();
    void logic();
    void pilot();
    bool hitGate(float gx, float bottom, float deck) const;
    void scrape(const char* why);
    void succeed();
    float deckAt(float x) const;
    void toneTick(float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet = false);
    void text(const char* s, float x, float y, float scale, int pal, int align = -1);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool duck_ = false;
    const char* why_ = "";
    float time_ = 0;
    float bikeX_ = 0;
    float speed_ = 0;
    float water_ = 0;
    float lower_ = 0;
    float upper_ = 0;
    float cam_ = 0;
    int still_ = 0;
    int wheel_ = 0;
    float shake_ = 0;
    float beep_ = 0;
};

}  // namespace bikelock
