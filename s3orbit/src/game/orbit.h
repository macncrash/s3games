// S3 ORBIT — dock the shuttle to the station arm. A hard contact fails the try.
#pragma once
#include <string>

#include "console/system.h"
#include "game/art.h"
#include "game/world.h"

namespace orbit {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ORBIT"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int contact() const { return contact_; }
    const char* why() const { return why_ && why_[0] ? why_ : "hung"; }

private:
    enum class Mode { Title, Play, Pause, Win, Fail };

    struct Arm {
        float ex, ey, tx, ty, tvx, tvy;
    };

    Arm pose(float t) const;
    void begin();
    void update(float dt);
    void human(float& ax, float& ay);
    void pilot(const Arm& a, float& ax, float& ay);
    void physics(float dt, float ax, float ay);
    bool hazards(const Arm& a);
    void dock(const Arm& a);
    void miss(const char* why);
    void lose(const char* why);
    void latch(float rel);
    void stick();
    void chime(float dt);
    void sky();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void boom(float x0, float y0, float x1, float y1, float jx, float jy);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    bool startPressed() const;
    bool ball(float cx, float cy, float r, float L, float T, float W, float H) const;
    float segDist(float px, float py, float ax, float ay, float bx, float by) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool fine_ = false;
    int contact_ = 0;
    int tries_ = kTries;
    int phase_ = 0;
    int chime_ = 0;
    float chimeT_ = 0;
    float t_ = 0;
    float x_ = 0, y_ = 0, vx_ = 0, vy_ = 0;
    float ax_ = 0, ay_ = 0;
    float dwell_ = 0, stun_ = 0, shake_ = 0, flash_ = 0, clock_ = kWindow;
    const char* why_ = "hung";
};

}  // namespace orbit
