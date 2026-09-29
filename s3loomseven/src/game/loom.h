// S3 LOOM SEVEN — two benches, one shed each. First to seven wefts.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace loomseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOM SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int house() const { return house_; }

private:
    enum class Mode { Title, Fly, Beat, Win, Lose };

    void begin();
    void throwShuttle(bool inShed);
    void houseTick();
    void finish();
    void blip(float freq);
    void draw();
    void sky();
    void bench(float cy, int rows, int pal, float shuttleX, bool showShuttle, bool rtl);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, float cx, float cy, int pal, bool flip = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int you_ = 0;
    int house_ = 0;
    int wind_ = 0;
    int beat_ = 0;
    int houseClock_ = 0;
    bool rtl_ = false;
};

}  // namespace loomseven
