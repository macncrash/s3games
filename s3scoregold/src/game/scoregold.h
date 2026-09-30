// S3 SCORE GOLD — one sheet. Only the gold counts double.
// Cream marks keep their face and cannot be the stroke that meets the line.
// File once every line was exact only because gold doubled, and the
// undoubled faces are still short of the day's total.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace scoregold {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCORE GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool filed() const { return filed_; }
    bool finisherGold() const { return finisherGold_; }
    int score() const { return score_; }
    int bare() const { return bare_; }
    int golds() const { return golds_; }
    int cream() const { return cream_; }
    int lines() const { return lines_; }
    int faults() const { return faults_; }
    int dueSum() const { return dueSum_; }
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Play, Hold, Pause, File, Win, Lose };
    enum Kind { Bare = 0, Cream = 1, Gold = 2 };

    void clearSheet();
    void toTitle();
    void startSheet();
    void readInput();
    void driveBot();
    void logic();
    void audio();
    void lights();
    void draw();
    void stamp();
    void undo();
    void post();
    void offerFile();
    void fileSheet();
    void winSheet();
    void loseSheet(const char* why);
    void blip(float freq, int frames);
    void tally(int& face, int& tot) const;
    int dueOf() const;
    void hud(int col, int row, const char* s);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void sprI(const gs::Image& img, float cx, float cy, float h, int pal);
    void solid(float x, float y, float w, float h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Play;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool filed_ = false;
    bool finisherGold_ = false;
    int age_ = 0;
    int idx_ = 0;
    int lines_ = 0;
    int faults_ = 0;
    int score_ = 0;
    int bare_ = 0;
    int golds_ = 0;
    int cream_ = 0;
    int dueSum_ = 0;
    int marks_[8] = {};
    int markN_ = 0;
    int cursor_ = 0;
    int wait_ = 0;
    int repL_ = 0;
    int repR_ = 0;
    int flashT_ = 0;
    int flashK_ = 0;
    int holdN_ = 0;
    int beepN_ = 0;
    float beepF_ = 0;
    float beepV_ = 0;
    int melStep_ = -1;
    int melWait_ = 0;
    const char* why_ = "";
};

}  // namespace scoregold
