// S3 CLOCK GOLD — play the clock until only the gold counts double.
// A gold hour is ten, counted twice. A cream hour is nine and does not double.
// Leave when the doubled gold is what clears the line and the bare faces are still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace clockgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCK GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool goldOut() const { return finisher_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int strikes() const { return strikes_; }
    int line() const { return kLine; }
    int hour() const { return hour_; }

private:
    enum class Mode { Title, Play, Strike, Pause, Over };

    void begin();
    void turn(int dir);
    void strike();
    void leave();
    void blip(float freq);
    void chime(bool gold);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool open() const;
    int nextGold() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisher_ = false;
    bool struck_[kHours] = {};
    int hour_ = 11;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int strikes_ = 0;
    int rep_ = 0;
    int anim_ = 0;
    int sec_ = 0;
    int secAcc_ = 0;
};

}  // namespace clockgold
