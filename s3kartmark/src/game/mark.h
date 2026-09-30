// S3 KARTMARK — the kart has one job: set down on the painted mark.
// Rolling through the box, or stopping short of it, misses the set.
// The pit clock is the other crew. When it runs out they own the mark.
#pragma once
#include <algorithm>
#include "console/system.h"
#include "game/art.h"

namespace kartmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KARTMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    const char* report() const { return report_; }
    float x() const { return x_; }
    float speed() const { return v_; }
    float seconds() const { return race_; }
    float crewLeft() const { return std::max(0.f, limit_ - race_); }
    // 0 title, 1 the strip, 2 wheels on the paint, 3 setting down, 4 finished
    int marker() const;

private:
    enum class Mode { Title, Run, Fail, Win };

    void begin();
    void controls(bool& gas, bool& brake);
    void pilot(bool& gas, bool& brake);
    void physics(bool gas, bool brake);
    bool onPaint() const;
    bool pastMark() const;
    void settleWin();
    void fail(const char* why);
    void draw();
    void sky();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    float sx(float wx, float par = 1.f) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    float x_ = 0, v_ = 0;
    float spin_ = 0;
    float hold_ = 0;
    float race_ = 0;
    float t_ = 0;
    float limit_ = 24.f;
    bool wasGas_ = false;
    uint64_t tick_ = 0;
    char why_[64] = {};
    char report_[160] = {};
};

}  // namespace kartmark
