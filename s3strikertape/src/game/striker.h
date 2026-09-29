// S3 STRIKERTAPE — play the striker until the drawer matches the tape, then leave.
// A strike in BELL, GOLD or DING, in that order, drops that slip in the drawer.
// TIN pays the same 10 as BELL and ends the night.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace strikertape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 STRIKERTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool rising() const { return mode_ == Mode::Rise && riseT_ > 0.08f && riseT_ < 0.42f; }
    int swings() const { return swings_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    // 0 title, 1 ready, 2 rising, 3 drawer full, 4 left
    int phase() const;

private:
    enum class Mode { Title, Ready, Rise, Call, Leave, Lose, Pause, Over };

    bool prove();
    void toTitle();
    void begin();
    void ready();
    void strike();
    void settle();
    void take(int line);
    void miss(const char* why);
    void trap();
    void beginLeave();
    void beginLose(const char* why);
    int nextOpen() const;
    int bandAt(float p) const;
    float meter() const;
    void blip(float freq, float vol, float hold);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void layYard();
    void backdrop();
    void draw();
    float puckY(float p) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Ready;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int swings_ = 0;
    int traps_ = 0;
    int hit_ = -1;
    float power_ = 0;
    float shown_ = 0;
    float meterT_ = 0;
    float clock_ = 0;
    float riseT_ = 0;
    float callT_ = 0;
    float leaveT_ = 0;
    float walk_ = 0;
    float toneT_ = 0;
    int shake_ = 0;
    const char* reason_ = "OPEN";
};

}  // namespace strikertape
