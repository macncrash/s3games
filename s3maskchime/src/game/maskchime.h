// S3 MASK CHIME — play mask until the hour has to chime. Leave when that is true.
// Four cuts: brow, left eye, right eye, mouth. Strike only in the open groove.
// A sealed mask on the hour is the only chime. Early asks wait. Three dead tries,
// or a mask still open after the hour, and the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace maskchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASK CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    int cuts() const { return locked_; }
    int tryNo() const { return tryNo_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Hold, Chime, Leave, Fail };

    void begin();
    void blank();
    void strike();
    void askHour();
    void dieTry();
    void beginChime();
    void beginFail(const char* why);
    void botAct();
    void human(const gs::Pad& pad);
    bool hot() const;
    bool sealed() const { return locked_ == kCuts; }
    int openCut() const;
    float needle() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hand(float cx, float cy, float ang, float len, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool cutOn_[kCuts] = {};
    int locked_ = 0;
    int phase_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int playFrames_ = 0;
    int hold_ = 0;
    int titleWait_ = 0;
    int flash_ = 0;
    int strikes_ = 0;
    int swing_ = 0;
};

}  // namespace maskchime
