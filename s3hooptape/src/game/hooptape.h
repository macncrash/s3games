// S3 HOOPTAPE — a short hoop. The drawer has to match the tape.
// A firm shot on SWISH, BANK or FREE drops that slip in the drawer.
// IRON, ARC and RIM pay the same and stay out. A close total is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hooptape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOPTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 6 && flightT_ <= 24; }
    int shots() const { return shots_; }
    int traps() const { return traps_; }
    int board() const { return board_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;

    struct Pt3 {
        float x = 0, y = 0, z = 0;
    };

private:
    enum class Mode { Title, Aim, Flight, Pocket, Judge, Leave, Lose, Pause, Over };
    enum class Arc { Clean, Bank, Rattle, Short, Hot, Brick };

    bool audit() const;
    void toTitle();
    void newGame();
    void beginAim();
    void launch();
    void stick();
    void beginLeave();
    void beginLose(const char* why);
    void finishLeave();
    void continuePlay();
    int openLine() const;
    bool sweet() const { return firmMeter(meter_); }
    Arc plan(float x, float z, float meter, Make& make) const;
    Pt3 handAt(float x, float z) const;
    Pt3 flightAt(float u) const;
    void botAim();
    void humanAim(const gs::Pad& pad, bool fire);
    void dribble();
    void holdBall();
    void blip(int ch, float freq, float vol, float hold);
    void chord(float a, float b, float c, float hold);
    void tickAudio(float dt);

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false, bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    Mode view() const { return mode_ == Mode::Pause ? heldMode_ : mode_; }
    float sx(float x) const;
    float sy(float y, float z) const;
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Arc arc_ = Arc::Clean;
    Make made_ = Make::Swish;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool wasSweet_ = false;
    bool hitSnd_ = false;
    int shots_ = 0;
    int traps_ = 0;
    int board_ = 0;
    int shake_ = 0;
    int flightT_ = 0;
    int fly_ = -1;
    int mark_ = -1;
    char reason_[40] = {};
    float feetX_ = kHomeX;
    float feetZ_ = kHomeZ;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float clock_ = 0.f;
    float pocketT_ = 0.f;
    float judgeT_ = 0.f;
    float leaveT_ = 0.f;
    float toneT_ = 0.f;
    float apex_ = 1.6f;
    float dribS_ = 0.f;
    float ballX_ = 0.f;
    float ballY_ = 0.f;
    float ballZ_ = 0.f;
    float slipFromX_ = 0.f;
    float slipFromY_ = 0.f;
    Pt3 from_{};
};

}  // namespace hooptape
