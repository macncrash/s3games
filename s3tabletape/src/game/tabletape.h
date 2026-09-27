// S3 TABLETAPE — play the table until the drawer matches the tape.
// A puck that crosses the far mouth in the next taped band drops that slip.
// Leave only when L4, C6 and R2 are in the till, in that order.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tabletape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < 3 && held_[i]; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Serve, Play, Goal, Pause, Leave, Lose };

    void begin();
    void faceoff();
    int nextOpen() const;
    void award(bool you, int band);
    void driveYou(float ix, float iy);
    void driveThem();
    void stepBodies();
    void walls();
    bool bump(float& mx, float& my, float& mvx, float& mvy);
    bool crossed(bool top) const;
    int bandOf(float x) const;
    void steer(float& vx, float& vy, float x, float y, float tx, float ty, float speed);

    void blip(float freq, float vol);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool live_ = false;
    bool yours_ = true;
    bool slapped_ = false;
    bool tapped_ = false;
    bool shot_ = false;
    bool held_[3] = {};
    int you_ = 0;
    int them_ = 0;
    int traps_ = 0;
    int lastBand_ = -1;
    float clock_ = 0;
    float serve_ = 0;
    float banner_ = 0;
    float beep_ = 0;
    float tapCd_ = 0;
    float stall_ = 0;
    char reason_[40] = {};
    float puckX_ = 160, puckY_ = 100, puckVX_ = 0, puckVY_ = 0;
    float px_ = 160, py_ = 140, pvx_ = 0, pvy_ = 0;
    float ox_ = 160, oy_ = 50, ovx_ = 0, ovy_ = 0;
};

}  // namespace tabletape
