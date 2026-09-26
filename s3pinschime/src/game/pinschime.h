// S3 PINSCHIME — Pins: the hour has to chime.
// Bowl the head pin down as the tower reaches twelve. Early does not chime.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace pinschime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PINSCHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rolling() const { return mode_ == Mode::Roll && rollFrames_ > 6 && rollFrames_ < kTravel; }
    int hour() const;
    int minute() const;
    int second() const;
    int pinsDown() const;
    const char* reason() const { return reason_; }

private:
    enum class Mode { Title, Approach, Swing, Roll, Reset, Chime, Fail, Over, Pause };

    struct Pin {
        float x = 0, z = 0, vx = 0, vz = 0;
        float homeX = 0, homeZ = 0;
        int side = 1;
        bool hour = false;
        bool down = false;
        float fall = 0;
    };

    static constexpr int kFpc = 6;
    static constexpr int kTravel = 33;
    static constexpr int kStar = 14;
    static constexpr int kPeriod = 56;
    static constexpr int kGraceSec = 24;
    static constexpr int kHourSec = 12 * 3600;
    static constexpr int kStartSec = kHourSec - 120;
    static constexpr int kLead = kTravel + kStar;

    void showTitle();
    void newGame();
    void resetRack();
    void beginSwing();
    void release();
    void topple(Pin& p, float vx, float vz);
    void resolveHit();
    void finishBall(const char* why);
    void beginChime();
    void beginReset(const char* why);
    void beginFail(const char* why);
    void settle(float dt);
    void tickSound();
    void blip(float freq);
    void strikeBell(int n);
    void silence();

    int clockSec() const;
    int framesUntilHour() const;
    bool onHour() const;
    bool pastHour() const;
    void split(int& h, int& m, int& s) const;
    float meter() const;
    float aimX() const;
    bool onPocket() const;
    int pose() const;

    void draw();
    void sky();
    void project(float x, float z, float& sx, float& sy, float& ppm) const;
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog = 0, bool feet = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void image(const gs::Image& img, float cx, float cy, float h, int pal);
    void clockAt(float cx, float cy, float size, bool live);
    void text(const char* s, float x, float y, float scale, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Approach;
    Pin pin_[10];
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ballLive_ = false;
    bool gutter_ = false;
    bool resolved_ = false;
    bool wasPocket_ = false;
    bool wasStar_ = false;
    const char* reason_ = "";
    int playFrames_ = 0;
    int titleFrames_ = 0;
    int swingFrames_ = 0;
    int rollFrames_ = 0;
    int resetFrames_ = 0;
    int chimeFrames_ = 0;
    int failFrames_ = 0;
    int balls_ = 3;
    int lastSec_ = -1;
    int beep_ = 0;
    float stance_ = 0;
    float ballX_ = 0, ballZ_ = 0;
};

}  // namespace pinschime
