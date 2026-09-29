// S3 SOLITAIRETAPE — play solitaire until the drawer has to match the tape.
// File ACE, then FIVE, then NINE. JACK pays the same 5 as FIVE and stays out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace solitairetape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SOLITAIRETAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const;
    bool held(int i) const { return i >= 0 && i < kTapeN && held_[i]; }
    bool rules() const { return rules_; }
    int faults() const { return faults_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const;
    int tapeScore(int i) const;
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Play, Leave, Lose };

    void begin();
    void fileCard();
    void passCard();
    void fail(const char* why);
    void depart();
    int nextWant() const;
    void paint();
    void blit(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void lineAt(int col, int row, const char* s, int pal);
    void lineC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool live_[kCards] = {};
    bool held_[kTapeN] = {};
    int drawer_[kTapeN] = {};
    int filled_ = 0;
    int cursor_ = 0;
    int faults_ = 0;
    int act_ = 0;
    int t_ = 0;
    const char* why_ = "";
};

}  // namespace solitairetape
