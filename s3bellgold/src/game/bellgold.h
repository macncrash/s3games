// S3 BELL GOLD — a short bell. Only the gold counts double.
// Cream rings keep their face. The bare sum stays under the line.
// Leave on the last gold, when the double is the only thing that clears it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace bellgold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BELL GOLD"; }
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
    int rings() const { return rings_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Swing, Toll, Gap, Pause, Over };

    bool open() const;
    void begin();
    void pull(bool hit);
    void leave();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, bool hflip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Swing;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool spoiled_ = false;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int rings_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    const char* why_ = "";
};

}  // namespace bellgold
