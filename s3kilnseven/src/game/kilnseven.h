// S3 KILN SEVEN — a short kiln. First shelf to seven sound pots wins.
// Each pot is one. A tally of six is still short. The other bench fires after you,
// unless you have already reached seven.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace kilnseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 KILN SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const;
    bool shortSix() const { return shortSix_; }
    int you() const { return you_; }
    int them() const { return them_; }
    const char* phase() const;

private:
    enum class Mode { Title, Heat, Glow, Rival, Win, Lose, Pause };

    struct Ware {
        bool yours = false;
        int pts = 0;
    };

    void toTitle();
    void resetMatch();
    void logic(bool fire, bool start);
    void botAct(bool& fire, bool& start);
    void scoreYou();
    void crack();
    void scoreThem();
    void winShelf();
    void loseShelf(const char* why);
    void blip(float freq);

    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Heat;
    Ware log_[16]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool shortSix_ = false;
    int you_ = 0;
    int them_ = 0;
    int faults_ = 0;
    int logN_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int wait_ = 0;
    const char* why_ = "";
};

}  // namespace kilnseven
