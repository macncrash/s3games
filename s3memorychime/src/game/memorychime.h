// S3 MEMORYCHIME — play memory until the hour has to chime. Leave when that is true.
// Ordinary pairs may be turned at any time. The bell pair chimes only on twelve.
// A bell match before the hour falls shut. After the grace, the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace memchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 100;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 MEMORYCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool onTheHour() const { return onHour(); }
    int pairs() const { return pairs_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Phase { Title, Study, Cover, Play, Show, Early, Chime, Leave, Fail, Over };

    struct Card {
        int face = 0;
        bool up = false;
        bool held = false;
    };

    void begin();
    void note(float freq, float vol);
    void flipAt(int i);
    void settle();
    void beginChime();
    void fail(const char* why);
    void botPlay();
    bool confirm() const;

    int secAt(int frames) const;
    int clockSec() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;

    void draw();
    void backdrop();
    void spr(const gs::Image& img, int x, int y, int w, int h);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    bool faceUp(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Card cards_[CARDS];
    Phase phase_ = Phase::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool clockOn_ = false;
    bool paused_ = false;
    int pairs_ = 0;
    int early_ = 0;
    int cursor_ = 0;
    int nOpen_ = 0;
    int open_[2] = {};
    int playFrames_ = 0;
    int t_ = 0;
    int beep_ = 0;
    int strikes_ = 0;
    int strikeWait_ = 0;
    bool hourVerdict_ = false;
    bool lateVerdict_ = false;
};

}  // namespace memchime
