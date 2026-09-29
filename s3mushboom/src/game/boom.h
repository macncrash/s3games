// S3 MUSH BOOM — the mush has one job: deliver the drive to the boom.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mushboom {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MUSH BOOM"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }
    const char* why() const { return why_ ? why_ : ""; }
    // 0 title, 1 carrying, 2 drive in the air, 3 drive on the boom, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void stepRun();
    void botInput(float& gas, bool& jump, bool& release);
    void fail(const char* why);
    void succeed();
    void blip(float freq);
    void paintSky();
    void spr(const gs::Mipped& m, float wx, float wy, float destH, int pal, bool flip = false, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float screenX(float wx) const;
    float screenY(float wy) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carried_ = true;
    bool onGround_ = true;
    bool driveDeck_ = false;
    bool jumped_ = false;
    float time_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float dx_ = 0, dy_ = 0, dvx_ = 0, dvy_ = 0;
    float still_ = 0;
    float cam_ = 0;
    int anim_ = 0;
    const char* why_ = "";
};

}  // namespace mushboom
