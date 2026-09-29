// S3 ANVIL SEVEN — play the anvil until first to seven.
// Gold on your bar is two. The rival's cream is one.
// A count that steps past seven still stands. Leave when you are first.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace anvilseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ANVIL SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Heat, Spark, Leave, Pause, Over };

    void begin();
    void strike(bool hit);
    void afterSpark();
    void leave();
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Heat;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int heats_ = 0;
    int sparkN_ = 0;
    const char* why_ = "";
};

}  // namespace anvilseven
