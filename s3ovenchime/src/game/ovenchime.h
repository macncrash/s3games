// S3 OVENCHIME — a short oven. The hour has to chime.
// Shut the door and hold the loaf in the gold band.
// Strike only while the clock sits on twelve. An early strike, an open door,
// or a loaf off the gold spends one of three. Miss the hour and it is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace ovenchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVENCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool doorShut() const { return door_; }
    bool inGold() const;
    int strikes() const { return strikes_; }
    int hour() const { return hour_; }
    int minute() const { return minute_; }
    int second() const { return second_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Tend, Chime, Fail, Over };

    void toTitle();
    void begin();
    void strike();
    void miss(const char* why);
    void chime();
    void fail(const char* why);
    void finish(bool win);
    void tickClock();
    void botIntent(bool& left, bool& right, bool& doorTap, bool& strikeTap);
    void tone(float freq, float vol);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool door_ = false;
    bool hourHold_ = false;
    int strikes_ = 0;
    int hold_ = 0;
    int tick_ = 0;
    int hour_ = 11;
    int minute_ = 59;
    int second_ = 50;
    float heat_ = 40.f;
    float swing_ = 0.f;
    const char* why_ = "";
    char note_[32] = {};
};

}  // namespace ovenchime
