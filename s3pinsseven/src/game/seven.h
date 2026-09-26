// S3 PINS SEVEN — a short rack. First bowler to seven wins.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinsseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINS SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    // 0 title, 1 approach, 2 the ball is running, 3 the match is decided
    int phase() const;

private:
    enum class Mode { Title, Approach, Swing, Roll, Count, Win, Lose, Pause };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        int row = 0;
        bool down = false;
        float fall = 0;
    };

    void newGame();
    void resetRack();
    void beginSwing();
    void release(float x, bool pocketShot);
    void sweep();
    void physics(float dt);
    void settle();
    void draw();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    float meter() const;
    float aimX() const;
    float standX() const;
    bool inPocket(float x) const;
    bool onPocket() const;
    int pose() const;
    int fallen() const;
    bool human() const { return !bot_ && yours_; }
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Approach;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    bool ballLive_ = false;
    bool gutter_ = false;
    bool gutterSnd_ = false;
    bool committed_ = false;
    bool pocket_ = false;
    bool entryTaken_ = false;
    int you_ = 0;
    int them_ = 0;
    int knock_ = 0;
    int fresh_ = 0;
    float stance_ = 0;
    float swingT_ = 0;
    float clock_ = 0;
    float rollT_ = 0;
    float countT_ = 0;
    float shake_ = 0;
    float beep_ = 0;
    float hook_ = 0;
    float entry_ = 0;
    float ballX_ = 0, ballZ_ = 0, ballVx_ = 0, ballVz_ = 0;
    Pin pin_[7];
};

}  // namespace pinsseven
