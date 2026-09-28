// S3 MARKETBELL — one short stall. Three tries at the change.
// The brass bell rings only on an exact hand. A short or heavy dish kills
// the try. The third dead try ends the market with the bell still.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace marketbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MARKETBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const;
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    int score() const { return score_; }

private:
    enum class Mode { Title, Play, Ring, Pause, Win, Lose };

    void clearRound();
    void toTitle();
    void openStall();
    void nextTry();
    void readInput();
    void driveBot();
    void logic();
    void audio();
    void lights();
    void draw();
    void dropCoin();
    void undoCoin();
    void hand();
    void ringBell();
    void killTry(const char* why);
    void blip(float freq, int frames);
    void nudge(int dir);
    int dueOf() const;
    void hud(int col, int row, const char* s);
    void hudAt(float cx, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprI(const gs::Image& img, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    void backdrop();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool ready_ = false;
    const char* why_ = "";
    int age_ = 0;
    int sale_ = 0;
    int dead_ = 0;
    int tryNo_ = 0;
    int score_ = 0;
    int dish_ = 0;
    int stack_[8] = {};
    int stackN_ = 0;
    int cursor_ = 0;
    int pat_ = 0;
    int wait_ = 0;
    int repL_ = 0;
    int repR_ = 0;
    int axisN_ = 0;
    int flashT_ = 0;
    int flashK_ = 0;
    int bellT_ = 0;
    int ringN_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
    float beepV_ = 0;
    int melStep_ = -1;
    int melWait_ = 0;
};

}  // namespace marketbell
