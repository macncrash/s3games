// S3 PRESSTAPE — play the press until the drawer has to match the tape, then leave.
// RIB, WEB and CAP go in that order. FLASH pays like WEB and stays out. SHIM is scrap.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace presstape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PRESSTAPE"; }
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
    int strokes() const { return strokes_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* reason() const { return why_ ? why_ : ""; }
    // 0 title, 1 on the bed, 2 drawer full, 3 left
    int phase() const;

private:
    enum class Mode { Title, Bed, Stroke, Clear, Pocket, Lose, Over };

    bool prove();
    void begin();
    void feed();
    void stamp();
    void refuse(const char* why);
    void passBlank();
    void depart();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int nextWant() const;
    const gs::Image& blankImg(int id) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool holding_ = false;
    bool held_[kTapeN] = {};
    int drawer_[kTapeN] = {};
    int filled_ = 0;
    int faults_ = 0;
    int strokes_ = 0;
    int offer_ = 0;
    int age_ = 0;
    int anim_ = 0;
    const char* why_ = "OPEN";
    // SHIM, RIB, FLASH, WEB, SHIM, CAP.
    static constexpr int kDeckN = 6;
    static constexpr int kDeck[kDeckN] = {4, 0, 3, 1, 4, 2};
};

}  // namespace presstape
