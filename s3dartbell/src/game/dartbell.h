// S3 DARTBELL — one short dart, three tries.
// The bell rings only from a firm dart in the bell. Soft dies on the floor.
// Hot dies outside the bell. The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace dartbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 DARTBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 4 && flightT_ <= 11; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Flight, Dead, Ring, Leave, Pause, Over };
    enum class Fate { True, Short, Hot };
    enum class Death { None, Short, Hot, Stuck };

    struct Pin {
        float x = 0, y = 0;
        bool on = false;
        bool board = false;
    };

    void toTitle();
    void newGame();
    void beginAim();
    void launch();
    void stick();
    void ring();
    void dieTry(Death why);
    void remember(float x, float y, bool board);
    bool audit() const;
    bool sweet() const;
    void moveAim(float mx, float my);
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void dartAt(float x, float y, float h);
    void meterRow();
    void candles();
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate fate_ = Fate::True;
    Death why_ = Death::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool wasSweet_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int pinN_ = 0;
    char last_[8] = {};
    float aimX_ = kCx;
    float aimY_ = kCy;
    float landX_ = kCx;
    float landY_ = kCy;
    float fromX_ = kCx;
    float fromY_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    int flightT_ = 0;
    Pin pin_[3];
};

}  // namespace dartbell
