// S3 DRAWER GOLD — file the drawer until only the gold counts double.
// A gold slip is ten, counted twice. A cream slip is eight and does not double.
// Leave when the doubled gold clears the line and the bare faces are still short.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace drawergold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DRAWER GOLD"; }
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
    int filed() const { return filed_; }
    int line() const { return kLine; }

private:
    enum class Mode { Title, Play, Slide, Over };

    void begin();
    void file();
    void skip();
    void leave();
    bool open() const;
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisher_ = false;
    bool held_[kSlips] = {};
    int index_ = 0;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int filed_ = 0;
    int anim_ = 0;
};

}  // namespace drawergold
