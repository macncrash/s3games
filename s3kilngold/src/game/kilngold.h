// S3 KILN GOLD — a short kiln. Only the gold counts double.
// Cream glaze keeps its face. The bare sum stays under the order.
// Leave on the last gold, when the double is the only thing that clears it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace kilngold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KILN GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int fires() const { return fires_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Slide, Glow, Gap, Pause, Over };

    bool open() const;
    void begin();
    void fire(bool hit);
    void leave();
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Slide;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool spoiled_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int fires_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int sparkN_ = 0;
    const char* why_ = "";
};

}  // namespace kilngold
