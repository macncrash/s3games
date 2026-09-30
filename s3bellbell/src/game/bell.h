// S3 BELL BELL — three bells on one beam.
// A flame marks the live bell. Strike that bell before the try dies.
// A cold bell, or a try that runs out, kills the try.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace bellbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BELL BELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Strike, Dead, Ring, Leave, Over };

    void toTitle();
    void begin();
    void armTry();
    void strike();
    void ring();
    void dieTry(const char* why);
    void steer();
    void botAct();
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int irnd(int n);
    float bellX(int i) const;

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int hold_ = 0;
    int sel_ = 1;
    int live_ = 0;
    int think_ = 0;
    const char* why_ = "";
    float life_ = 0;
    float lifeMax_ = 5.f;
    float clock_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.08f;
    uint32_t rng_ = 0xBE11u;
};

}  // namespace bellbell
