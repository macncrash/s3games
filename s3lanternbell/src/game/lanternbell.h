// S3 LANTERNBELL — one short night, three tries.
// Light the lamps in order. The last lamp hangs under the bell.
// A wrong lamp, or a flame that gutters, kills the try.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace lanternbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 LANTERNBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool lighting() const { return mode_ == Mode::Play && step_ > 0; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Watch, Play, Dead, Ring, Leave, Over };

    void begin();
    void audit();
    void enterWatch();
    void enterPlay();
    void light(int lamp);
    void ring();
    void dieTry(const char* why);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool lit_[kLamps] = {};
    int dead_ = 0;
    int tryNo_ = 0;
    int cursor_ = 0;
    int step_ = 0;
    int show_ = 0;
    int timer_ = 0;
    int hold_ = 0;
    int botWait_ = 0;
    const char* why_ = "";
    float clock_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.2f;
};

}  // namespace lanternbell
