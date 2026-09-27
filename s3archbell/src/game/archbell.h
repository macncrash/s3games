// S3 ARCHBELL — one short arch, three tries.
// The bell rings only from a firm arrow in the bell.
// A short loose dies in the grass. A hot hold dies outside the bell.
// The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace archbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 ARCHBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 3 && flightT_ <= 10; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Nock, Flight, Dead, Ring, Leave, Over };
    enum class Fate { True, Short, Hot };
    enum class Death { None, Short, Hot, Wide };

    void toTitle();
    void newGame();
    void beginAim();
    void enterNock();
    void nockTick();
    void loose();
    void arrive();
    void ring();
    void dieTry(Death why);
    bool audit() const;
    void steer();
    bool drawHeld() const;
    void blip(int ch, float freq, float vol, int frames);
    void pumpAudio();
    void draw();
    void backdrop();
    void stamp(const gs::Image& img, float x, float y, float w, float h, int pal, bool shadow = false);
    void at(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Fate fate_ = Fate::True;
    Death why_ = Death::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool skipHold_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int drawTick_ = 0;
    int steady_ = 0;
    int flightT_ = 0;
    int t_ = 0;
    int blipLeft_ = 0;
    int tone2Left_ = 0;
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float bellPh_ = 0;
    float bellAmp_ = 0.15f;
    int bellTick_ = 0;
    char last_[8] = {};
};

}  // namespace archbell
