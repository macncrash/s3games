// S3 MEMORYBELL — a short order of four bells, three tries.
// The bell rings only when the order comes back whole.
// A wrong strike dies the try. The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace memorybell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORYBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    int got() const { return got_; }

private:
    enum class Mode { Title, Show, Recall, Dead, Ring, Leave, Over };

    void toTitle();
    void newGame();
    void beginShow();
    void showTick();
    void strike(int which);
    void ring();
    void dieTry();
    bool audit() const;
    void botAct();
    void readPad();
    void pumpAudio();
    void draw();
    void backdrop();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void tone(int ch, float freq, float vol, int frames);
    float bellX(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    int seq_[SEQ] = {0, 2, 1, 3};
    int dead_ = 0;
    int tryNo_ = 0;
    int got_ = 0;
    int cursor_ = 0;
    int lit_ = -1;
    int t_ = 0;
    int tone0_ = 0;
    int tone1_ = 0;
    float swing_ = 0;
    int lock_ = 0;
};

}  // namespace memorybell
