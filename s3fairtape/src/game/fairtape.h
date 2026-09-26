// S3 FAIRTAPE — the drawer has to match the tape.
// A firm ring on ROSE, CANE or BEAR drops that slip in the till.
// BUD, STICK and CUB pay the same and stay out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairtape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIRTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 4 && flightT_ <= 12; }
    int rings() const { return rings_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;

private:
    enum class Mode { Title, Aim, Flight, Pocket, Judge, Leave, Lose, Pause, Over };

    bool audit() const;
    void toTitle();
    void newGame();
    void beginAim();
    void launch();
    void stick();
    void beginLeave();
    void beginLose(const char* why);
    void continuePlay();
    int openLine() const;
    bool sweet() const;
    void botAim();
    void humanAim(const gs::Pad& pad, bool fire);
    void blip(int ch, float freq, float vol, float hold);
    void chord(float a, float b, float c, float hold);
    void tickAudio(float dt);

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, int fog = 0);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog = 0);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    Mode view() const { return mode_ == Mode::Pause ? heldMode_ : mode_; }
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Fate fate_ = Fate::Firm;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool seated_[kShelfN] = {};
    bool wasSweet_ = false;
    int rings_ = 0;
    int traps_ = 0;
    int shake_ = 0;
    int flightT_ = 0;
    int fly_ = -1;
    char reason_[48] = {};
    float aimX_ = 160.f;
    float targetX_ = 160.f;
    float fromX_ = 160.f;
    float fromY_ = 68.f;
    float landX_ = 160.f;
    float landY_ = 108.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float clock_ = 0.f;
    float pocketT_ = 0.f;
    float judgeT_ = 0.f;
    float leaveT_ = 0.f;
    float toneT_ = 0.f;
    float slipFromX_ = 0.f;
    float slipFromY_ = 0.f;
};

}  // namespace fairtape
