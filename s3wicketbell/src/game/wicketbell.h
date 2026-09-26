// S3 WICKETBELL — three bowls at one wicket.
// A brass bell hangs between the stumps. Only a good length through the
// open mouth rings it. The lip, the wood, a wide, a short, and a ball
// over the top all die. Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace wicketbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 WICKETBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const;
    bool flying() const { return mode_ == Mode::Flight && flight_ > 0.22f && flight_ < 0.78f; }
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Flight, Dead, Ring, Leave, Pause, Over };
    enum class Fate { Bell, Wood, Wide, Short, Hot };

    static Fate judge(float x, float meter);
    static const char* fateName(Fate f);
    bool picture() const;

    void toTitle();
    void newGame();
    void beginAim();
    void release();
    void resolve();
    void ring();
    void dieTry();
    void advanceMeter(float dt);
    void tickTitle(float dt);
    void tickAim(float dt);
    void tickFlight(float dt);
    void tickDead(float dt);
    void tickRing(float dt);
    void tickLeave(float dt);
    void tickOver();
    void tickBell(float dt);

    int pose() const;
    void bowlerHand(int pose, float& x, float& y) const;
    void ballAt(float u, float& x, float& y) const;

    void blip(int ch, float freq, float vol, float hold);
    void audio(float dt);
    bool bowlPressed() const;
    bool startPressed() const;

    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate fate_ = Fate::Short;
    const char* why_ = "OPEN";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool armed_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    float aim_ = 0.f;
    float meter_ = 0.f;
    float meterDir_ = 1.f;
    float clock_ = 0.f;
    float titleT_ = 0.f;
    float flight_ = 0.f;
    float deadT_ = 0.f;
    float ringT_ = 0.f;
    float leaveT_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.2f;
    float shake_ = 0.f;
    float fromX_ = 0.f, fromY_ = 0.f, toX_ = 0.f, toY_ = 0.f, lob_ = 0.f;
    float hold_[3] = {};
};

}  // namespace wicketbell
