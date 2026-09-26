// S3 GATE CLER — one gate. Clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace cler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 GATE CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return 6 - dumped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the yard, 2 last piles, 3 ground clear, 4 clock died
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Mound {
        float x = 0, y = 0;
        int kind = 0;
        float work = 0;
        bool taken = false;
    };
    struct Dust {
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
    int focusMound() const;
    bool atCrib() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void fadeDust();
    void puff(float x, float y);
    float rnd();

    void draw();
    void sky();
    void actors();
    void gate();
    void messages();
    void hud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet,
             bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void tileText(int col, int row, const char* s, int pal);
    bool startPressed() const;
    bool modePressed() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mound mound_[6]{};
    Dust dust_[10]{};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool carrying_ = false;
    bool moving_ = false;
    bool raking_ = false;
    bool tipping_ = false;
    int dumped_ = 0;
    int focus_ = -1;
    const char* reason_ = "";
    float px_ = 0, py_ = 0;
    float face_ = 1;
    float tip_ = 0;
    float step_ = 0;
    float t_ = 0;
    float shake_ = 0;
    float blip_ = 0;
    float tick_ = 0;
    float fanT_ = 0;
    int clock_ = 0;
    int fanStep_ = -1;
    bool fanGood_ = false;
    uint32_t rng_ = 1;
};

}  // namespace cler
