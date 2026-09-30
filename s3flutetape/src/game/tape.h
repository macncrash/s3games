// S3 FLUTETAPE — blow the short flute until the drawer matches the tape, then leave.
// LIP, BODY and FOOT file in that order. WHISTLE pays like LIP and stays out.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace flutetape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FLUTETAPE"; }
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
    int blows() const { return blows_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return why_ ? why_ : ""; }
    int phase() const;

private:
    enum class Mode { Title, Rise, Hit, Rest, Full, Fail, Done };

    bool prove();
    void begin();
    void arm();
    void blow(bool inWindow);
    void expire();
    void fileNote();
    void fail(const char* why);
    void depart();
    void draw();
    void hall();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;
    const gs::Image& noteImg(int id) const;

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
    int blows_ = 0;
    int offer_ = 0;
    int age_ = 0;
    int anim_ = 0;
    const char* why_ = "OPEN";
    // WHISTLE, LIP, WHISTLE, BODY, FOOT. A whistle dies if you do not blow it.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 3, 1, 2};
};

}  // namespace flutetape
