// S3 BELL SEVEN — play the bell until first to seven.
// Your toll is two. Their chime is one. Leave when you are first.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace bellseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BELL SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int you() const { return you_; }
    int them() const { return them_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Swing, Toll, Leave, Pause, Over };

    void begin();
    void pull(bool hit);
    void afterToll();
    void leave();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Swing;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool yours_ = true;
    bool lastHit_ = false;
    int you_ = 0;
    int them_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int pulls_ = 0;
    const char* why_ = "";
};

}  // namespace bellseven
