// S3 MARKETCHIME — play market until the hour has to chime. Leave when that is true.
// Exact change moves the line. The stall clock only chimes on a correct hand
// while it is on twelve. Two bad hands, or a silent grace, and the hour is gone.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace marketchime {

constexpr int kFpc = 3;
constexpr int kGraceSec = 12;
constexpr int kLeadSec = 30;
constexpr int kHourSec = 12 * 3600;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MARKETCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const;
    int served() const { return served_; }
    int faults() const { return faults_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Chime, Leave, Fail };

    void toTitle();
    void openStall();
    void nextSale();
    void readInput();
    void driveBot();
    void logic();
    void audio();
    void draw();
    void dropCoin();
    void undoCoin();
    void hand();
    void beginChime();
    void beginFail(const char* why);
    bool audit();
    int dueOf() const;
    int clockOf(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void face(int sec, int& h, int& m, int& s) const;
    void blip(float freq, int frames);
    void nudge(int dir);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprI(const gs::Image& img, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);
    void backdrop();
    void clockAt(float cx, float cy);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool locked_ = false;
    int age_ = 0;
    int playFrames_ = 0;
    int lockedSec_ = 0;
    int served_ = 0;
    int faults_ = 0;
    int sale_ = 0;
    int dish_ = 0;
    int stack_[8] = {};
    int stackN_ = 0;
    int cursor_ = 0;
    int wait_ = 0;
    int repL_ = 0;
    int repR_ = 0;
    int chimeT_ = 0;
    int strikes_ = 0;
    int flashT_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
};

}  // namespace marketchime
