// S3 OVENBELL — three tries at one loaf.
// Heat climbs. Only a pull inside the gold band rings the oven bell.
// A pale pull, a burnt pull, or a loaf left to char kills the try.
// Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace ovenbell {

constexpr int kTries = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 OVENBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool baking() const { return mode_ == Mode::Bake && heat_ > 8.f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Bake, Dead, Ring, Leave, Over };

    void audit();
    void toTitle();
    void begin();
    void enterBake();
    void pull();
    void ring();
    void dieTry(const char* why);
    void lose();
    void blip(float freq);
    void chord(float a, float b, float c);
    int loafPal() const;
    void backdrop();
    void draw();
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
    int dead_ = 0;
    int tryNo_ = 0;
    int hold_ = 0;
    const char* why_ = "";
    float heat_ = 0.f;
    float clock_ = 0.f;
    float beep_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.18f;
    float peelX_ = 40.f;
    char note_[28] = {};
};

}  // namespace ovenbell
