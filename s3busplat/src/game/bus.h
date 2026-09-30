// S3 BUS PLAT — take the bus and stop level with the platform.
// The door has to sit on the stripe, and the sill has to meet the deck.
// The other crew is the clock: they take the stand if you are still rolling.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace busplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BUS PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_ ? why_ : ""; }
    float door() const { return s_; }
    float sill() const { return h_; }
    float speed() const { return v_; }
    // 0 title, 1 the approach, 2 alongside the platform, 3 holding level, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Pause, Fail, Win };

    void showTitle();
    void startRun();
    void pilot(float& gas, float& brake, float& lift) const;
    void physics(float gas, float brake, float lift);
    void win();
    void fail(const char* why);
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* why_ = "";
    float time_ = 0;
    float s_ = 0;      // door, metres along the stand
    float h_ = 0;      // sill height
    float v_ = 0;
    float rival_ = 0;  // other crew's door
    float gas_ = 0, brake_ = 0, lift_ = 0;
    float hold_ = 0;
    float dwell_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    int chime_ = -1;
    float chimeT_ = 0;
};

}  // namespace busplat
