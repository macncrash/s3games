// S3 LOOMTAPE — the drawer has to match the tape.
// Throw the shuttle on WARP, then WEFT, then REED.
// YARN, FILL and DENT pay the same and stay out of the drawer.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace loomtape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOMTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return next_ >= kTapeN; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    int throws() const { return throws_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_; }
    const char* modeName() const;

private:
    enum class Mode { Title, Offer, Call, Leave, Win, Lose };

    bool audit();
    void begin();
    void cast();
    void afterCall();
    void blip(int ch, float hz, float vol);
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int next_ = 0;
    int throws_ = 0;
    int traps_ = 0;
    int slot_ = 0;
    int slotAge_ = 0;
    int anim_ = 0;
    int dir_ = 1;
    char reason_[48] = {};
    char call_[28] = {};
};

}  // namespace loomtape
