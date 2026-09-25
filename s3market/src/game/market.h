// S3 MARKET — one stall. Right change. The line has to move.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace market {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MARKET"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    int served() const { return served_; }
    int faults() const { return faults_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Shift, Pause, Win, Lose };

    void clearRound();
    void toTitle();
    void startDay();
    void readInput();
    void driveBot();
    void logic();
    void audio();
    void lights();
    void draw();
    void dropCoin();
    void undoCoin();
    void hand();
    void endShift();
    void winDay();
    void loseDay(const char* why);
    void blip(float freq, int frames);
    void nudge(int dir);
    int dueOf() const;
    void hud(int col, int row, const char* s);
    void hudAt(float cx, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprI(const gs::Image& img, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    void shadeAt(float cx, float cy, float w);
    void digits(int n, float cx, float cy, int pal);
    void backdrop();
    void drawPeople();
    void drawCoins();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ready_ = false;
    int age_ = 0;
    int idx_ = 0;
    int served_ = 0;
    int faults_ = 0;
    int score_ = 0;
    int dish_ = 0;
    int stack_[16] = {};
    int stackN_ = 0;
    int cursor_ = 0;
    int tries_ = 0;
    int pat_ = 0;
    int wait_ = 0;
    int repL_ = 0;
    int repR_ = 0;
    int axisN_ = 0;
    int flashT_ = 0;
    int flashK_ = 0;
    int shiftN_ = 0;
    int bellT_ = 0;
    int dingN_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
    float beepV_ = 0;
    int melStep_ = -1;
    int melWait_ = 0;
    const char* why_ = "";
};

}  // namespace market
