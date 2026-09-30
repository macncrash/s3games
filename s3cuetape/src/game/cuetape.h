// S3 CUETAPE — play cue until the drawer matches the tape, then leave.
// BREAK, OBJECT and CUE drop those slips in the till.
// FOUL pays the same 6 and stays out. A short stroke is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace cuetape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CUETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool rolling() const { return mode_ == Mode::Roll && rollT_ > 0.05f && rollT_ < 0.62f; }
    int strokes() const { return strokes_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    int phase() const;

private:
    enum class Mode { Title, Aim, Roll, Pocket, Judge, Leave, Lose, Pause, Over };

    bool prove();
    void toTitle();
    void begin();
    void beginAim();
    void stroke();
    void finishRoll();
    void take(int line);
    void miss(const char* why);
    void beginLeave();
    void beginLose();
    int nextOpen() const;
    int pocketAt(float x, float y) const;
    void solve(int pocket, float& ang, float& pow) const;
    void blip(float freq, float vol, float hold);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void layTable();
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int strokes_ = 0;
    int throws_ = 0;
    int traps_ = 0;
    int fly_ = -1;
    int shake_ = 0;
    float aim_ = 0;
    float power_ = 0.55f;
    float landX_ = kHandX;
    float landY_ = kHandY;
    float ballX_ = kHandX;
    float ballY_ = kHandY;
    float clock_ = 0;
    float aimT_ = 0;
    float rollT_ = 0;
    float flyT_ = 0;
    float judgeT_ = 0;
    float leaveT_ = 0;
    float toneT_ = 0;
    float walk_ = 0;
    const char* reason_ = "OPEN";
};

}  // namespace cuetape
