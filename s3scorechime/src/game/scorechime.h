// S3 SCORE CHIME — a short score. The hour has to chime.
// Four marks go on the board. Leaving before twelve leaves the score open.
// The leave is only good while the hour is still chiming.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace scorechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SCORE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int marks() const { return marks_; }
    int misses() const { return misses_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* why() const { return why_; }

private:
    enum class Mode { Title, Roll, Ink, Gap, Hold, Pause, Over };

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    bool ready() const;

    void begin();
    void mark(bool hit);
    void leave();
    void fail(const char* why);
    void note(float freq);

    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Roll;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool spoiled_ = false;
    int marks_ = 0;
    int misses_ = 0;
    int wind_ = 0;
    int anim_ = 0;
    int playFrames_ = 0;
    int chimeAge_ = -1;
    const char* why_ = "";
};

}  // namespace scorechime
