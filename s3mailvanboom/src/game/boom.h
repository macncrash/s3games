// Take the mail van and deliver the drive to the boom.
// The clock is the other crew. Stop in the slot or they take the gate.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace boom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MAILVAN BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 the drive, 2 the approach, 3 the slot, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& steer) const;
    void physics(float gas, float steer);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void skyRoad();
    bool project(float wz, float wy, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float centerAt(float s) const;
    float halfAt(float s) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    int chime_ = -1;
    float chimeT_ = 0;
    float beep_ = 0;
    float time_ = 0;
    float clock_ = 0;
    float s_ = 0;
    float y_ = 0;
    float v_ = 0;
    float vy_ = 0;
    float dwell_ = 0;
    float shake_ = 0;
};

}  // namespace boom
