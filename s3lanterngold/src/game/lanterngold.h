// S3 LANTERN GOLD — light the lamps in order.
// A gold lamp counts two. A cream lamp counts one.
// A cream light that would reach the line does not count.
// Only a gold lamp can finish, and the undoubled lights stay under the line.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lanterngold {

constexpr int kLine = 6;
constexpr int kLamps = 6;
constexpr int kOrderN = 5;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERN GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int bare() const { return gold_ + cream_; }
    int line() const { return kLine; }
    int chain() const { return step_; }
    int handed() const { return handed_; }
    const char* phase() const;

private:
    enum class Mode { Title, Watch, Play, Miss, Refuse, Win };

    void begin();
    void enterWatch();
    void enterPlay();
    void enterMiss();
    void enterRefuse();
    void enterWin();
    void readHuman();
    void botAct();
    void nudge(int dir);
    void tryLight(int lamp);
    void undoLast();
    bool isGold(int lamp) const;
    void chime(int lamp, float vol);
    void thud();
    void quiet();
    void draw();
    void sky();
    bool lampOn(int i, int& pal) const;
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool faceLeft_ = false;
    int order_[kOrderN] = {0, 2, 1, 4, 3};
    int lit_[kOrderN] = {};
    int step_ = 0;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int handed_ = 0;
    int refused_ = 0;
    int cursor_ = 0;
    int show_ = 0;
    int timer_ = 0;
    int lock_ = 0;
    int hot_ = -1;
    int hotT_ = 0;
    int toneT_ = 0;
    bool hotBad_ = false;
    float mothX_ = 40;
    float mothY_ = 64;
};

}  // namespace lanterngold
