// S3 GOLFTAPE — a short golf. The drawer has to match the tape.
// A firm stroke on FADE, PITCH or DROP, in that order, drops that slip in.
// DRAW, FLIP and LIP pay the same and stay out. A close total is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace golftape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GOLFTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    int shots() const { return shots_; }
    int traps() const { return traps_; }
    int board() const { return board_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;

private:
    enum class Mode { Title, Aim, Flight, Pocket, Judge, Pause, Over };
    enum class Read { None, Fade, Draw, Pitch, Flip, Drop, Lip };

    bool audit() const;
    void toTitle();
    void newRound();
    void beginAim();
    void swing();
    void finish();
    Read readAim() const;
    int payOf(Read r) const;
    bool tapeRead(Read r) const;
    float landX(Read r) const;
    const char* readName(Read r) const;
    const char* clubName() const;
    void botAim();
    void humanAim(const gs::Pad& pad);
    void blip(int ch, float freq, float vol);
    void tickAudio();

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Read read_ = Read::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int shots_ = 0;
    int traps_ = 0;
    int board_ = 0;
    int stroke_ = 0;
    int flightT_ = 0;
    int pocketT_ = 0;
    int judgeT_ = 0;
    int titleT_ = 0;
    int toneKill_ = 0;
    char reason_[48] = {};
    float aim_ = 0.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float ballX_ = 48.f;
    float ballY_ = 0.f;
    float fromX_ = 48.f;
    float toX_ = 48.f;
    float apex_ = 40.f;
    float manX_ = 36.f;
    float slipX_ = 0.f;
    float slipY_ = 0.f;
    float slipToX_ = 0.f;
    float slipToY_ = 0.f;
};

}  // namespace golftape
