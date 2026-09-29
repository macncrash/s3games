// S3 CLIFF LANE — stay inside the painted lane for the whole cliff leg.
// One departure fails the run, even if the cart later reaches the gate.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace clifflane {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFFLANE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_; }
    // 0 title, 1 in the lane, 2 on the edge, 3 the gate is ahead, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& brake, float& steer) const;
    void physics(float gas, float brake, float steer);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void skyShelf();
    bool project(float wz, float wy, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float margin() const;

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
    float s_ = 0;
    float y_ = 0;
    float v_ = 0;
    float gas_ = 0, brake_ = 0, steer_ = 0;
    float shake_ = 0;
};

}  // namespace clifflane
