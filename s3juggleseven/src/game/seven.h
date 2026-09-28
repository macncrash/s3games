// S3 JUGGLE SEVEN — catch the cascade until you are first to seven.
// Them juggles the other side of the booth. Six is still short.
// A drop does not wipe the tally. Whoever touches seven first takes it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace juggleseven {

constexpr int kSeven = 7;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLE SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int catches() const { return catches_; }

private:
    enum class Mode { Title, Play, Drop, End };

    struct Air {
        bool on = false;
        int ball = 0;
        int to = 0;
        int age = 0;
    };

    void begin();
    void pattern();
    void miss();
    void noteCatch();
    void noteThem();
    void finish(bool youWon);
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
    bool rules_ = false;
    int you_ = 0;
    int them_ = 0;
    int catches_ = 0;
    int tick_ = 0;
    int clock_ = 0;
    int wait_ = 0;
    int titleWait_ = 0;
    int latch_[2] = {};
    int hold_[2][3] = {};
    int hn_[2] = {};
    Air air_[3]{};
    float beep_ = 0;
    float rivalPhase_ = 0;
};

}  // namespace juggleseven
