// S3 DARTCHIME — play dart until the hour has to chime. Leave when that is true.
// A firm dart in the gold double twelve, while the clock is on twelve, is the only chime.
// Early gold falls out. Anything else spends the dart. Three darts, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace dartchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DARTCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 3 && flightT_ <= 12; }
    int darts() const { return thrown_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* bed() const { return bed_; }

private:
    enum class Mode { Title, Aim, Flight, Early, Dead, Chime, Leave, Fail, Over, Pause };
    enum class Fate { True, Short, Hot };

    struct Pin {
        float x = 0, y = 0;
        bool on = false;
        bool held = false;
    };

    void toTitle();
    void newGame();
    void beginAim();
    void launch();
    void stick();
    void beginChime();
    void beginEarly();
    void beginDead();
    void beginFail(const char* why);
    void remember(float x, float y, bool held);
    bool audit();
    bool sweet() const;
    void moveAim(float mx, float my);
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);

    int secAt(int frames) const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void stepMeter(float& m, float& dir) const;

    void blip(int ch, float freq, float vol, float hold);
    void strikeBell();
    void tickAudio(float dt);
    void tickClock();

    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false, bool flip = false);
    void dartAt(float x, float y, float h);
    void clockAt();
    void meterRow();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate fate_ = Fate::True;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool clockOn_ = false;
    bool wasSweet_ = false;
    int thrown_ = 0;
    int pinN_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int flightT_ = 0;
    int showT_ = 0;
    int chimeFrames_ = 0;
    int leaveT_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int lastSec_ = -1;
    int expectStick_ = 0;
    char bed_[8] = {};
    char last_[12] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float fromX_ = kCx;
    float fromY_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float anim_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float toneT_ = 0;
    float tickT_ = 0;
    Pin pin_[3];
};

}  // namespace dartchime
