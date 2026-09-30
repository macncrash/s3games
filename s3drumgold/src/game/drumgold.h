// S3 DRUM GOLD — play the kit until only the gold counts double.
// A gold head is ten, counted twice. A cream head is nine and does not double.
// Leave when the doubled gold is what clears the line and the bare faces are still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drumgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRUM GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool goldOut() const { return finisher_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int hits() const { return hits_; }
    int line() const { return kLine; }

private:
    enum class Mode { Title, Play, Hit, Pause, Over };

    void begin();
    void turn(int dir);
    void strike();
    void leave();
    void blip(float freq);
    void drum(bool gold);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool open() const;
    int nextGold() const;
    void place(int i, float& x, float& y, bool& cym) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisher_ = false;
    bool hit_[kPads] = {};
    int pad_ = 1;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int hits_ = 0;
    int rep_ = 0;
    int anim_ = 0;
};

}  // namespace drumgold
