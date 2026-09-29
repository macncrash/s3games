// S3 CLOCKTAPE — the drawer has to match the tape.
// Turn the hands. File a face only when it is a line on the tape.
// A near time stays out. Three slips in the drawer, and you leave.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace clocktape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CLOCKTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool matched() const;
    bool rules() const { return rules_; }
    int faults() const { return faults_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int heldCount() const;

private:
    enum class Mode { Title, Play, Leave, Win, Lose };

    void begin();
    void turn(int dir);
    void fileFace();
    void botAct();
    void human(const gs::Pad& pad);
    void blip(float freq);
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();
    bool audit();
    int openLine() const;
    int faceLine() const;
    int hourStep() const;
    int minuteStep() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool held_[kTapeN] = {};
    bool pause_ = false;
    int hour_ = 1;
    int minute_ = 10;
    int grip_ = 0;
    int faults_ = 0;
    int leaveT_ = 0;
    int titleT_ = 0;
    int blipLeft_ = 0;
    char note_[40] = {};
};

}  // namespace clocktape
