// S3 WICKETTAPE — play wicket until the drawer matches the tape, then leave.
// A firm GLANCE, DRIVE or SIX drops that slip in the till.
// NUDGE, PUSH and LOFT pay the same and stay out.
// A till that totals the same and is not the tape is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wickettape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKETTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 12 && flightT_ <= 26; }
    int balls() const { return balls_; }
    int traps() const { return traps_; }
    int board() const { return board_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;

private:
    enum class Mode { Title, Aim, Flight, Pocket, Judge, Leave, Lose, Pause, Over };
    enum class Call { Play, Soon, Late };

    bool audit();
    void toTitle();
    void newGame();
    void beginAim();
    void play();
    void settle();
    void beginLeave();
    void beginLose(const char* why);
    void finishLeave();
    void continuePlay();
    int openLine() const;
    void nudge(int dir);
    void snapTape();
    void snapTwin();
    void botAim();
    void humanAim(const gs::Pad& pad, bool fire);
    float bowlerX() const;
    int bowlPose() const;
    int batPose() const;
    void flightPoint(float u, float& x, float& y, float& gx, float& gy) const;

    void blip(int ch, float freq, float vol, float hold);
    void chord(float a, float b, float c, float hold);
    void tickAudio(float dt);

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Call call_ = Call::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool sweetWas_ = false;
    int balls_ = 0;
    int traps_ = 0;
    int board_ = 0;
    int pick_ = 1;
    int shot_ = 1;
    int shake_ = 0;
    int flightT_ = 0;
    int fly_ = -1;
    char reason_[48] = {};
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float clock_ = 0.f;
    float pocketT_ = 0.f;
    float judgeT_ = 0.f;
    float leaveT_ = 0.f;
    float toneT_ = 0.f;
    float feetX_ = 74.f;
    float releaseX_ = 208.f;
    float handX_ = 0.f, handY_ = 0.f, batX_ = 0.f, batY_ = 0.f;
    float slipFromX_ = 0.f, slipFromY_ = 0.f;
    float axLatch_ = 0.f, ayLatch_ = 0.f;
};

}  // namespace wickettape
