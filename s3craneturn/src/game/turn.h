// Take the crane through three turns without tipping. The clock is the other crew.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"

namespace craneturn {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CRANE TURN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int turns() const { return turns_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void bake();
    void sample(float s, float& x, float& y, float& h) const;
    float curvAt(float s) const;
    int turnExit(float prev, float now);
    void update(float dt);
    void physics(float dt, float stick, float throttle, float brake, float tuck);
    void draw();
    void skyRoad();
    bool project(float s, float lat, float& sx, float& sy, float& sc) const;
    void blit(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip = false, bool shadow = false);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool tipped_ = false;
    bool got_[4] = {};
    int turns_ = 0;
    float s_ = 0, v_ = 0, x_ = 0, xvel_ = 0, boom_ = 0.78f, lean_ = 0;
    float clock_ = 0, t_ = 0;
    float wx_[800] = {}, wy_[800] = {}, hd_[800] = {};
    const char* why_ = "";
    const char* banner_ = "";
};

}  // namespace craneturn
