// S3 CURLBELL — one sheet, three tries.
// The bell rings when a stone stops on the button.
// Hogged, burned, wide, or off the button: that try dies.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace curlbell {

enum class Why { None, Hog, Burn, Wide, Light, Heavy, Miss };

struct Rock {
    float x = 0, y = 0, vx = 0, vy = 0;
    int handle = 0;
    bool dead = false;
    bool ours = false;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURLBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool onBell() const;
    bool rules() const { return solved_; }
    bool sliding() const { return mode_ == Mode::Slide && slideT_ > 0.12f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Slide, Dead, Ring, Leave, Pause, Over };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    struct ShotEnd {
        float x = 0, y = 0;
        bool dead = false;
        bool hit = false;
        bool bell = false;
    };

    void place();
    void toTitle();
    void newGame();
    void beginAim();
    void solve();
    ShotEnd cast(float vx, float vy, int handle, bool record);
    void launch();
    void settle();
    void ring();
    void dieTry(Why why);
    void blip(float a, float b, float hold);
    void decayAudio();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Why why_ = Why::None;
    Rock rock_[kMaxRock];
    Puff puff_[8];
    int n_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool onBell_ = false;
    bool solved_ = false;
    bool sweepArm_ = false;
    bool hitSnd_ = false;
    bool wasBell_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int thrown_ = 0;
    int handle_ = 1;
    float aimVx_ = 0;
    float aimVy_ = 15.f;
    float solVx_ = 0, solVy_ = 15.f, solDist_ = 99.f;
    int solHandle_ = 1;
    float sweep_ = 0;
    float clock_ = 0;
    float aimT_ = 0;
    float slideT_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.22f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float lieX_ = 0, lieY_ = 0;
    float ghostX_ = 0, ghostY_ = 0;
    bool ghostBell_ = false;
    bool ghostDead_ = false;
    float pathX_[8] = {};
    float pathY_[8] = {};
    int pathN_ = 0;
};

}  // namespace curlbell
