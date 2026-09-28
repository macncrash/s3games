// S3 JUGGLE GOLD — keep the cascade, then leave.
// A gold catch counts two. A cream catch counts one.
// Cream that would meet the line, or that would fill the
// undoubled pile, does not count. Leave only once gold
// has doubled the line and the undoubled pile is still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace jugglegold {

constexpr int kLine = 8;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool rules() const { return rules_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return gold_ * 2 + cream_; }
    int bare() const { return gold_ + cream_; }
    int catches() const { return catches_; }
    int line() const { return kLine; }

private:
    enum class Mode { Title, Play, Drop, Leave };

    struct Air {
        bool on = false;
        int ball = 0;
        int to = 0;
        int age = 0;
    };

    void begin();
    void pattern();
    void miss();
    void noteCatch(int ball);
    void leaveStage();
    bool canLeave() const;
    void stepPlay();
    void blip(float freq);
    void chord(float a, float b, float c);
    void ageTone();
    void draw();
    void backdrop();
    void ballAt(int ball, float x, float y, float scale);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool finisherGold_ = false;
    bool ready_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int catches_ = 0;
    int tick_ = 0;
    int wait_ = 0;
    int titleWait_ = 0;
    int latch_[2] = {};
    int hold_[2][3] = {};
    int hn_[2] = {};
    Air air_[3]{};
    float beep_ = 0;
};

}  // namespace jugglegold
