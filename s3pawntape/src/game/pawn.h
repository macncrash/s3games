// S3 PAWNTAPE — a pawn file. Pocket the tape into the drawer, then leave.
// FILE, RANK and PASS sit in that order. GHOST pays like RANK and stays out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pawntape {

constexpr int kTapeN = 3;
constexpr int kKinds = 4;

struct Kind {
    const char* name;
    int pay;
    bool decoy;
};

inline constexpr Kind kKind[kKinds] = {
    {"FILE", 5, false},
    {"RANK", 6, false},
    {"PASS", 4, false},
    {"GHOST", 6, true},
};

inline constexpr int kTape[kTapeN] = {0, 1, 2};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PAWNTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    int faults() const { return faults_; }
    int filled() const { return filled_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return why_ ? why_ : ""; }

private:
    enum class Mode { Title, March, Pocket, Leave, Lose, Over };

    void begin();
    void arm();
    void pocket();
    void passOff();
    void fail(const char* why);
    void depart();
    void botAct();
    void human();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;
    const gs::Mipped& pawnOf(int id) const;
    int palOf(int id) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool held_[kTapeN] = {};
    int drawer_[kTapeN] = {};
    int filled_ = 0;
    int faults_ = 0;
    int offer_ = 0;
    int x_ = 0;
    int anim_ = 0;
    int t_ = 0;
    const char* why_ = "OPEN";
    // GHOST, FILE, RANK, GHOST, PASS. A decoy walks off if you leave it.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};
};

}  // namespace pawntape
