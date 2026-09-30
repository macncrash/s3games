// S3 INKWELL CHIME — fill the well, then the hour has to chime.
// A drop swings over the lip. Dip only while the mouth is open.
// Four wet dips, still standing when the clock strikes, is the chime.
// An early ask waits. A dry well after the hour, or three dead tries, does not.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace inkwellchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 INKWELL CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    int dips() const { return dips_; }
    int tryNo() const { return tryNo_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Miss, Hold, Chime, Fail };

    void begin();
    void dip();
    void dieTry();
    void beginChime();
    void beginFail(const char* why);
    void botAct(bool& press);
    void tickClock();
    bool hot() const;
    bool full() const { return dips_ >= kDips; }
    float needle() const;
    float dropX() const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;
    void tone(float freq);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hand(float cx, float cy, float ang, float len, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void sky();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool armed_ = false;
    int dips_ = 0;
    int dead_ = 0;
    int tryNo_ = 1;
    int playFrames_ = 0;
    int phase_ = 0;
    int anim_ = 0;
    int titleWait_ = 0;
    float wellX_ = 160.f;
};

}  // namespace inkwellchime
