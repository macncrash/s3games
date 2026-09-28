// S3 JUGGLECHIME — keep the cascade until the clock has to strike.
// Catches before noon are only practice. The hour chimes while the
// pattern is still alive, then you leave. A dead pattern lets the hour pass.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace jugglechime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool live() const { return live_; }
    int hour() const;
    int minute() const;
    int second() const;
    int catches() const { return catches_; }
    int strikes() const { return strikes_; }
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Play, Drop, Chime, Leave, Fail };

    struct Air {
        bool on = false;
        int ball = 0;
        int to = 0;
        int age = 0;
    };

    void toTitle();
    void begin();
    void pattern();
    void miss();
    void beginChime();
    void beginFail(const char* why);
    void stepPlay();
    bool audit() const;

    int secAt(int frames) const;
    int clockSec() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;

    void blip(float freq);
    void chord(float a, float b, float c);
    void ageTone();

    void draw();
    void backdrop();
    void ballAt(int ball, float x, float y, float scale);
    void clockAt(float cx, float cy);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool clockOn_ = false;
    bool live_ = false;
    int catches_ = 0;
    int playFrames_ = 0;
    int titleWait_ = 0;
    int tick_ = 0;
    int wait_ = 0;
    int chimeFrames_ = 0;
    int strikes_ = 0;
    int leaveT_ = 0;
    int latch_[2] = {};
    int hold_[2][3] = {};
    int hn_[2] = {};
    Air air_[3]{};
    float beep_ = 0;
    float bellPh_ = 0;
};

}  // namespace jugglechime
