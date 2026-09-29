// S3 ANVILTAPE — play the anvil until the drawer matches the tape, then leave.
// BILLET, BLOOM and TONGS file in that order. SLAG pays like BLOOM and stays out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace anviltape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ANVILTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool rules() const { return rules_; }
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    int faults() const { return faults_; }
    int filled() const { return filled_; }
    int strikes() const { return strikes_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return why_ ? why_ : ""; }
    // 0 title, 1 heat, 2 drawer full, 3 left
    int phase() const;

private:
    enum class Mode { Title, Heat, Spark, Gap, Pocket, Leave, Lose, Over };

    bool prove();
    void begin();
    void arm();
    void strike(bool inWindow);
    void expire();
    void fileBar();
    void fail(const char* why);
    void depart();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;
    const gs::Image& barImg(int id) const;

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
    int strikes_ = 0;
    int offer_ = 0;
    int age_ = 0;
    int anim_ = 0;
    const char* why_ = "OPEN";
    // SLAG, BILLET, BLOOM, SLAG, TONGS. A decoy dies if you do not strike it.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};
};

}  // namespace anviltape
