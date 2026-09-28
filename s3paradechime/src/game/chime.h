// S3 PARADE CHIME — a short parade up the avenue to the clock tower.
// Floats own the lanes. A touch spends a try. The town clock runs whether
// you are standing or not. It chimes twelve only if you are in the square
// when the hour arrives. Early does not count. Late is silent.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace paradechime {

constexpr int kFpc = 8;
constexpr int kStartSec = 11 * 3600 + 59 * 60 + 36;  // 11:59:36
constexpr int kHourSec = 12 * 3600;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE CHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool chimed() const { return chimed_; }
    int hour() const;
    int minute() const;
    int second() const;
    int tries() const { return tries_; }
    int lives() const { return lives_; }
    const char* reason() const { return reason_; }
    const char* phase() const;

private:
    enum class Mode { Title, March, Hurt, Chime, Leave, Fail, Over, Pause };

    struct Lane {
        float y;
        float speed;
        float spacing;
        float phase;
        int kind;  // 0 wagon, 1 horse, 2 drum
    };

    void toTitle();
    void beginMarch();
    void respawn();
    void logic();
    void botMove();
    void humanMove();
    void reachHour();
    void missHour();
    void dieTry();
    int clockSec() const;
    bool inSquare() const;
    int laneAt(float y) const;
    int nextLane(float y) const;
    float gapX(const Lane& lane, float x) const;
    bool struck(float x, float y) const;
    void audio();
    void draw();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Lane lanes_[3] = {};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::March;
    const char* reason_ = "hour silent";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool chimed_ = false;
    bool faceLeft_ = false;
    int lives_ = 3;
    int tries_ = 0;
    int age_ = 0;
    int playFrames_ = 0;
    int hold_ = 0;
    int strikes_ = 0;
    int chimeFrames_ = 0;
    int flash_ = 0;
    float time_ = 0.f;
    float px_ = 160.f;
    float py_ = 204.f;
    float bellSwing_ = 0.f;
};

}  // namespace paradechime
