// S3 TILETAPE — set the short tiles until the drawer matches the tape, then leave.
// A count that is close is still open. The marks have to sit in the taped order.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tiletape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TILETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool rules() const { return rules_; }
    int moves() const { return moves_; }
    int holding() const { return hold_; }
    int slot(int i) const { return (i >= 0 && i < N) ? drawer_[i] : -1; }
    int tapeAt(int i) const { return (i >= 0 && i < N) ? art_.order[i] : -1; }
    const char* reason() const { return reason_ ? reason_ : ""; }
    // 0 title, 1 setting tiles, 2 the drawer just matched, 3 left
    int marker() const;

private:
    enum class Phase { Title, Set, Seal, Leave };

    bool prove();
    void update();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h, int pal);
    void blip(float freq);
    void pickOrDrop();
    int indexOf(int id) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int drawer_[N] = {};
    int cx_ = 0;
    int hold_ = -1;
    int moves_ = 0;
    int t_ = 0;
    int beep_ = 0;
    int botWait_ = 0;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool paused_ = false;
    const char* reason_ = "OPEN";
};

}  // namespace tiletape
