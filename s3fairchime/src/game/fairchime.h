// S3 FAIRCHIME — a ring booth under the midway clock.
// The gold bottle is the hour. A firm ring on its neck, while the clock is
// on twelve, is the only chime. Early gold falls off. Cream spends the ring.
// Three rings, then the hour is gone. Leave when the hour chimes.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 30;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIRCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 3 && flightT_ <= 12; }
    int rings() const { return thrown_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* bed() const { return bed_; }

private:
    enum class Mode { Title, Aim, Flight, Early, Dead, Chime, Leave, Fail, Over, Pause };
    enum class Bed { Miss, Cream, Gold };
    enum class Fate { Firm, Short, Hot };

    struct Ring {
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
    bool audit() const;

    int secAt(int frames) const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    Bed classify(float x, float y) const;
    void spot(float meter, float aim, float& x, float& y, Fate& fate) const;
    bool sweet() const;

    void botAim();
    void humanAim(const gs::Pad& p, bool fire);
    void blip(int ch, float freq, float vol, float hold);
    void strikeBell();
    void tickAudio(float dt);
    void tickClock();

    void draw();
    void backdrop();
    void clockAt(float cx, float cy, float size);
    void meterRow(float y);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool feet = false,
             int fog = 0, bool shadow = false);
    void sprI(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate fate_ = Fate::Firm;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool clockOn_ = false;
    bool wasSweet_ = false;
    int thrown_ = 0;
    int ringN_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int flightT_ = 0;
    int showT_ = 0;
    int chimeFrames_ = 0;
    int leaveT_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int lastSec_ = -1;
    char bed_[8] = {};
    char last_[12] = {};
    float aimX_ = 160.f;
    float landX_ = 160.f;
    float landY_ = 120.f;
    float fromX_ = 70.f;
    float fromY_ = 150.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float kidX_ = 58.f;
    float anim_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.2f;
    float toneT_ = 0.f;
    float tickT_ = 0.f;
    Ring ring_[3];
};

}  // namespace fairchime
