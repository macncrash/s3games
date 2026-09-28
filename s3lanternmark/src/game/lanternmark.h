// S3 LANTERNMARK — light the lamps in order. The gold lamp is the mark.
// A miss hands the newest light back to the dark. The order stays, so the mark remains.
// Lighting the gold lamp in its place opens the mark. Lifting it finishes the mark.
// That finished mark ends the cartridge. The rest of the night is not the job.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lanternmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERNMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool lifted() const { return lifted_; }
    bool onMark() const { return onMark_; }
    int order() const { return kOrderN; }
    int handed() const { return handed_; }
    int marks() const { return finished_ ? 1 : 0; }
    int closer() const { return closer_; }

private:
    enum class Mode { Title, Watch, Play, Miss, Open, Win };
    enum class Phase { Lead, Show, Gap };

    static constexpr int kLamps = 6;
    static constexpr int kMark = 3;
    static constexpr int kOrderN = 4;

    void begin();
    void enterWatch();
    void enterPlay();
    void enterMiss();
    void enterOpen();
    void enterWin();
    void readHuman();
    void light(int lamp);
    void lift();
    void nudge(int dir);
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
    Phase phase_ = Phase::Lead;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool lifted_ = false;
    bool onMark_ = false;
    bool hotBad_ = false;
    bool faceLeft_ = false;
    int order_[kOrderN] = {0, 5, 2, kMark};
    int cursor_ = 0;
    int handed_ = 0;
    int step_ = 0;
    int show_ = 0;
    int timer_ = 0;
    int lock_ = 0;
    int hold_ = 0;
    int hot_ = -1;
    int hotT_ = 0;
    int pending_ = -1;
    int toneT_ = 0;
    int openT_ = 0;
    int closer_ = -1;
    float mothX_ = 40;
    float mothY_ = 60;
};

}  // namespace lanternmark
