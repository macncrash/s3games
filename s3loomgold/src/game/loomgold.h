// S3 LOOM GOLD — a short loom. Only the gold counts double.
// Cream weft keeps its face. The bare sum stays under the order.
// Leave on the last gold, when the double is the only thing that clears it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace loomgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOM GOLD"; }
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
    int picks() const { return picks_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Shed, Beat, Gap, Pause, Over };

    bool open() const;
    void begin();
    void catchPick(bool hit);
    void leave();
    void blip(float freq);
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Shed;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool spoiled_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int picks_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    const char* why_ = "";
};

}  // namespace loomgold
