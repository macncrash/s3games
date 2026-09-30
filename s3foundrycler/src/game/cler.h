// S3 FOUNDRY CLER — clear the foundry floor before the pour clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace fcler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FOUNDRY CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int clockLeft() const { return clock_ / 60; }
    int pilesLeft() const;
    // 0 title, 1 the floor, 2 ground clear, 3 the clock died
    int marker() const;

private:
    static constexpr int kPiles = 6;
    static constexpr int kMotes = 12;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Pile {
        float x = 0;
        int hp = 0;
    };
    struct Mote {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };

    void resetFloor();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, bool& hold);
    void humanInput(float& ix, bool& hold);
    int nearestPile() const;
    bool allClear() const;
    float ladleX() const;
    void win();
    void lose();
    void blip(float freq);
    void serviceAudio();
    void puff(float x, float y);
    void tickMotes();

    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Pile pile_[kPiles]{};
    Mote mote_[kMotes]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool sweeping_ = false;
    float px_ = 48;
    float face_ = 1;
    float walk_ = 0;
    float t_ = 0;
    float blip_ = 0;
    int clock_ = 0;
    int stun_ = 0;
    int gate_ = 0;
    int secMark_ = -1;
};

}  // namespace fcler
