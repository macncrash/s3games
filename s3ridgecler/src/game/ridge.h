// S3 RIDGE CLER — at the ridge, clear the ground before the clock dies.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace rcler {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 RIDGE CLER"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int left() const { return kPiles - dumped_; }
    const char* reason() const { return reason_; }
    // 0 title, 1 the ridge, 2 the last piles, 3 ground clear, 4 the clock died
    int marker() const;

private:
    static constexpr int kPiles = 6;
    enum class Mode { Title, Play, Pause, Won, Lost };

    struct Pile {
        float z = 0, lat = 0;
        int kind = 0;
        float work = 0;
        bool taken = false;
    };
    struct Dust {
        float x = 0, y = 0, vx = 0, vy = 0, life = 0;
    };
    struct Proj {
        float x = 0, y = 0, ppm = 0;
        bool ok = false;
    };

    void resetField();
    void begin();
    void toTitle();
    void updatePlay();
    void botInput(float& ix, float& iz, bool& hold);
    void humanInput(float& ix, float& iz, bool& hold);
    void move(float ix, float iz);
    void updateWork(bool hold);
    int focusPile() const;
    bool atLip() const;
    void win();
    void lose();
    void blip(float freq, float hold);
    void serviceAudio();
    void fadeDust();
    void puff(float x, float y);
    float rnd();
    Proj project(float lat, float z) const;
    float bendAt(float row) const;
    int fogFor(float z) const;

    void draw();
    void drawRoad(float shx);
    void drawSky(float shx);
    void drawKit(float shx);
    void drawWorld(float shx);
    void messages(float shx);
    void hud();
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet,
             bool shadow = false);
    void text(const char* s, float x, float y, float scale, int pal);
    void tileText(int col, int row, const char* s, int pal);
    bool startPressed() const;
    bool rakeDown() const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Pile pile_[kPiles]{};
    Dust dust_[12]{};
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
    float lat_ = 0.f, z_ = 0.f;
    float face_ = -1.f;
    float tip_ = 0.f;
    float step_ = 0.f;
    float t_ = 0.f;
    float shake_ = 0.f;
    float blip_ = 0.f;
    float tick_ = 0.f;
    float fanT_ = 0.f;
    int clock_ = 0;
    int fanStep_ = -1;
    bool fanGood_ = false;
    uint32_t rng_ = 1;
};

}  // namespace rcler
