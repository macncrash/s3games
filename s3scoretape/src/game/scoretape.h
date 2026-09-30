// S3 SCORETAPE — a short score. File the tape into the drawer, then leave.
// QUAVER, MINIM and BREVE drop in that order.
// REST pays the same 7 as MINIM and stays out of the drawer.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace scoretape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCORETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    int filled() const { return filled_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return why_ ? why_ : ""; }
    // 0 title, 1 reading, 2 drawer full, 3 left
    int phase() const;

private:
    enum class Mode { Title, Read, Ink, Gap, Pocket, Lose, Over };

    bool prove();
    void begin();
    void arm();
    void stamp(bool inWindow);
    void expire();
    void fileMark();
    void fail(const char* why);
    void depart();
    void draw();
    void room();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;
    const gs::Image& markImg(int id) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    int drawer_[kTapeN] = {};
    int filled_ = 0;
    int faults_ = 0;
    int offer_ = 0;
    int age_ = 0;
    int anim_ = 0;
    const char* why_ = "OPEN";
    // REST, QUAVER, MINIM, REST, BREVE. A rest dies if you do not stamp it.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};
};

}  // namespace scoretape
