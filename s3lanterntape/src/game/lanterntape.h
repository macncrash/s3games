// S3 LANTERNTAPE — a short lantern. The drawer matches the tape, then it shuts.
// WICK, GLASS and HOOK drop those slips in the till.
// SNUFF pays the same 5 and stays out. A till that is close is still open.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lanterntape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERNTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool shut() const { return shut_; }
    bool matched() const { return held_[0] && held_[1] && held_[2] && !junk_; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    bool walking() const { return mode_ == Mode::Walk && px_ > 28.f; }
    int sets() const { return sets_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return reason_ ? reason_ : ""; }
    // 0 title, 1 the lane, 2 a slip lands, 3 the drawer, 4 the lantern is shut
    int phase() const;

private:
    enum class Mode { Title, Walk, Pocket, Drawer, Shut, Lose, Pause };

    bool prove();
    void toTitle();
    void begin();
    void setHere();
    void take(int line);
    void miss(const char* why);
    void beginDrawer();
    void beginShut();
    void beginLose();
    int nearest() const;
    int nextOpen() const;
    void blip(float freq, float vol);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Walk;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shut_ = false;
    bool rules_ = false;
    bool junk_ = false;
    bool faceL_ = false;
    bool held_[kTapeN] = {};
    int sets_ = 0;
    int traps_ = 0;
    int timer_ = 0;
    int pocket_ = -1;
    float px_ = 20.f;
    const char* reason_ = "OPEN";
};

}  // namespace lanterntape
