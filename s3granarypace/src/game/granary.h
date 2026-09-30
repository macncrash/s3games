// S3 GRANARYPACE — one granary. Wait until the third pace before you fire.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace granary {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GRANARYPACE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int pace() const { return pace_; }
    int early() const { return early_; }
    int shots() const { return shots_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Pace, Done, Over };

    void begin();
    void shoot();
    void fail(const char* why);
    void tickPace();

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fired_ = false;
    int pace_ = 0;
    int step_ = 0;
    int hold_ = 0;
    int early_ = 0;
    int shots_ = 0;
    int anim_ = 0;
};

}  // namespace granary
