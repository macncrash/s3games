// S3 DRAWERTAPE — a short drawer. File the tape, then leave.
// CLIP, STAMP and KEY drop into the drawer in that order.
// TAG pays the same 7 as STAMP and stays out of the drawer.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drawertape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWERTAPE"; }
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
    const char* reason() const { return reason_ ? reason_ : ""; }
    // 0 title, 1 filing, 2 drawer full, 3 left
    int phase() const;

private:
    enum class Mode { Title, File, Leave, Lose, Over };

    void begin();
    void arm();
    void file();
    void expire();
    void fail(const char* why);
    void depart();
    void botAct();
    void human();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void box(float x, float y, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    float binX(int i) const;
    int nextWant() const;

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
    int cursor_ = 0;
    int offer_ = 0;
    int life_ = 0;
    int t_ = 0;
    int act_ = 0;
    int beep_ = 0;
    int flash_ = 0;
    const char* reason_ = "OPEN";
    // TAG, CLIP, STAMP, TAG, KEY. A decoy dies on its own.
    static constexpr int kDeckN = 5;
    static constexpr int kDeck[kDeckN] = {3, 0, 1, 3, 2};
};

}  // namespace drawertape
