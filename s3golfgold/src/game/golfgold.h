// S3 GOLF GOLD — the line is 6.
// A gold cup counts two. A cream cup counts one.
// A cream putt that would reach the line does not count.
// Only a gold cup can finish, and the undoubled holes must stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golfgold {

constexpr int kLine = 6;
constexpr int kHoles = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLF GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int shots() const { return shot_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* phase() const;
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Aim, Roll, Call, Win, Lose, Pause };

    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
    };

    void beginRound();
    void beginHole();
    void toTitle();
    void launch(float meter);
    void stepBall(float dt);
    void finishPutt();
    void afterCall();
    void win();
    void lose();
    bool paid() const;
    bool cupGold() const;
    void tickMeter(const Input& in, float dt);
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;

    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void hush();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool willHole_ = false;
    bool sunk_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int shot_ = 0;
    int result_[kHoles] = {};
    float ballX_ = 0;
    float ballY_ = 0;
    float ballV_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float rollT_ = 0;
    float callT_ = 0;
    float beep_ = 0;
    float swingT_ = 0;
    const char* say_ = "";
};

}  // namespace golfgold
