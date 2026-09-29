// S3 CLOCKMARK — one short clock.
// The gold pip is the mark. Seat the hour hand on it, then lift the coin.
// That finished mark ends the cartridge. Chiming the hour is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace clockmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCKMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool marked() const { return marked_; }
    int hour() const { return hour_; }
    int tries() const { return tries_; }

private:
    enum class Mode { Title, Play, Lift, Pause, Over };

    void begin();
    void turn(int dir);
    void lift();
    void finish();
    void fail();
    void blip(float freq);
    void chord();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool onMark() const { return hour_ == kMark; }

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool finished_ = false;
    bool lifted_ = false;
    bool marked_ = false;
    int hour_ = 9;
    int tries_ = 3;
    int rep_ = 0;
    int liftT_ = 0;
    int botWait_ = 0;
    float coinY_ = 0;
};

}  // namespace clockmark
