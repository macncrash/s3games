// S3 OVEN GOLD — a short oven. The line is 4.
// A gold crust in the gold band counts two. Cream counts one.
// Cream that would reach the line does not count.
// Only a gold double can finish, and the undoubled draws stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace ovengold {

constexpr int kLine = 4;
constexpr int kPans = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVEN GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int loaves() const { return gold_ + cream_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* phase() const;
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Bake, Show, Win, Lose, Pause };

    void toTitle();
    void begin();
    void nextPan();
    void pull();
    void burn();
    void win();
    void lose();
    void afterShow();
    bool paid() const;
    void blip(float freq);
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool goldKind_[kPans] = {true, false, true};
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int pan_ = 0;
    int heat_ = 0;
    int lock_ = 0;
    int show_ = 0;
    int mark_[kPans] = {-2, -2, -2};
    float anim_ = 0;
    char say_[24] = {};
};

}  // namespace ovengold
