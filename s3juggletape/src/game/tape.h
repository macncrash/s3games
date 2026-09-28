// S3 JUGGLE TAPE — file the cascade until the drawer matches the tape, then leave.
// The tape wants RED, GOLD, BLUE in that order.
// A catch that is not the next slip is a trap and stays out of the drawer.
// A drop opens the drawer again. Leave only when the three slips match.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace juggletape {

constexpr int kTapeN = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLE TAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool tossing() const { return mode_ == Mode::Play && !ready_; }
    int catches() const { return catches_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    int phase() const;

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
    void noteCatch(int ball, bool file);
    void leaveStage();
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
    bool ready_ = false;
    bool held_[kTapeN] = {};
    int catches_ = 0;
    int traps_ = 0;
    int tick_ = 0;
    int wait_ = 0;
    int titleWait_ = 0;
    int fileLatch_ = 0;
    int latch_[2] = {};
    int hold_[2][3] = {};
    int hn_[2] = {};
    Air air_[3]{};
    float beep_ = 0;
    const char* reason_ = "OPEN";
};

}  // namespace juggletape
