// S3 CLIFF PLAT — hoist the cliff cage and stop level with the station platform.
// The other crew is the clock. If they reach the stripe first, the stand is theirs.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace cliffplat {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLIFF PLAT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    float seconds() const { return time_; }

private:
    enum class Mode { Title, Run, Win, Lose };

    void begin();
    void pilot(float& gas, float& brake, float& lift) const;
    void physics(float gas, float brake, float lift);
    void draw();
    void sky();
    void spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float time_ = 0;
    float s_ = 0;      // cage, metres along the shelf
    float h_ = 0;      // cage floor height
    float v_ = 0;
    float rival_ = 0;
    float hold_ = 0;
    float idle_ = 0;
    float shake_ = 0;
};

}  // namespace cliffplat
