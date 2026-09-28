// S3 PARADE BELL — three tries up the avenue.
// Floats own the lanes. A touch kills the try.
// The bell at the plaza rings only if you reach it before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace paradebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 PARADE BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;

private:
    enum class Mode { Title, March, Dead, Ring, Leave, Over, Pause };

    struct Lane {
        float y;
        float speed;
        float spacing;
        float phase;
        int kind;  // 0 wagon, 1 horse
    };

    void toTitle();
    void beginMarch();
    void respawn();
    void logic(float dt);
    void botMove();
    void humanMove();
    void reachBell();
    void dieTry();
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
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool faceLeft_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int age_ = 0;
    float clock_ = 0.f;
    float time_ = 0.f;
    float px_ = 160.f;
    float py_ = 200.f;
    float bellSwing_ = 0.f;
    float bellAmp_ = 0.f;
    float hold_ = 0.f;
    float flash_ = 0.f;
};

}  // namespace paradebell
