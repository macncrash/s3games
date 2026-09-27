// S3 TABLE GOLD — the line is 4 on a short table.
// A gold cross counts two. A cream cross counts one.
// A cream cross that would reach the line does not count.
// Only a gold cross can finish, and the undoubled crosses must stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tablegold {

constexpr int kLine = 4;
constexpr int kRallies = 5;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int goals() const { return goals_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* phase() const;
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Face, Play, Call, Win, Lose, Pause };

    struct Input {
        float x = 0;
        float y = 0;
        bool slap = false;
        bool start = false;
        bool back = false;
    };

    void toTitle();
    void beginRound();
    void faceoff();
    void driveYou(const Input& in, float dt);
    void driveThem(float dt);
    void physics(float dt);
    void finishCross(bool you);
    void afterCall();
    void win();
    void lose();
    bool paid() const;
    bool markGold() const;
    bool crossed(bool top) const;
    void bounceMallet(float mx, float my, float mvx, float mvy);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;

    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void hush();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int goals_ = 0;
    int them_ = 0;
    int mark_ = 0;
    int result_[kRallies] = {};
    float puckX_ = 160.f;
    float puckY_ = 120.f;
    float puckVX_ = 0;
    float puckVY_ = 0;
    float px_ = 160.f, py_ = 168.f, pvx_ = 0, pvy_ = 0;
    float ox_ = 160.f, oy_ = 78.f, ovx_ = 0, ovy_ = 0;
    float clock_ = 0;
    float callT_ = 0;
    float faceT_ = 0;
    float beep_ = 0;
    const char* say_ = "";
};

}  // namespace tablegold
