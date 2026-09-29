// S3 BEDSBELL — water the six beds.
// Each bed fills on its own. Pour only while the bar is gold.
// A dry pour or a flood kills the try. The bell rings, and the
// cartridge ends, only if all six hold before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace bedsbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BEDSBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int tryNo() const { return tryNo_; }
    int deadTries() const { return dead_; }
    int wet() const { return wet_; }

private:
    enum class Mode { Title, Play, Ring, Lose };

    void begin();
    void resetBed();
    void pour();
    void dieTry();
    void ring();
    void advance(float dt);
    void botAct();
    void human(const gs::Pad& pad);
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, float h, int pal, bool flip = false);
    void solid(float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool inGold() const { return moist_ >= kGoldLo && moist_ < kGoldHi; }

    static constexpr int kBeds = 6;
    static constexpr int kMaxDead = 3;
    static constexpr float kFill = 1.35f;
    static constexpr float kGoldLo = 0.52f;
    static constexpr float kGoldHi = 0.78f;
    static constexpr float kSweet = 0.64f;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool pause_ = false;
    bool held_[kBeds] = {};
    float moist_ = 0;
    float anim_ = 0;
    float splash_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int wet_ = 0;
    int swing_ = 0;
    int hold_ = 0;
    int foul_ = 0;
};

}  // namespace bedsbell
