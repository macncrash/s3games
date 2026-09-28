// S3 SAFEMARK — three dials, and the room has the numbers.
// Matching them is not the end. The gold clue is the mark.
// Seat every dial, then pull with the gold dial chosen.
// That finished mark ends the cartridge. Leave when it is true.
#pragma once

#include <random>

#include "console/system.h"
#include "game/art.h"

namespace safemark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SAFEMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool marked() const { return marked_; }
    bool solved() const;
    int dial(int i) const { return dial_[i]; }
    int combo(int i) const { return code_[i]; }
    int gold() const { return gold_; }

private:
    enum class Mode { Title, Play, Stamp, Over };

    void deal();
    void botAct();
    void human();
    void nudgeSel(int d);
    void nudgeDial(int d);
    void pull();
    void blip(float freq);
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool marked_ = false;
    int code_[3] = {};
    int dial_[3] = {};
    int gold_ = 0;
    int sel_ = 0;
    int t_ = 0;
    int stampT_ = 0;
    int shake_ = 0;
    int wrong_ = 0;
    int beep_ = 0;
    int hold_[4] = {};
    float slide_ = 0;
    std::mt19937 rng_{1};
};

}  // namespace safemark
