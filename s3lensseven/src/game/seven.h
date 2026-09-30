// S3 LENS SEVEN — play the lens until first to seven.
// Your gold plate is two. The rival's cream is one.
// A count that steps past seven still stands. Leave when you are first.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lensseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LENS SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Rack, Seat, Leave, Pause, Over };

    void begin();
    void seat(bool hit);
    void afterSeat();
    void leave();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Rack;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int seated_ = 0;
    int flareN_ = 0;
    const char* why_ = "";
};

}  // namespace lensseven
