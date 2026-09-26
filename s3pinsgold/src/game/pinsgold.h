// S3 PINS GOLD — one frame, two balls.
// Cream counts one. Gold counts two. Ten wins.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinsgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINS GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int goldDown() const { return gold_; }
    int creamDown() const { return cream_; }
    const char* verdict() const { return verdict_; }

private:
    enum class Mode { Title, Approach, Swing, Roll, Sweep, Banner, Over, Pause };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        bool gold = false;
        bool down = false;
        bool hidden = false;
        int side = 1;
        float fall = 0;
    };
    struct Pop {
        float x = 0, z = 0, t = 0;
        int pts = 0;
    };

    void showTitle();
    void newGame();
    void resetRack();
    void beginSwing();
    void release();
    void tryDrive(float entry);
    void topple(Pin& p);
    void physics(float dt);
    void judge();
    void beginSweep();
    void beginBanner();
    void draw();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    float meter() const;
    float aimX() const;
    float aimTarget() const;
    bool onPocket() const;
    bool starReady() const;
    int pose() const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blip(float freq);
    void silence();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    const char* verdict_ = "";
    int score_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int ball_ = 0;
    int popN_ = 0;
    float stance_ = 0;
    float swingT_ = 0;
    float clock_ = 0;
    float rollT_ = 0;
    float sweepT_ = 0;
    float bannerT_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float steer_ = 0;
    bool ballLive_ = false;
    bool gutter_ = false;
    bool gutterSnd_ = false;
    bool driven_ = false;
    bool wasPocket_ = false;
    bool cheered_ = false;
    float ballX_ = 0, ballZ_ = 0, ballVx_ = 0, ballVz_ = 0;
    Pin pin_[10];
    Pop pop_[8];
};

}  // namespace pinsgold
