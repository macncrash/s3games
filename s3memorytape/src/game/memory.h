// S3 MEMORY TAPE — recall the tape into the drawer.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace memorytape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORY TAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool matched() const;
    int tries() const { return tries_; }
    // 0 title, 1 studying the tape, 2 filling the drawer, 3 sealed, 4 finished
    int marker() const;

private:
    enum class Phase { Title, Study, Recall, Seal, Victory };

    void update();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h, int pal);
    void blip(float freq);
    void place();
    void check();

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int drawer_[LEN] = {};
    int filled_ = 0;
    int cursor_ = 0;
    int tries_ = 0;
    int t_ = 0;
    int beep_ = 0;
    bool showTape_ = true;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool paused_ = false;
};

}  // namespace memorytape
