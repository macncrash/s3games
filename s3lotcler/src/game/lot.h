// S3 LOT CLER — the lot is yours. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace lotcler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LOT CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kPiles - cleared_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the lot, 2 the last piles, 3 ground clear, 4 the clock died
    int marker() const;

private:
    static constexpr int kPiles = 7;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Pile {
        float x = 0, y = 0;
        int kind = 0;
        float work = 0;
        bool gone = false;
    };
    struct Dust {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void layLot();
    void begin();
    void toTitle();
    void tickPlay();
    void steerBot(float& ix, float& iy, bool& sweep);
    void steerHuman(float& ix, float& iy, bool& sweep);
    int nearestPile() const;
    void win();
    void lose();
    void chirp(float freq, float hold);
    void serviceAudio();
    void kickDust(float x, float y);
    float rnd();

    void draw();
    void sky();
    void lot();
    void messages();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet,
             bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    bool startPressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Pile pile_[kPiles]{};
    Dust dust_[12]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool brushing_ = false;
    int aim_ = -1;
    int cleared_ = 0;
    const char* reason_ = "";
    float mx_ = 0, my_ = 0;
    float face_ = 1;
    float spin_ = 0;
    float t_ = 0;
    float shake_ = 0;
    float chirp_ = 0;
    float tick_ = 0;
    int clock_ = 0;
    uint32_t rng_ = 11;
};

}  // namespace lotcler
