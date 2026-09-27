// S3 ARCHTAPE — the drawer has to match the tape.
// A firm arrow in PIN, GOLD or RED drops that slip in the till.
// DOT, PALE and RUST pay the same and stay out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace archtape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCHTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ > 2 && flightT_ < kFlight; }
    int arrows() const { return arrows_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;

private:
    enum class Mode { Title, Aim, Nock, Flight, Call, Leave, Win, Lose };

    bool audit();
    void begin();
    void steer();
    bool drawHeld() const;
    void loose();
    void arrive();
    void afterCall();
    void blip(int ch, float hz, float vol, int frames);
    void pump();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Hit last_{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool skipHold_ = false;
    int arrows_ = 0;
    int traps_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flightT_ = 0;
    int callT_ = 0;
    int leaveT_ = 0;
    int t_ = 0;
    int blipCh_ = 0;
    int blipLeft_ = 0;
    int marks_ = 0;
    float markX_[12] = {};
    float markY_[12] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    char reason_[40] = {};
    char call_[24] = {};
};

}  // namespace archtape
