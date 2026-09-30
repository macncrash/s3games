// S3 INKWELLTAPE — a short inkwell. The drawer has to match the tape, then leave.
// GALL, SEPIA and LAMP file in that order. WASH pays like SEPIA and stays out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace inkwelltape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 INKWELLTAPE"; }
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
    // 0 title, 1 dipping, 2 drawer full, 3 left
    int phase() const;

private:
    enum class Mode { Title, Dip, Splash, Gap, Drawer, Lose, Over };

    bool prove();
    void begin();
    void arm();
    void dip(bool inWindow);
    void expire();
    void fileInk();
    void fail(const char* why);
    void depart();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;

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
    // WASH, GALL, SEPIA, WASH, LAMP. A blot dies if you do not dip it.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};
};

}  // namespace inkwelltape
