// S3 TABLE SEVEN — one table, two mallets, a puck that has to cross.
// A point counts only when the whole puck is past the goal line and still in the mouth.
// First tally to reach 7 wins, and the match ends. A 6 is still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tableseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool live() const { return live_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Serve, Play, Goal, Pause, Leave, Lose };

    void begin();
    void faceoff();
    void award(bool you);
    void driveYou(float ix, float iy);
    void driveThem();
    void stepBodies();
    void walls();
    bool bump(float& mx, float& my, float& mvx, float& mvy);
    bool crossed(bool top) const;
    void steer(float& vx, float& vy, float x, float y, float tx, float ty, float speed);

    void blip(float freq, float vol);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void pips(int col, int row, int n, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool live_ = false;
    bool yours_ = true;
    bool slapped_ = false;
    bool tapped_ = false;
    int you_ = 0;
    int them_ = 0;
    int goals_ = 0;
    float clock_ = 0;
    float serve_ = 0;
    float banner_ = 0;
    float beep_ = 0;
    float tapCd_ = 0;
    float stall_ = 0;
    float fanT_ = 0;
    int fanStep_ = -1;
    float puckX_ = 160, puckY_ = 116, puckVX_ = 0, puckVY_ = 0;
    float px_ = 160, py_ = 160, pvx_ = 0, pvy_ = 0;
    float ox_ = 160, oy_ = 70, ovx_ = 0, ovy_ = 0;
};

}  // namespace tableseven
