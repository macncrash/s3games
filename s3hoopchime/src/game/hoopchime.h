// S3 HOOPCHIME — Hoop: the hour has to chime.
// A clean count while the tower is still short of twelve is waved off.
// The ball has to pass through the rim as twelve strikes. Three shots.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace hoopchime {

constexpr int kFpc = 6;
constexpr int kGraceSec = 24;
constexpr int kShots = 3;

enum class Kind { Swish, Iron, Short, Hot, Wide, Miss };

struct Ball {
    float x = 0, y = 0, z = 0;
    float vx = 0, vy = 0, vz = 0;
};

struct Trace {
    bool clean = false;
    bool rim = false;
    bool bank = false;
    bool crossed = false;
    float rad = 99.f;
    float crossX = 0;
    float crossZ = 0;
    float flight = 0;
    float scoreAt = 0;
};

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 HOOPCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool clean() const { return clean_; }
    bool flying() const { return mode_ == Mode::Flight && flightFrames_ > 8 && flightFrames_ + 8 < scoreFrames_; }
    int shots() const { return shots_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* phase() const;

private:
    enum class Mode { Title, Aim, Flight, Call, Chime, Fail, Over, Pause };

    void showTitle();
    void newGame();
    void beginAim();
    bool launch(float meter);
    void fly();
    void finishShot();
    void beginCall(const char* why);
    void beginChime();
    void beginFail(const char* why);
    void solve();
    void humanAim(const gs::Pad& pad, bool fire);
    void dribble();

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;

    void blip(float freq);
    void strikeBell();
    void decayAudio();

    void backdrop();
    void draw();
    void clockAt(float cx, float cy, float size, bool live);
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    const gs::Image* wordFor(const char* why) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Kind hint_ = Kind::Miss;
    Kind last_ = Kind::Miss;
    Ball ball_{};
    Trace tr_{};
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool clean_ = false;
    bool wasSweet_ = false;
    bool wasWindow_ = false;
    bool sawClean_ = false;
    int shots_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int flightFrames_ = 0;
    int splash_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int strikes_ = 0;
    int scoreFrames_ = 0;
    int lastSec_ = -1;
    int beep_ = 0;
    int flash_ = 0;
    float feet_ = 0;
    float aimZ_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float anim_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float dribS_ = 0;
};

}  // namespace hoopchime
