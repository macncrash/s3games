// S3 PAWNCHIME — push the pawn up the file, then leave only while the hour chimes.
// Four quiet steps. A fifth square is blocked. Leaving early keeps the file open.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace pawnchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PAWNCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int steps() const { return steps_; }
    int misses() const { return misses_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* why() const { return why_; }

private:
    enum class Mode { Title, File, Over };

    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    bool ready() const;

    void begin();
    void push();
    void leave();
    void fail(const char* why);

    void paint();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    int steps_ = 0;
    int square_ = 1;
    int misses_ = 0;
    int playFrames_ = 0;
    const char* why_ = "";
};

}  // namespace pawnchime
