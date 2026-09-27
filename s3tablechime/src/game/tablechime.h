// S3 TABLECHIME — a short table. You are done when the hour has to chime.
// A firm ball in the far cup, while the clock is on twelve, is the only chime.
// An early cup is lifted. Anything else spends the try. Three tries, then the hour is gone.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace tablechime {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 TABLECHIME"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool rules() const { return rules_; }
    bool onTheHour() const { return onHour(); }
    bool flying() const { return mode_ == Mode::Flight && flightT_ >= 6 && flightT_ <= 22; }
    int tries() const { return tries_; }
    int hour() const;
    int minute() const;
    int second() const;
    const char* reason() const { return reason_; }
    const char* bed() const { return bed_; }

private:
    enum class Mode { Title, Aim, Flight, Early, Dead, Chime, Leave, Fail, Over, Pause };

    void toTitle();
    void newGame();
    void beginAim();
    void launch(bool firm);
    void resolve();
    void beginEarly();
    void beginDead(const char* why);
    void beginChime();
    void beginFail(const char* why);
    bool audit() const;
    bool sweet() const;
    void stepMeter(float& m, float& dir) const;
    void botAim();
    void humanAim(const gs::Pad& p, bool fire);

    int secAt(int frames) const;
    int clockSec() const;
    int framesUntilHour() const;
    bool onHourAt(int frames) const;
    bool onHour() const;
    bool pastHour() const;
    void face(int& h, int& m, int& s) const;

    void blip(int ch, float freq, float vol, float hold);
    void tickAudio(float dt);
    void draw();
    void backdrop();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool shadow = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode held_ = Mode::Aim;
    const char* reason_ = "";
    const char* bed_ = "";
    const char* miss_ = "";
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool rules_ = false;
    bool firm_ = false;
    bool clockOn_ = true;
    int tries_ = 0;
    int playFrames_ = 0;
    int flightT_ = 0;
    int titleFrames_ = 0;
    int showT_ = 0;
    int chimeFrames_ = 0;
    int strikes_ = 0;
    int failFrames_ = 0;
    int leaveT_ = 0;
    int lastSec_ = -1;
    float aimX_ = kServeX;
    float aimY_ = 148.f;
    float landX_ = kCupX;
    float landY_ = kCupY;
    float ballX_ = kServeX;
    float ballY_ = kServeY;
    float hop_ = 0;
    float meter_ = 0.2f;
    float meterDir_ = 1.f;
    float bellPh_ = 0;
    float bellAmp_ = 0.2f;
    float toneT_ = 0;
    float tickT_ = 0;
    float bellTick_ = 0;
};

}  // namespace tablechime
