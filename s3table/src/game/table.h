// S3 TABLE — first to seven. The puck has to cross the line.
#pragma once

#include <string>

#include "console/system.h"
#include "game/art.h"
#include "game/pitch.h"

namespace table {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }

private:
    enum class Mode { Title, Serve, Play, Goal, Pause, Over };

    void newGame();
    void faceoff();
    void drivePlayer();
    void driveOpp();
    void physics();
    void score(bool you);
    bool crossed(bool top) const;
    void clampMallet(float& x, float& y, float& vx, float& vy, bool you);
    void splitMallets();
    void walls();
    bool hitMallet(float& mx, float& my, float& mvx, float& mvy);
    void steer(float& vx, float& vy, float x, float y, float tx, float ty, float speed, bool charge, float nx, float ny);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void text(const std::string& s, float x, float y, float scale, int pal);
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void pips(int col, int row, int n, int on, int off);
    void blip(float freq, float vol);
    void draw();
    float openX() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool live_ = false;
    bool tapped_ = false;
    bool slapped_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int biasI_ = 0;
    int fanStep_ = -1;
    float bias_ = 18.f;
    float clock_ = 0;
    float serve_ = 0;
    float banner_ = 0;
    float stall_ = 0;
    float rally_ = 0;
    float beep_ = 0;
    float flash_ = 0;
    float fanT_ = 0;
    float tapCd_ = 0;
    float track_ = kCenter;
    float puckX_ = kCenter, puckY_ = kMid, puckVX_ = 0, puckVY_ = 0;
    float px_ = kCenter, py_ = 158, pvx_ = 0, pvy_ = 0;
    float ox_ = kCenter, oy_ = 74, ovx_ = 0, ovy_ = 0;
    float aimNX_ = 0, aimNY_ = -1;
};

}  // namespace table
