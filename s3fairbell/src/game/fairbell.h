// S3 FAIRBELL — one high striker on a short midway.
// The puck rides the tower. Stopping it on the gold plate rings the bell.
// The red cap is hot and that try dies. Under the plate is short and dies.
// Three tries. Leave when the bell rings before the third try dies.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace fairbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 FAIRBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rung() const { return rung_; }
    bool rules() const;
    int deadTries() const { return dead_; }
    int tryNo() const { return tryNo_; }
    const char* phase() const;
    const char* reason() const { return why_; }

private:
    enum class Mode { Title, Aim, Strike, Fall, Dead, Ring, Leave, Pause, Over };
    enum class Fate { Bell, Short, Hot };

    void newGame();
    void toTitle();
    void beginAim();
    void strike();
    void startFall();
    void ring();
    void dieTry();
    void advanceMeter(float dt);
    Fate classify(float m) const;
    float meterToImg(float m) const;
    float imgToMeter(float img) const;
    float towerTop() const;
    float trackY(float imgY) const;
    float puckY(float m) const;
    float shownPuckY() const;
    bool hammerUp() const;

    void blip(float freq, float vol, float hold);
    void chord(float a, float b, float c, float hold);
    void hush();
    void tickAudio(float dt);

    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false, bool feet = false,
             int fog = 0, bool shadow = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog = 0, bool shadow = false);
    void word(const gs::Image& img, float cx, float cy, int pal);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void backdrop();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    Fate fate_ = Fate::Short;
    Fate wasFate_ = Fate::Short;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rung_ = false;
    bool wasSweet_ = false;
    int dead_ = 0;
    int tryNo_ = 0;
    int meterDir_ = 1;
    float meter_ = 0.f;
    float power_ = 0.f;
    float clock_ = 0.f;
    float strikeT_ = 0.f;
    float fallT_ = 0.f;
    float deadT_ = 0.f;
    float ringT_ = 0.f;
    float leaveT_ = 0.f;
    float burstT_ = 0.f;
    float beep_ = 0.f;
    float tickT_ = 0.f;
    float bellPh_ = 0.f;
    float bellAmp_ = 0.12f;
    float bellTick_ = 0.f;
    float kidX_ = 96.f;
    const char* why_ = "OPEN";
};

}  // namespace fairbell
