// S3 PARADE TAPE — march the street until the drawer matches the tape, then leave.
// The tape wants RED, GOLD, BLUE in that order.
// Reaching the square on the next colour files that slip. A wrong column is a trap
// and stays out of the drawer. Leave only when the three slips match.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace paradetape {

constexpr int kTapeN = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE TAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool marching() const { return mode_ == Mode::March && py_ < 180.f; }
    int crossings() const { return crossings_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    int phase() const;

private:
    enum class Mode { Title, March, Leave };

    struct Lane {
        float y;
        float speed;
        float spacing;
        float width;
        float phase;
    };

    void checkRules();
    void toTitle();
    void begin();
    void logic();
    void human();
    void bot();
    void arrive();
    void bump();
    bool danger(float x, float y, int fr) const;
    bool climbClear(float x, float y, int fr) const;
    int columnAt(float x) const;
    int nextSlip() const;
    void audio();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Lane lanes_[3] = {};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool committed_ = false;
    int crossings_ = 0;
    int traps_ = 0;
    int frame_ = 0;
    int age_ = 0;
    int inv_ = 0;
    int flash_ = 0;
    int fan_ = -1;
    int lastBeat_ = -1;
    int aim_ = 0;
    float px_ = 160.f;
    float py_ = 198.f;
    float face_ = 1.f;
    const char* reason_ = "OPEN";
};

}  // namespace paradetape
