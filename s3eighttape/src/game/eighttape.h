// S3 EIGHTTAPE — play eight until the drawer matches the tape, then leave.
// EIGHT, SOLID and STRIPE drop those slips in the till.
// The spot pays the same 8 and stays out. A close eight is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace eighttape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHTTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool rolling() const { return mode_ == Mode::Roll && rollT_ > 0.08f && rollT_ < 1.6f; }
    int strokes() const { return strokes_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    int phase() const;

    enum class Mode { Title, Aim, Roll, Pocket, Judge, Leave, Lose, Pause, Over };

    struct Body {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };
    struct Sim {
        Body b[kBalls]{};
        bool guide = false;
        bool struck = false;
        int gball = -1;
        int pocket = -1;
        float ux = 1, uy = 0;
    };
    struct Lock {
        int ball = -1;
        int pocket = -1;
    };
    struct Verdict {
        bool made = false;
        bool spot = false;
        bool scratch = false;
        bool off = false;
        bool other = false;
    };

private:
    bool prove();
    void toTitle();
    void begin();
    void beginAim();
    void shoot();
    void finishRoll();
    void take(int line);
    void miss(const char* why);
    void beginLeave();
    void beginLose();
    int nextOpen() const;
    void blip(float freq, float vol, float hold);
    void fanfare();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Sim sim_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool solved_ = false;
    bool held_[kTapeN] = {};
    int strokes_ = 0;
    int traps_ = 0;
    int call_ = 0;
    int fly_ = -1;
    int shake_ = 0;
    int fanStep_ = -1;
    float aim_ = 0;
    float power_ = 0.66f;
    float solvedAim_[kTapeN] = {};
    float clock_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float flyT_ = 0;
    float judgeT_ = 0;
    float leaveT_ = 0;
    float toneT_ = 0;
    float fanT_ = 0;
    float walk_ = 0;
    const char* reason_ = "OPEN";
};

}  // namespace eighttape
