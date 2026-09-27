// S3 ARCHCHIME — play arch until the hour has to chime. Leave when that is true.
// A firm arrow in the gold, while the clock is on twelve, is the only chime.
// Early gold is pulled. Anything else spends the arrow. Three arrows, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace archchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCHCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 3 && flightT_ <= 12; }
    int arrows() const { return arrows_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* bed() const { return bed_; }

private:
    enum class Mode { Title, Aim, Nock, Flight, Early, Chime, Leave, Fail, Over };

    void toTitle();
    void beginAim();
    void enterNock();
    void loose(bool firm);
    void arrive();
    void beginChime();
    void beginEarly();
    void beginFail(const char* why);
    bool audit() const;
    void steer();
    bool drawHeld() const;
    int wind() const;

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;

    void draw();
    void backdrop();
    void hand(float cx, float cy, float ang, float len, int pal);
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal);
    void at(const gs::Image& img, float cx, float cy, int pal);
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
    bool skipHold_ = false;
    bool firm_ = false;
    int arrows_ = 0;
    int playFrames_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flightT_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    char bed_[8] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float archX_ = kArchX;
    float walk_ = 0;
};

}  // namespace archchime
