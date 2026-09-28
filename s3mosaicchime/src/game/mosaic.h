// S3 MOSAIC CHIME — lay a short mosaic. Leave when the hour has to chime.
// The picture is a clock face. Tiles may be set early, but the last tessera
// only chimes on twelve. Finish before that and the board wipes. Miss the
// grace and the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace mosaicchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 90;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MOSAIC CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int tiles() const { return set_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return why_; }

private:
    enum class Phase { Title, Lay, Early, Chime, Leave, Fail, Over };

    void begin();
    void stampHere();
    void wipeEarly();
    void beginChime();
    void fail(const char* why);
    bool filled() const;
    bool confirm() const;
    int secAt(int frames) const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void note(float freq, float vol);
    void backdrop();
    void draw();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Image& img, int x, int y, int w, int h);
    const char* inkName() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Phase phase_ = Phase::Title;
    int board_[CELLS] = {};
    int cx_ = 0;
    int cy_ = 0;
    int ink_ = 1;
    int set_ = 0;
    int early_ = 0;
    int t_ = 0;
    int playFrames_ = 0;
    int beep_ = 0;
    int strikes_ = 0;
    int strikeWait_ = 0;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool clockOn_ = false;
    bool paused_ = false;
    const char* why_ = "";
};

}  // namespace mosaicchime
