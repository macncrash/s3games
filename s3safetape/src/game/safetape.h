// S3 SAFETAPE — a short safe. The drawer has to match the tape, then you leave.
// LEFT, SPINE and RIGHT are the only stops that file. A sum that matches is still shut.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace safetape {

constexpr int kTapeN = 3;

struct Stop {
    const char* name;
    int digit;
};

// Order is the tape. kNear adds to the same total and does not open the drawer.
constexpr Stop kTape[kTapeN] = {{"LEFT", 4}, {"SPINE", 1}, {"RIGHT", 8}};
constexpr int kNear[kTapeN] = {5, 1, 7};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool rules() const { return rules_; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool nearRejected() const { return nearRejected_; }
    int drawerDigit(int i) const { return (i >= 0 && i < kTapeN) ? drawer_[i] : -1; }
    const char* tapeLabel(int i) const;
    int tapeDigit(int i) const;
    int nearDigit(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    int phase() const;

private:
    enum class Mode { Title, Dial, Leave, Lose, Over };

    bool prove();
    void begin();
    void file();
    void clearSel();
    void beginLeave();
    void beginLose(const char* why);
    void botAct();
    void human();
    void nudgeSel(int d);
    void nudgeDial(int d);
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(float x, float y, float w, float h, int pal);
    int drawerSum() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool botNear_ = true;
    bool nearRejected_ = false;
    bool filed_[kTapeN] = {};
    bool held_[kTapeN] = {};
    int drawer_[kTapeN] = {-1, -1, -1};
    int dial_[kTapeN] = {};
    int sel_ = 0;
    int t_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int hold_[4] = {};
    float walk_ = 0;
    const char* reason_ = "SHUT";
};

}  // namespace safetape
