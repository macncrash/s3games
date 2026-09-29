// S3 LOOMMARK — a short loom. One house mark, nine picks.
// Catch the crimson inlay as the shuttle passes a marked end.
// The finished mark ends the cartridge.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace loommark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOOMMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    bool markWoven() const { return markWoven_; }
    int picks() const { return picks_; }
    int misses() const { return misses_; }

private:
    enum class Mode { Title, Weave, Beat, Pause, Win, Lose };

    void toTitle();
    void begin();
    void commitEnd();
    bool rowMatches() const;
    void lockRow();
    void snarl();
    void win();
    void lose();
    int dwellFor() const { return bot_ ? 3 : 16; }
    void blip(float freq, float vol);
    void chord();
    void decayAudio();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Weave;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool markWoven_ = false;
    bool caught_ = false;
    bool pendingWin_ = false;
    int picks_ = 0;
    int misses_ = 0;
    int row_ = 0;
    int col_ = 0;
    int dir_ = 1;
    int dwell_ = 0;
    int beat_ = 0;
    int shake_ = 0;
    int attempt_[kCols] = {};
    int cloth_[kRows][kCols] = {};
    float clock_ = 0;
    float toneT_ = 0;
};

}  // namespace loommark
