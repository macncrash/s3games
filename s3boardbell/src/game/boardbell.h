// S3 BOARD BELL — one night desk, three tries.
// A lamp marks the live trunk. The brass bell hangs on one line.
// The bell rings only if that trunk is plugged into the bell before the lamp dies.
// A wrong jack, an empty seat, or a dead lamp kills the try.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace boardbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 BOARD BELL"; }
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
    enum class Mode { Title, Patch, Dead, Ring, Leave, Over };

    void toTitle();
    void begin();
    void armTry();
    void seat();
    void ring();
    void dieTry(const char* why);
    void steer();
    void botAct();
    void draw();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    int irnd(int n);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool held_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int hold_ = 0;
    int bank_ = 0;
    int row_ = 0;
    int trunk_ = 0;
    int line_ = 0;
    const char* why_ = "";
    float life_ = 0;
    float lifeMax_ = 5.f;
    float clock_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.12f;
    uint32_t rng_ = 0xB04Du;
};

}  // namespace boardbell
