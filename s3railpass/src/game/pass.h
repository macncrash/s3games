// S3 RAIL PASS — one rail, one job: clear the pass before the storm clock.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace railpass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RAIL PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float clockLeft() const { return clock_; }
    float meters() const { return carX_; }
    const char* why() const { return why_; }
    // 0 title, 1 lower slope, 2 the drifts, 3 the mouth, 4 cleared
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };
    struct Drift {
        float x;
        bool live;
    };

    void begin();
    void logic();
    void pilot(float& accel);
    void fail(const char* why);
    void succeed();
    void toneTick(float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet = false);
    void text(const char* s, float x, float y, float scale, int pal, int align = -1);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Drift drifts_[5]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool plow_ = false;
    const char* why_ = "";
    float time_ = 0;
    float clock_ = 0;
    float carX_ = 0;
    float speed_ = 0;
    float cam_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int roll_ = 0;
    int cleared_ = 0;
};

}  // namespace railpass
