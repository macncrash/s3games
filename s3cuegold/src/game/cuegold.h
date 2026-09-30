// S3 CUE GOLD — a short cue. The line is 4.
// A gold pot counts two. A cream pot counts one.
// A cream pot that would reach the line does not count.
// Only a gold pot can finish, and the undoubled pots must stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace cuegold {

constexpr int kLine = 4;
constexpr int kShots = 5;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CUE GOLD"; }
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
    void beginShot();
    void toTitle();
    void stroke(float meter);
    void stepRoll(float dt);
    void finishShot();
    void afterCall();
    void win();
    void lose();
    bool paid() const;
    bool objectGold() const;
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
    bool willPot_ = false;
    bool potted_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int shot_ = 0;
    int result_[kShots] = {};
    float cueX_ = 0;
    float objX_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float rollT_ = 0;
    float callT_ = 0;
    float beep_ = 0;
    float swingT_ = 0;
    const char* say_ = "";
};

}  // namespace cuegold
