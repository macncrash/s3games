// S3 BUS PASS — take the bus through the mountain pass.
// Clear it before the storm. The clock is the other crew.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace buspass {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS PASS"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    float crewLeft() const { return crewLeft_; }
    const char* why() const { return why_; }
    // 0 title, 1 climbing, 2 at a snow gate, 3 last shelf, 4 cleared
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void begin();
    void logic();
    void pilot(float& accel, float& steer);
    void failRun(const char* why);
    void succeed();
    float roadY(float x) const;
    float halfAt(float x) const;
    void blip(float freq, float vol);
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool feet = false);
    void text(const char* s, float x, float y, float scale, int pal, int align = -1);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float crewLeft_ = 0;
    float busX_ = 0;
    float offset_ = 0;
    float speed_ = 0;
    float gateOpen_[3] = {};
    float cam_ = 0;
    int wheel_ = 0;
    float shake_ = 0;
    float beep_ = 0;
};

}  // namespace buspass
