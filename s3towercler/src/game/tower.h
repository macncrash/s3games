// S3 TOWERCLER — one tower. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace tower {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TOWERCLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kPiles - cleared_; }
    float clockLeft() const { return clock_ / 60.f; }

private:
    static constexpr int kPiles = 6;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Pile {
        float x = 0, y = 0;
        int kind = 0;
        float work = 0;
        bool gone = false;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void resetYard();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, float& iy, bool& hold);
    void humanInput(float& ix, float& iy, bool& hold);
    void move(float ix, float iy);
    void updateWork(bool hold);
    int focusPile() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void fadeMotes();
    void puff(float x, float y);
    float rnd();

    void draw();
    void sky();
    void towerDraw();
    void piles();
    void keeperDraw();
    void messages();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet);
    void text(const char* s, float x, float y, float scale, int pal);
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Pile pile_[kPiles]{};
    Mote mote_[20]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool sweeping_ = false;
    bool moving_ = false;
    int focus_ = -1;
    int cleared_ = 0;
    float px_ = 0, py_ = 0;
    float face_ = 1;
    float step_ = 0;
    float t_ = 0;
    float clock_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    int chime_ = -1;
    uint32_t rng_ = 1;
};

}  // namespace tower
