// S3 JUGGLEBELL — one short juggle, three tries.
// Loft the live ball into the hanging bell. A drop or a sail-off dies.
// The bell rings only from a true loft. The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace jugglebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 JUGGLEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool lofting() const { return mode_ == Mode::Flight && flightT_ >= 8 && flightT_ <= 28; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Flight, Dead, Ring, Leave, Pause, Over };
    enum class Death { None, Drop, Hot };

    void toTitle();
    void newGame();
    void beginAim();
    void launch();
    void ring();
    void dieTry(Death why);
    void caught();
    bool audit() const;
    bool sweet() const;
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void pattern(int i, float& x, float& y) const;
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Death why_ = Death::None;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool rules_ = false;
    bool wasSweet_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int flightT_ = 0;
    float aim_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float ballX_ = 0, ballY_ = 0, ballVx_ = 0, ballVy_ = 0;
    float clock_ = 0;
    float deadT_ = 0;
    float ringT_ = 0;
    float leaveT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
};

}  // namespace jugglebell
