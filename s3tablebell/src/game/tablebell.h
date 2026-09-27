// S3 TABLEBELL — one table, three tries.
// The bell rings only from a firm ball in the cup on the far cloth.
// Short dies on your half. The net dies in the tape. Hot dies off the wood.
// The third dead try ends it.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tablebell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLEBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const { return rules_; }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 6 && flightT_ <= 22; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }

private:
    enum class Mode { Title, Aim, Flight, Dead, Ring, Leave, Pause, Over };
    enum class Death { None, Short, Net, Hot, Lip };

    void toTitle();
    void newGame();
    void beginAim();
    void launch(bool firm);
    void resolve();
    void ring();
    void dieTry(Death why);
    bool audit() const;
    bool sweet() const;
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal);
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
    bool firm_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    float aimX_ = kServeX;
    float aimY_ = 150.f;
    float landX_ = kBellX;
    float landY_ = kBellY;
    float ballX_ = kServeX;
    float ballY_ = kServeY;
    float hop_ = 0;
    float meter_ = 0;
    float meterDir_ = 1;
    float clock_ = 0;
    float holdT_ = 0;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float bellTick_ = 0;
    float toneT_ = 0;
    float tickT_ = 0;
    int flightT_ = 0;
};

}  // namespace tablebell
