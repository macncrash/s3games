// S3 MASKBELL — a short mask.
// Four cuts ride one punch: brow, left eye, right eye, mouth.
// Strike only inside that cut's groove. Ring the bell only when all four
// are locked. A false strike or an early ring kills the try. The bell
// rings, and the cartridge ends, only if that happens before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace maskbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MASKBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    int locked() const { return locked_; }
    bool sealed() const { return locked_ == kCuts && rung_; }

private:
    enum class Mode { Title, Play, Ring, Lose };

    void begin();
    void resetBlank(int which);
    void strike();
    void ring();
    void dieTry();
    void botAct();
    void human(const gs::Pad& pad);
    bool hot(int i) const;
    bool allLocked() const { return locked_ == kCuts; }
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool pause_ = false;
    bool cutOn_[kCuts] = {};
    int locked_ = 0;
    int phase_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int swing_ = 0;
    int hold_ = 0;
    int foul_ = 0;
    int skip_ = 0;
};

}  // namespace maskbell
