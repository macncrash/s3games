// S3 LANTERN — light the lamps in order. A miss hands one back to the dark.
#pragma once
#include <vector>

#include "console/system.h"
#include "game/art.h"

namespace lantern {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int chain() const { return (int)seq_.size(); }
    int handed() const { return handed_; }
    // 0 title, 1 the order is showing, 2 the player's turn, 3 between, 4 the night is lit
    int marker() const;

private:
    enum class Mode { Title, Watch, Play, Miss, Grow, Win };
    enum class Phase { Lead, Show, Gap };

    void begin();
    void enterWatch();
    void enterPlay();
    void enterGrow();
    void enterWin();
    void enterMiss();
    void updateTitle();
    void updateWatch();
    void updatePlay();
    void updateMiss();
    void updateGrow();
    void updateWin();
    void readHuman();
    void light(int lamp);
    void nudge(int dir);
    int roll();
    void chime(int lamp, float vol);
    void thud();
    void quiet();
    void draw();
    void sky();
    void hud(int col, int row, const std::string& s, int pal);
    void hudC(int row, const std::string& s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    bool lampOn(int i, int& pal) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Phase phase_ = Phase::Lead;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool hotBad_ = false;
    bool wave_[6] = {};
    bool faceLeft_ = false;
    int cursor_ = 0;
    int handed_ = 0;
    int run_ = 1;
    int step_ = 0;
    int show_ = 0;
    int timer_ = 0;
    int lock_ = 0;
    int hold_ = 0;
    int hot_ = -1;
    int hotT_ = 0;
    int pending_ = -1;
    int toneT_ = 0;
    int fanStep_ = 0;
    int fanT_ = 0;
    float mothX_ = 40;
    float mothY_ = 64;
    uint32_t rng_ = 1;
    std::vector<int> seq_;
};

}  // namespace lantern
