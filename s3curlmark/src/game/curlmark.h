// S3 CURLMARK — one sheet. The gold coin is the mark.
// A stone that stops on it opens the mark. Lifting the coin finishes it.
// That finished mark ends the cartridge. The rest of an end is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace curlmark {

struct Rock {
    float x = 0, y = 0, vx = 0, vy = 0;
    int handle = 0;
    bool dead = false;
    bool ours = false;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURLMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool onMark() const { return onMark_; }
    bool sliding() const { return mode_ == Mode::Slide; }
    int stone() const { return stone_; }
    int throws() const { return thrown_; }
    float lieX() const { return lieX_; }
    float lieY() const { return lieY_; }

private:
    enum class Mode { Title, Aim, Slide, Open, Pause, Win, Lose };

    struct Puff {
        float x = 0, y = 0, life = 0;
    };
    struct ShotEnd {
        float x = 0, y = 0;
        bool dead = false;
        bool hit = false;
    };

    void place();
    void begin();
    void toTitle();
    void launch(float vx, float vy);
    void settle();
    void finish();
    void fail();
    bool solve(float& vx, float& vy, int& handle);
    ShotEnd cast(float vx, float vy, int handle, bool record);
    void blip(float freq, float vol);
    void chord();
    void decayAudio();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Rock rock_[kMaxRock];
    Puff puff_[8];
    int n_ = 0;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool lifted_ = false;
    bool bitten_ = false;
    bool onMark_ = false;
    bool solved_ = false;
    bool wasOn_ = false;
    bool hitSnd_ = false;
    bool sweepArm_ = false;
    int thrown_ = 0;
    int stone_ = 0;
    int handle_ = 1;
    float aimVx_ = 0;
    float aimVy_ = 15.4f;
    float sweep_ = 0;
    float broomX_ = 0, broomY_ = 0;
    float lieX_ = 0, lieY_ = 0;
    float solVx_ = 0, solVy_ = 16, solDist_ = 99;
    int solHandle_ = 1;
    float ghostX_ = 0, ghostY_ = 0;
    bool ghostOn_ = false;
    bool ghostDead_ = false;
    float pathX_[6] = {};
    float pathY_[6] = {};
    int pathN_ = 0;
    float clock_ = 0;
    float toneT_ = 0;
    bool chord_ = false;
};

}  // namespace curlmark
