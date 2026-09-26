// S3 EIGHTCHIME — a short eight. The hour has to chime.
// The 8 has to fall in the hour pocket as twelve strikes.
// A pocket while the clock is still short of the hour is given back.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace eightchime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 EIGHTCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool pocketed() const { return eightIn_; }
    bool rules() const { return solved_; }
    bool rolling() const { return mode_ == Mode::Roll && rollFrames_ > 6 && rollFrames_ + 6 < travel_; }
    int strokes() const { return strokes_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Aim, Roll, Early, Miss, Chime, Fail, Pause, Over };
    enum class Lie { Hour, Scratch, Wide, Miss };

    struct Body {
        float x = 0, y = 0, vx = 0, vy = 0;
        bool down = false;
        bool fell = false;
        int pocket = -1;
    };

    void spot();
    void toTitle();
    void newGame();
    void beginAim(bool keepAim);
    void shoot();
    void beginChime();
    void beginEarly();
    void beginMiss(const char* why);
    void beginFail(const char* why);
    void resolve();
    bool prove();
    bool layout() const;
    bool clockHits(int travel) const;
    float lineAim() const;
    Lie roll(Body* b, float aim, float power, int* travel) const;
    void step(Body* b, bool* clack) const;
    bool moving(const Body* b) const;
    bool inCloth(float x, float y) const;

    int secFrom(int play) const;
    int untilFrom(int play) const;
    bool onHourPlay(int play) const;
    bool pastHourPlay(int play) const;
    bool shouldShoot(int play, int travel) const;
    void split(int& h, int& m, int& s) const;
    void faceTime(int& h, int& m, int& s) const;
    bool onHour() const { return onHourPlay(playFrames_); }
    bool pastHour() const { return pastHourPlay(playFrames_); }

    void blip(float freq, float vol);
    void strikeBell(int n);
    void silenceTicks();
    void tickClock();

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool shadow = false);
    void image(const gs::Image& img, float cx, float cy, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void aimAid();
    void cueDraw();
    void clockAt(float cx, float cy, float size);
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Body ball_[kBalls]{};
    const char* reason_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool eightIn_ = false;
    bool solved_ = false;
    int strokes_ = 0;
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int rollFrames_ = 0;
    int earlyFrames_ = 0;
    int missFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int travel_ = 0;
    int lastSec_ = -1;
    int tickLeft_ = 0;
    int strikes_ = 0;
    int eightPocket_ = -1;
    float aim_ = -0.6f;
    float power_ = 0.48f;
    float solvedAim_ = -0.6f;
    float solvedPower_ = 0.48f;
    float bellPh_ = 0;
    float bellAmp_ = 0.12f;
};

}  // namespace eightchime
