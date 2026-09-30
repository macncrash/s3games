// S3 HORNCHIME — a short horn. The hour has to chime.
// The note is brief. It has to still be in the air when noon arrives.
// A blast that dies early does not count. Silence through the hour does not count.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace hornchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HORNCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool sounding() const { return mode_ == Mode::Note && noteLeft_ > 8 && noteLeft_ < kNote - 4; }
    int hour() const;
    int minute() const;
    int second() const;
    int calls() const { return calls_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Wait, Note, Chime, Fail, Over, Pause };

    void begin();
    void sound();
    void beginChime();
    void beginFail(const char* why);
    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void tickClock();
    void draw();
    void sky();
    void spr(const gs::Image& img, float cx, float cy, int pal, int dw = 0, int dh = 0);
    void hand(float cx, float cy, float ang, float len, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Wait;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool froze_ = false;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int noteLeft_ = 0;
    int anim_ = 0;
    int calls_ = 0;
    int misses_ = 0;
    int lastSec_ = -1;
    int tickLeft_ = 0;
    int struckH_ = 12;
    int struckM_ = 0;
    int struckS_ = 0;
    int bellSwing_ = 0;
    const char* reason_ = "";
};

}  // namespace hornchime
