// S3 HOOP GOLD — the line is 6.
// A gold rim counts two. A cream rim counts one.
// A cream basket that would reach the line does not count.
// Only a gold basket can finish, and the undoubled makes must stay under the line.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hoopgold {

constexpr int kLine = 6;
constexpr int kRack = 6;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOP GOLD"; }
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
    enum class Mode { Title, Aim, Flight, Call, Win, Lose, Pause };
    enum class Call { None, Swish, Count, Bank, Cream, Refuse, Rim, Short, Long, Air };

    struct Input {
        bool action = false;
        bool start = false;
        bool back = false;
        float x = 0;
    };
    struct Ball {
        float x = 0, y = 0, vx = 0, vy = 0;
    };

    void beginRound();
    void beginAim();
    void toTitle();
    void launch(float meter);
    void stepBall(float dt);
    void fly(float dt);
    void finishShot();
    void afterCall();
    void win();
    void lose();
    bool paid() const;
    void aimControl(const Input& in, float dt);
    void tickMeter(const Input& in, float dt);
    void dribble();
    Mode view() const;
    bool pocketNow() const;
    Input readPad(const gs::Pad& pad) const;
    Input botInput() const;

    void blip(float freq);
    void chord(float a, float b, float c, float hold);
    void hush();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void blob(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Call call_ = Call::None;
    Ball ball_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool callGold_ = true;
    bool scored_ = false;
    bool rimHit_ = false;
    bool bank_ = false;
    bool crossed_ = false;
    bool rimSnd_ = false;
    bool bankSnd_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int shot_ = 0;
    int refused_ = 0;
    int result_[kRack] = {};
    float aim_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float flight_ = 0;
    float scoreAt_ = -1;
    float callT_ = 0;
    float beep_ = 0;
    float flash_ = 0;
    float crossX_ = 0;
    const char* say_ = "";
};

}  // namespace hoopgold
