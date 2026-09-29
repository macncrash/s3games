// S3 CLOCKBELL — a short clock.
// Two hands. Haul the rope only when they sit on the posted hour.
// A false haul kills the try. The bell rings, and the cartridge ends,
// only if that happens before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace clockbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCKBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int posted() const { return kTarget; }

private:
    enum class Mode { Title, Play, Ring, Lose };

    void begin();
    void resetFace(int which);
    void turn(int dir);
    void haul();
    void dieTry();
    void ring();
    void botAct();
    void human(const gs::Pad& pad);
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool trueHour() const { return hour_ == kTarget && minute_ == 0; }
    int hourStep() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool pause_ = false;
    int hour_ = 11;
    int minute_ = 37;
    int grip_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int rep_ = 0;
    int swing_ = 0;
    int ropeY_ = 0;
    int hold_ = 0;
    int foul_ = 0;
};

}  // namespace clockbell
