// S3 WICKETCHIME — a short wicket. The hour has to chime.
// Three balls at one close set of stumps. A straight good length is timber.
// Timber before twelve is put back. Timber as the hour strikes is the chime.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicketchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kFlight = 36;
constexpr int kBalls = 3;
constexpr int kHourSec = 12 * 3600;
constexpr int kStartSec = kHourSec - 120;
constexpr float kSweetLo = 0.36f;
constexpr float kSweetHi = 0.72f;
constexpr float kStump = 16.f;
constexpr float kWide = 44.f;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKETCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool wicket() const { return wicket_; }
    bool flying() const { return mode_ == Mode::Flight && flightFrames_ > 8 && flightFrames_ < kFlight - 6; }
    int balls() const { return balls_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* phase() const;

private:
    enum class Mode { Title, Aim, Flight, Call, Chime, Fail, Over, Pause };
    enum class Call { Timber, Wide, Edge, Short, Full };

    void showTitle();
    void newGame();
    void beginAim();
    void release();
    void resolve();
    void beginCall();
    void beginChime();
    void beginFail(const char* why);
    bool audit();

    static Call judge(float aim, float meter);
    static const char* callName(Call c);

    int secAt(int frames) const;
    int untilAt(int frames) const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;

    void steer(const gs::Pad& pad);
    void oscillate();
    int bowlPose(Mode view) const;
    void handPos(int pose, float& x, float& y) const;
    void spot(Call c, float aim, float meter, float& x, float& y) const;
    void ballAt(float u, float& x, float& y) const;

    void blip(int ch, float freq, float vol, int hold);
    void pumpAudio();
    void strikeBell(int n);
    void tickClock();

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void clockAt(float cx, float cy, float size);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Call fate_ = Call::Full;
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool wicket_ = false;
    int balls_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int flightFrames_ = 0;
    int callFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int lastSec_ = -1;
    int releasePose_ = 0;
    int toneUntil_[3] = {};
    float aim_ = 0.f;
    float meter_ = 0.54f;
    float meterDir_ = 1.f;
    float fromX_ = 0.f, fromY_ = 0.f, toX_ = 0.f, toY_ = 0.f;
    float anim_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.14f;
    float cloud_ = 0.f;
};

}  // namespace wicketchime
